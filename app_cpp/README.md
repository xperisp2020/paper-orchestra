# Yadavarifork — C++ portable desktop

This is the primary native desktop implementation, targeting Windows portable first.

## Stack
- C++20
- Qt 6.12 LTS (Qt Widgets for the initial wizard shell)
- CMake 3.25+

## Build

```powershell
cmake -S app_cpp -B build -DCMAKE_PREFIX_PATH="$env:Qt6_DIR"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Install Qt 6.12 with the matching MSVC 2022 kit and Qt Widgets module. The GitHub Actions workflow builds the app on Windows and runs the smoke test.

## Current scope
The first increment implements the language, AI mode, and workflow selection screens. It does not yet implement proposal generation, research agents, local persistence, document export, printing, or portable packaging. The no-API mode is a local workflow and does not claim to provide a generative model.
