#pragma once
#include <QString>
namespace apex {
class OpenDocumentText {
public:
  QString text;
  QString error;
  int paragraphCount{};
  int slideCount{};
  bool load(const QString &path, bool presentation = false);
};
} // namespace apex
