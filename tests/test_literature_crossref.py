"""Pipeline integration tests for optional, non-destructive Crossref DOI checks."""
import threading
import unittest
from pathlib import Path
from unittest.mock import patch

from methods.agents.literature_review_agent import CandidatePaper, HybridLiteratureAgent


class LiteratureCrossrefIntegrationTests(unittest.TestCase):
    def setUp(self):
        self.agent = HybridLiteratureAgent.__new__(HybridLiteratureAgent)
        self.agent.verify_dois_with_crossref = False
        self.agent.registry_lock = threading.Lock()
        self.agent.print_lock = threading.Lock()
        self.agent._safe_print = lambda message: None
        self.candidate = CandidatePaper(
            title="A Reproducible Research Paper",
            year=2024,
            reason="Test candidate",
        )
        self.task = {"section": "Introduction"}
        self.s2_record = {
            "title": "A Reproducible Research Paper",
            "year": 2024,
            "authors": [{"name": "Researcher One"}],
            "venue": "Test Venue",
            "abstract": "A sufficiently useful abstract.",
            "citationCount": 12,
            "publicationDate": "2024-01-15",
            "journal": {"name": "Test Journal", "volume": "1", "pages": "1-5"},
            "externalIds": {"DOI": "https://doi.org/10.1000/Research.Test"},
        }

    def _run_enrichment(self, verification_result=None, enabled=True, verifier_error=None):
        self.agent.verify_dois_with_crossref = enabled
        registry = {}
        with patch(
            "methods.agents.literature_review_agent.s2_title_search",
            return_value=self.s2_record,
        ):
            with patch(
                "methods.agents.literature_review_agent.verify_doi_with_crossref",
                return_value=verification_result,
                side_effect=verifier_error,
            ) as verify:
                self.agent._enrich_and_register_paper(
                    self.candidate, self.task, registry, "2024-12"
                )
        return registry, verify

    def test_crossref_is_opt_in_and_default_path_does_not_call_network(self):
        registry, verify = self._run_enrichment(enabled=False)
        verify.assert_not_called()
        self.assertEqual(len(registry), 1)
        paper = next(iter(registry.values()))
        self.assertEqual(paper.doi, "10.1000/Research.Test")
        self.assertIsNone(paper.doi_verification_status)

    def test_verified_status_is_exported_without_rewriting_doi(self):
        registry, verify = self._run_enrichment({"status": "verified"})
        verify.assert_called_once_with("10.1000/Research.Test")
        paper = next(iter(registry.values()))
        self.assertEqual(paper.doi, "10.1000/Research.Test")
        self.assertEqual(paper.doi_verification_status, "verified")
        bibtex = self.agent._generate_bibtex([paper])
        citation_map = self.agent._generate_citation_map([paper])
        self.assertIn("doi={10.1000/Research.Test}", bibtex)
        self.assertEqual(citation_map[paper.citation_key]["doi"], "10.1000/Research.Test")
        self.assertEqual(citation_map[paper.citation_key]["doi_verification_status"], "verified")

    def test_not_found_and_unavailable_never_remove_or_change_doi(self):
        for status in ("not_found", "unavailable"):
            with self.subTest(status=status):
                registry, _ = self._run_enrichment({"status": status})
                self.assertEqual(len(registry), 1)
                paper = next(iter(registry.values()))
                self.assertEqual(paper.doi, "10.1000/Research.Test")
                self.assertEqual(paper.doi_verification_status, status)

    def test_unexpected_verifier_failure_is_non_blocking(self):
        registry, _ = self._run_enrichment(verifier_error=RuntimeError("network down"))
        self.assertEqual(len(registry), 1)
        paper = next(iter(registry.values()))
        self.assertEqual(paper.doi, "10.1000/Research.Test")
        self.assertEqual(paper.doi_verification_status, "unavailable")

    def test_cli_and_both_pipeline_variants_expose_the_opt_in(self):
        root = Path(__file__).resolve().parents[1]
        cli = (root / "paper_writing_cli.py").read_text(encoding="utf-8")
        plain = (root / "methods" / "paper_writer.py").read_text(encoding="utf-8")
        plotting = (root / "methods" / "paper_writer_with_plotting.py").read_text(encoding="utf-8")
        self.assertIn('"--verify_dois_with_crossref"', cli)
        self.assertEqual(cli.count("verify_dois_with_crossref=args.verify_dois_with_crossref"), 2)
        self.assertIn("verify_dois_with_crossref=verify_dois_with_crossref", plain)
        self.assertIn("verify_dois_with_crossref=verify_dois_with_crossref", plotting)
        self.assertIn('"verify_dois_with_crossref": bool(verify_dois_with_crossref)', plain)
        self.assertIn('"verify_dois_with_crossref": bool(verify_dois_with_crossref)', plotting)


if __name__ == "__main__":
    unittest.main()
