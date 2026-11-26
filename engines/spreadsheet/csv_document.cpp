#include "csv_document.h"
#include "../text_document.h"

#include <utility>

namespace apex {
namespace {
constexpr qsizetype MaxCellCount = 500000;
constexpr qsizetype MaxColumns = 10000;

QString columnName(int number) {
    QString name;
    for (++number; number > 0; number = (number - 1) / 26)
        name.prepend(QChar(QLatin1Char('A').unicode() + ((number - 1) % 26)));
    return name;
}

bool parseDelimited(const QString& input, QChar delimiter, QVector<QStringList>& rows, QString& error) {
    QStringList row;
    QString field;
    bool quoted = false;
    bool fieldStarted = false;
    bool afterQuote = false;
    qsizetype cells = 0;
    const auto appendRow = [&]() {
        row.append(field);
        field.clear();
        fieldStarted = false;
        cells += row.size();
        if (cells > MaxCellCount || row.size() > MaxColumns) return false;
        rows.append(row);
        row.clear();
        return true;
    };
    for (qsizetype i = 0; i < input.size(); ++i) {
        const QChar c = input.at(i);
        if (quoted) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < input.size() && input.at(i + 1) == QLatin1Char('"')) { field += QLatin1Char('"'); ++i; }
                else { quoted = false; afterQuote = true; }
            } else field += c;
            continue;
        }
        if (afterQuote) {
            if (c == delimiter) { row.append(field); field.clear(); fieldStarted = false; afterQuote = false; continue; }
            if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
                if (c == QLatin1Char('\r') && i + 1 < input.size() && input.at(i + 1) == QLatin1Char('\n')) ++i;
                afterQuote = false;
                if (!appendRow()) { error = QStringLiteral("The delimited file exceeds the 500,000-cell or 10,000-column safety limit."); return false; }
                continue;
            }
            error = QStringLiteral("Unexpected character after a closing quote at character %1.").arg(i);
            return false;
        }
        if (c == QLatin1Char('"')) {
            if (fieldStarted || !field.isEmpty()) { error = QStringLiteral("A quote appeared in an unquoted field at character %1.").arg(i); return false; }
            quoted = true;
            fieldStarted = true;
        } else if (c == delimiter) {
            row.append(field); field.clear(); fieldStarted = false;
        } else if (c == QLatin1Char('\n') || c == QLatin1Char('\r')) {
            if (c == QLatin1Char('\r') && i + 1 < input.size() && input.at(i + 1) == QLatin1Char('\n')) ++i;
            if (!appendRow()) { error = QStringLiteral("The delimited file exceeds the 500,000-cell or 10,000-column safety limit."); return false; }
        } else {
            field += c;
            fieldStarted = true;
        }
    }
    if (quoted) { error = QStringLiteral("The file ends inside a quoted field."); return false; }
    if (fieldStarted || !field.isEmpty() || !row.isEmpty()) {
        row.append(field);
        cells += row.size();
        if (cells > MaxCellCount || row.size() > MaxColumns) { error = QStringLiteral("The delimited file exceeds the 500,000-cell or 10,000-column safety limit."); return false; }
        rows.append(row);
    }
    return true;
}
}

CsvDocument::CsvDocument(QObject* parent) : QAbstractTableModel(parent) {}

bool CsvDocument::load(const QString& path, QChar delimiter, QString* error) {
    if (error) error->clear();
    TextDocument source;
    if (!source.load(path)) { if (error) *error = source.error; return false; }
    QString parseError;
    QVector<QStringList> parsed;
    QString parseText = source.text;
    if (parseText.startsWith(QChar(0xFEFF))) parseText.remove(0, 1);
    if (!parseDelimited(parseText, delimiter, parsed, parseError)) { if (error) *error = parseError; return false; }
    int parsedColumns = 0;
    for (const auto& row : parsed) parsedColumns = qMax(parsedColumns, row.size());
    beginResetModel();
    rows_ = std::move(parsed);
    rawText_ = source.text;
    columns_ = parsedColumns;
    endResetModel();
    return true;
}

int CsvDocument::rowCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : rows_.size(); }
int CsvDocument::columnCount(const QModelIndex& parent) const { return parent.isValid() ? 0 : columns_; }

QVariant CsvDocument::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole || index.row() < 0 || index.row() >= rows_.size()) return {};
    const auto& row = rows_.at(index.row());
    return index.column() >= 0 && index.column() < row.size() ? QVariant(row.at(index.column())) : QVariant();
}

QVariant CsvDocument::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || section < 0) return {};
    if (orientation == Qt::Vertical) return section + 1;
    if (!rows_.isEmpty() && section < rows_.first().size() && !rows_.first().at(section).trimmed().isEmpty()) return rows_.first().at(section);
    return QStringLiteral("Column %1").arg(columnName(section));
}

QModelIndex CsvDocument::findNext(const QString& query, const QModelIndex& after) const {
    if (query.isEmpty() || rows_.isEmpty() || columns_ == 0) return {};
    const qsizetype total = static_cast<qsizetype>(rows_.size()) * columns_;
    const bool continueAfter = after.isValid() && after.model() == this;
    const qsizetype start = continueAfter ? (static_cast<qsizetype>(after.row()) * columns_ + after.column() + 1) % total : 0;
    for (qsizetype offset = 0; offset < total; ++offset) {
        const qsizetype cell = (start + offset) % total;
        const int row = static_cast<int>(cell / columns_);
        const int column = static_cast<int>(cell % columns_);
        if (column < rows_.at(row).size() && rows_.at(row).at(column).contains(query, Qt::CaseInsensitive)) return index(row, column);
    }
    return {};
}
}
