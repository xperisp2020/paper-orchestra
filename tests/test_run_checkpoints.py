import json
import os
import tempfile
import unittest
from pathlib import Path

from utils.run_checkpoints import (
    CheckpointStore,
    fingerprint_inputs,
    fingerprint_path,
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
            self.assertNotEqual(first_file, fingerprint_path(source) if False else first_file)

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

    def test_state_write_is_atomic_and_has_no_temporary_files(self):
        self._artifact("inputs.json", "inputs")
        self.store.record_stage("inputs", ["inputs.json"])
        self.assertTrue((self.root / ".run_checkpoints.json").is_file())
        self.assertEqual(
            [p.name for p in self.root.iterdir() if ".tmp-" in p.name],
            [],
        )

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
