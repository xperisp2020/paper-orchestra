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

    def test_cli_exposes_option_in_both_pipeline_variants(self):
        source = self.read("paper_writing_cli.py")
        ast.parse(source)
        self.assertIn('"--verify_dois_with_crossref"', source)
        self.assertEqual(source.count("verify_dois_with_crossref=args.verify_dois_with_crossref"), 2)


if __name__ == "__main__":
    unittest.main()
