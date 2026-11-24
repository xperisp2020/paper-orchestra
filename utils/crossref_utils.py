"""Best-effort Crossref DOI check using only the Python standard library."""
from functools import lru_cache
from urllib.parse import quote
from urllib.request import Request, urlopen
from urllib.error import HTTPError
import json
import math
import os


from utils.doi_utils import normalize_doi


class _TransientCrossrefResult(Exception):
    """Internal signal that tells the LRU cache not to retain a transient failure."""

    def __init__(self, result):
        super().__init__("Transient Crossref lookup failure")
        self.result = result


def _lookup_crossref(normalized, timeout, mailto):
    headers = {
        "User-Agent": "PaperOrchestra/1.0 (DOI metadata verification)",
        "Accept": "application/json",
    }
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
            return {
                "status": "verified",
                "doi": normalized,
                "message": "DOI confirmed as registered in Crossref",
            }
        return {
            "status": "unavailable",
            "doi": normalized,
            "message": "Crossref response did not contain a matching DOI",
        }
    except HTTPError as exc:
        if exc.code == 404:
            return {
                "status": "not_found",
                "doi": normalized,
                "message": "DOI not found in Crossref; it may be registered elsewhere",
            }
        return {
            "status": "unavailable",
            "doi": normalized,
            "message": "Crossref returned HTTP {}".format(exc.code),
        }
    except Exception:
        # A registry/network/decoder failure must never invalidate the source DOI.
        return {
            "status": "unavailable",
            "doi": normalized,
            "message": "Crossref could not be reached or returned invalid metadata",
        }


@lru_cache(maxsize=4096)
def _cached_crossref_lookup(normalized, timeout, mailto):
    result = _lookup_crossref(normalized, timeout, mailto)
    if result["status"] == "unavailable":
        # Raise instead of returning: functools.lru_cache never caches exceptions,
        # so a later retry can recover after a transient timeout/429/5xx.
        raise _TransientCrossrefResult(result)
    return result


def verify_doi_with_crossref(doi, timeout=6):
    """Return verified/not_found/unavailable/invalid without destructive edits.

    A 404 means only that Crossref did not find this DOI. Transient failures are
    deliberately not cached; verified/not_found responses are cached per process.
    """
    normalized = normalize_doi(doi)
    if not normalized:
        return {"status": "invalid", "doi": None, "message": "Malformed DOI"}

    try:
        timeout_value = float(timeout)
    except (TypeError, ValueError):
        return {
            "status": "unavailable",
            "doi": normalized,
            "message": "Invalid Crossref timeout configuration",
        }
    if not math.isfinite(timeout_value) or timeout_value <= 0:
        return {
            "status": "unavailable",
            "doi": normalized,
            "message": "Invalid Crossref timeout configuration",
        }

    mailto = os.environ.get("CROSSREF_MAILTO", "").strip()
    try:
        cached_result = _cached_crossref_lookup(normalized, timeout_value, mailto)
    except _TransientCrossrefResult as transient:
        cached_result = transient.result
    # The LRU stores mutable dictionaries. Return a copy so callers cannot
    # accidentally corrupt the process-wide cached result for later papers.
    return dict(cached_result)


# Keep the small testing/diagnostic cache-reset interface on the public function.
verify_doi_with_crossref.cache_clear = _cached_crossref_lookup.cache_clear


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
