# APEX OFFICE READER

APEX is a native C++20 Windows document workspace developed from Termux and built for Windows x64 with GitHub Actions. The repository is the source of truth: https://github.com/webinooforwork/Apex-Reader

## Current state

This is an early release and the universal workspace is being built in stages. The Qt 6 desktop app reads UTF-8 text (`.txt`, `.log`, `.md`), opens CSV/TSV files in a bounded read-only table, and, when Qt PDF is installed, uses PDFium through Qt PDF for PDF viewing. PDF controls include continuous or single-page mode, fit/custom zoom, full-text search with highlights, search-result navigation, password prompts, and text export. All current readers are read-only; Markdown is displayed as source text. DOCX uses a bounded ZIP/XML extractor for paragraphs, heading styles, and table-cell text, without page-layout rendering. PDF is listed as supported only in builds that include the PDF engine. Other Office formats remain unsupported. ODT and ODP provide text extraction, while ODS provides a read-only sheet table.

See [FEATURE_STATUS.md](FEATURE_STATUS.md) for the implementation ledger. PPTX, OCR, and AI engines are not implemented yet. XLSX now provides bounded value and formula inspection without calculation or style rendering.

## Build

- Termux: `sh scripts/termux_setup.sh`, then `APEX_JOBS=2 sh scripts/termux_build.sh` (requires a Termux Qt 6 package providing CMake config files).
- Windows x64: push to GitHub to run `.github/workflows/windows-build.yml`; download the workflow artifact. Qt's MSVC build toolchain is installed on the runner, not on Android.
- Linux development: install CMake, Ninja, Qt 6 Widgets, libzip, and SQLite 3 with FTS5, then run `cmake -S . -B build -G Ninja && cmake --build build` to compile the app.

See [BUILDING.md](BUILDING.md) and [DEPENDENCIES.md](DEPENDENCIES.md).

## Document DNA

Open a supported text, DOCX, or PDF document, then select **Inspect → Document DNA** (Ctrl+Shift+D) for content-derived structure metrics, repeated paragraph detection, heading markers, and an estimated reading time. Metrics reflect extracted text only; image content and unsupported embedded parts are not analyzed.

## Local document search

Use **Library → Index folder** to build or refresh the per-user SQLite FTS5 index for text, Markdown source, logs, CSV/TSV, DOCX paragraph text, XLSX values/formulas, ODS cell values, and ODT/ODP text. Scanning runs in a cancellable background thread. Use **Library → Search indexed documents** (Ctrl+Shift+F) to search all terms and open a result. PDF and image OCR indexing are not included yet. Extracted content remains in the local application data directory.

## CSV and TSV

CSV and TSV open as read-only tables with quoted fields, embedded line breaks in quoted cells, cell selection, and find-next. Files are capped at 500,000 cells and the existing 32 MiB text limit. Formula evaluation and workbook features are not available for these formats.

## OpenDocument reading

ODT and ODP provide extracted paragraph/slide text. ODS opens as multiple read-only sheets with cached values and stored formula text. These readers do not render original page or cell styles, calculate formulas, or render slides.

## XLSX inspection

XLSX opens as multiple worksheet tabs with read-only cells, cached formula results, and an option to show stored formula expressions. APEX does not evaluate formulas or reproduce Excel formatting, charts, or pivots.
