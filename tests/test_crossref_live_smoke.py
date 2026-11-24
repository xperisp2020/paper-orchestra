"""Regression tests for the Crossref live-smoke argument normalization."""
import unittest

from scripts.crossref_live_smoke import is_verified_result


class CrossrefLiveSmokeTests(unittest.TestCase):
    def test_verified_url_prefixed_doi_matches_normalized_api_response(self):
        self.assertTrue(
            is_verified_result(
                "https://doi.org/10.1038/nature12373",
                {"status": "verified", "doi": "10.1038/nature12373"},
            )
        )

    def test_verified_doi_matching_is_case_insensitive(self):
        self.assertTrue(
            is_verified_result(
                "10.1000/ABC",
                {"status": "verified", "doi": "10.1000/abc"},
            )
        )

    def test_not_found_and_mismatched_doi_do_not_pass(self):
        self.assertFalse(
            is_verified_result(
                "10.1000/ABC",
                {"status": "not_found", "doi": "10.1000/ABC"},
            )
        )
        self.assertFalse(
            is_verified_result(
                "10.1000/ABC",
                {"status": "verified", "doi": "10.1000/OTHER"},
            )
        )

    def test_malformed_or_missing_doi_does_not_pass(self):
        self.assertFalse(is_verified_result("not-a-doi", {"status": "verified", "doi": None}))
        self.assertFalse(is_verified_result("10.1000/ABC", None))


if __name__ == "__main__":
    unittest.main()
