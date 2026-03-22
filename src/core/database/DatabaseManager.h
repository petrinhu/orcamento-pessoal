#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QByteArray>

// Manages the SQLite connection lifecycle:
//   open (decrypt .pbd → .db) / close (encrypt .db → .pbd)
// In QT_DEBUG: uses plain .db with no encryption.
class DatabaseManager {
public:
    static DatabaseManager &instance();

    // Opens (or creates) the database for the given user slug.
    // In release: decrypts data/<slug>.pbd using key.
    // In debug:   opens data/<slug>.db directly.
    bool open(const QString &slug, const QByteArray &key = {});
    void close();
    bool isOpen() const;

    // File-level operations (release mode only; debug: plain copy)
    bool exportToFile(const QString &destPath);   // encrypt current DB → destPath
    bool importFromFile(const QString &srcPath);  // decrypt srcPath → replace session

    QSqlDatabase db() const { return m_db; }

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    void createSchema();
    bool encryptAndSave(const QString &srcDbPath, const QByteArray &key,
                        const QString &destPath);
    bool decryptAndLoad(const QString &encPath, const QString &dbPath,
                        const QByteArray &key);

    QSqlDatabase m_db;
    QString      m_dbPath;
    QString      m_slug;
    QByteArray   m_key;
};
