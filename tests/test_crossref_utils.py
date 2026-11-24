import io
import json
import unittest
from urllib.error import HTTPError, URLError
from unittest.mock import patch

from utils.crossref_utils import verify_doi_with_crossref, safe_crossref_verification_status


class FakeResponse:
    def __init__(self, payload):
        self.payload = payload

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return False

    def read(self):
        return json.dumps(self.payload).encode("utf-8")


class CrossrefVerificationTests(unittest.TestCase):
    def tearDown(self):
        verify_doi_with_crossref.cache_clear()

    @patch("utils.crossref_utils.urlopen")
    def test_verified_doi(self, get):
        get.return_value = FakeResponse({"message": {"DOI": "10.1000/ABC"}})
        result = verify_doi_with_crossref("https://doi.org/10.1000/ABC")
        self.assertEqual(result["status"], "verified")
        self.assertIn("/10.1000%2FABC", get.call_args.args[0].full_url)

    @patch("utils.crossref_utils.urlopen", side_effect=HTTPError("url", 404, "missing", {}, None))
    def test_crossref_404_is_not_found_not_global_invalidity(self, get):
        result = verify_doi_with_crossref("10.9999/elsewhere")
        self.assertEqual(result["status"], "not_found")
        self.assertIn("may be registered elsewhere", result["message"])

    @patch("utils.crossref_utils.urlopen", side_effect=HTTPError("url", 429, "limited", {}, None))
    def test_rate_limit_is_unavailable(self, get):
        self.assertEqual(verify_doi_with_crossref("10.1000/rate")["status"], "unavailable")

    @patch("utils.crossref_utils.urlopen", side_effect=HTTPError("url", 503, "server", {}, None))
    def test_server_error_is_unavailable(self, get):
        self.assertEqual(verify_doi_with_crossref("10.1000/server")["status"], "unavailable")

    @patch("utils.crossref_utils.urlopen", side_effect=URLError("offline"))
    def test_network_failure_is_unavailable(self, get):
        self.assertEqual(verify_doi_with_crossref("10.1000/offline")["status"], "unavailable")

    @patch("utils.crossref_utils.urlopen")
    def test_mismatched_response_is_unavailable(self, get):
        get.return_value = FakeResponse({"message": {"DOI": "10.1000/other"}})
        self.assertEqual(verify_doi_with_crossref("10.1000/requested")["status"], "unavailable")

    def test_malformed_doi_is_invalid_without_network(self):
        with patch("utils.crossref_utils.urlopen") as get:
            self.assertEqual(verify_doi_with_crossref("not-a-doi")["status"], "invalid")
            get.assert_not_called()

    @patch("utils.crossref_utils.verify_doi_with_crossref")
    def test_safe_status_is_disabled_without_explicit_opt_in(self, verify):
        self.assertIsNone(safe_crossref_verification_status("10.1000/disabled"))
        self.assertIsNone(safe_crossref_verification_status("10.1000/disabled", enabled=False))
        verify.assert_not_called()

    @patch("utils.crossref_utils.verify_doi_with_crossref", return_value={"status": "verified"})
    def test_safe_status_passes_through_verified(self, verify):
        self.assertEqual(
            safe_crossref_verification_status("https://doi.org/10.1000/checked", enabled=True),
            "verified",
        )
        verify.assert_called_once_with("10.1000/checked")

    @patch("utils.crossref_utils.verify_doi_with_crossref", return_value={"status": "not_found"})
    def test_safe_status_preserves_crossref_specific_not_found(self, verify):
        self.assertEqual(
            safe_crossref_verification_status("10.9999/other-registry", enabled=True),
            "not_found",
        )

    @patch("utils.crossref_utils.verify_doi_with_crossref", return_value={"status": "unexpected"})
    def test_safe_status_maps_unrecognized_response_to_unavailable(self, verify):
        self.assertEqual(
            safe_crossref_verification_status("10.1000/unexpected", enabled=True),
            "unavailable",
        )

    @patch("utils.crossref_utils.verify_doi_with_crossref", side_effect=RuntimeError("offline"))
    def test_safe_status_swallows_unexpected_failure(self, verify):
        self.assertEqual(
            safe_crossref_verification_status("10.1000/offline", enabled=True),
            "unavailable",
        )

    @patch("utils.crossref_utils.verify_doi_with_crossref")
    def test_safe_status_rejects_malformed_doi_without_network(self, verify):
        self.assertIsNone(safe_crossref_verification_status("not-a-doi", enabled=True))
        verify.assert_not_called()


if __name__ == "__main__":
    unittest.main()
