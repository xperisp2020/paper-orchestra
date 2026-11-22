# Yadavarifork — Native Windows desktop

The primary desktop application is implemented in C++20 with Qt 6 Widgets. Windows portable is the first delivery target.

## Current implemented scope
- Persian and English startup, with RTL/LTR layout
- AI mode selection: no-API local workflow, free-tier provider category, or paid provider category
- Proposal and research workflow selection
- Proposal section editor backed by SQLite persistence
- Export to PDF, DOCX (Office Open XML), and paginated PNG images
- Print through the Qt print dialog
- Existing project output files and folders are not overwritten
- Portable Windows packaging with Qt runtime deployment and a second smoke test against the deployed executable

**No-API mode is a deterministic local workspace, not a generative model.** The free-tier and paid choices currently select workflow categories; provider authentication and live research-agent execution are not yet complete. The research workflow is not represented as completed AI research.

## Proposal export
The export engine groups proposal sections into the 16-page sequence derived from the Shiraz Islamic Azad University proposal form. It is a structured editable draft, not an official university-issued form; signatures, approvals, and committee decisions must be completed by the authorized people. Long user-entered content can expand a page group onto additional physical pages.

DOCX uses Office Open XML, which is supported by Word 2010 and newer. PDF/PNG output uses Qt text layout. Verify Persian typography, pagination, and any university-specific revisions before formal submission.

## Build on Windows
Install Qt 6.12.0 for MSVC 2022 x64 and CMake 3.25+.

```powershell
cmake -S app_cpp -B build -DCMAKE_PREFIX_PATH="$env:Qt6_DIR"
cmake --build build --config Release --parallel --target yadavarifork yadavarifork_smoke
ctest --test-dir build -C Release --output-on-failure
```

The smoke target is a console executable used only for diagnostics and is excluded from the portable ZIP. The shipped application is the GUI executable.

## Delivery gates
1. Portable Windows: build, smoke tests, runtime deployment, and artifact verification.
2. Installer: built with pinned Inno Setup and tested through silent install, launch, uninstall, and user-data preservation on a clean Windows runner.
3. Android: after both Windows distributions are stable.

A successful CI build is not a substitute for clean-machine acceptance, Word 2010 compatibility testing, or user review of the university form layout.


## Windows installer
The CI pipeline produces both the portable ZIP and a per-user Inno Setup installer. The installer defaults to `%LOCALAPPDATA%\\Programs\\Yadavarifork`, so the application can write its local SQLite data without administrator rights. The `data` directory is explicitly preserved by uninstall; removing the application does not silently remove project databases.
