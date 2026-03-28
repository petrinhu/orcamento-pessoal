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
    // In release: decrypts data/<slug>.pbd.
    //   keyNew    — Argon2id-derived key (new format: AES-GCM / XChaCha20).
    //   keyLegacy — PBKDF2-derived key   (legacy format: AES-256-CBC).
    //               Pass empty if no legacy file can exist (new user).
    // On first open after a format upgrade, the file is re-encrypted automatically
    // with keyNew when the database is closed (transparent migration).
    // In debug: opens data/<slug>.db directly (no encryption).
    bool open(const QString &slug,
              const QByteArray &keyNew    = {},
              const QByteArray &keyLegacy = {});
    bool save();  // flush DB → .pbd sem fechar (release); no-op em debug
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
                        const QByteArray &keyNew, const QByteArray &keyLegacy);

    QSqlDatabase m_db;
    QString      m_dbPath;
    QString      m_slug;
    QByteArray   m_key;        // Argon2id key (new format)
    QByteArray   m_keyLegacy;  // PBKDF2 key (legacy format — cleared after migration)
    bool         m_needsReencrypt = false; // true when a legacy file was opened
};
