import unittest

from methods.agents.literature_review_agent import (
    HybridLiteratureAgent,
    PaperData,
    normalize_doi,
)


class DoiMetadataTests(unittest.TestCase):
    def test_normalizes_common_doi_forms(self):
        self.assertEqual(normalize_doi("https://doi.org/10.1000/ABC.123"), "10.1000/ABC.123")
        self.assertEqual(normalize_doi("doi: 10.5555/test-1"), "10.5555/test-1")
        self.assertEqual(normalize_doi("10.1234/ABC_(2024)"), "10.1234/ABC_(2024)")

    def test_rejects_malformed_or_non_string_doi(self):
        for value in (None, 123, "", "not-a-doi", "10.123/no spaces", "https://example.com/10.1234/x"):
            with self.subTest(value=value):
                self.assertIsNone(normalize_doi(value))

    def test_bibtex_and_citation_map_preserve_only_normalized_doi(self):
        agent = HybridLiteratureAgent.__new__(HybridLiteratureAgent)
        paper = PaperData(
            citation_key="Smith2024Example",
            title="Example Research",
            authors=["A. Smith"],
            venue="Example Conference",
            year=2024,
            abstract="An abstract.",
            citation_count=3,
            found_in_section="Related Work",
            reason="Relevant method",
            doi=normalize_doi("https://doi.org/10.1000/example"),
        )
        bibtex = agent._generate_bibtex([paper])
        citation_map = agent._generate_citation_map([paper])
        self.assertIn("doi={10.1000/example}", bibtex)
        self.assertEqual(citation_map["Smith2024Example"]["doi"], "10.1000/example")

        paper_without_doi = PaperData(
            citation_key="Jones2023NoDoi",
            title="No DOI",
            authors=["B. Jones"],
            venue="Workshop",
            year=2023,
            abstract="Abstract.",
            citation_count=0,
            found_in_section="Introduction",
            reason="Background",
            doi=normalize_doi("invalid"),
        )
        bibtex_without_doi = agent._generate_bibtex([paper_without_doi])
        self.assertNotIn("doi=", bibtex_without_doi)
        self.assertIsNone(agent._generate_citation_map([paper_without_doi])["Jones2023NoDoi"]["doi"])


if __name__ == "__main__":
    unittest.main()
