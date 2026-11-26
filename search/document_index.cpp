#include "document_index.h"
#include "../engines/spreadsheet/xlsx_workbook.h"
#include "../engines/text_document.h"
#include "../engines/word/docx_document.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <sqlite3.h>

#include <memory>
#include <utility>

namespace apex {
namespace {
struct DatabaseCloser {
  void operator()(sqlite3 *db) const {
    if (db)
      sqlite3_close(db);
  }
};
using Database = std::unique_ptr<sqlite3, DatabaseCloser>;
struct StatementCloser {
  void operator()(sqlite3_stmt *statement) const {
    if (statement)
      sqlite3_finalize(statement);
  }
};
using Statement = std::unique_ptr<sqlite3_stmt, StatementCloser>;

bool openDatabase(const QString &path, Database &database, QString &error) {
  sqlite3 *raw = nullptr;
  if (sqlite3_open16(path.utf16(), &raw) != SQLITE_OK) {
    error =
        raw ? QString::fromUtf8(sqlite3_errmsg(raw))
            : QStringLiteral("SQLite could not allocate a database handle.");
    if (raw)
      sqlite3_close(raw);
    return false;
  }
  database.reset(raw);
  QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
  sqlite3_busy_timeout(database.get(), 5000);
  return true;
}

bool execute(sqlite3 *db, const char *sql, QString &error) {
  char *message = nullptr;
  if (sqlite3_exec(db, sql, nullptr, nullptr, &message) == SQLITE_OK)
    return true;
  error = QString::fromUtf8(message ? message : sqlite3_errmsg(db));
  sqlite3_free(message);
  return false;
}

Statement prepare(sqlite3 *db, const char *sql, QString &error) {
  sqlite3_stmt *raw = nullptr;
  if (sqlite3_prepare_v2(db, sql, -1, &raw, nullptr) != SQLITE_OK) {
    error = QString::fromUtf8(sqlite3_errmsg(db));
    return {};
  }
  return Statement(raw);
}

bool bindText(sqlite3_stmt *statement, int column, const QString &value) {
  const QByteArray utf8 = value.toUtf8();
  return sqlite3_bind_text(statement, column, utf8.constData(), utf8.size(),
                           SQLITE_TRANSIENT) == SQLITE_OK;
}

QString ftsQuery(const QString &input) {
  const auto terms = input.toCaseFolded().split(
      QRegularExpression("[^\\p{L}\\p{N}_]+"), Qt::SkipEmptyParts);
  QStringList quoted;
  quoted.reserve(terms.size());
  for (const auto &term : terms)
    quoted.append(QStringLiteral("\"") + term + QLatin1Char('"'));
  return quoted.join(" AND ");
}

QStringList indexableExtensions() {
  return {"txt", "md", "log", "csv", "tsv", "docx", "xlsx"};
}

QString extractText(const QString &path, const QString &extension,
                    QString &error) {
  if (extension == QStringLiteral("xlsx")) {
    XlsxWorkbook workbook;
    if (!workbook.load(path)) {
      error = workbook.error;
      return {};
    }
    return workbook.searchableText();
  }
  if (extension == QStringLiteral("docx")) {
    DocxDocument document;
    if (!document.load(path)) {
      error = document.error;
      return {};
    }
    return document.text;
  }
  TextDocument document;
  if (!document.load(path)) {
    error = document.error;
    return {};
  }
  return document.text;
}

bool prepareSchema(sqlite3 *db, QString &error) {
  if (!execute(db, "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;",
               error))
    return false;
  return execute(
      db,
      "CREATE VIRTUAL TABLE IF NOT EXISTS indexed_documents USING fts5("
      "path UNINDEXED, extension UNINDEXED, file_size UNINDEXED, modified_ms "
      "UNINDEXED, filename, content, "
      "tokenize='unicode61 remove_diacritics 2');",
      error);
}
} // namespace

QString DocumentIndex::defaultDatabasePath() {
  QString base =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  if (base.isEmpty())
    base = QDir::homePath() + QStringLiteral("/.apex-office-reader");
  QDir().mkpath(base);
  QFile::setPermissions(base, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner);
  return QDir(base).filePath(QStringLiteral("document-index.sqlite3"));
}

QVector<SearchHit> DocumentIndex::search(const QString &query,
                                         const QString &databasePath,
                                         QString *error, int limit) {
  QVector<SearchHit> hits;
  if (error)
    error->clear();
  const QString match = ftsQuery(query);
  if (match.isEmpty())
    return hits;
  if (!QFileInfo::exists(databasePath)) {
    if (error)
      *error =
          QStringLiteral("No search index exists yet. Index a folder first.");
    return hits;
  }

  Database database;
  QString message;
  if (!openDatabase(databasePath, database, message)) {
    if (error)
      *error = message;
    return hits;
  }
  auto statement =
      prepare(database.get(),
              "SELECT path, extension, modified_ms, snippet(indexed_documents, "
              "5, '<b>', '</b>', '…', 24) "
              "FROM indexed_documents WHERE indexed_documents MATCH ? ORDER BY "
              "rank LIMIT ?;",
              message);
  if (!statement) {
    if (error)
      *error = message;
    return hits;
  }
  if (!bindText(statement.get(), 1, match) ||
      sqlite3_bind_int(statement.get(), 2, qBound(1, limit, 1000)) !=
          SQLITE_OK) {
    if (error)
      *error = QString::fromUtf8(sqlite3_errmsg(database.get()));
    return hits;
  }
  int result = SQLITE_ROW;
  while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
    const auto columnText = [&statement](int index) {
      const auto *value = sqlite3_column_text(statement.get(), index);
      return value ? QString::fromUtf8(reinterpret_cast<const char *>(value))
                   : QString();
    };
    const QString path = columnText(0);
    if (QFileInfo::exists(path))
      hits.append({path, columnText(1), columnText(3),
                   sqlite3_column_int64(statement.get(), 2)});
  }
  if (result != SQLITE_DONE && error)
    *error = QString::fromUtf8(sqlite3_errmsg(database.get()));
  return hits;
}

