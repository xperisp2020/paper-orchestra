import contextlib
import io
import json
import unittest
import urllib.error
from unittest.mock import Mock, patch

from scripts import provider_live_smoke as smoke


class ProviderLiveSmokeTests(unittest.TestCase):
    def run_capture(self, *args, **kwargs):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            status = smoke.run_provider(*args, **kwargs)
        return status, output.getvalue()

    def test_missing_credential_is_blocked_without_request(self):
        with patch.object(smoke, "_send_request") as send:
            status, output = self.run_capture(
                "openrouter",
                "nvidia/nemotron-3.5-super-vl:free",
                "",
                enabled=True,
            )
        self.assertEqual(status, "BLOCKED")
        self.assertIn("credential_missing", output)
        send.assert_not_called()

    def test_billable_provider_requires_explicit_opt_in(self):
        with patch.object(smoke, "_send_request") as send:
            status, output = self.run_capture(
                "openai", "gpt-4o-mini", "test-secret", enabled=True, allow_billable=False
            )
        self.assertEqual(status, "BLOCKED")
        self.assertIn("billable_opt_in_required", output)
        self.assertNotIn("test-secret", output)
        send.assert_not_called()

    def test_disabled_provider_is_blocked_without_request(self):
        with patch.object(smoke, "_send_request") as send:
            status, output = self.run_capture(
                "gemini", "gemini-3-flash-preview", "test-secret", enabled=False,
                allow_billable=True
            )
        self.assertEqual(status, "BLOCKED")
        self.assertIn("not_enabled", output)
        send.assert_not_called()

    def test_openrouter_rejects_non_free_model(self):
        with patch.object(smoke, "_send_request") as send:
            status, output = self.run_capture(
                "openrouter", "some/provider:paid", "test-secret", enabled=True
            )
        self.assertEqual(status, "BLOCKED")
        self.assertIn("model_is_not_marked_free", output)
        send.assert_not_called()

    def test_success_uses_exactly_one_request_and_prints_metadata_only(self):
        payload = {"choices": [{"message": {"content": "OK"}}]}
        with patch.object(smoke, "_send_request", return_value=(payload, 200)) as send:
            status, output = self.run_capture(
                "openrouter",
                "nvidia/nemotron-3.5-super-vl:free",
                "private-api-secret",
                enabled=True,
            )
        self.assertEqual(status, "LIVE PASS")
        self.assertIn("http_status=200", output)
        self.assertNotIn("private-api-secret", output)
        send.assert_called_once()

    def test_http_429_does_not_log_body_or_secret(self):
        secret = "private-api-secret"
        error = urllib.error.HTTPError(
            "https://openrouter.ai/api/v1/chat/completions",
            429,
            "rate limited",
            {},
            io.BytesIO(b"provider diagnostic contains private-api-secret"),
        )
        with patch.object(smoke, "_send_request", side_effect=error):
            status, output = self.run_capture(
                "openrouter",
                "nvidia/nemotron-3.5-super-vl:free",
                secret,
                enabled=True,
            )
        self.assertEqual(status, "LIVE FAIL")
        self.assertIn("http_status=429", output)
        self.assertNotIn(secret, output)
        self.assertNotIn("provider diagnostic", output)
        self.assertNotIn("rate limited", output)

    def test_timeout_does_not_log_exception_message(self):
        with patch.object(smoke, "_send_request", side_effect=TimeoutError("contains-private-secret")):
            status, output = self.run_capture(
                "openrouter",
                "nvidia/nemotron-3.5-super-vl:free",
                "private-api-secret",
                enabled=True,
            )
        self.assertEqual(status, "LIVE FAIL")
        self.assertIn("error_type=TimeoutError", output)
        self.assertNotIn("contains-private-secret", output)
        self.assertNotIn("private-api-secret", output)

    def test_request_keeps_credentials_out_of_url_and_body(self):
        captured = {}

        class FakeResponse:
            status = 200

            def __enter__(self):
                return self

            def __exit__(self, *args):
                return False

            def read(self, size):
                return json.dumps({"choices": [{"message": {"content": "OK"}}]}).encode()

        def fake_urlopen(request, timeout):
            captured["url"] = request.full_url
            captured["headers"] = dict(request.header_items())
            captured["body"] = request.data.decode("utf-8")
            captured["timeout"] = timeout
            return FakeResponse()

        with patch.object(smoke.urllib.request, "urlopen", side_effect=fake_urlopen) as opener:
            payload, status = smoke._send_request(
                "openrouter", "nvidia/nemotron-3.5-super-vl:free", "private-api-secret"
            )
        self.assertEqual(status, 200)
        self.assertTrue(smoke._has_text(payload, "openrouter"))
        request_body = json.loads(captured["body"])
        self.assertEqual(request_body["reasoning"], {"effort": "none"})
        self.assertEqual(request_body["max_tokens"], 16)
        self.assertNotIn("private-api-secret", captured["url"])
        self.assertNotIn("private-api-secret", captured["body"])
        self.assertEqual(captured["headers"]["Authorization"], "Bearer private-api-secret")
        self.assertEqual(captured["timeout"], smoke.REQUEST_TIMEOUT_SECONDS)
        opener.assert_called_once()


if __name__ == "__main__":
    unittest.main()
