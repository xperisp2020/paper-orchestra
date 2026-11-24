from __future__ import annotations

import sys
import unittest
from pathlib import Path
from unittest.mock import patch
from urllib.parse import parse_qs, urlparse

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from scripts import no_api_scholarly_research as research


class NoApiScholarlyResearchTests(unittest.TestCase):
    def test_semantic_scholar_normalization_keeps_doi_and_abstract(self):
        record = research.normalize_semantic_scholar({
            "title": "A credible scholarly paper title",
            "authors": [{"name": "A. Researcher"}],
            "year": 2024,
            "abstract": "An abstract with useful evidence.",
            "venue": "Journal of Research",
            "externalIds": {"DOI": "10.1234/example"},
            "url": "https://example.org/paper",
            "publicationTypes": ["JournalArticle"],
            "isOpenAccess": True,
        })
        self.assertEqual(record["doi"], "10.1234/example")
        self.assertEqual(record["authors"], "A. Researcher")
        self.assertEqual(record["provider"], "Semantic Scholar")
        self.assertIn("useful evidence", record["abstract"])

    def test_discovery_uses_three_indexes_and_deduplicates_records(self):
        def openalex_record(index):
            return {
                "title": f"OpenAlex scholarly paper number {index}",
                "doi": f"https://doi.org/10.1111/openalex{index}",
                "publication_year": 2024,
                "authorships": [{"author": {"display_name": f"Author {index}"}}],
                "abstract_inverted_index": None,
            }

        def crossref_record(index):
            return {
                "title": [f"Crossref scholarly paper number {index}"],
                "DOI": f"10.2222/crossref{index}",
                "author": [{"given": "C.", "family": f"Author{index}"}],
                "published-print": {"date-parts": [[2023]]},
                "container-title": ["Crossref Journal"],
            }

        def semantic_record(index):
            return {
                "title": f"Semantic Scholar paper number {index}",
                "authors": [{"name": f"S. Author{index}"}],
                "year": 2022,
                "abstract": "Abstract text.",
                "venue": "Semantic Journal",
                "externalIds": {"DOI": f"10.3333/semantic{index}"},
                "url": "https://semanticscholar.org/paper",
                "publicationTypes": ["JournalArticle"],
                "isOpenAccess": True,
            }

        responses = {
            "api.openalex.org": {"results": [openalex_record(i) for i in range(4)]},
            "api.crossref.org": {"message": {"items": [crossref_record(i) for i in range(4)]}},
            "api.semanticscholar.org": {"data": [semantic_record(i) for i in range(4)]},
        }
        requested = []

        def fake_request(url):
            requested.append(url)
            return responses[urlparse(url).netloc]

        with patch.object(research, "request_json", side_effect=fake_request):
            records, errors = research.discover("research methods", target=12, sleeper=lambda _: None)

        self.assertEqual(errors, [])
        self.assertEqual(len(records), 12)
        self.assertEqual(
            {record["provider"] for record in records},
            {"OpenAlex", "Crossref", "Semantic Scholar"},
        )
        self.assertEqual(len({record["doi"] for record in records}), 12)
        self.assertEqual(len(requested), 3)
        crossref_params = parse_qs(urlparse(requested[1]).query)
        self.assertIn("query.bibliographic", crossref_params)
        semantic_params = parse_qs(urlparse(requested[2]).query)
        self.assertIn("abstract", semantic_params["fields"][0])

    def test_draft_discloses_metadata_only_limitations_and_citations(self):
        records = [{
            "title": "Evidence based research methods",
            "authors": "A. Author",
            "year": "2025",
            "doi": "10.5555/evidence",
            "url": "https://doi.org/10.5555/evidence",
            "abstract": "A short abstract.",
            "venue": "Evidence Journal",
            "provider": "OpenAlex",
            "type": "article",
            "is_oa": "true",
        }]
        sections = research.draft_sections("research methods", records, "fa")
        self.assertIn("10.5555/evidence", sections["evidence_synthesis"])
        self.assertIn("متن کامل", sections["limitations"])
        self.assertIn("Google Scholar", sections["search_strategy"])


if __name__ == "__main__":
    unittest.main()
