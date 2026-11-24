import unittest
from unittest.mock import Mock, patch

from utils.crossref_utils import verify_doi_with_crossref


class CrossrefVerificationTests(unittest.TestCase):
    def tearDown(self):
        verify_doi_with_crossref.cache_clear()

    def response(self, status, payload=None):
        result = Mock()
        result.status_code = status
        result.json.return_value = payload or {}
        if status >= 400 and status != 404:
            import requests
            result.raise_for_status.side_effect = requests.HTTPError("http error")
        return result

    @patch("utils.crossref_utils.requests.get")
    def test_verified_doi(self, get):
        get.return_value = self.response(200, {"message": {"DOI": "10.1000/ABC"}})
        self.assertEqual(verify_doi_with_crossref("https://doi.org/10.1000/ABC")["status"], "verified")
        self.assertIn("/10.1000%2FABC", get.call_args.args[0])

    @patch("utils.crossref_utils.requests.get")
    def test_crossref_404_is_not_found_not_global_invalidity(self, get):
        get.return_value = self.response(404)
        result = verify_doi_with_crossref("10.9999/elsewhere")
        self.assertEqual(result["status"], "not_found")
        self.assertIn("may be registered elsewhere", result["message"])

    @patch("utils.crossref_utils.requests.get")
    def test_rate_limit_and_server_error_are_unavailable(self, get):
        get.return_value = self.response(429)
        self.assertEqual(verify_doi_with_crossref("10.1000/rate")["status"], "unavailable")
        get.return_value = self.response(503)
        self.assertEqual(verify_doi_with_crossref("10.1000/server")["status"], "unavailable")

    @patch("utils.crossref_utils.requests.get", side_effect=__import__("requests").Timeout("timeout"))
    def test_network_failure_is_unavailable(self, get):
        self.assertEqual(verify_doi_with_crossref("10.1000/timeout")["status"], "unavailable")

    @patch("utils.crossref_utils.requests.get")
    def test_mismatched_response_is_unavailable(self, get):
        get.return_value = self.response(200, {"message": {"DOI": "10.1000/other"}})
        self.assertEqual(verify_doi_with_crossref("10.1000/requested")["status"], "unavailable")

    def test_malformed_doi_is_invalid_without_network(self):
        with patch("utils.crossref_utils.requests.get") as get:
            self.assertEqual(verify_doi_with_crossref("not-a-doi")["status"], "invalid")
            get.assert_not_called()


if __name__ == "__main__":
    unittest.main()
