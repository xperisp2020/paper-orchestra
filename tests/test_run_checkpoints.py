import json
import os
import tempfile
import unittest
from pathlib import Path

from utils.run_checkpoints import (
    CheckpointStore,
    fingerprint_inputs,
    fingerprint_path,
    create_or_resume_attempt,
)


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
        cli = (repo_root / "paper_writing_cli.py").read_text(encoding="utf-8")
        self.assertIn('"--resume"', cli)
        self.assertIn("resume=args.resume", cli)
        self.assertIn("fingerprint_path(args.raw_materials_dir)", cli)
        self.assertIn("fingerprint_path(target_raw_materials)", cli)

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
