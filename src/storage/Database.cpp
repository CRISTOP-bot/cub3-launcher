// CUB3 Launcher - Database implementation
#include "Database.h"
#include "Schema.h"
#include "core/Logger.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QDir>
#include <QFileInfo>
#include <QUuid>

namespace cub3::storage {

Database::Database(const QString& filePath, QObject* parent)
    : QObject(parent)
    , m_filePath(filePath)
    , m_connectionName(QStringLiteral("cub3_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
    // Ensure the parent directory exists
    QFileInfo fi(filePath);
    QDir().mkpath(fi.absolutePath());
}

Database::~Database() {
    close();
}

bool Database::open() {
    QMutexLocker locker(&m_mutex);

    if (m_db.isOpen())
        return true;

    if (QSqlDatabase::contains(m_connectionName))
        m_db = QSqlDatabase::database(m_connectionName);
    else
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                          m_connectionName);
    m_db.setDatabaseName(m_filePath);

    m_db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));

    if (!m_db.open()) {
        CUB3_ERROR(lcStorage) << "Failed to open database:"
                              << m_db.lastError().text();
        return false;
    }

    QSqlQuery journalMode(m_db);
    if (!journalMode.exec(QStringLiteral("PRAGMA journal_mode=WAL"))
        || !journalMode.next()
        || journalMode.value(0).toString().compare(
               QStringLiteral("wal"), Qt::CaseInsensitive) != 0) {
        CUB3_WARN(lcStorage) << "Could not enable SQLite WAL mode:"
                             << journalMode.lastError().text();
    }

    // Run migrations
    Schema schema(m_db);
    if (!schema.migrate()) {
        CUB3_ERROR(lcStorage) << "Schema migration failed";
        m_db.close();
        return false;
    }

    CUB3_INFO(lcStorage) << "Database opened:" << m_filePath;
    return true;
}

void Database::close() {
    QMutexLocker locker(&m_mutex);
    if (m_db.isOpen()) {
        m_db.close();
    }
    const QString connectionName = m_db.connectionName();
    m_db = QSqlDatabase();
    if (!connectionName.isEmpty())
        QSqlDatabase::removeDatabase(connectionName);
}

QSqlQuery Database::exec(const QString& sql, const QVariantList& params) {
    QMutexLocker locker(&m_mutex);

    QSqlQuery query(m_db);
    query.prepare(sql);
    for (int i = 0; i < params.size(); ++i)
        query.addBindValue(params[i]);

    if (!query.exec()) {
        CUB3_ERROR(lcStorage) << "Query failed:" << sql
                              << "Error:" << query.lastError().text()
                              << "Params:" << params.size();
    }
    return query;
}

QVariant Database::queryValue(const QString& sql, const QVariantList& params) {
    auto query = exec(sql, params);
    if (query.next())
        return query.value(0);
    return {};
}

QList<QHash<QString, QVariant>> Database::queryRows(
    const QString& sql, const QVariantList& params)
{
    QList<QHash<QString, QVariant>> rows;
    auto query = exec(sql, params);

    const int colCount = query.record().count();
    while (query.next()) {
        QHash<QString, QVariant> row;
        for (int i = 0; i < colCount; ++i)
            row[query.record().fieldName(i)] = query.value(i);
        rows.append(std::move(row));
    }
    return rows;
}

void Database::beginTransaction() {
    exec(QStringLiteral("BEGIN TRANSACTION"));
}

void Database::commit() {
    exec(QStringLiteral("COMMIT"));
}

void Database::rollback() {
    exec(QStringLiteral("ROLLBACK"));
}

} // namespace cub3::storage
