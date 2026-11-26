#include "opendocument_text.h"

#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <zip.h>

namespace apex {
namespace {
constexpr qint64 MaxPackageBytes = 256LL * 1024LL * 1024LL;
constexpr zip_int64_t MaxPackageEntries = 20000;
constexpr zip_uint64_t MaxContentBytes = 64ULL * 1024ULL * 1024ULL;
constexpr int MaxParagraphs = 1000000;
constexpr qsizetype MaxExtractedTextChars = 64 * 1024 * 1024;
const QString TextNamespace =
    QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:text:1.0");

bool readContent(zip_t *archive, QByteArray &xml, QString &error) {
  zip_stat_t stat{};
  zip_stat_init(&stat);
  if (zip_stat(archive, "content.xml", ZIP_FL_ENC_GUESS, &stat) != 0 ||
      !(stat.valid & ZIP_STAT_SIZE) || stat.size == 0 ||
      stat.size > MaxContentBytes) {
    error = QStringLiteral("The OpenDocument package has no readable "
                           "content.xml within the 64 MiB limit.");
    return false;
  }
  zip_file_t *entry = zip_fopen(archive, "content.xml", ZIP_FL_ENC_GUESS);
  if (!entry) {
    error = QStringLiteral("Could not decompress OpenDocument content.xml.");
    return false;
  }
  xml.resize(static_cast<qsizetype>(stat.size));
  zip_uint64_t offset = 0;
  while (offset < stat.size) {
    const auto requested = qMin<zip_uint64_t>(64 * 1024, stat.size - offset);
    const zip_int64_t count = zip_fread(
        entry, xml.data() + static_cast<qsizetype>(offset), requested);
    if (count <= 0) {
      error = QStringLiteral(
          "OpenDocument content.xml ended early or failed integrity checks.");
      zip_fclose(entry);
      return false;
    }
    offset += static_cast<zip_uint64_t>(count);
  }
  if (zip_fclose(entry) != 0) {
    error =
        QStringLiteral("OpenDocument content.xml failed its integrity check.");
    return false;
  }
  return true;
}

int repeatCount(const QXmlStreamAttributes &attributes, const QString &name) {
  bool valid = false;
  const int count =
      attributes.value(TextNamespace, name).toString().toInt(&valid);
  return valid && count > 0 ? count : 1;
}
} // namespace

bool OpenDocumentText::load(const QString &path, bool presentation) {
  text.clear();
  error.clear();
  paragraphCount = 0;
  slideCount = 0;
  const QFileInfo info(path);
  if (!info.isFile() || info.size() <= 0 || info.size() > MaxPackageBytes) {
    error =
        QStringLiteral("The OpenDocument package is empty or exceeds 256 MiB.");
    return false;
  }
  int zipError = ZIP_ER_OK;
  const QByteArray encoded = QFile::encodeName(path);
  zip_t *archive = zip_open(encoded.constData(), ZIP_RDONLY, &zipError);
  if (!archive) {
    error = QStringLiteral("This is not a readable OpenDocument ZIP package.");
    return false;
  }
  const zip_int64_t entries = zip_get_num_entries(archive, 0);
  if (entries < 0 || entries > MaxPackageEntries) {
    error = QStringLiteral("The OpenDocument package has an invalid or "
                           "excessive number of ZIP entries.");
    zip_discard(archive);
    return false;
  }
  QByteArray xml;
  if (!readContent(archive, xml, error)) {
    zip_discard(archive);
    return false;
  }
  zip_discard(archive);

  QXmlStreamReader reader(xml);
  bool inParagraph = false;
  bool inSlide = false;
  while (!reader.atEnd()) {
    const auto token = reader.readNext();
    if (token == QXmlStreamReader::DTD) {
      error = QStringLiteral(
          "DOCTYPE declarations are refused in OpenDocument content.");
      text.clear();
      return false;
    }
    if (token == QXmlStreamReader::StartElement) {
      const QString name = reader.name().toString();
      if (presentation && name == QStringLiteral("page")) {
        ++slideCount;
        if (!text.isEmpty() && !text.endsWith(QLatin1Char('\n')))
          text += QLatin1Char('\n');
        text += QStringLiteral("Slide %1\n").arg(slideCount);
        inSlide = true;
      } else if (name == QStringLiteral("p") || name == QStringLiteral("h")) {
        if (!presentation || inSlide) {
          if (paragraphCount >= MaxParagraphs) {
            error = QStringLiteral(
                "OpenDocument text exceeds the paragraph safety limit.");
            text.clear();
            return false;
          }
          ++paragraphCount;
          inParagraph = true;
        }
      } else if (inParagraph && name == QStringLiteral("tab")) {
        text += QLatin1Char('\t');
      } else if (inParagraph && name == QStringLiteral("line-break")) {
        text += QLatin1Char('\n');
      } else if (inParagraph && name == QStringLiteral("s")) {
        const int count = qBound(
            1, repeatCount(reader.attributes(), QStringLiteral("c")), 1024);
        text += QString(count, QLatin1Char(' '));
      }
    } else if (token == QXmlStreamReader::Characters && inParagraph) {
      text += reader.text();
    } else if (token == QXmlStreamReader::EndElement) {
      const QString name = reader.name().toString();
      if ((name == QStringLiteral("p") || name == QStringLiteral("h")) &&
          inParagraph) {
        text += QLatin1Char('\n');
        inParagraph = false;
      } else if (presentation && name == QStringLiteral("page") && inSlide) {
        text += QLatin1Char('\n');
        inSlide = false;
      }
    }
    if (text.size() > MaxExtractedTextChars) {
      error = QStringLiteral("OpenDocument extracted text exceeds 64 MiB.");
      text.clear();
      return false;
    }
  }
  if (reader.hasError()) {
    error = QStringLiteral("Malformed OpenDocument content.xml: %1")
                .arg(reader.errorString());
    text.clear();
    return false;
  }
  return true;
}
} // namespace apex
