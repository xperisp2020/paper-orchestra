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

    def test_discovery_uses_six_indexes_and_deduplicates_records(self):
        def openalex_record(i):
            return {"title": f"OpenAlex scholarly paper number {i}", "doi": f"10.1111/openalex{i}",
                    "publication_year": 2024, "authorships": [], "abstract_inverted_index": None}
        def crossref_record(i):
            return {"title": [f"Crossref scholarly paper number {i}"], "DOI": f"10.2222/crossref{i}",
                    "author": [], "published-print": {"date-parts": [[2023]]}, "container-title": ["Journal"]}
        def semantic_record(i):
            return {"title": f"Semantic Scholar paper number {i}", "authors": [], "year": 2022,
                    "abstract": "Abstract text.", "venue": "Semantic Journal",
                    "externalIds": {"DOI": f"10.3333/semantic{i}"}, "url": "https://example.org/paper",
                    "publicationTypes": ["JournalArticle"], "isOpenAccess": True}
        def europe_record(i):
            return {"title": f"Europe PMC scholarly article number {i}", "doi": f"10.4444/epmc{i}",
                    "authorString": "E. Author", "pubYear": "2021", "abstractText": "Abstract",
                    "journalTitle": "Europe Journal"}
        def datacite_record(i):
            return {"id": f"10.5555/datacite{i}", "attributes": {"doi": f"10.5555/datacite{i}",
                    "titles": [{"title": f"DataCite scholarly work number {i}"}],
                    "creators": [{"name": "D. Author"}], "publicationYear": 2020,
                    "publisher": "Data Publisher"}}
        responses = {
            "api.openalex.org": {"results": [openalex_record(i) for i in range(4)]},
            "api.crossref.org": {"message": {"items": [crossref_record(i) for i in range(4)]}},
            "api.semanticscholar.org": {"data": [semantic_record(i) for i in range(4)]},
            "www.ebi.ac.uk": {"resultList": {"result": [europe_record(i) for i in range(4)]}},
            "api.datacite.org": {"data": [datacite_record(i) for i in range(4)]},
        }
        requested = []
        def fake_request(url):
            requested.append(url)
            return responses[urlparse(url).netloc]
        atom = """<feed xmlns="http://www.w3.org/2005/Atom">
        <entry><id>https://arxiv.org/abs/2401.00001</id><title>arXiv scholarly paper number one</title>
        <published>2024-01-01T00:00:00Z</published><summary>Preprint abstract</summary>
        <author><name>A. Researcher</name></author></entry></feed>"""
        with patch.object(research, "request_json", side_effect=fake_request), \
             patch.object(research, "request_xml", return_value=atom):
            records, errors = research.discover("research methods", target=12, sleeper=lambda _: None)
        self.assertEqual(errors, [])
        self.assertEqual(len(records), 12)
        self.assertEqual({r["provider"] for r in records},
                         {"OpenAlex", "Crossref", "Semantic Scholar", "Europe PMC", "DataCite", "arXiv"})
        self.assertEqual(len({r["doi"] or r["title"] for r in records}), 12)
        self.assertEqual(len(requested), 5)
        self.assertEqual(records[-1]["provider"], "arXiv")

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
