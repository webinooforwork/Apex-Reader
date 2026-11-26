#pragma once
#include <QString>
#include <QStringList>
namespace apex { struct TextDocument { QString text; QString error; bool load(const QString& path); qsizetype words() const; }; }
