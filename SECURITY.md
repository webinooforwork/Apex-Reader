# Security

APEX currently reads a bounded UTF-8 text file as read-only content. It does not execute macros, scripts, or external links. DOCX reading opens only word/document.xml from the ZIP package; package size, entry count, and XML size are bounded, and DTD declarations are rejected. XLSX relationship targets are restricted to the package xl directory; each XML part and total expanded XML have size limits; DTD declarations are rejected. XLSX macros are not executed. Unsupported formats are rejected with an explicit message rather than being interpreted as text or claimed as successfully parsed.

Future archive/XML/PDF parsers must enforce size and resource limits, reject unsafe paths in archives, avoid external entity loading, disable active content, and report malformed input. Temporary-file and sandbox policies remain to be implemented before complex formats are enabled.
