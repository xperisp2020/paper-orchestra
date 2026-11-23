import importlib.util
import io
import json
import os
import sys
import tempfile
import types
import unittest
from contextlib import redirect_stderr, redirect_stdout
from unittest.mock import patch

import paper_writing_cli
from pathlib import Path

from utils.run_checkpoints import (
    CheckpointStore,
    fingerprint_inputs,
    fingerprint_path,
    create_or_resume_attempt,
)


class WriterResumeIntegrationTests(unittest.TestCase):
    def test_both_writers_resume_from_last_valid_stage_without_recalling_agents(self):
        repo_root = Path(__file__).resolve().parents[1]
        for relative in ("methods/paper_writer.py", "methods/paper_writer_with_plotting.py"):
            with self.subTest(writer=relative), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                base_dir = root / "output"
                materials = base_dir / "raw_materials"
                template = root / "template"
                materials.mkdir(parents=True)
                template.mkdir()
                (materials / "idea_sparse.md").write_text("idea", encoding="utf-8")
                (materials / "experimental_log.md").write_text("experiments", encoding="utf-8")
                (template / "template.tex").write_text("template", encoding="utf-8")
                (template / "guidelines.md").write_text("guidelines", encoding="utf-8")

                calls = {
                    "outline": 0, "literature": 0, "section": 0,
                    "plotting": 0, "refinement": 0, "refinement_failures": 3,
                }

                class FakeOutlineAgent:
                    def __init__(self, **kwargs):
                        pass

                    def run(self, output_filepath, **kwargs):
                        calls["outline"] += 1
                        Path(output_filepath).write_text('{"plotting_plan": []}', encoding="utf-8")

                class FakeLiteratureAgent:
                    def __init__(self, output_dir, **kwargs):
                        self.output_dir = Path(output_dir)

                    def run(self, **kwargs):
                        calls["literature"] += 1
                        self.output_dir.mkdir(parents=True, exist_ok=True)
                        for name, content in {
                            "outline_v1.json": '{"plotting_plan": []}',
                            "updated_template.tex": "updated template",
                            "references.bib": "% references",
                            "citation_map.json": "{}",
                        }.items():
                            (self.output_dir / name).write_text(content, encoding="utf-8")

                class FakeSectionWritingAgent:
                    def __init__(self, **kwargs):
                        pass

                    def run(self, output_path):
                        calls["section"] += 1
                        Path(output_path).write_text("draft", encoding="utf-8")

                class FakeContentRefinementAgent:
                    def __init__(self, work_dir, **kwargs):
                        self.work_dir = Path(work_dir)

                    def run(self, **kwargs):
                        calls["refinement"] += 1
                        if calls["refinement_failures"]:
                            calls["refinement_failures"] -= 1
                            raise RuntimeError("simulated interruption after draft checkpoint")
                        output = self.work_dir / "final_refined_paper.pdf"
                        output.write_bytes(b"%PDF-mocked")
                        return str(output)

                class FakePlottingAgent:
                    def __init__(self, **kwargs):
                        pass

                    def run(self, output_filepath, **kwargs):
                        calls["plotting"] += 1
                        Path(output_filepath).write_text("[]", encoding="utf-8")
                        return []

                agents_package = types.ModuleType("methods.agents")
                agents_package.__path__ = []
                common_utils = types.ModuleType("utils.common_utils")
                common_utils.create_log_folder = lambda **kwargs: str(root / "unused")
                tqdm_module = types.ModuleType("tqdm")
                tqdm_module.tqdm = lambda iterable, **kwargs: iterable
                modules = {
                    "methods.agents": agents_package,
                    "methods.agents.outline_agent": types.ModuleType("methods.agents.outline_agent"),
                    "methods.agents.literature_review_agent": types.ModuleType("methods.agents.literature_review_agent"),
                    "methods.agents.section_writing_agent": types.ModuleType("methods.agents.section_writing_agent"),
                    "methods.agents.content_refinement_agent": types.ModuleType("methods.agents.content_refinement_agent"),
                    "utils.common_utils": common_utils,
                    "tqdm": tqdm_module,
                }
                modules["methods.agents.outline_agent"].OutlineAgent = FakeOutlineAgent
                modules["methods.agents.literature_review_agent"].HybridLiteratureAgent = FakeLiteratureAgent
                modules["methods.agents.section_writing_agent"].SectionWritingAgent = FakeSectionWritingAgent
                modules["methods.agents.content_refinement_agent"].ContentRefinementAgent = FakeContentRefinementAgent
                if relative.endswith("paper_writer_with_plotting.py"):
                    plotting_module = types.ModuleType("methods.agents.plotting_agent")
                    plotting_module.PlottingAgent = FakePlottingAgent
                    modules["methods.agents.plotting_agent"] = plotting_module

                module_name = "_mocked_" + ("plotting" if "with_plotting" in relative else "plain") + "_writer"
                spec = importlib.util.spec_from_file_location(module_name, repo_root / relative)
                writer_module = importlib.util.module_from_spec(spec)
                with patch.dict(sys.modules, modules):
                    spec.loader.exec_module(writer_module)
                    first_result = writer_module.write_single_paper(
                        base_dir=str(base_dir),
                        latex_template_dir=str(template),
                        idea_filename="idea_sparse.md",
                        experimental_log_filename="experimental_log.md",
                        resume=False,
                    )
                    self.assertFalse(first_result)
                    self.assertEqual(calls["outline"], 3)
                    self.assertEqual(calls["literature"], 3)
                    self.assertEqual(calls["section"], 3)
                    if "with_plotting" in relative:
                        self.assertEqual(calls["plotting"], 3)

                    resumed_result = writer_module.write_single_paper(
                        base_dir=str(base_dir),
                        latex_template_dir=str(template),
                        idea_filename="idea_sparse.md",
                        experimental_log_filename="experimental_log.md",
                        resume=True,
                    )
                self.assertTrue(resumed_result)
                self.assertTrue((base_dir / "final_paper.pdf").is_file())
                self.assertEqual(calls["outline"], 3)
                self.assertEqual(calls["literature"], 3)
                self.assertEqual(calls["section"], 3)
                self.assertEqual(calls["refinement"], 4)
                completed_again = writer_module.write_single_paper(
                    base_dir=str(base_dir),
                    latex_template_dir=str(template),
                    idea_filename="idea_sparse.md",
                    experimental_log_filename="experimental_log.md",
                    resume=True,
                )
                self.assertTrue(completed_again)
                self.assertEqual(calls["outline"], 3)
                self.assertEqual(calls["literature"], 3)
                self.assertEqual(calls["section"], 3)
                self.assertEqual(calls["refinement"], 4)
                if "with_plotting" in relative:
                    self.assertEqual(calls["plotting"], 3)


