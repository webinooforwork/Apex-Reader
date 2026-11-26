#include "../engines/word/docx_document.h"

#include <QFile>
#include <QTemporaryDir>
#include <cassert>
#include <zip.h>

static bool addEntry(zip_t* archive, const char* name, const QByteArray& bytes) {
    zip_source_t* source = zip_source_buffer(archive, bytes.constData(), static_cast<zip_uint64_t>(bytes.size()), 0);
    if (!source)
        return false;
    const zip_int64_t index = zip_file_add(archive, name, source, ZIP_FL_ENC_UTF_8);
    if (index < 0) {
        zip_source_free(source);
        return false;
    }
    return zip_set_file_compression(archive, static_cast<zip_uint64_t>(index), ZIP_CM_DEFLATE, 6) == 0;
}

static bool writePackage(const QString& path, const QByteArray& documentXml) {
    int error = ZIP_ER_OK;
    const QByteArray localPath = QFile::encodeName(path);
    zip_t* archive = zip_open(localPath.constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!archive)
        return false;
    const QByteArray contentTypes =
        "<?xml version=\"1.0\"?><Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\"/>";
    const bool entriesAdded = addEntry(archive, "[Content_Types].xml", contentTypes) &&
                              addEntry(archive, "word/document.xml", documentXml);
    if (!entriesAdded) {
        zip_discard(archive);
        return false;
    }
    return zip_close(archive) == 0;
}

int main() {
    QTemporaryDir directory;
    assert(directory.isValid());
    const QString validPath = directory.filePath("sample.docx");
    const QByteArray xml =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\"><w:body>"
        "<w:p><w:pPr><w:pStyle w:val=\"Heading1\"/></w:pPr><w:r><w:t>Overview</w:t></w:r></w:p>"
        "<w:p><w:r><w:t>APEX reads </w:t></w:r><w:r><w:t>DOCX content.</w:t></w:r></w:p>"
        "<w:tbl><w:tr><w:tc><w:p><w:r><w:t>Sales</w:t></w:r></w:p></w:tc>"
        "<w:tc><w:p><w:r><w:t>2026</w:t></w:r></w:p></w:tc></w:tr></w:tbl>"
        "</w:body></w:document>";
    assert(writePackage(validPath, xml));

    apex::DocxDocument document;
    assert(document.load(validPath));
    assert(document.paragraphCount == 4);
    assert(document.text.contains("# Overview"));
    assert(document.text.contains("APEX reads DOCX content."));
    assert(document.text.contains("Sales"));
    assert(document.text.contains("2026"));
    assert(document.words() == 8);

    const QString unsafePath = directory.filePath("unsafe.docx");
    assert(writePackage(unsafePath, "<!DOCTYPE document [<!ENTITY x SYSTEM \"file:///etc/passwd\">]><document>&x;</document>"));
    apex::DocxDocument unsafe;
    assert(!unsafe.load(unsafePath));
    assert(unsafe.error.contains("DOCTYPE"));

    const QString invalidPath = directory.filePath("invalid.docx");
    QFile invalid(invalidPath);
    assert(invalid.open(QIODevice::WriteOnly));
    invalid.write("not a zip file");
    invalid.close();
    apex::DocxDocument malformed;
    assert(!malformed.load(invalidPath));
    return 0;
}
