#include <QCoreApplication>
#include <QFile>
#include <QPdfDocument>
#include <QTemporaryDir>
#include <QVector>
#include <cassert>

static QByteArray makeOnePagePdf() {
    const QByteArray stream = "BT\n/F1 18 Tf\n72 720 Td\n(APEX PDF engine test) Tj\nET\n";
    const QVector<QByteArray> objects{
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 5 0 R >> >> /Contents 4 0 R >>",
        QByteArray("<< /Length ") + QByteArray::number(stream.size()) + " >>\nstream\n" + stream + "endstream",
        "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"
    };
    QByteArray pdf = "%PDF-1.4\n";
    QVector<qint64> offsets{0};
    for (qsizetype i = 0; i < objects.size(); ++i) {
        offsets.append(pdf.size());
        pdf += QByteArray::number(i + 1) + " 0 obj\n" + objects[i] + "\nendobj\n";
    }
    const qint64 xrefOffset = pdf.size();
    pdf += "xref\n0 6\n0000000000 65535 f \n";
    for (qsizetype i = 1; i < offsets.size(); ++i)
        pdf += QByteArray::number(offsets[i]).rightJustified(10, '0') + " 00000 n \n";
    pdf += "trailer\n<< /Size 6 /Root 1 0 R >>\nstartxref\n";
    pdf += QByteArray::number(xrefOffset) + "\n%%EOF\n";
    return pdf;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir directory;
    assert(directory.isValid());

    const QString validPath = directory.filePath("valid.pdf");
    QFile valid(validPath);
    assert(valid.open(QIODevice::WriteOnly));
    assert(valid.write(makeOnePagePdf()) > 0);
    valid.close();

    QPdfDocument document;
    assert(document.load(validPath) == QPdfDocument::Error::None);
    assert(document.pageCount() == 1);
    const QString extracted = document.getAllText(0).text();
    assert(extracted.contains("APEX PDF engine test"));

    document.close();
    const QString malformedPath = directory.filePath("malformed.pdf");
    QFile malformed(malformedPath);
    assert(malformed.open(QIODevice::WriteOnly));
    malformed.write("%PDF-1.4\nnot a document\n");
    malformed.close();
    assert(document.load(malformedPath) != QPdfDocument::Error::None);
    return 0;
}
