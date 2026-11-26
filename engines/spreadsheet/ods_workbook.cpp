#include "ods_workbook.h"

#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <zip.h>

namespace apex {
namespace {
constexpr qint64 MaxPackageBytes = 256LL * 1024LL * 1024LL;
constexpr zip_int64_t MaxPackageEntries = 20000;
constexpr zip_uint64_t MaxContentBytes = 64ULL * 1024ULL * 1024ULL;
constexpr int MaxSheets = 256;
constexpr int MaxRows = 1048576;
constexpr int MaxColumns = 16384;
constexpr int MaxCells = 300000;
constexpr qsizetype MaxCellTextChars = 32 * 1024 * 1024;
const QString OfficeNs =
    QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:office:1.0");
const QString TableNs =
    QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:table:1.0");
const QString TextNs =
    QStringLiteral("urn:oasis:names:tc:opendocument:xmlns:text:1.0");

struct CellRun {
  int column{};
  int repeat{};
  SpreadsheetCell cell;
};

bool readContent(zip_t *archive, QByteArray &xml, QString &error) {
  zip_stat_t stat{};
  zip_stat_init(&stat);
  if (zip_stat(archive, "content.xml", ZIP_FL_ENC_GUESS, &stat) != 0 ||
      !(stat.valid & ZIP_STAT_SIZE) || stat.size == 0 ||
      stat.size > MaxContentBytes) {
    error = QStringLiteral(
        "ODS content.xml is missing, empty, or exceeds its 64 MiB limit.");
    return false;
  }
  zip_file_t *entry = zip_fopen(archive, "content.xml", ZIP_FL_ENC_GUESS);
  if (!entry) {
    error = QStringLiteral("Could not decompress ODS content.xml.");
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
          "ODS content.xml ended early or failed integrity checks.");
      zip_fclose(entry);
      return false;
    }
    offset += static_cast<zip_uint64_t>(count);
  }
  if (zip_fclose(entry) != 0) {
    error = QStringLiteral("ODS content.xml failed its integrity check.");
    return false;
  }
  return true;
}

int positiveRepeat(const QString &raw, int fallback = 1) {
  if (raw.isEmpty())
    return fallback;
  bool valid = false;
  const int value = raw.toInt(&valid);
  return valid && value > 0 ? value : 0;
}

QString cellTextValue(const QString &type, const QString &value,
                      const QString &stringValue, const QString &booleanValue,
                      const QString &dateValue, const QString &timeValue,
                      const QString &paragraphText) {
  if (type == QStringLiteral("boolean"))
    return booleanValue == QStringLiteral("true") ? QStringLiteral("TRUE")
                                                  : QStringLiteral("FALSE");
  if (type == QStringLiteral("date"))
    return dateValue;
  if (type == QStringLiteral("time"))
    return timeValue;
  if (type == QStringLiteral("float") || type == QStringLiteral("currency") ||
      type == QStringLiteral("percentage"))
    return value;
  if (!paragraphText.isEmpty())
    return paragraphText;
  return stringValue;
}
} // namespace

