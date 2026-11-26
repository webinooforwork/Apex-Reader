# Dependencies

## Required

- C++20 compiler
- CMake 3.21+
- Ninja
- Qt 6.4+ Widgets

## Build environments

- Termux packages: `cmake`, `ninja`, `clang`, `pkg-config`, and `qt6-qtbase` (package availability may vary by repository).
- Windows CI: Qt 6.8.3 MSVC 2022 x64 via `jurplel/install-qt-action`, CMake, Ninja, and MSVC runner tools.
- Linux CI: distribution Qt 6 Widgets development package, CMake, Ninja.

## PDF engine

The Windows workflow installs Qt PDF (qtpdf), which provides PDFium based parsing and rendering through Qt APIs. CMake detects it optionally; builds without the module report PDF as unsupported. Qt PDF is distributed as a separate Qt extension for Qt 6.8.

## Not yet dependencies

SQLite, Tesseract, OOXML/OpenDocument parsers, and AI provider SDKs are not configured or linked. Capabilities depending on them are marked unsupported or planned.
