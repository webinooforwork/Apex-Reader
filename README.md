# APEX OFFICE READER

APEX is a native C++20 Windows document workspace developed from Termux and built for Windows x64 with GitHub Actions. The repository is the source of truth: https://github.com/webinooforwork/Apex-Reader

## Current state

This is an early release and the universal workspace is being built in stages. The Qt 6 desktop app reads UTF-8 text (`.txt`, `.log`, `.md`, `.csv`, `.tsv`) and, when Qt PDF is installed, uses PDFium through Qt PDF for PDF viewing. PDF controls include continuous or single-page mode, fit/custom zoom, full-text search with highlights, search-result navigation, password prompts, and text export. The text-family reader is read-only; Markdown, CSV, and TSV are displayed as text rather than interpreted. DOCX uses a bounded ZIP/XML extractor for paragraphs, heading styles, and table-cell text, without page-layout rendering. PDF is listed as supported only in builds that include the PDF engine. Other Office and OpenDocument formats remain unsupported.

See [FEATURE_STATUS.md](FEATURE_STATUS.md) for the implementation ledger. XLSX/PPTX, OCR, and AI engines are not implemented yet.

## Build

- Termux: `sh scripts/termux_setup.sh`, then `APEX_JOBS=2 sh scripts/termux_build.sh` (requires a Termux Qt 6 package providing CMake config files).
- Windows x64: push to GitHub to run `.github/workflows/windows-build.yml`; download the workflow artifact. Qt's MSVC build toolchain is installed on the runner, not on Android.
- Linux development: install CMake, Ninja, Qt 6 Widgets, and libzip, then run `sh scripts/run_tests.sh` which builds and runs the available CTest checks.

See [BUILDING.md](BUILDING.md) and [DEPENDENCIES.md](DEPENDENCIES.md).

## Document DNA

Open a supported text, DOCX, or PDF document, then select **Inspect → Document DNA** (Ctrl+Shift+D) for content-derived structure metrics, repeated paragraph detection, heading markers, and an estimated reading time. Metrics reflect extracted text only; image content and unsupported embedded parts are not analyzed.