bool OdsWorkbook::load(const QString &path) {
  sheets.clear();
  error.clear();
  cellCount = 0;
  const QFileInfo info(path);
  if (!info.isFile() || info.size() <= 0 || info.size() > MaxPackageBytes) {
    error = QStringLiteral("The ODS package is empty or exceeds 256 MiB.");
    return false;
  }
  int zipError = ZIP_ER_OK;
  const QByteArray encoded = QFile::encodeName(path);
  zip_t *archive = zip_open(encoded.constData(), ZIP_RDONLY, &zipError);
  if (!archive) {
    error = QStringLiteral("This is not a readable ODS ZIP package.");
    return false;
  }
  const zip_int64_t entries = zip_get_num_entries(archive, 0);
  if (entries < 0 || entries > MaxPackageEntries) {
    error = QStringLiteral(
        "The ODS package has an invalid or excessive number of ZIP entries.");
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
  SpreadsheetSheet *sheet = nullptr;
  int rowIndex = -1;
  int rowRepeat = 1;
  int nextColumn = 0;
  bool inRow = false;
  bool inCell = false;
  bool inCoveredCell = false;
  bool inParagraph = false;
  int cellRepeat = 1;
  QString type, value, stringValue, booleanValue, dateValue, timeValue, formula,
      paragraphText;
  QVector<CellRun> rowCells;
  while (!reader.atEnd()) {
    const auto token = reader.readNext();
    if (token == QXmlStreamReader::DTD) {
      error =
          QStringLiteral("DOCTYPE declarations are refused in ODS content.");
      return false;
    }
    if (token == QXmlStreamReader::StartElement) {
      const QString tag = reader.name().toString();
      const auto attrs = reader.attributes();
      if (tag == QStringLiteral("table")) {
        if (sheets.size() >= MaxSheets) {
          error =
              QStringLiteral("The ODS workbook exceeds the 256-sheet limit.");
          return false;
        }
        SpreadsheetSheet newSheet;
        newSheet.name = attrs.value(TableNs, QStringLiteral("name")).toString();
        if (newSheet.name.isEmpty())
          newSheet.name = QStringLiteral("Sheet %1").arg(sheets.size() + 1);
        sheets.append(std::move(newSheet));
        sheet = &sheets.last();
      } else if (tag == QStringLiteral("table-row") && sheet) {
        inRow = true;
        rowRepeat = positiveRepeat(
            attrs.value(TableNs, QStringLiteral("number-rows-repeated"))
                .toString());
        if (rowRepeat == 0) {
          error = QStringLiteral("ODS row repetition count is invalid.");
          return false;
        }
        rowIndex = sheet->rows;
        if (rowIndex < 0 || rowRepeat > MaxRows - rowIndex) {
          error = QStringLiteral(
              "ODS row range exceeds the spreadsheet row limit.");
          return false;
        }
        nextColumn = 0;
        rowCells.clear();
      } else if ((tag == QStringLiteral("table-cell") ||
                  tag == QStringLiteral("covered-table-cell")) &&
                 inRow) {
        inCoveredCell = tag == QStringLiteral("covered-table-cell");
        inCell = !inCoveredCell;
        cellRepeat = positiveRepeat(
            attrs.value(TableNs, QStringLiteral("number-columns-repeated"))
                .toString());
        if (cellRepeat == 0) {
          error = QStringLiteral("ODS column repetition count is invalid.");
          return false;
        }
        if (cellRepeat > MaxColumns - nextColumn) {
          error = QStringLiteral("ODS row exceeds the 16,384-column limit.");
          return false;
        }
        if (inCell) {
          type = attrs.value(OfficeNs, QStringLiteral("value-type")).toString();
          value = attrs.value(OfficeNs, QStringLiteral("value")).toString();
          stringValue =
              attrs.value(OfficeNs, QStringLiteral("string-value")).toString();
          booleanValue =
              attrs.value(OfficeNs, QStringLiteral("boolean-value")).toString();
          dateValue =
              attrs.value(OfficeNs, QStringLiteral("date-value")).toString();
          timeValue =
              attrs.value(OfficeNs, QStringLiteral("time-value")).toString();
          formula = attrs.value(TableNs, QStringLiteral("formula")).toString();
          paragraphText.clear();
        }
      } else if (tag == QStringLiteral("p") && inCell)
        inParagraph = true;
      else if (tag == QStringLiteral("s") && inParagraph) {
        const int spaces = qBound(
            1,
            positiveRepeat(attrs.value(TextNs, QStringLiteral("c")).toString()),
            1024);
        paragraphText += QString(spaces, QLatin1Char(' '));
      } else if (tag == QStringLiteral("tab") && inParagraph)
        paragraphText += QLatin1Char('\t');
      else if (tag == QStringLiteral("line-break") && inParagraph)
        paragraphText += QLatin1Char('\n');
    } else if (token == QXmlStreamReader::Characters && inParagraph)
      paragraphText += reader.text();
    else if (token == QXmlStreamReader::EndElement) {
      const QString tag = reader.name().toString();
      if (tag == QStringLiteral("p") && inParagraph) {
        paragraphText += QLatin1Char('\n');
        inParagraph = false;
      } else if (tag == QStringLiteral("table-cell") && inCell) {
        QString extractedParagraphs = paragraphText;
        if (extractedParagraphs.endsWith(QLatin1Char('\n')))
          extractedParagraphs.chop(1);
        const QString cellValue =
            cellTextValue(type, value, stringValue, booleanValue, dateValue,
                          timeValue, extractedParagraphs);
        if (cellValue.size() > MaxCellTextChars ||
            formula.size() > MaxCellTextChars) {
          error = QStringLiteral(
              "ODS cell content exceeds the 32 MiB safety limit.");
          return false;
        }
        if (!cellValue.isEmpty() || !formula.isEmpty())
          rowCells.append({nextColumn, cellRepeat, {cellValue, formula}});
        nextColumn += cellRepeat;
        const qint64 repeatedCellCount =
            static_cast<qint64>(cellRepeat) * rowRepeat;
        if (repeatedCellCount > MaxCells - cellCount) {
          error = QStringLiteral(
              "ODS workbook exceeds the 300,000-cell safety limit.");
          return false;
        }
        cellCount += static_cast<int>(repeatedCellCount);
        inCell = false;
      } else if (tag == QStringLiteral("covered-table-cell") && inCoveredCell) {
        nextColumn += cellRepeat;
        const qint64 repeatedCellCount =
            static_cast<qint64>(cellRepeat) * rowRepeat;
        if (repeatedCellCount > MaxCells - cellCount) {
          error = QStringLiteral(
              "ODS workbook exceeds the 300,000-cell safety limit.");
          return false;
        }
        cellCount += static_cast<int>(repeatedCellCount);
        inCoveredCell = false;
      } else if (tag == QStringLiteral("table-row") && inRow && sheet) {
        sheet->rows = rowIndex + rowRepeat;
        sheet->columns = qMax(sheet->columns, nextColumn);
        if (!rowCells.isEmpty())
          for (int r = 0; r < rowRepeat; ++r) {
            auto &cells = sheet->cells[rowIndex + r];
            for (const auto &run : rowCells) {
              if (run.cell.value.isEmpty() && run.cell.formula.isEmpty())
                continue;
              for (int c = 0; c < run.repeat; ++c)
                cells.insert(run.column + c, run.cell);
            }
          }
        inRow = false;
      } else if (tag == QStringLiteral("table") && sheet)
        sheet = nullptr;
    }
    if (paragraphText.size() > MaxCellTextChars) {
      error = QStringLiteral("ODS cell text exceeds the 32 MiB safety limit.");
      return false;
    }
  }
  if (reader.hasError()) {
    error = QStringLiteral("Malformed ODS content.xml: %1")
                .arg(reader.errorString());
    sheets.clear();
    cellCount = 0;
    return false;
  }
  if (sheets.isEmpty()) {
    error = QStringLiteral("The ODS workbook contains no worksheets.");
    return false;
  }
  return true;
}

QString OdsWorkbook::searchableText(qsizetype characterLimit) const {
  QString text;
  for (const auto &sheet : sheets) {
    text += sheet.name + QLatin1Char('\n');
    for (auto row = sheet.cells.cbegin(); row != sheet.cells.cend(); ++row) {
      for (auto cell = row->cbegin(); cell != row->cend(); ++cell) {
        if (!cell->value.isEmpty())
          text += cell->value + QLatin1Char(' ');
        if (!cell->formula.isEmpty())
          text += cell->formula + QLatin1Char(' ');
        if (text.size() >= characterLimit) {
          text.truncate(characterLimit);
          return text;
        }
      }
      text += QLatin1Char('\n');
    }
  }
  return text;
}
} // namespace apex
