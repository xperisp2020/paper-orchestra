"""Small, dependency-free DOI normalization helpers."""
import re
from typing import Optional


def normalize_doi(value: Optional[str]) -> Optional[str]:
    """Normalize a DOI supplied by scholarly metadata; reject malformed values."""
    if not isinstance(value, str):
        return None
    doi = value.strip()
    doi = re.sub(r"^(?:https?://)?(?:dx\.)?doi\.org/", "", doi, flags=re.IGNORECASE)
    doi = re.sub(r"^doi\s*:\s*", "", doi, flags=re.IGNORECASE).strip()
    doi = doi.rstrip(".,;")
    if not re.fullmatch(r"10\.\d{4,9}/[-._;()/:A-Z0-9]+", doi, flags=re.IGNORECASE):
        return None
    return doi
