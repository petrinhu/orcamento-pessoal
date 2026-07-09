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
    // Fecha a conexão e persiste (release: cifra .db → .pbd). Retorna false
    // se a cifragem foi ABORTADA por falha de integrity_check (#191/FIN-02):
    // nesse caso o .pbd anterior (último bom) é preservado intacto — o
    // chamador deve alertar o usuário (dado da sessão pode ter sido perdido,
    // mas o cofre em disco não foi corrompido).
    bool close();
    bool isOpen() const;

    // File-level operations (release mode only; debug: plain copy)
    bool exportToFile(const QString &destPath);   // encrypt current DB → destPath
    bool importFromFile(const QString &srcPath);  // decrypt srcPath → replace session

    QSqlDatabase db() const { return m_db; }

    // (#191/FIN-02) Verifica a integridade de um arquivo SQLite via
    // `PRAGMA integrity_check`, usando uma conexão própria e somente-leitura
    // (não interfere com m_db). Retorna true SOMENTE quando o resultado é
    // exatamente "ok". Falha ao abrir, arquivo ausente, ou qualquer resultado
    // diferente de "ok" retornam false (fail-safe: na dúvida, não persiste).
    // Público e static para ser testável isoladamente (sem depender do
    // singleton nem de uma sessão aberta) e para servir de gate único dentro
    // de encryptAndSave() (chamado por save()/close()/exportToFile()).
    static bool checkFileIntegrity(const QString &dbFilePath);

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
