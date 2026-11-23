"""Redact provider credentials from diagnostic messages before logging."""

import os
import re

_SECRET_ENV_VARS = (
    "GEMINI_API_KEY",
    "GOOGLE_API_KEY",
    "GOOGLE_GENAI_API_KEY",
    "OPENAI_API_KEY",
    "OPENROUTER_API_KEY",
    "ANTHROPIC_API_KEY",
    "COHERE_API_KEY",
    "MISTRAL_API_KEY",
    "SEMANTIC_SCHOLAR_API_KEY",
)


def redact_secrets(value) -> str:
    """Mask configured provider keys and common authorization fields."""
    message = str(value)
    for variable in _SECRET_ENV_VARS:
        secret = os.environ.get(variable)
        if secret:
            message = message.replace(secret, "[REDACTED]")
    message = re.sub(
        r"(?i)(authorization\s*[:=]\s*bearer\s+)[^\s,;\"']+",
        r"\1[REDACTED]",
        message,
    )
    message = re.sub(
        r"(?i)((?:x-api-key|api[_-]?key|access[_-]?token|client[_-]?secret)\s*[:=]\s*)[^\s,;\"']+",
        r"\1[REDACTED]",
        message,
    )
    return message
