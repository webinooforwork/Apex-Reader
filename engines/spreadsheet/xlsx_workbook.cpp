#include "xlsx_workbook.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVariant>
#include <QXmlStreamReader>
#include <utility>
#include <zip.h>

namespace apex {
namespace {
constexpr qint64 MaxPackageBytes = 256LL * 1024LL * 1024LL;
constexpr zip_int64_t MaxPackageEntries = 20000;
constexpr zip_uint64_t MaxXmlPartBytes = 64ULL * 1024ULL * 1024ULL;
constexpr zip_uint64_t MaxExpandedBytes = 256ULL * 1024ULL * 1024ULL;
constexpr int MaxSheets = 256;
constexpr int MaxCells = 300000;
constexpr int MaxRows = 1048576;
constexpr int MaxColumns = 16384;
const QString RelationshipNamespace = QStringLiteral(
    "http://schemas.openxmlformats.org/officeDocument/2006/relationships");

bool readPart(zip_t *archive, const QString &name, QByteArray &xml,
              QString &error, zip_uint64_t &expandedBytes) {
  const QByteArray encoded = name.toUtf8();
  const zip_int64_t index =
      zip_name_locate(archive, encoded.constData(), ZIP_FL_ENC_GUESS);
  if (index < 0) {
    error = QStringLiteral("The XLSX package is missing %1.").arg(name);
    return false;
  }
  zip_stat_t stat{};
  zip_stat_init(&stat);
  if (zip_stat_index(archive, static_cast<zip_uint64_t>(index),
                     ZIP_FL_ENC_GUESS, &stat) != 0 ||
      !(stat.valid & ZIP_STAT_SIZE) || stat.size == 0 ||
      stat.size > MaxXmlPartBytes) {
    error = QStringLiteral("XLSX XML part %1 is empty, unreadable, or exceeds "
                           "the 64 MiB safety limit.")
                .arg(name);
    return false;
  }
  if (stat.size > MaxExpandedBytes - expandedBytes) {
    error = QStringLiteral(
        "XLSX expanded XML exceeds the 256 MiB package safety limit.");
    return false;
  }
  expandedBytes += stat.size;
  zip_file_t *file = zip_fopen_index(archive, static_cast<zip_uint64_t>(index),
                                     ZIP_FL_ENC_GUESS);
  if (!file) {
    error = QStringLiteral("Could not decompress XLSX part %1.").arg(name);
    return false;
  }
  xml.resize(static_cast<qsizetype>(stat.size));
  zip_uint64_t offset = 0;
  while (offset < stat.size) {
    const auto requested = qMin<zip_uint64_t>(64 * 1024, stat.size - offset);
    const zip_int64_t count =
        zip_fread(file, xml.data() + static_cast<qsizetype>(offset), requested);
    if (count <= 0) {
      error =
          QStringLiteral("XLSX part %1 ended early or failed integrity checks.")
              .arg(name);
      zip_fclose(file);
      return false;
    }
    offset += static_cast<zip_uint64_t>(count);
  }
  if (zip_fclose(file) != 0) {
    error =
        QStringLiteral("XLSX part %1 failed its decompression integrity check.")
            .arg(name);
    return false;
  }
  return true;
}

bool parseCellReference(const QString &reference, int &row, int &column) {
  int i = 0;
  qint64 columnNumber = 0;
  while (i < reference.size() && reference.at(i).isLetter()) {
    const ushort letter = reference.at(i).toUpper().unicode();
    if (letter < 'A' || letter > 'Z')
      return false;
    columnNumber = columnNumber * 26 + (letter - 'A' + 1);
    if (columnNumber > MaxColumns)
      return false;
    ++i;
  }
  if (i == 0 || i >= reference.size())
    return false;
  qint64 rowNumber = 0;
  for (; i < reference.size(); ++i) {
    const QChar digit = reference.at(i);
    if (!digit.isDigit())
      return false;
    rowNumber = rowNumber * 10 + digit.digitValue();
    if (rowNumber > MaxRows)
      return false;
  }
  if (rowNumber < 1 || columnNumber < 1)
    return false;
  row = static_cast<int>(rowNumber - 1);
  column = static_cast<int>(columnNumber - 1);
  return true;
}

bool loadSharedStrings(zip_t *archive, QVector<QString> &strings,
                       QString &error, zip_uint64_t &expandedBytes) {
  QByteArray xml;
  if (!readPart(archive, QStringLiteral("xl/sharedStrings.xml"), xml, error,
                expandedBytes))
    return false;
  QXmlStreamReader reader(xml);
  bool inItem = false;
  QString item;
  qsizetype totalChars = 0;
  while (!reader.atEnd()) {
    const auto token = reader.readNext();
    if (token == QXmlStreamReader::DTD) {
      error =
          QStringLiteral("DOCTYPE declarations are refused in shared strings.");
      return false;
    }
    if (token == QXmlStreamReader::StartElement &&
        reader.name() == QStringLiteral("si")) {
      inItem = true;
      item.clear();
    } else if (token == QXmlStreamReader::StartElement && inItem &&
               reader.name() == QStringLiteral("t")) {
      item += reader.readElementText(QXmlStreamReader::IncludeChildElements);
    } else if (token == QXmlStreamReader::EndElement &&
               reader.name() == QStringLiteral("si") && inItem) {
      totalChars += item.size();
      if (strings.size() >= MaxCells || totalChars > 32 * 1024 * 1024) {
        error = QStringLiteral("XLSX shared strings exceed the safety limit.");
        return false;
      }
      strings.append(item);
      inItem = false;
    }
  }
  if (reader.hasError()) {
    error = QStringLiteral("Malformed XLSX sharedStrings.xml: %1")
                .arg(reader.errorString());
    return false;
  }
  return true;
}

bool loadRelationships(zip_t *archive, QHash<QString, QString> &targets,
                       QString &error, zip_uint64_t &expandedBytes) {
  QByteArray xml;
  if (!readPart(archive, QStringLiteral("xl/_rels/workbook.xml.rels"), xml,
                error, expandedBytes))
    return false;
  QXmlStreamReader reader(xml);
  while (!reader.atEnd()) {
    const auto token = reader.readNext();
    if (token == QXmlStreamReader::DTD) {
      error = QStringLiteral(
          "DOCTYPE declarations are refused in workbook relationships.");
      return false;
    }
    if (token != QXmlStreamReader::StartElement ||
        reader.name() != QStringLiteral("Relationship"))
      continue;
    const auto attributes = reader.attributes();
    const QString id = attributes.value(QStringLiteral("Id")).toString();
    const QString target =
        attributes.value(QStringLiteral("Target")).toString();
    const QString mode =
        attributes.value(QStringLiteral("TargetMode")).toString();
    if (id.isEmpty() || target.isEmpty() ||
        mode.compare(QStringLiteral("External"), Qt::CaseInsensitive) == 0)
      continue;
    QString normalized = target.startsWith(QLatin1Char('/'))
                             ? target.mid(1)
                             : QStringLiteral("xl/") + target;
    normalized = QDir::cleanPath(normalized);
    if (!normalized.startsWith(QStringLiteral("xl/")) ||
        normalized.contains(QStringLiteral("../"))) {
      error = QStringLiteral("Workbook relationship %1 points outside the XLSX "
                             "package's xl directory.")
                  .arg(id);
      return false;
    }
    targets.insert(id, normalized);
  }
  if (reader.hasError()) {
    error = QStringLiteral("Malformed workbook relationships: %1")
                .arg(reader.errorString());
    return false;
  }
  return true;
}

bool loadWorkbookSheets(zip_t *archive, const QHash<QString, QString> &targets,
                        QVector<SpreadsheetSheet> &sheets, QString &error,
                        zip_uint64_t &expandedBytes) {
  QByteArray xml;
  if (!readPart(archive, QStringLiteral("xl/workbook.xml"), xml, error,
                expandedBytes))
    return false;
  QXmlStreamReader reader(xml);
  while (!reader.atEnd()) {
    const auto token = reader.readNext();
    if (token == QXmlStreamReader::DTD) {
      error =
          QStringLiteral("DOCTYPE declarations are refused in workbook.xml.");
      return false;
    }
    if (token != QXmlStreamReader::StartElement ||
        reader.name() != QStringLiteral("sheet"))
      continue;
    const auto attributes = reader.attributes();
    const QString name = attributes.value(QStringLiteral("name")).toString();
    const QString id =
        attributes.value(RelationshipNamespace, QStringLiteral("id"))
            .toString();
    const auto target = targets.constFind(id);
    if (name.isEmpty() || target == targets.cend()) {
      error = QStringLiteral(
          "A workbook sheet has an invalid or unresolved relationship.");
      return false;
    }
    if (sheets.size() >= MaxSheets) {
      error =
          QStringLiteral("The workbook exceeds the 256-sheet safety limit.");
      return false;
    }
    SpreadsheetSheet sheet;
    sheet.name = name;
    sheet.partPath = *target;
    sheets.append(std::move(sheet));
  }
  if (reader.hasError()) {
    error =
        QStringLiteral("Malformed workbook.xml: %1").arg(reader.errorString());
    return false;
  }
  if (sheets.isEmpty()) {
    error = QStringLiteral("The workbook has no readable worksheets.");
    return false;
  }
  return true;
}

bool loadWorksheet(zip_t *archive, SpreadsheetSheet &sheet,
                   const QVector<QString> &shared, int &allCells,
                   QString &error, zip_uint64_t &expandedBytes) {
  QByteArray xml;
  if (!readPart(archive, sheet.partPath, xml, error, expandedBytes))
    return false;
  QXmlStreamReader reader(xml);
  int row = -1, nextColumn = 0, column = -1;
  QString type, value, formula, inlineText;
  bool inCell = false, inInline = false;
  while (!reader.atEnd()) {
    const auto token = reader.readNext();
    if (token == QXmlStreamReader::DTD) {
      error = QStringLiteral("DOCTYPE declarations are refused in %1.")
                  .arg(sheet.partPath);
      return false;
    }
    if (token == QXmlStreamReader::StartElement) {
      const QString tag = reader.name().toString();
      if (tag == QStringLiteral("row")) {
        bool valid = false;
        const int declared = reader.attributes()
                                 .value(QStringLiteral("r"))
                                 .toString()
                                 .toInt(&valid);
        row = valid ? declared - 1 : row + 1;
        if (row < 0 || row >= MaxRows) {
          error = QStringLiteral("Worksheet row is outside Excel's row limit.");
          return false;
        }
        nextColumn = 0;
        sheet.rows = qMax(sheet.rows, row + 1);
      } else if (tag == QStringLiteral("c")) {
        inCell = true;
        type = reader.attributes().value(QStringLiteral("t")).toString();
        value.clear();
        formula.clear();
        inlineText.clear();
        const QString reference =
            reader.attributes().value(QStringLiteral("r")).toString();
        if (!reference.isEmpty()) {
          int refRow = 0;
          if (!parseCellReference(reference, refRow, column) ||
              (row >= 0 && refRow != row)) {
            error = QStringLiteral("Invalid cell reference %1 in %2.")
                        .arg(reference, sheet.partPath);
            return false;
          }
          row = refRow;
        } else
          column = nextColumn;
        if (row < 0 || row >= MaxRows || column < 0 || column >= MaxColumns) {
          error = QStringLiteral(
              "Worksheet cell is outside Excel's row or column limit.");
          return false;
        }
        nextColumn = column + 1;
      } else if (tag == QStringLiteral("is") && inCell)
        inInline = true;
      else if (inCell && tag == QStringLiteral("f"))
        formula =
            reader.readElementText(QXmlStreamReader::IncludeChildElements);
      else if (inCell && tag == QStringLiteral("v"))
        value = reader.readElementText(QXmlStreamReader::IncludeChildElements);
      else if (inCell && inInline && tag == QStringLiteral("t"))
        inlineText +=
            reader.readElementText(QXmlStreamReader::IncludeChildElements);
    } else if (token == QXmlStreamReader::EndElement) {
      const QString tag = reader.name().toString();
      if (tag == QStringLiteral("is"))
        inInline = false;
      else if (tag == QStringLiteral("c") && inCell) {
        if (type == QStringLiteral("s")) {
          bool valid = false;
          const int sharedIndex = value.toInt(&valid);
          if (!valid || sharedIndex < 0 || sharedIndex >= shared.size()) {
            error =
                QStringLiteral("A shared string reference is invalid in %1.")
                    .arg(sheet.partPath);
            return false;
          }
          value = shared.at(sharedIndex);
        } else if (type == QStringLiteral("inlineStr"))
          value = inlineText;
        else if (type == QStringLiteral("b"))
          value = value == QStringLiteral("1") ? QStringLiteral("TRUE")
                                               : QStringLiteral("FALSE");
        if (++allCells > MaxCells) {
          error =
              QStringLiteral("Workbook exceeds the 300,000-cell safety limit.");
          return false;
        }
        sheet.cells[row].insert(column, {value, formula});
        sheet.columns = qMax(sheet.columns, column + 1);
        inCell = false;
      }
    }
  }
  if (reader.hasError()) {
    error = QStringLiteral("Malformed worksheet %1: %2")
                .arg(sheet.partPath, reader.errorString());
    return false;
  }
  return true;
}
} // namespace

namespace {
QString spreadsheetColumnName(int number) {
  QString name;
  for (++number; number > 0; number = (number - 1) / 26)
    name.prepend(QChar(static_cast<ushort>('A' + ((number - 1) % 26))));
  return name;
}
} // namespace

SpreadsheetTableModel::SpreadsheetTableModel(const SpreadsheetSheet &sheet,
                                             QObject *parent)
    : QAbstractTableModel(parent), sheet_(sheet) {}

int SpreadsheetTableModel::rowCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : sheet_.rows;
}
int SpreadsheetTableModel::columnCount(const QModelIndex &parent) const {
  return parent.isValid() ? 0 : sheet_.columns;
}

