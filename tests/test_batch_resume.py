import tempfile
import unittest
from pathlib import Path
from utils.safe_workspace import create_next_workspace, find_resumable_workspace


class BatchWorkspaceResumeTests(unittest.TestCase):
    def make_inputs(self, root):
        source = root / "source"
        source.mkdir()
        (source / "idea.md").write_text("idea", encoding="utf-8")
        (source / "empty").mkdir()
        pdf = root / "source.pdf"
        pdf.write_bytes(b"%PDF-1.4")
        return source, pdf

    def make_workspace(self, output, source, pdf):
        import shutil
        workspace = Path(create_next_workspace(str(output), "paper"))
        raw = workspace / "raw_materials"
        shutil.copytree(source, raw)
        shutil.copy2(pdf, raw / "original_paper.pdf")
        return workspace

    def test_latest_matching_workspace_is_reused(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, pdf = self.make_inputs(root)
            output = root / "output"
            output.mkdir()
            self.make_workspace(output, source, pdf)
            newest = self.make_workspace(output, source, pdf)
            self.assertEqual(
                find_resumable_workspace(str(output), "paper", str(source), str(pdf)),
                str(newest),
            )

    def test_changed_source_or_pdf_is_never_resumed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, pdf = self.make_inputs(root)
            output = root / "output"
            output.mkdir()
            self.make_workspace(output, source, pdf)
            (source / "idea.md").write_text("changed", encoding="utf-8")
            self.assertIsNone(find_resumable_workspace(str(output), "paper", str(source), str(pdf)))
            (source / "idea.md").write_text("idea", encoding="utf-8")
            pdf.write_bytes(b"%PDF-1.5")
            self.assertIsNone(find_resumable_workspace(str(output), "paper", str(source), str(pdf)))

    def test_rejects_unsafe_workspace_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, pdf = self.make_inputs(root)
            with self.assertRaises(ValueError):
                find_resumable_workspace(str(root), "../paper", str(source), str(pdf))


if __name__ == "__main__":
    unittest.main()
