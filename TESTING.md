# Testing

The `document_basics` test exercises extension normalization, registered support, UTF-8 text reading, word counting, and file-open errors. When Qt PDF is installed, `pdf_engine` loads a generated one-page PDF, checks page count and extracted text, then rejects malformed input. Windows and Linux CI build and run the tests available in their dependency set. Database, OCR, and comparison coverage remains to be added. `scripts/run_tests.sh` builds and runs CTest.