QVariant SpreadsheetTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return {};
  const auto row = sheet_.cells.constFind(index.row());
  if (row == sheet_.cells.cend())
    return {};
  const auto cell = row->constFind(index.column());
  if (cell == row->cend())
    return {};
  if (role == Qt::DisplayRole)
    return showFormulas_ && !cell->formula.isEmpty()
               ? QStringLiteral("=") + cell->formula
               : cell->value;
  if (role == Qt::ToolTipRole && !cell->formula.isEmpty())
    return QStringLiteral("Formula: =%1\nCached value: %2\nFormula evaluation "
                          "is not performed by APEX.")
        .arg(cell->formula, cell->value);
  return {};
}

QVariant SpreadsheetTableModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const {
  if (role != Qt::DisplayRole || section < 0)
    return {};
  return orientation == Qt::Vertical ? QVariant(section + 1)
                                     : QVariant(spreadsheetColumnName(section));
}

void SpreadsheetTableModel::setShowFormulas(bool enabled) {
  if (showFormulas_ == enabled)
    return;
  showFormulas_ = enabled;
  if (sheet_.rows > 0 && sheet_.columns > 0)
    emit dataChanged(index(0, 0), index(sheet_.rows - 1, sheet_.columns - 1),
                     {Qt::DisplayRole});
}