class CheckpointFingerprintTests(unittest.TestCase):
    def test_file_and_directory_fingerprints_are_stable_and_change_with_content(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "source.txt"
            source.write_text("alpha", encoding="utf-8")
            first_file = fingerprint_path(source)
            self.assertEqual(first_file, fingerprint_path(source))
            tree = root / "tree"
            tree.mkdir()
            (tree / "b.txt").write_text("bravo", encoding="utf-8")
            (tree / "a.txt").write_text("alpha", encoding="utf-8")
            first_tree = fingerprint_path(tree)
            self.assertEqual(first_tree, fingerprint_path(tree))
            (tree / "a.txt").write_text("changed", encoding="utf-8")
            self.assertNotEqual(first_tree, fingerprint_path(tree))
            empty_tree = root / "empty-tree"
            empty_tree.mkdir()
            before_empty_child = fingerprint_path(empty_tree)
            (empty_tree / "empty-child").mkdir()
            self.assertNotEqual(before_empty_child, fingerprint_path(empty_tree))
            source.write_text("changed", encoding="utf-8")
            self.assertNotEqual(first_file, fingerprint_path(source))

    def test_fingerprint_inputs_is_order_independent_for_mapping_keys(self):
        self.assertEqual(
            fingerprint_inputs({"model": "x", "cutoff": 4}),
            fingerprint_inputs({"cutoff": 4, "model": "x"}),
        )

    def test_fingerprint_rejects_symlink_sources(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            target = root / "target.txt"
            target.write_text("data", encoding="utf-8")
            link = root / "link.txt"
            try:
                link.symlink_to(target)
            except (OSError, NotImplementedError):
                self.skipTest("Symlinks are unavailable on this runner.")
            with self.assertRaises(ValueError):
                fingerprint_path(link)


class CheckpointStoreTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name) / "attempt_1"
        self.root.mkdir()
        self.dependencies = {
            "inputs": (),
            "outline": ("inputs",),
            "literature": ("outline",),
            "plotting": ("outline",),
            "draft": ("literature",),
            "final": ("draft", "plotting"),
        }
        self.fingerprint = fingerprint_inputs({"materials": "abc", "model": "test"})
        self.store = CheckpointStore(
            self.root, self.fingerprint, dependencies=self.dependencies
        )

    def _artifact(self, relative, content):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path

    def test_stage_requires_valid_dependencies_and_records_hashes(self):
        self._artifact("outline.json", "{}")
        with self.assertRaises(RuntimeError):
            self.store.record_stage("literature", ["outline.json"])
        self._artifact("inputs.json", "{}")
        self.store.record_stage("inputs", ["inputs.json"])
        self._artifact("outline.json", "{}")
        self.store.record_stage("outline", ["outline.json"])
        self.assertTrue(self.store.is_stage_valid("outline"))
        state = json.loads((self.root / ".run_checkpoints.json").read_text(encoding="utf-8"))
        self.assertEqual(state["stages"]["outline"]["artifacts"]["outline.json"], fingerprint_path(self.root / "outline.json"))

    def test_missing_or_modified_artifact_invalidates_downstream(self):
        for stage, artifact in (("inputs", "inputs.json"), ("outline", "outline.json"),
                                ("literature", "refs.bib"), ("draft", "draft.tex")):
            self._artifact(artifact, stage)
            self.store.record_stage(stage, [artifact])
        self._artifact("plot.json", "plot")
        self.store.record_stage("plotting", ["plot.json"])
        self._artifact("final.pdf", "pdf")
        self.store.record_stage("final", ["final.pdf"])
        self.assertTrue(self.store.is_stage_valid("final"))
        (self.root / "refs.bib").write_text("tampered", encoding="utf-8")
        self.assertFalse(self.store.is_stage_valid("literature"))
        self.assertFalse(self.store.is_stage_valid("draft"))
        self.assertFalse(self.store.is_stage_valid("final"))

    def test_missing_artifact_is_invalid(self):
        self._artifact("inputs.json", "inputs")
        self.store.record_stage("inputs", ["inputs.json"])
        (self.root / "inputs.json").unlink()
        self.assertFalse(self.store.is_stage_valid("inputs"))

    def test_rejects_traversal_absolute_and_non_regular_artifacts(self):
        outside = Path(self.temp.name) / "outside.txt"
        outside.write_text("secret", encoding="utf-8")
        with self.assertRaises(ValueError):
            self.store._artifact_path("../outside.txt")
        with self.assertRaises(ValueError):
            self.store._artifact_path(str(outside))
        (self.root / "folder").mkdir()
        with self.assertRaises(FileNotFoundError):
            self.store._artifact_path("folder")

    def test_rejects_symlink_artifact(self):
        target = self._artifact("real.txt", "real")
        link = self.root / "linked.txt"
        try:
            link.symlink_to(target)
        except (OSError, NotImplementedError):
            self.skipTest("Symlinks are unavailable on this runner.")
        with self.assertRaises(ValueError):
            self.store._artifact_path("linked.txt")

    def test_lock_is_exclusive_and_not_auto_deleted(self):
        lock = self.root / ".run_checkpoints.lock"
        lock.write_text("owner", encoding="utf-8")
        with self.assertRaises(RuntimeError):
            self.store.is_stage_valid("inputs")
        self.assertEqual(lock.read_text(encoding="utf-8"), "owner")

    def test_run_lock_covers_nested_checkpoint_writes_and_excludes_other_store(self):
        other = CheckpointStore(
            self.root, self.fingerprint, dependencies=self.dependencies
        )
        with self.store.run_lock():
            self._artifact("inputs.json", "inputs")
            self.store.record_stage("inputs", ["inputs.json"])
            with self.assertRaises(RuntimeError):
                other.is_stage_valid("inputs")
            self.assertTrue((self.root / ".run_checkpoints.lock").is_file())
        self.assertFalse((self.root / ".run_checkpoints.lock").exists())
        self.assertTrue(self.store.is_stage_valid("inputs"))

    def test_state_write_is_atomic_and_has_no_temporary_files(self):
        self._artifact("inputs.json", "inputs")
        self.store.record_stage("inputs", ["inputs.json"])
        self.assertTrue((self.root / ".run_checkpoints.json").is_file())
        self.assertEqual(
            [p.name for p in self.root.iterdir() if ".tmp-" in p.name],
            [],
        )

    def test_resume_reuses_only_latest_attempt_with_matching_fingerprint(self):
        base = Path(self.temp.name) / "attempt-base"
        attempt, store, resumed = create_or_resume_attempt(base, self.fingerprint, dependencies=self.dependencies)
        self.assertFalse(resumed)
        self._artifact("inputs.json", "inputs")
        # Use the store selected by the helper for the attempt it created.
        local_artifact = Path(attempt) / "inputs.json"
        local_artifact.write_text("inputs", encoding="utf-8")
        store.record_stage("inputs", ["inputs.json"])
        same_attempt, same_store, resumed = create_or_resume_attempt(
            base, self.fingerprint, resume=True, dependencies=self.dependencies
        )
        self.assertTrue(resumed)
        self.assertEqual(os.path.realpath(attempt), os.path.realpath(same_attempt))
        self.assertTrue(same_store.is_stage_valid("inputs"))
        changed_attempt, _, resumed = create_or_resume_attempt(
            base, fingerprint_inputs({"materials": "new"}), resume=True, dependencies=self.dependencies
        )
        self.assertFalse(resumed)
        self.assertNotEqual(os.path.realpath(attempt), os.path.realpath(changed_attempt))

    def test_read_only_resume_lookup_does_not_create_an_attempt(self):
        base = Path(self.temp.name) / "empty-resume"
        attempt, store, resumed = create_or_resume_attempt(
            base, self.fingerprint, resume=True, create_if_missing=False
        )
        self.assertIsNone(attempt)
        self.assertIsNone(store)
        self.assertFalse(resumed)
        self.assertEqual(list(base.iterdir()), [])

    def test_resume_skips_corrupt_attempt_without_overwriting_it(self):
        base = Path(self.temp.name) / "corrupt-base"
        attempt, _, _ = create_or_resume_attempt(base, self.fingerprint)
        state_path = Path(attempt) / ".run_checkpoints.json"
        state_path.write_text("{broken", encoding="utf-8")
        next_attempt, _, resumed = create_or_resume_attempt(
            base, self.fingerprint, resume=True
        )
        self.assertFalse(resumed)
        self.assertNotEqual(os.path.realpath(attempt), os.path.realpath(next_attempt))
        self.assertEqual(state_path.read_text(encoding="utf-8"), "{broken")

    def test_writer_and_cli_integrate_explicit_resume(self):
        repo_root = Path(__file__).resolve().parents[1]
        for relative in ("methods/paper_writer.py", "methods/paper_writer_with_plotting.py"):
            source = (repo_root / relative).read_text(encoding="utf-8")
            self.assertIn("create_or_resume_attempt", source)
            self.assertIn("resume: bool = False", source)
            self.assertIn('checkpoint_store.record_stage("draft"', source)
            self.assertIn("checkpoint_inputs/template.tex", source)
            self.assertIn("outline_initial.json", source)
            self.assertIn("resumed_attempt and not prepare_inputs_valid", source)
            self.assertIn("isinstance(exc, RuntimeError)", source)
            self.assertIn("checkpoint_lock_acquired = True", source)
            self.assertIn("create_if_missing=False", source)
            self.assertIn("if checkpoint_lock_acquired and checkpoint_run_lock is not None:", source)
            self.assertIn('"agent_sources"', source)
            self.assertIn('"checkpoint_engine"', source)
            self.assertIn('"prompt_sources"', source)
        cli = (repo_root / "paper_writing_cli.py").read_text(encoding="utf-8")
        self.assertIn('"--resume"', cli)
        self.assertIn("resume=args.resume", cli)
        self.assertIn("Resume requires an existing --output_dir", cli)
        self.assertIn("fingerprint_path(args.raw_materials_dir)", cli)
        self.assertIn("fingerprint_path(target_raw_materials)", cli)
        desktop = (repo_root / "app_cpp" / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn('QStringLiteral("--resume")', desktop)
        self.assertIn('QStringLiteral("research_engine_last_output")', desktop)
        self.assertIn('QStringLiteral("final_paper.pdf")', desktop)

    def test_immutable_stage_snapshots_survive_downstream_mutations(self):
        dependencies = {
            "prepare_inputs": (),
            "outline": ("prepare_inputs",),
            "literature": ("outline",),
            "draft": ("literature",),
        }
        store = CheckpointStore(
            self.root, self.fingerprint, dependencies=dependencies
        )
        self._artifact("checkpoint_inputs/template.tex", "original template")
        self._artifact("checkpoint_inputs/guidelines.md", "guidelines")
        store.record_stage("prepare_inputs", [
            "checkpoint_inputs/template.tex", "checkpoint_inputs/guidelines.md"
        ])
        self._artifact("outline_initial.json", "initial outline")
        self._artifact("outline.json", "initial outline")
        store.record_stage("outline", ["outline_initial.json"])
        # Later stages legitimately update working copies; immutable snapshots
        # must keep their upstream stages valid.
        (self.root / "outline.json").write_text("literature-adjusted outline", encoding="utf-8")
        self._artifact("literature_agent_output/outline_v1.json", "literature outline")
        self._artifact("literature_agent_output/updated_template.tex", "updated template")
        store.record_stage("literature", [
            "literature_agent_output/outline_v1.json",
            "literature_agent_output/updated_template.tex",
            "outline.json",
        ])
        self.assertTrue(store.is_stage_valid("prepare_inputs"))
        self.assertTrue(store.is_stage_valid("outline"))
        self.assertTrue(store.is_stage_valid("literature"))

    def test_cli_rejects_resume_without_existing_output_directory(self):
        with tempfile.TemporaryDirectory() as raw_materials:
            missing_output = os.path.join(raw_materials, "not-created")
            stdout = io.StringIO()
            stderr = io.StringIO()
            arguments = [
                "paper_writing_cli.py",
                "--raw_materials_dir", raw_materials,
                "--output_dir", missing_output,
                "--latex_template_dir", raw_materials,
                "--resume",
                "--events_jsonl",
            ]
            with patch("sys.argv", arguments):
                with redirect_stdout(stdout), redirect_stderr(stderr):
                    with self.assertRaises(SystemExit) as raised:
                        paper_writing_cli.main()
            self.assertEqual(raised.exception.code, 1)
            self.assertIn("resume_preflight", stdout.getvalue())
            self.assertIn("existing --output_dir", stderr.getvalue())

    def test_changed_input_fingerprint_never_reuses_prior_state(self):
        self._artifact("inputs.json", "inputs")
        self.store.record_stage("inputs", ["inputs.json"])
        other = CheckpointStore(
            self.root,
            fingerprint_inputs({"materials": "changed", "model": "test"}),
            dependencies=self.dependencies,
        )
        self.assertFalse(other.is_stage_valid("inputs"))


if __name__ == "__main__":
    unittest.main()
