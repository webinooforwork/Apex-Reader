# APEX OFFICE READER

APEX is a native C++20 Windows document workspace developed from Termux and built for Windows x64 with GitHub Actions. The repository is the source of truth: https://github.com/webinooforwork/Apex-Reader

## Current state

This is an early foundation, not a universal reader yet. The Qt 6 desktop shell currently opens UTF-8 plain text (`.txt`, `.log`, `.md`, `.csv`, `.tsv`) in read-only mode and reports word count. Markdown, CSV, and TSV are shown as text, not interpreted. The capability registry labels formats without a connected engine as unsupported, including PDF, Office, and OpenDocument.

See [FEATURE_STATUS.md](FEATURE_STATUS.md) for the implementation ledger. No PDF/OOXML/OCR/AI engine is bundled yet.

## Build

- Termux: `sh scripts/termux_setup.sh`, then `APEX_JOBS=2 sh scripts/termux_build.sh` (requires a Termux Qt 6 package providing CMake config files).
- Windows x64: push to GitHub to run `.github/workflows/windows-build.yml`; download the workflow artifact. Qt's MSVC build toolchain is installed on the runner, not on Android.
- Linux development: install CMake, Ninja, and Qt 6 Widgets, then run `sh scripts/run_tests.sh` (currently this builds; no automated test cases are present yet).

See [BUILDING.md](BUILDING.md) and [DEPENDENCIES.md](DEPENDENCIES.md).
