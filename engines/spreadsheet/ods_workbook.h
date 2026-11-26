#pragma once
#include "xlsx_workbook.h"
namespace apex {
class OdsWorkbook {
public:
  QVector<SpreadsheetSheet> sheets;
  QString error;
  int cellCount{};
  bool load(const QString &path);
  QString searchableText(qsizetype characterLimit = 32 * 1024 * 1024) const;
};
} // namespace apex
