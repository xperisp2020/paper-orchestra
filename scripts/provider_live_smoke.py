#!/usr/bin/env python3
"""One-request provider smoke checks with secret-safe, non-retrying diagnostics.

Gemini and OpenAI requests require explicit billable-smoke opt-in.
OpenRouter is restricted to model IDs that are explicitly free.
"""
from __future__ import annotations

import json
import os
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Any

REQUEST_TIMEOUT_SECONDS = 20
MAX_RESPONSE_BYTES = 65536
DEFAULT_MODELS = {
    "gemini": "gemini-3-flash-preview",
    "openai": "gpt-4o-mini",
    "openrouter": "nvidia/nemotron-3.5-super-vl:free",
}
ENDPOINTS = {
    "openai": "https://api.openai.com/v1/chat/completions",
    "openrouter": "https://openrouter.ai/api/v1/chat/completions",
}


def env_flag(name: str) -> bool:
    return os.environ.get(name, "").strip().lower() in {"1", "true", "yes", "on"}


def _valid_model_name(model: str) -> bool:
    return bool(model) and len(model) <= 200 and not any(ord(ch) < 32 for ch in model)


def _has_text(payload: Any, provider: str) -> bool:
    if not isinstance(payload, dict):
        return False
    if provider == "gemini":
        candidates = payload.get("candidates")
        if not isinstance(candidates, list):
            return False
        for candidate in candidates:
            if not isinstance(candidate, dict):
                continue
            content = candidate.get("content")
            parts = content.get("parts") if isinstance(content, dict) else None
            if isinstance(parts, list) and any(
                isinstance(part, dict)
                and isinstance(part.get("text"), str)
                and part["text"].strip()
                for part in parts
            ):
                return True
        return False

    choices = payload.get("choices")
    if not isinstance(choices, list) or not choices:
        return False
    message = choices[0].get("message") if isinstance(choices[0], dict) else None
    content = message.get("content") if isinstance(message, dict) else None
    if isinstance(content, str):
        return bool(content.strip())
    if isinstance(content, list):
        return any(
            isinstance(item, dict)
            and isinstance(item.get("text"), str)
            and item["text"].strip()
            for item in content
        )
    return False


def _send_request(provider: str, model: str, api_key: str) -> tuple[dict[str, Any], int]:
    """Send exactly one HTTP request. Never place credentials in a URL or body."""
    if not _valid_model_name(model):
        raise ValueError("invalid model identifier")

    if provider == "gemini":
        model_path = urllib.parse.quote(model, safe="._-")
        url = (
            "https://generativelanguage.googleapis.com/v1beta/models/"
            f"{model_path}:generateContent"
        )
        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json",
            "x-goog-api-key": api_key,
        }
        body = {
            "contents": [{"parts": [{"text": "Reply with exactly OK."}]}],
            "generationConfig": {"temperature": 0, "maxOutputTokens": 8},
        }
    elif provider in {"openai", "openrouter"}:
        if provider == "openrouter" and not (
            model == "openrouter/free" or model.lower().endswith(":free")
        ):
            raise ValueError("OpenRouter smoke accepts free model IDs only")
        url = ENDPOINTS[provider]
        headers = {
            "Content-Type": "application/json",
            "Accept": "application/json",
            "Authorization": f"Bearer {api_key}",
        }
        body = {
            "model": model,
            "messages": [{"role": "user", "content": "Reply with exactly OK."}],
            "temperature": 0,
            "max_tokens": 8,
        }
    else:
        raise ValueError("unsupported provider")

    request = urllib.request.Request(
        url,
        data=json.dumps(body, separators=(",", ":")).encode("utf-8"),
        headers=headers,
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=REQUEST_TIMEOUT_SECONDS) as response:
        raw = response.read(MAX_RESPONSE_BYTES + 1)
        status = int(getattr(response, "status", 200))
    if len(raw) > MAX_RESPONSE_BYTES:
        raise ValueError("provider response exceeded the smoke size limit")
    decoded = json.loads(raw.decode("utf-8"))
    if not isinstance(decoded, dict):
        raise ValueError("provider response is not a JSON object")
    return decoded, status


