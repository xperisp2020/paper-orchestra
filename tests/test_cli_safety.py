import ast
import os
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch

from paper_writing_cli import prepare_raw_materials
from utils.safe_workspace import create_new_workspace, create_next_workspace, ensure_paths_absent, paths_are_same, publish_file_no_overwrite
from utils.security_utils import redact_secrets


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





class OutputPreflightSafetyTests(unittest.TestCase):
    def test_preflight_rejects_any_existing_output(self):
        with tempfile.TemporaryDirectory() as temp:
            existing_file = os.path.join(temp, "outline.json")
            existing_directory = os.path.join(temp, "literature_agent_output")
            with open(existing_file, "w", encoding="utf-8") as f:
                f.write("keep")
            os.mkdir(existing_directory)
            with self.assertRaises(FileExistsError):
                ensure_paths_absent((os.path.join(temp, "latex_writeup"), existing_file, existing_directory))
            with open(existing_file, encoding="utf-8") as f:
                self.assertEqual(f.read(), "keep")

    def test_preflight_accepts_all_new_outputs(self):
        with tempfile.TemporaryDirectory() as temp:
            ensure_paths_absent((os.path.join(temp, "latex_writeup"), os.path.join(temp, "final.pdf")))


class WorkspaceSafetyTests(unittest.TestCase):
    def test_existing_workspace_is_preserved_and_rejected(self):
        with tempfile.TemporaryDirectory() as parent:
            existing = os.path.join(parent, "paper-1")
            os.makedirs(existing)
            marker = os.path.join(existing, "keep.txt")
            with open(marker, "w", encoding="utf-8") as f:
                f.write("preserve")
            with self.assertRaises(FileExistsError):
                create_new_workspace(parent, "paper-1")
            with open(marker, encoding="utf-8") as f:
                self.assertEqual(f.read(), "preserve")

    def test_workspace_name_cannot_escape_parent(self):
        with tempfile.TemporaryDirectory() as parent:
            for invalid in ("../outside", ".." + chr(92) + "outside", "C:" + chr(92) + "outside", ".", "..", ""):
                with self.subTest(name=invalid):
                    with self.assertRaises(ValueError):
                        create_new_workspace(parent, invalid)

    def test_new_workspace_is_created(self):
        with tempfile.TemporaryDirectory() as parent:
            path = create_new_workspace(parent, "paper-2")
            self.assertTrue(os.path.isdir(path))




class PdfPathCompatibilityTests(unittest.TestCase):
    def test_compile_source_and_requested_output_can_be_same_path(self):
        with tempfile.TemporaryDirectory() as temp:
            path = os.path.join(temp, "initial_draft.pdf")
            self.assertTrue(paths_are_same(path, path))
            self.assertTrue(paths_are_same(path, os.path.join(temp, ".", "initial_draft.pdf")))

    def test_distinct_compile_output_paths_are_detected(self):
        with tempfile.TemporaryDirectory() as temp:
            source = os.path.join(temp, "template.pdf")
            destination = os.path.join(temp, "final.pdf")
            self.assertFalse(paths_are_same(source, destination))


class PdfOutputSafetyTests(unittest.TestCase):
    def test_publish_creates_destination_and_preserves_source(self):
        with tempfile.TemporaryDirectory() as temp:
            source = os.path.join(temp, "generated.pdf")
            destination = os.path.join(temp, "published", "final.pdf")
            with open(source, "wb") as f:
                f.write(b"%PDF-generated")
            self.assertEqual(publish_file_no_overwrite(source, destination), destination)
            self.assertTrue(os.path.exists(source))
            with open(source, "rb") as f:
                self.assertEqual(f.read(), b"%PDF-generated")
            with open(destination, "rb") as f:
                self.assertEqual(f.read(), b"%PDF-generated")

    def test_existing_destination_is_preserved(self):
        with tempfile.TemporaryDirectory() as temp:
            source = os.path.join(temp, "generated.pdf")
            destination = os.path.join(temp, "final.pdf")
            with open(source, "wb") as f:
                f.write(b"new")
            with open(destination, "wb") as f:
                f.write(b"keep")
            with self.assertRaises(FileExistsError):
                publish_file_no_overwrite(source, destination)
            with open(destination, "rb") as f:
                self.assertEqual(f.read(), b"keep")
            self.assertTrue(os.path.exists(source))


    def test_failed_copy_preserves_source_and_partial_destination(self):
        with tempfile.TemporaryDirectory() as temp:
            source = os.path.join(temp, "generated.pdf")
            destination = os.path.join(temp, "final.pdf")
            with open(source, "wb") as f:
                f.write(b"generated")

            def fail_copy(input_file, output_file):
                output_file.write(b"partial output")
                raise OSError("simulated copy failure")

            with patch("utils.safe_workspace.shutil.copyfileobj", side_effect=fail_copy):
                with self.assertRaises(OSError):
                    publish_file_no_overwrite(source, destination)

            with open(destination, "rb") as f:
                self.assertEqual(f.read(), b"partial output")
            with open(source, "rb") as f:
                self.assertEqual(f.read(), b"generated")

    def test_source_is_retained_as_recoverable_intermediate(self):
        with tempfile.TemporaryDirectory() as temp:
            source = os.path.join(temp, "generated.pdf")
            destination = os.path.join(temp, "final.pdf")
            with open(source, "wb") as f:
                f.write(b"generated")
            publish_file_no_overwrite(source, destination)
            with open(source, "rb") as f:
                self.assertEqual(f.read(), b"generated")
            with open(destination, "rb") as f:
                self.assertEqual(f.read(), b"generated")

    def test_source_must_be_regular_file_and_distinct(self):
        with tempfile.TemporaryDirectory() as temp:
            source = os.path.join(temp, "source")
            with open(source, "wb") as f:
                f.write(b"data")
            with self.assertRaises(ValueError):
                publish_file_no_overwrite(source, source)
            with self.assertRaises(FileNotFoundError):
                publish_file_no_overwrite(os.path.join(temp, "missing"), os.path.join(temp, "out"))



