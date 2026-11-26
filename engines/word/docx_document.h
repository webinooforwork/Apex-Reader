#pragma once
#include <QString>
#include <QVector>

namespace apex {
struct DocxDocument {
  struct Comment {
    QString id;
    QString author;
    QString date;
    QString text;
  };

  QString text;
  QString error;
  qsizetype paragraphCount{};
  qsizetype insertedRevisionCount{};
  qsizetype deletedRevisionCount{};
  QVector<Comment> comments;

  bool load(const QString &path);
  qsizetype words() const;
};
} // namespace apex
