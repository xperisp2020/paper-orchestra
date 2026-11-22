import ast
import os
import tempfile
from pathlib import Path
import unittest

from paper_writing_cli import prepare_raw_materials


class RawMaterialsSafetyTests(unittest.TestCase):
    def test_existing_destination_is_preserved_and_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            source = os.path.join(temp, "source")
            target = os.path.join(temp, "output", "raw_materials")
            os.makedirs(source)
            os.makedirs(target)
            with open(os.path.join(source, "new.txt"), "w", encoding="utf-8") as f:
                f.write("new")
            sentinel = os.path.join(target, "keep.txt")
            with open(sentinel, "w", encoding="utf-8") as f:
                f.write("keep")

            with self.assertRaises(FileExistsError):
                prepare_raw_materials(source, target)

            self.assertTrue(os.path.isfile(sentinel))
            with open(sentinel, encoding="utf-8") as f:
                self.assertEqual(f.read(), "keep")
            self.assertFalse(os.path.exists(os.path.join(target, "new.txt")))

    def test_new_destination_receives_copy(self):
        with tempfile.TemporaryDirectory() as temp:
            source = os.path.join(temp, "source")
            target = os.path.join(temp, "output", "raw_materials")
            os.makedirs(source)
            with open(os.path.join(source, "idea.md"), "w", encoding="utf-8") as f:
                f.write("idea")
            self.assertTrue(prepare_raw_materials(source, target))
            with open(os.path.join(target, "idea.md"), encoding="utf-8") as f:
                self.assertEqual(f.read(), "idea")

    def test_same_directory_is_a_noop(self):
        with tempfile.TemporaryDirectory() as source:
            self.assertFalse(prepare_raw_materials(source, source))

    def test_destination_inside_source_is_rejected(self):
        with tempfile.TemporaryDirectory() as source:
            target = os.path.join(source, "nested", "raw_materials")
            with self.assertRaises(ValueError):
                prepare_raw_materials(source, target)



class SecretLoggingTests(unittest.TestCase):
    def test_semantic_scholar_api_key_is_not_printed_anywhere(self):
        repo_root = Path(__file__).resolve().parents[1]
        for source in repo_root.rglob("*.py"):
            if ".git" in source.parts:
                continue
            tree = ast.parse(source.read_text(encoding="utf-8"))
            for node in ast.walk(tree):
                if isinstance(node, ast.Call) and isinstance(node.func, ast.Name) and node.func.id == "print":
                    rendered = ast.unparse(node)
                    self.assertNotIn("S2_API_KEY", rendered, str(source))
                    self.assertNotIn("SEMANTIC_SCHOLAR_API_KEY", rendered, str(source))


if __name__ == "__main__":
    unittest.main()
