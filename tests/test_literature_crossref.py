"""Dependency-free wiring checks for the Crossref literature pipeline."""
import ast
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class LiteratureCrossrefWiringTests(unittest.TestCase):
    def read(self, relative_path):
        return (ROOT / relative_path).read_text(encoding="utf-8")

    def test_literature_agent_uses_safe_opt_in_verifier_and_exports_status(self):
        source = self.read("methods/agents/literature_review_agent.py")
        ast.parse(source)
        self.assertIn("from utils.crossref_utils import safe_crossref_verification_status", source)
        self.assertIn("safe_crossref_verification_status(", source)
        self.assertIn("enabled=self.verify_dois_with_crossref", source)
        self.assertIn("doi_verification_status=doi_verification_status", source)
        self.assertIn('"doi_verification_status": p.doi_verification_status,', source)
        self.assertIn("if p.doi:", source)
        self.assertIn('entry += f",\\n  doi={{{p.doi}}}"', source)

    def test_both_writers_propagate_option_and_fingerprint_it(self):
        for path in ("methods/paper_writer.py", "methods/paper_writer_with_plotting.py"):
            with self.subTest(path=path):
                source = self.read(path)
                ast.parse(source)
                self.assertIn("verify_dois_with_crossref: bool = False", source)
                self.assertIn("verify_dois_with_crossref=verify_dois_with_crossref", source)
                self.assertIn('"verify_dois_with_crossref": bool(verify_dois_with_crossref)', source)

    def test_batch_api_propagates_crossref_option_to_both_parallel_workers(self):
        for path in ("methods/paper_writer.py", "methods/paper_writer_with_plotting.py"):
            with self.subTest(path=path):
                tree = ast.parse(self.read(path))
                functions = {
                    node.name: node for node in ast.walk(tree)
                    if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
                }
                worker = functions["process_paper_task"]
                worker_args = {arg.arg for arg in worker.args.args + worker.args.kwonlyargs}
                self.assertIn("verify_dois_with_crossref", worker_args)

                batch = functions["run_batch_writeup"]
                batch_args = {
                    arg.arg: default for arg, default in zip(
                        batch.args.args[-len(batch.args.defaults):], batch.args.defaults
                    )
                }
                self.assertIn("verify_dois_with_crossref", batch_args)
                self.assertIsInstance(batch_args["verify_dois_with_crossref"], ast.Constant)
                self.assertIs(batch_args["verify_dois_with_crossref"].value, False)

                submit_calls = [
                    node for node in ast.walk(batch)
                    if isinstance(node, ast.Call)
                    and isinstance(node.func, ast.Attribute)
                    and node.func.attr == "submit"
                ]
                self.assertTrue(submit_calls, "Batch worker submission not found")
                self.assertTrue(any(
                    keyword.arg == "verify_dois_with_crossref"
                    and isinstance(keyword.value, ast.Name)
                    and keyword.value.id == "verify_dois_with_crossref"
                    for call in submit_calls for keyword in call.keywords
                ), "Crossref option is not forwarded to ProcessPoolExecutor worker")

    def test_cli_exposes_option_in_both_pipeline_variants(self):
        source = self.read("paper_writing_cli.py")
        ast.parse(source)
        self.assertIn('"--verify_dois_with_crossref"', source)
        self.assertEqual(source.count("verify_dois_with_crossref=args.verify_dois_with_crossref"), 2)
        gui = self.read("app_cpp/src/main.cpp")
        self.assertIn("crossrefCheck->isChecked()", gui)
        self.assertIn('QStringLiteral("--verify_dois_with_crossref")', gui)


if __name__ == "__main__":
    unittest.main()
