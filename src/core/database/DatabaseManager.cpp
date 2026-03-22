#include "DatabaseManager.h"
#include "core/crypto/AesCryptoService.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>
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

    m_key    = key;
    m_dbPath = QStringLiteral("data/.%1.db").arg(slug);

#ifndef QT_DEBUG
    const QString encPath = QStringLiteral("data/%1.enc").arg(slug);
    if (QFile::exists(encPath)) {
        if (!decryptAndLoad(encPath, m_dbPath, key))
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

void DatabaseManager::close()
{
    if (!m_db.isOpen()) return;

    const QString dbPath = m_db.databaseName();
    m_db.close();
    QSqlDatabase::removeDatabase(QStringLiteral("main"));

#ifndef QT_DEBUG
    const QString slug    = QFileInfo(dbPath).baseName().mid(1); // remove leading dot
    const QString encPath = QStringLiteral("data/%1.enc").arg(slug);
    encryptAndSave(dbPath, m_key);
#endif

    QFile::remove(dbPath);
}

bool DatabaseManager::isOpen() const
{
    return m_db.isOpen();
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

bool DatabaseManager::encryptAndSave(const QString &dbPath, const QByteArray &key)
{
    QFile f(dbPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray plain = f.readAll();
    f.close();

    AesCryptoService crypto;
    const QByteArray cipher = crypto.encrypt(plain, key);

    const QString tmpPath = dbPath + QStringLiteral(".new");
    QFile out(tmpPath);
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(cipher);
    out.close();

    // Atomic rename
    const QString encPath = QStringLiteral("data/%1.enc")
        .arg(QFileInfo(dbPath).baseName().mid(1));
    QFile::remove(encPath);
    return QFile::rename(tmpPath, encPath);
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
