import io
import json
import unittest
from urllib.error import HTTPError, URLError
from unittest.mock import patch

from utils.crossref_utils import verify_doi_with_crossref


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


if __name__ == "__main__":
    unittest.main()
