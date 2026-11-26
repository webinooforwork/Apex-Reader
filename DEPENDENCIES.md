# Dependencies

## Required

- C++20 compiler
- CMake 3.21+
- Ninja
- Qt 6.4+ Widgets
- libzip
- SQLite 3 with FTS5 enabled

## Build environments

- Termux packages: `cmake`, `ninja`, `clang`, `pkg-config`, `libzip`, `sqlite`, and `qt6-qtbase` (package availability may vary by repository).
- Windows CI: Qt 6.8.3 MSVC 2022 x64 via `jurplel/install-qt-action`, CMake, Ninja, and MSVC runner tools.
- Linux CI: distribution Qt 6 Widgets development package, `libsqlite3-dev` with FTS5, CMake, Ninja.

## OOXML parsers

The local full-text catalog uses SQLite FTS5 and stores indexed content in the per-user application data directory. Indexable content is limited to UTF-8 text, Markdown source, logs, CSV/TSV text, DOCX paragraph text, bounded XLSX/ODS values/formulas, and extracted ODT/ODP text. Text readers retain their existing size limits. The catalog updates changed files by path, size, and modification time and never uploads content.

The DOCX reader uses libzip and Qt Core QXmlStreamReader to extract bounded paragraph text. The XLSX reader uses the same dependencies to resolve workbook relationships and parse shared strings and worksheet values. ODT, ODP, and ODS use libzip and QXmlStreamReader to extract content.xml with DTD rejection and bounded package/content sizes. It rejects DTD declarations and limits the archive to 256 MiB, each XML part to 64 MiB, expanded XML to 256 MiB, 256 sheets, and 300,000 cells. Windows obtains libzip from vcpkg; Termux and Linux install it from their package managers.

## PDF engine

The Windows workflow installs Qt PDF (qtpdf), which provides PDFium based parsing and rendering through Qt APIs. CMake detects it optionally; builds without the module report PDF as unsupported. Qt PDF is distributed as a separate Qt extension for Qt 6.8.

## Not yet dependencies

Tesseract, the PPTX parser, and AI provider SDKs are not configured or linked. Capabilities depending on them are marked unsupported or planned.

### Windows package setup

The Windows workflow uses vcpkg manifest mode to build libzip and SQLite for x64 and copies its runtime DLLs into the downloadable package.
