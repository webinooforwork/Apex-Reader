#include "engines/text_document.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QStringConverter>
namespace apex {
bool TextDocument::load(const QString& path) { QFile f(path); if(!f.open(QIODevice::ReadOnly)){error=f.errorString();return false;} if(f.size()>32*1024*1024){error="Text file exceeds the 32 MiB safety limit.";return false;} QTextStream in(&f); in.setEncoding(QStringConverter::Utf8); text=in.readAll(); if(in.status()!=QTextStream::Ok){error="Could not decode this file as readable UTF-8 text.";return false;} return true; }
qsizetype TextDocument::words() const { return text.split(QRegularExpression("\\s+"),Qt::SkipEmptyParts).size(); }
}
