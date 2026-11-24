"""Best-effort Crossref DOI existence check; never treats outages as invalid DOIs."""
from functools import lru_cache
from urllib.parse import quote
import os
import requests

from utils.doi_utils import normalize_doi


@lru_cache(maxsize=4096)
def verify_doi_with_crossref(doi, timeout=6):
    """Return verified, not_found, unavailable, or invalid without destructive edits.

    Crossref is an independent metadata source, not an authority for every DOI
    (e.g. non-Crossref registrants). Network errors and rate limits are
    explicitly reported as unavailable.
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

    try:
        response = requests.get(
            "https://api.crossref.org/works/" + quote(normalized, safe=""),
            headers=headers,
            timeout=timeout,
        )
        if response.status_code == 404:
            return {"status": "not_found", "doi": normalized,
                    "message": "DOI not found in Crossref; it may be registered elsewhere"}
        if response.status_code == 429 or response.status_code >= 500:
            return {"status": "unavailable", "doi": normalized,
                    "message": "Crossref temporarily unavailable"}
        response.raise_for_status()
        payload = response.json()
        message = payload.get("message", {}) if isinstance(payload, dict) else {}
        returned = normalize_doi(message.get("DOI")) if isinstance(message, dict) else None
        if returned and returned.casefold() == normalized.casefold():
            return {"status": "verified", "doi": normalized, "message": "DOI confirmed by Crossref"}
        return {"status": "unavailable", "doi": normalized,
                "message": "Crossref response did not contain a matching DOI"}
    except (requests.RequestException, ValueError, TypeError):
        return {"status": "unavailable", "doi": normalized,
                "message": "Crossref could not be reached or returned invalid metadata"}
