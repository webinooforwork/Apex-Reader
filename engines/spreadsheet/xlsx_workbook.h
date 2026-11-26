#pragma once
#include <QAbstractTableModel>
#include <QHash>
#include <QMap>
#include <QString>
#include <QVector>
namespace apex {
struct XlsxCell {
  QString value;
  QString formula;
};
struct XlsxSheet {
  QString name;
  QString partPath;
  int rows{};
  int columns{};
  QMap<int, QHash<int, XlsxCell>> cells;
};
class XlsxTableModel final : public QAbstractTableModel {
  Q_OBJECT
public:
  explicit XlsxTableModel(const XlsxSheet &sheet, QObject *parent = nullptr);
  int rowCount(const QModelIndex &parent = {}) const override;
  int columnCount(const QModelIndex &parent = {}) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  void setShowFormulas(bool enabled);

private:
  XlsxSheet sheet_;
  bool showFormulas_{};
};

class XlsxWorkbook {
public:
  QVector<XlsxSheet> sheets;
  QString error;
  int cellCount{};
  bool load(const QString &path);
  QString searchableText(qsizetype characterLimit = 32 * 1024 * 1024) const;
};
} // namespace apex
