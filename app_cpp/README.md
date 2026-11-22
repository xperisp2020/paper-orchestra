# Yadavarifork — C++ portable desktop

This is the primary native desktop implementation, targeting Windows portable first.

## Stack
- C++20
- Qt 6.12.0 (Qt Widgets for the initial wizard shell)
- CMake 3.25+

## Build

```powershell
cmake -S app_cpp -B build -DCMAKE_PREFIX_PATH="$env:Qt6_DIR"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Install Qt 6.12.0 with the matching MSVC 2022 kit and Qt Widgets module. The GitHub Actions workflow installs the upstream aqtinstall revision containing Qt 6.11+ repository-layout support. It deliberately omits an explicit module list: Qt's base desktop package is implicit, while the previous explicit `qtbase` module selector was rejected by the 6.11 repository metadata. The workflow then builds on Windows and runs a UI smoke test that checks language direction and wizard navigation.

The CI workflow pins the aqtinstall revision that added Qt 6.11+ repository-layout support and validates configure, compile, and the UI smoke test against Qt 6.12.0 LTS. It then runs `windeployqt`, re-runs the smoke test against the deployed executable, and uploads a ZIP artifact. This is an initial portable-directory packaging check, not yet a clean-machine acceptance test.

## Current scope
The first increment implements the language, AI mode, and workflow selection screens. It does not yet implement proposal generation, research agents, local persistence, document export, printing, or portable packaging. The no-API mode is a local workflow and does not claim to provide a generative model.