IndexTask::IndexTask(QString root, QString databasePath)
    : root_(QDir(root).absolutePath()), databasePath_(std::move(databasePath)) {
}

void IndexTask::cancel() noexcept {
  cancelled_.store(true, std::memory_order_relaxed);
}

void IndexTask::run() {
  QString error;
  const QFileInfo rootInfo(root_);
  if (!rootInfo.isDir() || !rootInfo.isReadable()) {
    emit finished(0, 0, false,
                  QStringLiteral("The selected folder is not readable."));
    return;
  }
  Database database;
  if (!openDatabase(databasePath_, database, error) ||
      !prepareSchema(database.get(), error)) {
    emit finished(0, 0, false, error);
    return;
  }

  QVector<QFileInfo> candidates;
  QDirIterator iterator(root_,
                        QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);
  const auto extensions = indexableExtensions();
  while (iterator.hasNext() && !cancelled_.load(std::memory_order_relaxed)) {
    iterator.next();
    const QFileInfo info = iterator.fileInfo();
    if (!info.isSymLink() && extensions.contains(info.suffix().toLower()))
      candidates.append(info);
  }

  auto unchanged = prepare(database.get(),
                           "SELECT 1 FROM indexed_documents WHERE path=? AND "
                           "file_size=? AND modified_ms=? LIMIT 1;",
                           error);
  auto removeOld = prepare(
      database.get(), "DELETE FROM indexed_documents WHERE path=?;", error);
  auto insert = prepare(database.get(),
                        "INSERT INTO "
                        "indexed_documents(path,extension,file_size,modified_"
                        "ms,filename,content) VALUES(?,?,?,?,?,?);",
                        error);
  if (!unchanged || !removeOld || !insert) {
    emit finished(0, 0, false, error);
    return;
  }

  int indexed = 0;
  int skipped = 0;
  const int total = candidates.size();
  for (int i = 0; i < total && !cancelled_.load(std::memory_order_relaxed);
       ++i) {
    const QFileInfo &info = candidates.at(i);
    const QString path = info.absoluteFilePath();
    const QString extension = info.suffix().toLower();
    emit progress(i, total, path);

    sqlite3_reset(unchanged.get());
    sqlite3_clear_bindings(unchanged.get());
    bindText(unchanged.get(), 1, path);
    sqlite3_bind_int64(unchanged.get(), 2, info.size());
    sqlite3_bind_int64(unchanged.get(), 3,
                       info.lastModified().toMSecsSinceEpoch());
    const bool isUnchanged = sqlite3_step(unchanged.get()) == SQLITE_ROW;
    if (isUnchanged)
      continue;

    QString parseError;
    const QString content = extractText(path, extension, parseError);
    if (!parseError.isEmpty()) {
      sqlite3_reset(removeOld.get());
      sqlite3_clear_bindings(removeOld.get());
      bindText(removeOld.get(), 1, path);
      sqlite3_step(removeOld.get());
      ++skipped;
      continue;
    }

    if (!execute(database.get(), "BEGIN IMMEDIATE;", error))
      break;
    sqlite3_reset(removeOld.get());
    sqlite3_clear_bindings(removeOld.get());
    if (!bindText(removeOld.get(), 1, path) ||
        sqlite3_step(removeOld.get()) != SQLITE_DONE) {
      execute(database.get(), "ROLLBACK;", error);
      break;
    }
    sqlite3_reset(insert.get());
    sqlite3_clear_bindings(insert.get());
    const bool bound =
        bindText(insert.get(), 1, path) &&
        bindText(insert.get(), 2, extension) &&
        sqlite3_bind_int64(insert.get(), 3, info.size()) == SQLITE_OK &&
        sqlite3_bind_int64(insert.get(), 4,
                           info.lastModified().toMSecsSinceEpoch()) ==
            SQLITE_OK &&
        bindText(insert.get(), 5, info.fileName()) &&
        bindText(insert.get(), 6, content);
    if (!bound || sqlite3_step(insert.get()) != SQLITE_DONE) {
      error = QString::fromUtf8(sqlite3_errmsg(database.get()));
      execute(database.get(), "ROLLBACK;", error);
      ++skipped;
      continue;
    }
    if (!execute(database.get(), "COMMIT;", error)) {
      execute(database.get(), "ROLLBACK;", error);
      break;
    }
    ++indexed;
  }
  emit progress(total, total, {});
  emit finished(indexed, skipped, cancelled_.load(std::memory_order_relaxed),
                error);
}
} // namespace apex
