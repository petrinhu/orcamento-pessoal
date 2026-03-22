#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QByteArray>

// Manages the SQLite connection lifecycle:
//   open (decrypt .enc → .db) / close (encrypt .db → .enc)
// In QT_DEBUG: uses plain .db with no encryption.
class DatabaseManager {
public:
    static DatabaseManager &instance();

    // Opens (or creates) the database for the given user slug.
    // In release: decrypts data/<slug>.enc using key.
    // In debug:   opens data/<slug>.db directly.
    bool open(const QString &slug, const QByteArray &key = {});
    void close();
    bool isOpen() const;

    QSqlDatabase db() const { return m_db; }

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    void createSchema();
    bool encryptAndSave(const QString &path, const QByteArray &key);
    bool decryptAndLoad(const QString &encPath, const QString &dbPath,
                        const QByteArray &key);

    QSqlDatabase m_db;
    QString      m_dbPath;
    QByteArray   m_key;
};