def _http_status(error: Exception) -> int | None:
    status = getattr(error, "code", None)
    if status is None:
        status = getattr(error, "status_code", None)
    try:
        return int(status) if status is not None else None
    except (TypeError, ValueError):
        return None


def run_provider(
    provider: str,
    model: str,
    api_key: str,
    *,
    enabled: bool,
    allow_billable: bool = False,
) -> str:
    """Run one provider probe and print metadata only, never exception text."""
    provider_label = {"gemini": "Gemini", "openai": "OpenAI", "openrouter": "OpenRouter Free"}.get(
        provider, "Unknown"
    )
    if not api_key or not api_key.strip():
        print(f"{provider_label}: BLOCKED reason=credential_missing")
        return "BLOCKED"
    if not enabled:
        print(f"{provider_label}: BLOCKED reason=not_enabled")
        return "BLOCKED"
    if provider in {"gemini", "openai"} and not allow_billable:
        print(f"{provider_label}: BLOCKED reason=billable_opt_in_required")
        return "BLOCKED"
    if not _valid_model_name(model):
        print(f"{provider_label}: BLOCKED reason=invalid_model")
        return "BLOCKED"
    if provider == "openrouter" and not (
        model == "openrouter/free" or model.lower().endswith(":free")
    ):
        print(f"{provider_label}: BLOCKED reason=model_is_not_marked_free")
        return "BLOCKED"

    started = time.monotonic()
    try:
        payload, http_status = _send_request(provider, model, api_key)
        if http_status < 200 or http_status >= 300 or not _has_text(payload, provider):
            elapsed_ms = int((time.monotonic() - started) * 1000)
            print(
                f"{provider_label}: LIVE FAIL model={model} "
                f"http_status={http_status} duration_ms={elapsed_ms} "
                "reason=empty_or_unexpected_response"
            )
            return "LIVE FAIL"
    except Exception as error:
        elapsed_ms = int((time.monotonic() - started) * 1000)
        status = _http_status(error)
        status_text = f" http_status={status}" if status is not None else ""
        print(
            f"{provider_label}: LIVE FAIL model={model}{status_text} "
            f"duration_ms={elapsed_ms} error_type={type(error).__name__}"
        )
        return "LIVE FAIL"

    elapsed_ms = int((time.monotonic() - started) * 1000)
    print(f"{provider_label}: LIVE PASS model={model} http_status={http_status} duration_ms={elapsed_ms}")
    return "LIVE PASS"


def main() -> int:
    billable_opt_in = env_flag("ALLOW_BILLABLE_PROVIDER_SMOKES")
    results = [
        run_provider(
            "gemini",
            os.environ.get("GEMINI_SMOKE_MODEL", DEFAULT_MODELS["gemini"]),
            os.environ.get("GEMINI_API_KEY", ""),
            enabled=env_flag("RUN_GEMINI_LIVE"),
            allow_billable=billable_opt_in,
        ),
        run_provider(
            "openai",
            os.environ.get("OPENAI_SMOKE_MODEL", DEFAULT_MODELS["openai"]),
            os.environ.get("OPENAI_API_KEY", ""),
            enabled=env_flag("RUN_OPENAI_LIVE"),
            allow_billable=billable_opt_in,
        ),
        run_provider(
            "openrouter",
            os.environ.get("OPENROUTER_SMOKE_MODEL", DEFAULT_MODELS["openrouter"]),
            os.environ.get("OPENROUTER_API_KEY", ""),
            enabled=env_flag("RUN_OPENROUTER_FREE_LIVE"),
            allow_billable=False,
        ),
    ]
    return 1 if "LIVE FAIL" in results else 0


if __name__ == "__main__":
    raise SystemExit(main())
