"""Best-effort Crossref DOI check using only the Python standard library."""
from functools import lru_cache
from urllib.parse import quote
from urllib.request import Request, urlopen
from urllib.error import HTTPError, URLError
import json
import os
import socket

from utils.doi_utils import normalize_doi


@lru_cache(maxsize=4096)
def verify_doi_with_crossref(doi, timeout=6):
    """Return verified/not_found/unavailable/invalid without destructive edits.

    Crossref does not cover every DOI registrant. A 404 means only that Crossref
    did not find this DOI; network errors and rate limits are unavailable.
    """
    normalized = normalize_doi(doi)
    if not normalized:
        return {"status": "invalid", "doi": None, "message": "Malformed DOI"}

    headers = {
        "User-Agent": "PaperOrchestra/1.0 (DOI metadata verification)",
        "Accept": "application/json",
    }
    mailto = os.environ.get("CROSSREF_MAILTO", "").strip()
    if mailto:
        headers["User-Agent"] += " mailto:" + mailto

    request = Request(
        "https://api.crossref.org/works/" + quote(normalized, safe=""),
        headers=headers,
    )
    try:
        with urlopen(request, timeout=timeout) as response:
            payload = json.loads(response.read().decode("utf-8"))
        message = payload.get("message", {}) if isinstance(payload, dict) else {}
        returned = normalize_doi(message.get("DOI")) if isinstance(message, dict) else None
        if returned and returned.casefold() == normalized.casefold():
            return {"status": "verified", "doi": normalized, "message": "DOI confirmed by Crossref"}
        return {"status": "unavailable", "doi": normalized,
                "message": "Crossref response did not contain a matching DOI"}
    except HTTPError as exc:
        if exc.code == 404:
            return {"status": "not_found", "doi": normalized,
                    "message": "DOI not found in Crossref; it may be registered elsewhere"}
        return {"status": "unavailable", "doi": normalized,
                "message": "Crossref returned HTTP {}".format(exc.code)}
    except (URLError, socket.timeout, TimeoutError, OSError, ValueError, TypeError):
        return {"status": "unavailable", "doi": normalized,
                "message": "Crossref could not be reached or returned invalid metadata"}


def safe_crossref_verification_status(doi, enabled=False):
    """Return an allowed Crossref status without allowing verification to block research.

    Disabled checks and missing/malformed DOIs return None. Any unexpected
    registry response or exception returns unavailable; the caller must retain
    the original DOI in its citation records.
    """
    normalized = normalize_doi(doi)
    if not enabled or not normalized:
        return None
    try:
        result = verify_doi_with_crossref(normalized)
    except Exception:
        return "unavailable"
    if not isinstance(result, dict):
        return "unavailable"
    status = result.get("status")
    if status not in {"verified", "not_found", "unavailable", "invalid"}:
        return "unavailable"
    return status
