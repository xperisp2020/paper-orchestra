"""One-request live smoke test for the public Crossref works endpoint."""
import argparse
import sys
from pathlib import Path

# Running this file directly sets sys.path[0] to scripts/, not the repository root.
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from utils.crossref_utils import verify_doi_with_crossref
from utils.doi_utils import normalize_doi


def is_verified_result(requested_doi, result) -> bool:
    """Match verified metadata against a normalized requested DOI."""
    requested = normalize_doi(requested_doi)
    returned = normalize_doi(result.get("doi")) if isinstance(result, dict) else None
    return bool(
        isinstance(result, dict)
        and result.get("status") == "verified"
        and requested
        and returned
        and returned.casefold() == requested.casefold()
    )


DEFAULT_DOI = "10.1038/nature12373"


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Verify one public DOI against the live Crossref API."
    )
    parser.add_argument("--doi", default=DEFAULT_DOI)
    parser.add_argument("--timeout", type=float, default=8.0)
    args = parser.parse_args()

    result = verify_doi_with_crossref(args.doi, timeout=args.timeout)
    status = result.get("status")
    returned_doi = result.get("doi")

    if is_verified_result(args.doi, result):
        print("Crossref live smoke PASS: status=verified doi={}".format(returned_doi))
        return 0

    # Only public DOI/status metadata is emitted; no headers or environment secrets.
    print(
        "Crossref live smoke BLOCKED/FAILED: status={} doi={}".format(
            status or "unknown", returned_doi or "unavailable"
        ),
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
