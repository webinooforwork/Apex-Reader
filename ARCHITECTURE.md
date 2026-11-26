# Architecture

The project uses a small, layered C++20 application. `app/` owns the Qt entry point and main window. `core/` contains format capability declarations, `engines/` contains format-specific readers, and `ui/`, `search/`, `database/`, and further subsystem directories are reserved for implementations as they become real.

`CapabilityRegistry` is the contract shown to users: extensions without an implementation report `Unsupported`. The text reader handles bounded UTF-8 files. The DOCX reader uses libzip and QXmlStreamReader to extract bounded main-part text without running macros or loading external parts. When Qt PDF is present, the PDF workspace uses Qt PDF/PDFium for PDF rendering, text search, bookmark outline navigation, metadata inspection, and extraction. It does not render DOCX layout or parse HTML, Markdown syntax, XLSX, PPTX, or OpenDocument.

Planned engine interfaces should isolate format parsing from UI widgets, return explicit errors, and never execute macros or embedded scripts. Expensive parsing should later run on cancellable background tasks. SQLite is not currently linked. Windows builds request Qt PDF, which uses PDFium internally.
