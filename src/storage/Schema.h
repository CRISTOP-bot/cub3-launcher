// CUB3 Launcher - Database schema migrations
#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QList>

namespace cub3::storage {

/// Simple version-based schema migration system.
/// Each migration has a version number and an SQL script.
/// Migrations are tracked in a `schema_version` table.
/// Migrations are applied in order and are idempotent.
class Schema {
public:
    explicit Schema(QSqlDatabase& db) : m_db(db) {}

    /// Run all pending migrations. Returns true on success.
    [[nodiscard]] bool migrate();

    /// Get the current schema version.
    [[nodiscard]] int currentVersion() const;

private:
    bool ensureVersionTable();
    bool runMigration(int version, const QString& sql);

    QSqlDatabase& m_db;

    // Migration definitions
    struct Migration {
        int version;
        QString sql;
    };

    static const QList<Migration>& migrations();
};

} // namespace cub3::storage