class RetryIsolationRegressionTests(unittest.TestCase):
    def test_retry_attempt_workspaces_are_distinct_and_preserved(self):
        with tempfile.TemporaryDirectory() as base:
            first = create_new_workspace(base, "attempt_1")
            with open(os.path.join(first, "partial.txt"), "w", encoding="utf-8") as f:
                f.write("first attempt")
            second = create_new_workspace(base, "attempt_2")
            self.assertNotEqual(first, second)
            self.assertTrue(os.path.isfile(os.path.join(first, "partial.txt")))
            self.assertTrue(os.path.isdir(second))

    def test_writer_retries_use_isolated_attempt_directories(self):
        repo_root = Path(__file__).resolve().parents[1]
        for relative in ("methods/paper_writer.py", "methods/paper_writer_with_plotting.py"):
            source = (repo_root / relative).read_text(encoding="utf-8")
            self.assertIn('create_new_workspace(base_dir, f"attempt_{cur_try + 1}")', source)
            self.assertNotIn("shutil.rmtree(content_refinement_workdir)", source)
            self.assertIn("content_refinement_workdir = osp.join(attempt_dir", source)


class BatchResumeWorkspaceTests(unittest.TestCase):
    def test_retry_allocates_new_workspace_and_preserves_failed_run(self):
        with tempfile.TemporaryDirectory() as output:
            first = create_next_workspace(output, "paper-42")
            marker = os.path.join(first, "partial-output.txt")
            with open(marker, "w", encoding="utf-8") as f:
                f.write("diagnostic artifact")
            second = create_next_workspace(output, "paper-42")
            self.assertEqual(os.path.basename(first), "paper-42")
            self.assertEqual(os.path.basename(second), "paper-42_attempt_2")
            with open(marker, encoding="utf-8") as f:
                self.assertEqual(f.read(), "diagnostic artifact")

    def test_concurrent_name_collision_is_skipped(self):
        with tempfile.TemporaryDirectory() as output:
            os.mkdir(os.path.join(output, "paper-7"))
            os.mkdir(os.path.join(output, "paper-7_attempt_2"))
            path = create_next_workspace(output, "paper-7")
            self.assertEqual(os.path.basename(path), "paper-7_attempt_3")

    def test_batch_workers_use_collision_safe_workspace_allocator(self):
        repo_root = Path(__file__).resolve().parents[1]
        for relative in ("methods/paper_writer.py", "methods/paper_writer_with_plotting.py"):
            source = (repo_root / relative).read_text(encoding="utf-8")
            self.assertIn('create_next_workspace(output_dir, str(paper_id))', source)


class SecretRedactionTests(unittest.TestCase):
    def test_provider_keys_and_authorization_headers_are_redacted(self):
        secrets = {
            "GEMINI_API_KEY": "gemini-secret-value",
            "OPENAI_API_KEY": "openai-secret-value",
            "SEMANTIC_SCHOLAR_API_KEY": "scholar-secret-value",
        }
        with patch.dict(os.environ, secrets, clear=False):
            diagnostic = (
                "gemini-secret-value openai-secret-value scholar-secret-value "
                "Authorization: Bearer bearer-secret-value "
                "X-API-KEY: header-secret-value api_key=query-secret-value"
            )
            safe = redact_secrets(diagnostic)
        for secret in (*secrets.values(), "bearer-secret-value", "header-secret-value", "query-secret-value"):
            self.assertNotIn(secret, safe)
        self.assertGreaterEqual(safe.count("[REDACTED]"), 6)

    def test_nonsecret_diagnostic_is_preserved(self):
        message = "Request failed with HTTP 429 after 2 retries."
        self.assertEqual(redact_secrets(message), message)


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
