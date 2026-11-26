# Feature status

Status values describe code in this repository. “Implemented” means present in source; “Tested” means automated checks pass in CI; “Verified” requires broader observed runtime evidence.

| Feature | Status | Current scope |
|---|---|---|
| C++20 / CMake project | Implemented | Qt 6 Widgets target |
| Core text and registry tests | Tested | CTest `document_basics` target; Windows/Linux CI passes |
| PDF load, text extraction, malformed-file test | Tested | CTest PDF engine test passes in Windows CI |
| Windows x64 CI packaging | Implemented | Release ZIP artifact builds and uploads successfully in Windows CI |
| Termux setup/build scripts | Implemented | Uses available Termux Qt package |
| Plain text reading | Implemented | UTF-8, read-only, 32 MiB file limit |
| Word count | Implemented | Whitespace token count |
| Capability registry | Implemented | Explicit unsupported reporting |
| PDF rendering, search, navigation, text extraction/export | Implemented | Qt PDF / PDFium; read-only; Windows workflow installs qtpdf; Windows CI compiles and packages it |
| PDF edit/merge/split/signature/repair/validation | Unsupported | No write/edit tools or validators |
| DOCX text extraction | Tested | libzip + QXmlStreamReader; paragraphs, headings, table-cell text; bounded ZIP/XML; Windows/Linux regression tests pass |
| CSV/TSV table view | Implemented | Read-only model with quoted/multiline cells, cell selection and find-next, bounded to 500,000 cells; not runtime-tested in this turn |
| XLSX workbook inspection | Implemented | Read-only multi-sheet cells, shared/inline strings, cached values, formula text view; 300,000-cell and 256 MiB expanded XML limits; no calculation/styles/charts; not runtime-tested in this turn |
| PPTX | Unsupported | No presentation parser/renderers |
| ODS workbook view | Implemented | Read-only multiple sheets, repeated cells/rows, typed cached values and formula text; bounded XML/package and cell counts; not runtime-tested in this turn |
| ODT text extraction | Implemented | Paragraph/heading text, tabs, line breaks, repeated spaces; not page-layout rendering; not runtime-tested in this turn |
| ODP slide text extraction | Implemented | Slide-numbered text extraction from content.xml; no slide rendering; not runtime-tested in this turn |
| Persistent full-text search | Implemented | SQLite FTS5; cancellable recursive indexing for UTF-8 text, Markdown source, logs, CSV/TSV, DOCX/XLSX/ODS extracted content and ODT/ODP text; incremental by file size and modification time; not runtime-tested in this turn |
| PDF/OCR/metadata indexing, regex, saved searches, semantic search | Planned | — |
| OCR | Planned | — |
| Comparison | Planned | — |
| AI assistance | Planned | — |
| Plugin system | Planned | — |
| Document DNA | Implemented | On-demand text structure analysis for open text, DOCX, and PDF documents: word/character/paragraph counts, reading-time estimate, repeated paragraphs, headings, and paragraph-length profile; not runtime-tested in this turn |
| Document timeline / knowledge graph | Planned | — |
| Automated checks | Implemented | Existing CTest targets retained; CI only runs them through manual workflow dispatch when selected; no tests were run for current changes |
| Performance benchmarks | Planned | — |
