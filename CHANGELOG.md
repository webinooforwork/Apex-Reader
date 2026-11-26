# Changelog

## 0.1.0 — foundation

- Added C++20 / Qt 6 desktop shell and honest format capability registry.
- Added bounded, read-only UTF-8 text reader and word count.
- Added Termux build scripts and Windows/Linux GitHub Actions workflows.
- Documented unsupported engines and current limitations.

## 0.2.0 — PDF reading

- Added an optional Qt PDF/PDFium reading workspace with page modes, zoom, full-text search highlighting, search-result navigation, bookmarks outline, metadata inspection, protected-file password prompt, and extracted text export.
- Added qtpdf to the Windows CI dependency set and capability reporting follows the modules present in the build. Added CTest coverage for normal text and PDF load/extraction, plus malformed PDF rejection.

## 0.3.0 — DOCX inspection

- Added bounded DOCX ZIP/XML text extraction for paragraphs, heading styles, tabs, line breaks, and table-cell text. Macros and external package parts are not loaded.
- Added malformed archive and DTD regression tests and libzip dependency wiring for Termux, Linux CI, and Windows vcpkg builds.
