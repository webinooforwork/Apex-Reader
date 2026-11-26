#pragma once
#include <QString>

namespace apex {
struct DocxDocument {
    QString text;
    QString error;
    qsizetype paragraphCount{};

    bool load(const QString& path);
    qsizetype words() const;
};
}
