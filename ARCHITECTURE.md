# Architecture

The project uses a small, layered C++20 application. `app/` owns the Qt entry point and main window. `core/` contains format capability declarations, `engines/` contains format-specific readers, and `ui/`, `search/`, `database/`, and further subsystem directories are reserved for implementations as they become real.

`CapabilityRegistry` is the contract shown to users: extensions without an implementation report `Unsupported`. The text reader handles bounded UTF-8 files. The DOCX reader uses libzip and QXmlStreamReader to extract bounded main-part text without running macros or loading external parts. When Qt PDF is present, the PDF workspace uses Qt PDF/PDFium for PDF rendering, text search, bookmark outline navigation, metadata inspection, and extraction. It does not render DOCX layout or parse HTML, Markdown syntax, XLSX, PPTX, or OpenDocument.

Planned engine interfaces should isolate format parsing from UI widgets, return explicit errors, and never execute macros or embedded scripts. Expensive parsing should later run on cancellable background tasks. SQLite is not currently linked. Windows builds request Qt PDF, which uses PDFium internally.

The Inspect → Document DNA panel computes an on-demand structural summary from text exposed by the active reader. It reports word and character counts, paragraph estimates, heading markers, repeated paragraph instances, a paragraph-length profile, and estimated reading time. PDF text is extracted page by page through Qt PDF when available. This is not a full visual/layout analysis; embedded images and unsupported package parts are excluded. The analysis currently runs on the UI thread and is intended for ordinary document sizes.

The local search catalog is an SQLite FTS5 database under the current user’s application data directory. A cancellable worker recursively scans a chosen folder and indexes UTF-8 text, Markdown source, logs, CSV/TSV text, and extracted DOCX paragraph content. Entries are refreshed when path, size, or modification time changes. Search requires all tokenized terms and returns bounded snippets; it does not index PDF, images/OCR, metadata, or unsupported formats yet. The catalog stores extracted text locally and does not send it to a network service.

CSV and TSV are parsed into a bounded read-only `QAbstractTableModel`. The parser recognizes quoted fields, doubled quote escapes, CRLF/LF rows, and multiline quoted values. It rejects malformed quote placement and refuses files exceeding its cell or column limits.
