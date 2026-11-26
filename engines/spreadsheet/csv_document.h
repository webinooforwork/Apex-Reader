#pragma once

#include <QAbstractTableModel>
#include <QStringList>
#include <QVector>

namespace apex {
class CsvDocument final : public QAbstractTableModel {
  Q_OBJECT
public:
  explicit CsvDocument(QObject *parent = nullptr);
  bool load(const QString &path, QChar delimiter, QString *error = nullptr);
  QModelIndex findNext(const QString &query,
                       const QModelIndex &after = {}) const;
  QString rawText() const { return rawText_; }
  int rowCount(const QModelIndex &parent = {}) const override;
  int columnCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

private:
  QVector<QStringList> rows_;
  QString rawText_;
  int columns_{};
};
} // namespace apex
