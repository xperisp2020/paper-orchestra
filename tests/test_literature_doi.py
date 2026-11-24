import unittest
from pathlib import Path

from utils.doi_utils import normalize_doi


class DoiMetadataTests(unittest.TestCase):
    def test_normalizes_common_doi_forms(self):
        self.assertEqual(normalize_doi("https://doi.org/10.1000/ABC.123"), "10.1000/ABC.123")
        self.assertEqual(normalize_doi("doi: 10.5555/test-1"), "10.5555/test-1")
        self.assertEqual(normalize_doi("10.1234/ABC_(2024)"), "10.1234/ABC_(2024)")

    def test_rejects_malformed_or_non_string_doi(self):
        for value in (None, 123, "", "not-a-doi", "10.123/no spaces", "https://example.com/10.1234/x"):
            with self.subTest(value=value):
                self.assertIsNone(normalize_doi(value))

    def test_literature_pipeline_exports_normalized_doi_to_bibtex_and_citation_map(self):
        source = (Path(__file__).resolve().parents[1] / "methods" / "agents" / "literature_review_agent.py").read_text(encoding="utf-8")
        self.assertIn('normalized_doi = normalize_doi((s2_data.get("externalIds") or {}).get("DOI"))', source)
        self.assertIn('entry += f",\\n  doi={{{p.doi}}}"', source)
        self.assertIn('"doi": p.doi,', source)
        self.assertIn("from utils.doi_utils import normalize_doi", source)


if __name__ == "__main__":
    unittest.main()