bool XlsxWorkbook::load(const QString &path) {
  sheets.clear();
  error.clear();
  cellCount = 0;
  const QFileInfo info(path);
  if (!info.isFile() || info.size() <= 0 || info.size() > MaxPackageBytes) {
    error = QStringLiteral(
        "The XLSX package is empty or exceeds the 256 MiB safety limit.");
    return false;
  }
  int zipError = ZIP_ER_OK;
  const QByteArray encoded = QFile::encodeName(path);
  zip_t *archive = zip_open(encoded.constData(), ZIP_RDONLY, &zipError);
  if (!archive) {
    error = QStringLiteral("This is not a readable XLSX ZIP package.");
    return false;
  }
  const zip_int64_t entryCount = zip_get_num_entries(archive, 0);
  if (entryCount < 0 || entryCount > MaxPackageEntries) {
    error = QStringLiteral(
        "The XLSX package has an invalid or excessive number of ZIP entries.");
    zip_discard(archive);
    return false;
  }
  zip_uint64_t expandedBytes = 0;
  QHash<QString, QString> relationships;
  bool okay =
      loadRelationships(archive, relationships, error, expandedBytes) &&
      loadWorkbookSheets(archive, relationships, sheets, error, expandedBytes);
  QVector<QString> shared;
  if (okay &&
      zip_name_locate(archive, "xl/sharedStrings.xml", ZIP_FL_ENC_GUESS) >= 0)
    okay = loadSharedStrings(archive, shared, error, expandedBytes);
  for (auto &sheet : sheets)
    if (okay)
      okay = loadWorksheet(archive, sheet, shared, cellCount, error,
                           expandedBytes);
  zip_discard(archive);
  if (!okay) {
    sheets.clear();
    cellCount = 0;
    return false;
  }
  return true;
}

QString XlsxWorkbook::searchableText(qsizetype characterLimit) const {
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
