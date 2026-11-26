# Feature status

Status values describe code in this repository. “Implemented” means present in source; “Verified” requires observed build/runtime evidence. No feature is marked verified yet.

| Feature | Status | Current scope |
|---|---|---|
| C++20 / CMake project | Implemented | Qt 6 Widgets target |
| Windows x64 CI packaging | Implemented | Release artifact workflow; not yet observed running |
| Termux setup/build scripts | Implemented | Uses available Termux Qt package |
| Plain text reading | Implemented | UTF-8, read-only, 32 MiB file limit |
| Word count | Implemented | Whitespace token count |
| Capability registry | Implemented | Explicit unsupported reporting |
| PDF rendering, search, navigation, text extraction/export | Implemented | Qt PDF / PDFium; read-only; Windows workflow installs qtpdf; needs successful CI verification |
| PDF edit/merge/split/signature/repair/validation | Unsupported | No write/edit tools or validators |
| DOCX/XLSX/PPTX | Unsupported | No OOXML parser/renderers |
| OpenDocument | Unsupported | No parser |
| Search and persistent index | Planned | — |
| OCR | Planned | — |
| Comparison | Planned | — |
| AI assistance | Planned | — |
| Plugin system | Planned | — |
| Document DNA / timeline / graph | Planned | — |
| Automated tests and benchmarks | Planned | No test cases currently |
