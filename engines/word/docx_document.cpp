#include "docx_document.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamReader>
#include <zip.h>

#include <limits>

namespace apex {
namespace {
constexpr zip_uint64_t MaxDocumentXmlBytes = 32ULL * 1024ULL * 1024ULL;
constexpr qint64 MaxPackageBytes = 256LL * 1024LL * 1024LL;
constexpr zip_int64_t MaxPackageEntries = 20000;

bool readEntry(zip_t *archive, const char *name, QByteArray &xml,
               QString &error, bool optional = false) {
  zip_stat_t stat{};
  zip_stat_init(&stat);
  if (zip_stat(archive, name, ZIP_FL_ENC_GUESS, &stat) != 0 ||
      !(stat.valid & ZIP_STAT_SIZE)) {
    if (optional) {
      xml.clear();
      return true;
    }
    error = QString("The DOCX package has no %1 part.")
                .arg(QString::fromLatin1(name));
    return false;
  }
  if (stat.size == 0 || stat.size > MaxDocumentXmlBytes ||
      stat.size >
          static_cast<zip_uint64_t>(std::numeric_limits<qsizetype>::max())) {
    error =
        QString(
            "The DOCX XML part %1 is empty or exceeds the 32 MiB safety limit.")
            .arg(QString::fromLatin1(name));
    return false;
  }

  zip_file_t *entry = zip_fopen(archive, name, ZIP_FL_ENC_GUESS);
  if (!entry) {
    error = QString("Could not read the DOCX XML part: %1")
                .arg(QString::fromLocal8Bit(zip_strerror(archive)));
    return false;
  }

  xml.resize(static_cast<qsizetype>(stat.size));
  zip_uint64_t offset = 0;
  while (offset < stat.size) {
    const zip_uint64_t requested =
        qMin<zip_uint64_t>(64 * 1024, stat.size - offset);
    const zip_int64_t count = zip_fread(
        entry, xml.data() + static_cast<qsizetype>(offset), requested);
    if (count <= 0) {
      error = "The DOCX XML part ended early or could not be decompressed.";
      zip_fclose(entry);
      return false;
    }
    offset += static_cast<zip_uint64_t>(count);
  }
  if (zip_fclose(entry) != 0) {
    error = "The DOCX XML part failed its decompression or integrity check.";
    return false;
  }
  return true;
}

bool parseComments(const QByteArray &xml,
                   QVector<DocxDocument::Comment> &comments, QString &error) {
  if (xml.isEmpty())
    return true;
  QXmlStreamReader reader(xml);
  bool insideComment = false;
  DocxDocument::Comment comment;
  while (!reader.atEnd()) {
    const auto token = reader.readNext();
    if (token == QXmlStreamReader::DTD) {
      error = "DOCTYPE declarations are not accepted in DOCX comments.";
      return false;
    }
    if (token == QXmlStreamReader::StartElement) {
      const auto name = reader.name();
      if (name == QStringLiteral("comment")) {
        if (comments.size() >= 10000) {
          error =
              "The DOCX comments part exceeds the 10,000-comment safety limit.";
          return false;
        }
        insideComment = true;
        comment = {};
        const auto attrs = reader.attributes();
        comment.id = attrs
                         .value("http://schemas.openxmlformats.org/"
                                "wordprocessingml/2006/main",
                                "id")
                         .toString();
        comment.author = attrs
                             .value("http://schemas.openxmlformats.org/"
                                    "wordprocessingml/2006/main",
                                    "author")
                             .toString();
        comment.date = attrs
                           .value("http://schemas.openxmlformats.org/"
                                  "wordprocessingml/2006/main",
                                  "date")
                           .toString();
      } else if (insideComment && name == QStringLiteral("tab")) {
        comment.text += QChar('\t');
      } else if (insideComment && (name == QStringLiteral("br") ||
                                   name == QStringLiteral("cr"))) {
        comment.text += QChar('\n');
      }
    } else if (token == QXmlStreamReader::Characters && insideComment) {
      comment.text += reader.text();
    } else if (token == QXmlStreamReader::EndElement &&
               reader.name() == QStringLiteral("comment") && insideComment) {
      comment.text = comment.text.trimmed();
      comments.append(comment);
      insideComment = false;
    }
  }
  if (reader.hasError()) {
    error =
        QString("Malformed DOCX comments XML: %1").arg(reader.errorString());
    return false;
  }
  return true;
}
} // namespace

bool DocxDocument::load(const QString &path) {
  text.clear();
  error.clear();
  paragraphCount = 0;
  insertedRevisionCount = 0;
  deletedRevisionCount = 0;
  comments.clear();

  const QFileInfo packageInfo(path);
  if (!packageInfo.isFile() || packageInfo.size() <= 0 ||
      packageInfo.size() > MaxPackageBytes) {
    error = "The DOCX package is empty or exceeds the 256 MiB safety limit.";
    return false;
  }

  int zipError = ZIP_ER_OK;
  const QByteArray localPath = QFile::encodeName(path);
  zip_t *archive = zip_open(localPath.constData(), ZIP_RDONLY, &zipError);
  if (!archive) {
    zip_error_t details;
    zip_error_init_with_code(&details, zipError);
    error = QString("This file is not a readable DOCX ZIP package: %1")
                .arg(QString::fromLocal8Bit(zip_error_strerror(&details)));
    zip_error_fini(&details);
    return false;
  }

  const zip_int64_t entryCount = zip_get_num_entries(archive, 0);
  if (entryCount < 0 || entryCount > MaxPackageEntries) {
    error =
        "The DOCX package has an invalid or excessive number of ZIP entries.";
    zip_discard(archive);
    return false;
  }

  QByteArray xmlBytes;
  QByteArray commentsBytes;
  const bool extracted =
      readEntry(archive, "word/document.xml", xmlBytes, error) &&
      readEntry(archive, "word/comments.xml", commentsBytes, error, true);
  zip_discard(archive);
  if (!extracted || !parseComments(commentsBytes, comments, error))
    return false;

  QXmlStreamReader reader(xmlBytes);
  QStringList paragraphs;
  QString currentParagraph;
  QString currentStyle;
  bool insideParagraph = false;

  while (!reader.atEnd()) {
    const auto token = reader.readNext();
    if (token == QXmlStreamReader::DTD) {
      error = "DOCTYPE declarations are not accepted in DOCX content.";
      return false;
    }
    if (token == QXmlStreamReader::StartElement) {
      const QString name = reader.name().toString();
      if (name == "ins")
        ++insertedRevisionCount;
      else if (name == "del")
        ++deletedRevisionCount;
      if (name == "p") {
        insideParagraph = true;
        currentParagraph.clear();
        currentStyle.clear();
      } else if (insideParagraph && name == "pStyle") {
        currentStyle = reader.attributes()
                           .value("http://schemas.openxmlformats.org/"
                                  "wordprocessingml/2006/main",
                                  "val")
                           .toString();
      } else if (insideParagraph && name == "tab") {
        currentParagraph += QChar('\t');
      } else if (insideParagraph && (name == "br" || name == "cr")) {
        currentParagraph += QChar('\n');
      }
    } else if (token == QXmlStreamReader::Characters && insideParagraph) {
      currentParagraph += reader.text();
    } else if (token == QXmlStreamReader::EndElement &&
               reader.name() == QStringLiteral("p") && insideParagraph) {
      const QString normalizedStyle = currentStyle.toLower();
      if (normalizedStyle.startsWith("heading")) {
        int level = 1;
        for (const QChar c : normalizedStyle) {
          if (c.isDigit()) {
            level = qBound(1, c.digitValue(), 6);
            break;
          }
        }
        currentParagraph.prepend(QString(level, QChar('#')) + " ");
      }
      paragraphs.append(currentParagraph);
      ++paragraphCount;
      insideParagraph = false;
    }
  }

  if (reader.hasError()) {
    error =
        QString("Malformed DOCX document XML: %1").arg(reader.errorString());
    text.clear();
    paragraphCount = 0;
    return false;
  }
  text = paragraphs.join(QChar('\n'));
  return true;
}

qsizetype DocxDocument::words() const {
  return text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).size();
}
} // namespace apex
