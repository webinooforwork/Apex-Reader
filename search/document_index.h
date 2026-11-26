#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <atomic>

namespace apex {
struct SearchHit {
    QString path;
    QString extension;
    QString excerpt;
    qint64 modifiedMs{};
};

class DocumentIndex {
public:
    static QString defaultDatabasePath();
    static QVector<SearchHit> search(const QString& query, const QString& databasePath,
                                     QString* error = nullptr, int limit = 200);
};

class IndexTask final : public QObject {
    Q_OBJECT
public:
    IndexTask(QString root, QString databasePath);
    void cancel() noexcept;

public slots:
    void run();

signals:
    void progress(int completed, int total, const QString& currentPath);
    void finished(int indexed, int skipped, bool cancelled, const QString& error);

private:
    QString root_;
    QString databasePath_;
    std::atomic_bool cancelled_{false};
};
}
