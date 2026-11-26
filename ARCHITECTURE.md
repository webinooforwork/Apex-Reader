# Architecture

The project uses a small, layered C++20 application. `app/` owns the Qt entry point and main window. `core/` contains format capability declarations, `engines/` contains format-specific readers, and `ui/`, `search/`, `database/`, and further subsystem directories are reserved for implementations as they become real.

`CapabilityRegistry` is the contract shown to users: extensions without an implementation report `Unsupported`. The text reader handles bounded UTF-8 files. The DOCX reader uses libzip and QXmlStreamReader to extract bounded main-part text without running macros or loading external parts. When Qt PDF is present, the PDF workspace uses Qt PDF/PDFium for PDF rendering, text search, bookmark outline navigation, metadata inspection, and extraction. It does not render DOCX layout or parse HTML, Markdown syntax, PPTX.

Format parsers are isolated from the UI and return explicit errors. Folder indexing runs in a cancellable background worker; opening DOCX, CSV/TSV, XLSX/ODS, and ODT/ODP currently parses on the UI thread. SQLite FTS5 stores the local index. Windows builds request Qt PDF, which uses PDFium internally.

The Inspect → Document DNA panel computes an on-demand structural summary from text exposed by the active reader. It reports word and character counts, paragraph estimates, heading markers, repeated paragraph instances, a paragraph-length profile, and estimated reading time. PDF text is extracted page by page through Qt PDF when available. This is not a full visual/layout analysis; embedded images and unsupported package parts are excluded. The analysis currently runs on the UI thread and is intended for ordinary document sizes.

The local search catalog is an SQLite FTS5 database under the current user’s application data directory. A cancellable worker recursively scans a chosen folder and indexes UTF-8 text, Markdown source, logs, CSV/TSV text, extracted DOCX paragraph content, bounded XLSX/ODS cell/formula text, and ODT/ODP extracted text. Entries are refreshed when path, size, or modification time changes. Search requires all tokenized terms and returns bounded snippets; it does not index PDF, images/OCR, metadata, or unsupported formats yet. The catalog stores extracted text locally and does not send it to a network service.

CSV and TSV are parsed into a bounded read-only `QAbstractTableModel`. The parser recognizes quoted fields, doubled quote escapes, CRLF/LF rows, and multiline quoted values. It rejects malformed quote placement and refuses files exceeding its cell or column limits.

The XLSX reader resolves workbook relationships inside the OOXML package, extracts shared and inline strings, and reads worksheet cell references, cached values, and formula text using bounded ZIP/XML parsing. A read-only table model renders sparse cell maps across worksheet tabs; formula evaluation and workbook styling are unsupported.

OpenDocument text extraction reads bounded `content.xml` from ODT/ODP archives and emits paragraph or slide text. The ODS parser expands bounded repeated rows/cells into the shared sparse spreadsheet model and retains cached typed values and formula text; it does not evaluate formulas or load external links.
