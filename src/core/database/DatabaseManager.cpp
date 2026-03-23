#include "DatabaseManager.h"
#include "core/crypto/AesCryptoService.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <stdexcept>

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::open(const QString &slug, const QByteArray &key)
{
    QDir().mkpath(QStringLiteral("data"));

    m_slug   = slug;
    m_key    = key;
    m_dbPath = QStringLiteral("data/.%1.db").arg(slug);

#ifndef QT_DEBUG
    const QString pbdPath = QStringLiteral("data/%1.pbd").arg(slug);
    if (QFile::exists(pbdPath)) {
        if (!decryptAndLoad(pbdPath, m_dbPath, key))
            return false;
    }
#endif

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("main"));
    m_db.setDatabaseName(m_dbPath);

    if (!m_db.open()) {
        qWarning() << "DatabaseManager::open:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery q(m_db);
    q.exec(QStringLiteral("PRAGMA journal_mode = MEMORY"));
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    q.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));

    createSchema();
    return true;
}

bool DatabaseManager::save()
{
#ifndef QT_DEBUG
    if (!m_db.isOpen()) return false;
    const QString pbdPath = QStringLiteral("data/%1.pbd").arg(m_slug);
    return encryptAndSave(m_dbPath, m_key, pbdPath);
#else
    return true; // debug: SQLite persiste imediatamente
#endif
}

void DatabaseManager::close()
{
    if (!m_db.isOpen()) return;

    const QString dbPath = m_db.databaseName();
    m_db.close();
    m_db = QSqlDatabase();  // libera a referência antes de removeDatabase
    QSqlDatabase::removeDatabase(QStringLiteral("main"));

#ifndef QT_DEBUG
    const QString pbdPath = QStringLiteral("data/%1.pbd").arg(m_slug);
    encryptAndSave(dbPath, m_key, pbdPath);
#endif

    QFile::remove(dbPath);
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
}

bool DatabaseManager::exportToFile(const QString &destPath)
{
    if (!m_db.isOpen()) return false;
#ifndef QT_DEBUG
    return encryptAndSave(m_dbPath, m_key, destPath);
#else
    // Debug: plain copy
    QFile::remove(destPath);
    return QFile::copy(m_dbPath, destPath);
#endif
}

bool DatabaseManager::importFromFile(const QString &srcPath)
{
    if (!QFile::exists(srcPath)) return false;

    const bool wasOpen = m_db.isOpen();
    if (wasOpen) {
        m_db.close();
        m_db = QSqlDatabase();  // libera a referência antes de removeDatabase
        QSqlDatabase::removeDatabase(QStringLiteral("main"));
    }

    bool ok = false;
#ifndef QT_DEBUG
    ok = decryptAndLoad(srcPath, m_dbPath, m_key);
#else
    QFile::remove(m_dbPath);
    ok = QFile::copy(srcPath, m_dbPath);
#endif

    if (!ok) {
        // Restore connection if we were open
        if (wasOpen) {
            m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                             QStringLiteral("main"));
            m_db.setDatabaseName(m_dbPath);
            m_db.open();
        }
        return false;
    }

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("main"));
    m_db.setDatabaseName(m_dbPath);
    if (!m_db.open()) return false;

    QSqlQuery q(m_db);
    q.exec(QStringLiteral("PRAGMA journal_mode = MEMORY"));
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    q.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
    return true;
}

void DatabaseManager::createSchema()
{
    QSqlQuery q(m_db);

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS cartoes (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            apelido TEXT NOT NULL UNIQUE
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS compras (
            id               INTEGER PRIMARY KEY AUTOINCREMENT,
            nome             TEXT    NOT NULL,
            valor_total      INTEGER,
            num_parcelas     INTEGER NOT NULL,
            parcela_inicial  INTEGER NOT NULL DEFAULT 1,
            mes_inicio       INTEGER NOT NULL,
            ano_inicio       INTEGER NOT NULL,
            juros_mensais    INTEGER,
            moeda            TEXT    NOT NULL DEFAULT 'BRL',
            cartao_id        INTEGER REFERENCES cartoes(id) ON DELETE SET NULL,
            criado_em        TEXT    NOT NULL
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS parcelas (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            compra_id   INTEGER NOT NULL REFERENCES compras(id) ON DELETE CASCADE,
            numero      INTEGER NOT NULL,
            valor       INTEGER NOT NULL,
            data_fatura TEXT,
            modificada  INTEGER NOT NULL DEFAULT 0
        )
    )"));
}

bool DatabaseManager::encryptAndSave(const QString &srcDbPath,
                                     const QByteArray &key,
                                     const QString &destPath)
{
    QFile f(srcDbPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray plain = f.readAll();
    f.close();

    AesCryptoService crypto;
    const QByteArray cipher = crypto.encrypt(plain, key);

    const QString tmpPath = destPath + QStringLiteral(".new");
    QFile out(tmpPath);
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(cipher);
    out.close();

    QFile::remove(destPath);
    return QFile::rename(tmpPath, destPath);
}

bool DatabaseManager::decryptAndLoad(const QString &encPath,
                                     const QString &dbPath,
                                     const QByteArray &key)
{
    QFile f(encPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray cipher = f.readAll();
    f.close();

    AesCryptoService crypto;
    QByteArray plain;
    try {
        plain = crypto.decrypt(cipher, key);
    } catch (const std::exception &e) {
        qWarning() << "Decrypt failed:" << e.what();
        return false;
    }

    QFile out(dbPath);
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(plain);
    return true;
}
