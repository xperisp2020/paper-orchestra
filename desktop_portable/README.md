# Yadavarifork — Windows Portable (development)

This is the first implementation track. Only the portable Windows build is in scope until its acceptance gate passes.

## Development baseline
- Python 3.15 (use the latest available 3.15.x security/maintenance release)
- PySide6 6.12.0
- PyInstaller 6.22.3 for packaging evaluation
- SQLite from Python's standard library

Exact dependency pins are intentional for reproducible builds; refresh them only after compatibility and regression checks.

## Run from source
```powershell
py -3.15 -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install --upgrade pip
python -m pip install -e .
python -m yadavarifork.app
```

## Current implementation scope
- Language selection (Persian/English)
- AI mode selection with explicit limitations
- Workflow selection (proposal/research)
- Proposal section editing and local SQLite persistence
- No network calls and no API key collection in this first slice

The no-API mode is a deterministic local workspace, not a claim of generative AI. Research-agent integration, source discovery/verification, document exports, packaging and full QA are not implemented yet.
