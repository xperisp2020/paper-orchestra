# ShirazTipsy Research Institute — Native Windows desktop

**نام فارسی:** پژوهشکده سرخوش شیرازی  
**English name:** ShirazTipsy Research Institute

The desktop application uses a custom deep-navy and gold visual identity, a bundled 3D-inspired emblem, native Windows executable/installer icons, a branded header, action icons, bilingual application branding, and an About dialog. The About dialog links to Telegram and Instagram at `@shiraztipsy` (`https://t.me/shiraztipsy` and `https://www.instagram.com/shiraztipsy/`). The executable target remains `yadavarifork.exe` for upgrade and packaging compatibility.

The primary desktop application is implemented in C++20 with Qt 6 Widgets. Windows portable is the first delivery target.

## Current implemented scope
- Persian and English startup, with RTL/LTR layout
- AI mode selection: no-API local workflow, free-tier provider category, or paid provider category
- Proposal and research workflow selection
- Proposal section editor backed by SQLite persistence, with atomic all-section AI drafting through configured Gemini/OpenAI/OpenRouter models
- Export to PDF, DOCX (Office Open XML), and paginated PNG images
- Print through the Qt print dialog
- Initial asynchronous Python CLI bridge for research runs, with streamed output, cancellation, environment-only credentials, and output redaction
- Existing project output files and folders are not overwritten
- Portable Windows packaging with Qt runtime deployment and a second smoke test against the deployed executable

**AI proposal drafting is available through the explicit free-tier or paid provider route; no-API mode remains a deterministic local workspace, not a generative model. Proposal generation writes no partial section set: all requested scientific sections must validate before a single SQLite transaction saves them. Student/supervisor details and administrative approvals remain outside AI generation. No live paid API request is performed by CI.\n\nNo-API mode is a deterministic local workspace, not a generative model.** The free-tier mode uses OpenRouter free model IDs only; its default is the multimodal NVIDIA Nemotron 3.5 Super VL free endpoint. It requires OPENROUTER_API_KEY and an explicit privacy acknowledgement. Current OpenRouter free limits are 20 requests per minute and 50 per day; providers may log or use submitted content, so do not send confidential data. Paid mode accepts Gemini, OpenAI, or OpenRouter model IDs and checks the matching environment credentials before launching the engine. The writer/literature model and reflection/refinement model are configurable; a blank reflection model inherits the writer model. Python engine source, templates, assets, requirements, CLI, and the runtime bootstrap script are included in the Windows portable ZIP and installer. The Python interpreter, Python packages, and LaTeX distribution are not bundled. The user creates an isolated Python environment with the packaged bootstrap script and selects its interpreter in the app. The CLI preflight checks installed distributions and both LaTeX executables before starting the agent pipeline. The non-plotting writer, literature, review, and refinement stages route through Gemini or OpenAI-compatible providers; OpenAI/OpenRouter literature discovery uses Semantic Scholar keyword search. PaperBanana plotting remains Gemini-only. Each run maintains an atomic run_manifest.json with a stable run ID, project ID, timestamps, lifecycle status, and current stage; the Qt workspace exposes a per-project run-history dialog. Diagnostics redact provider keys, and API keys are not stored by the application. No real provider API run is implied by a successful CI build.

## Proposal export
The export engine groups proposal sections into the 16-page sequence derived from the Shiraz Islamic Azad University proposal form. It is a structured editable draft, not an official university-issued form; signatures, approvals, and committee decisions must be completed by the authorized people. Long user-entered content can expand a page group onto additional physical pages.

DOCX uses Office Open XML, which is supported by Word 2010 and newer. PDF/PNG output uses Qt text layout. Verify Persian typography, pagination, and any university-specific revisions before formal submission.

## Build on Windows
Install Qt 6.12.0 (Widgets, Sql, PrintSupport) for MSVC 2022 x64 and CMake 3.25+. The institute PNG icon is bundled in the Qt resource file; a matching ICO is embedded into the Windows executable and installer without requiring QtSvg.

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
The CI pipeline produces both the portable ZIP and a per-user Inno Setup installer. The installer defaults to `%LOCALAPPDATA%\\Programs\\ShirazTipsy Research Institute`, so the application can write its local SQLite data without administrator rights. The `data` directory is explicitly preserved by uninstall; removing the application does not silently remove project databases.


## Preparing the Windows research runtime

The portable ZIP and installer include the Python research source, CLI, templates, assets, requirements file, and runtime bootstrap script. They do not bundle a Python interpreter, Python packages, or a LaTeX distribution.

1. Install Python 3.12 or newer.
2. Open PowerShell in the application directory (the default per-user installer path is under %LOCALAPPDATA%\Programs\Yadavarifork).
3. Create the isolated runtime outside the install directory so it survives application updates and uninstall:

```powershell
$python = (Get-Command python).Source
powershell -ExecutionPolicy Bypass -File .\scripts\setup_research_runtime.ps1 -PythonExecutable $python -RuntimeDirectory "$env:LOCALAPPDATA\Yadavarifork\research-runtime"
```

The script installs requirements-research.txt, runs pip check, verifies core imports, and checks for pdflatex and bibtex on PATH. It refuses to overwrite an existing runtime directory. If LaTeX tools are missing, it exits with code 3 after reporting the missing executables. Install MiKTeX or TeX Live for Windows and reopen the application.

Configure the selected provider in the process environment before launching the app:
- Free tier: OPENROUTER_API_KEY. The free-tier wizard defaults to nvidia/nemotron-3.5-super-vl:free and requires acknowledgement of third-party data processing.
- Gemini: GEMINI_API_KEY or GOOGLE_API_KEY, or Vertex AI project/location plus Application Default Credentials.
- OpenAI: OPENAI_API_KEY.

In the research dialog, the CLI script and LaTeX template folder default to the bundled source. Select the created venv's Scripts\python.exe. Leave the reflection model blank to reuse the writer model. The current PaperBanana plotting workflow remains Gemini-only and is not enabled by the desktop research dialog.
