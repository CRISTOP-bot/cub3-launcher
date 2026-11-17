// CUB3 Launcher - SQLite database wrapper
#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QVariant>
#include <QSqlQuery>
#include <QHash>
#include <QMutex>

namespace cub3::storage {

/// Thin wrapper around Qt's SQL module for SQLite.
///
/// Design:
///   - A dedicated connection, used from any thread via serialised access.
///   - Schema migrations are run on open() automatically.
///   - All queries use parameter binding — no string interpolation of values.
///   - Bulk operations are wrapped in transactions.
class Database : public QObject {
    Q_OBJECT

public:
    explicit Database(const QString& filePath, QObject* parent = nullptr);
    ~Database() override;

    /// Open the database and run migrations. Returns false on failure.
    [[nodiscard]] bool open();

    /// Close the database cleanly.
    void close();

    /// Execute a parameterised query.
    /// Example: db.exec("INSERT INTO accounts (name) VALUES (?)", {"Steve"})
    [[nodiscard]] QSqlQuery exec(const QString& sql,
                                   const QVariantList& params = {});

    /// Execute a query and return the first row's first column.
    /// Returns an invalid QVariant if no rows.
    [[nodiscard]] QVariant queryValue(const QString& sql,
                                      const QVariantList& params = {});

    /// Execute a query and return all matching rows.
    /// Each row is a QHash of column name -> value.
    [[nodiscard]] QList<QHash<QString, QVariant>> queryRows(
        const QString& sql, const QVariantList& params = {});

    /// Begin a transaction.
    void beginTransaction();
    /// Commit the current transaction.
    void commit();
    /// Roll back the current transaction.
    void rollback();

    /// Check if the database is open.
    [[nodiscard]] bool isOpen() const { return m_db.isOpen(); }

    /// Get the database file path.
    [[nodiscard]] const QString& filePath() const { return m_filePath; }

private:
    QString m_filePath;
    QString m_connectionName;
    QSqlDatabase m_db;
    QMutex m_mutex;
};

} // namespace cub3::storage
