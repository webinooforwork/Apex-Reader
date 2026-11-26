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
| XLSX/PPTX | Unsupported | No workbook or presentation parser/renderers |
| OpenDocument | Unsupported | No parser |
| Search and persistent index | Planned | — |
| OCR | Planned | — |
| Comparison | Planned | — |
| AI assistance | Planned | — |
| Plugin system | Planned | — |
| Document DNA / timeline / graph | Planned | — |
| Text, PDF, and DOCX automated checks | Implemented | CTest targets run in CI; Windows PDF/DOCX/text tests pass; Linux DOCX/text tests pass |
| Performance benchmarks | Planned | — |
