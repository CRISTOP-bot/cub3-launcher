// CUB3 Launcher - Schema migration implementation
#include "Schema.h"
#include "core/Logger.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace cub3::storage {

const QList<Schema::Migration>& Schema::migrations() {
    static const QList<Migration> list = {
        // --- v1: Initial schema ---
        {1, R"(
            -- Accounts
            CREATE TABLE IF NOT EXISTS accounts (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                provider     TEXT NOT NULL,           -- 'microsoft' | 'elyby'
                username     TEXT NOT NULL,
                uuid         TEXT,
                access_token TEXT,
                refresh_token TEXT,
                expires_at   INTEGER,
                skin_url     TEXT,
                created_at   INTEGER DEFAULT (strftime('%s', 'now')),
                updated_at   INTEGER DEFAULT (strftime('%s', 'now'))
            );

            -- Active account pointer (singleton)
            CREATE TABLE IF NOT EXISTS active_account (
                account_id  INTEGER REFERENCES accounts(id)
            );

            -- Installations (instances)
            CREATE TABLE IF NOT EXISTS installations (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                name        TEXT NOT NULL,
                version     TEXT NOT NULL,
                loader      TEXT DEFAULT 'vanilla',   -- vanilla|fabric|forge|neoforge|quilt
                loader_version TEXT,
                directory   TEXT NOT NULL,
                java_path   TEXT,
                ram_min     INTEGER DEFAULT 512,
                ram_max     INTEGER DEFAULT 2048,
                resolution_w INTEGER DEFAULT 854,
                resolution_h INTEGER DEFAULT 480,
                created_at  INTEGER DEFAULT (strftime('%s', 'now')),
                updated_at  INTEGER DEFAULT (strftime('%s', 'now'))
            );

            -- Settings (key-value)
            CREATE TABLE IF NOT EXISTS settings (
                key   TEXT PRIMARY KEY,
                value TEXT
            );

            -- Download history
            CREATE TABLE IF NOT EXISTS download_history (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                file_path   TEXT NOT NULL,
                file_size   INTEGER,
                file_hash   TEXT,
                source_url  TEXT,
                status      TEXT DEFAULT 'completed',
                started_at  INTEGER,
                completed_at INTEGER
            );

            -- Cached manifests
            CREATE TABLE IF NOT EXISTS manifest_cache (
                url         TEXT PRIMARY KEY,
                content     TEXT NOT NULL,
                etag        TEXT,
                last_modified TEXT,
                fetched_at  INTEGER DEFAULT (strftime('%s', 'now'))
            );

            -- Indices
            CREATE INDEX IF NOT EXISTS idx_accounts_provider ON accounts(provider);
            CREATE INDEX IF NOT EXISTS idx_installations_version ON installations(version);
            CREATE INDEX IF NOT EXISTS idx_settings_key ON settings(key);
        )"}
    };
    return list;
}

bool Schema::ensureVersionTable() {
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS schema_version ("
        "  version INTEGER PRIMARY KEY,"
        "  applied_at INTEGER DEFAULT (strftime('%s','now'))"
        ")"))) {
        CUB3_ERROR(lcStorage) << "Could not create schema version table:"
                              << query.lastError().text();
        return false;
    }
    return true;
}

int Schema::currentVersion() const {
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT MAX(version) FROM schema_version"))) {
        CUB3_ERROR(lcStorage) << "Could not read schema version:"
                              << query.lastError().text();
        return -1;
    }
    if (query.next())
        return query.value(0).toInt();
    return -1;
}

bool Schema::migrate() {
    if (!ensureVersionTable())
        return false;

    const int current = currentVersion();
    if (current < 0)
        return false;
    const auto& all = migrations();

    int applied = 0;
    for (const auto& m : all) {
        if (m.version <= current)
            continue;

        if (!runMigration(m.version, m.sql))
            return false;
        ++applied;
    }

    if (applied > 0)
        CUB3_INFO(lcStorage) << "Migrations applied:" << applied
                             << "Current version:" << currentVersion();
    return true;
}

bool Schema::runMigration(int version, const QString& sql) {
    if (!m_db.transaction()) {
        CUB3_ERROR(lcStorage) << "Could not start migration transaction:"
                              << m_db.lastError().text();
        return false;
    }

    QSqlQuery query(m_db);
    // Execute each statement separately (SQLite doesn't support multi-statement
    // exec in a single call reliably)
    for (const auto& stmt : sql.split(';')) {
        QString trimmed = stmt.trimmed();
        if (trimmed.isEmpty())
            continue;
        if (!query.exec(trimmed)) {
            CUB3_ERROR(lcStorage) << "Migration" << version
                                  << "failed:" << query.lastError().text()
                                  << "SQL:" << trimmed.left(80);
            m_db.rollback();
            return false;
        }
    }

    // Record the migration
    query.prepare(QStringLiteral(
        "INSERT INTO schema_version (version) VALUES (?)"));
    query.addBindValue(version);
    if (!query.exec()) {
        CUB3_ERROR(lcStorage) << "Could not record migration" << version
                              << ":" << query.lastError().text();
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        CUB3_ERROR(lcStorage) << "Could not commit migration" << version
                              << ":" << m_db.lastError().text();
        m_db.rollback();
        return false;
    }
    CUB3_INFO(lcStorage) << "Migration" << version << "applied";
    return true;
}

} // namespace cub3::storage
