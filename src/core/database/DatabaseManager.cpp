#include "DatabaseManager.h"
#include "core/crypto/AesCryptoService.h"
#include "core/crypto/SodiumCryptoService.h"
#include "services/FechamentoMensalService.h"
#include "services/AuditLogService.h"

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

bool DatabaseManager::open(const QString &slug,
                           const QByteArray &keyNew,
                           const QByteArray &keyLegacy)
{
    QDir().mkpath(QStringLiteral("data"));

    m_slug        = slug;
    m_key         = keyNew;
    m_keyLegacy   = keyLegacy;
    m_needsReencrypt = false;
    m_dbPath      = QStringLiteral("data/.%1.db").arg(slug);

#ifndef QT_DEBUG
    const QString pbdPath = QStringLiteral("data/%1.pbd").arg(slug);
    if (QFile::exists(pbdPath)) {
        if (!decryptAndLoad(pbdPath, m_dbPath, keyNew, keyLegacy))
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

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS fontes_renda (
            id   INTEGER PRIMARY KEY AUTOINCREMENT,
            nome TEXT    NOT NULL UNIQUE
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS lancamentos_entradas (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            mes         INTEGER NOT NULL,
            ano         INTEGER NOT NULL,
            dia         INTEGER NOT NULL DEFAULT 1,
            fonte       TEXT    NOT NULL,
            fonte_id    INTEGER REFERENCES fontes_renda(id) ON DELETE SET NULL,
            valor_bruto INTEGER NOT NULL DEFAULT 0,
            irpf        INTEGER NOT NULL DEFAULT 0,
            inss        INTEGER NOT NULL DEFAULT 0,
            obs         TEXT    NOT NULL DEFAULT '',
            criado_em   TEXT    NOT NULL
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS lancamentos_gastos (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            mes         INTEGER NOT NULL,
            ano         INTEGER NOT NULL,
            dia         INTEGER NOT NULL DEFAULT 1,
            descricao   TEXT    NOT NULL DEFAULT '',
            categoria   TEXT    NOT NULL DEFAULT '',
            tipo_pgto   TEXT    NOT NULL DEFAULT 'outro',
            valor       INTEGER NOT NULL DEFAULT 0,
            obs         TEXT    NOT NULL DEFAULT '',
            criado_em   TEXT    NOT NULL
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS orcamento_itens (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            nome           TEXT    NOT NULL,
            tipo           TEXT    NOT NULL DEFAULT 'despesa',
            valor_mensal   INTEGER NOT NULL DEFAULT 0,
            categoria_ref  TEXT    NOT NULL DEFAULT '',
            criado_em      TEXT    NOT NULL
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS lancamentos_investimentos (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            mes            INTEGER NOT NULL,
            ano            INTEGER NOT NULL,
            dia            INTEGER NOT NULL DEFAULT 1,
            nome           TEXT    NOT NULL DEFAULT '',
            tipo_ativo     TEXT    NOT NULL DEFAULT 'outro',
            tipo_operacao  TEXT    NOT NULL DEFAULT 'aporte',
            valor          INTEGER NOT NULL DEFAULT 0,
            obs            TEXT    NOT NULL DEFAULT '',
            criado_em      TEXT    NOT NULL
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS transacoes_recorrentes (
            id                INTEGER PRIMARY KEY AUTOINCREMENT,
            descricao         TEXT    NOT NULL DEFAULT '',
            tipo              TEXT    NOT NULL DEFAULT 'gasto',
            valor             INTEGER NOT NULL DEFAULT 0,
            fonte_id          INTEGER REFERENCES fontes_renda(id) ON DELETE SET NULL,
            fonte             TEXT    NOT NULL DEFAULT '',
            categoria         TEXT    NOT NULL DEFAULT '',
            tipo_pgto         TEXT    NOT NULL DEFAULT '',
            obs               TEXT    NOT NULL DEFAULT '',
            frequencia        TEXT    NOT NULL DEFAULT 'mensal',
            data_inicio       TEXT    NOT NULL DEFAULT '',
            data_fim          TEXT,
            ativo             INTEGER NOT NULL DEFAULT 1,
            ultimo_lancamento TEXT,
            criado_em         TEXT    NOT NULL
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS categorias (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            nome    TEXT    NOT NULL,
            tipo    TEXT    NOT NULL DEFAULT 'gasto',
            pai_id  INTEGER REFERENCES categorias(id) ON DELETE SET NULL
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS pagadores (
            id   INTEGER PRIMARY KEY AUTOINCREMENT,
            nome TEXT    NOT NULL UNIQUE,
            tipo TEXT    NOT NULL DEFAULT 'ambos',
            obs  TEXT    NOT NULL DEFAULT ''
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS dividas (
            id            INTEGER PRIMARY KEY AUTOINCREMENT,
            tipo          TEXT    NOT NULL DEFAULT 'divida',
            contraparte   TEXT    NOT NULL DEFAULT '',
            descricao     TEXT    NOT NULL DEFAULT '',
            valor_total   INTEGER NOT NULL DEFAULT 0,
            valor_pago    INTEGER NOT NULL DEFAULT 0,
            juros_mensal  INTEGER NOT NULL DEFAULT 0,
            data_inicio   TEXT    NOT NULL DEFAULT '',
            data_venc     TEXT    NOT NULL DEFAULT '',
            status        TEXT    NOT NULL DEFAULT 'em_andamento',
            obs           TEXT    NOT NULL DEFAULT '',
            criado_em     TEXT    NOT NULL DEFAULT ''
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS metas (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            nome        TEXT    NOT NULL DEFAULT '',
            categoria   TEXT    NOT NULL DEFAULT 'outro',
            valor_meta  INTEGER NOT NULL DEFAULT 0,
            valor_atual INTEGER NOT NULL DEFAULT 0,
            data_inicio TEXT    NOT NULL DEFAULT '',
            data_prazo  TEXT    NOT NULL DEFAULT '',
            status      TEXT    NOT NULL DEFAULT 'em_andamento',
            obs         TEXT    NOT NULL DEFAULT '',
            criado_em   TEXT    NOT NULL DEFAULT ''
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS lembretes (
            id                INTEGER PRIMARY KEY AUTOINCREMENT,
            descricao         TEXT    NOT NULL DEFAULT '',
            valor             INTEGER NOT NULL DEFAULT 0,
            data_venc         TEXT    NOT NULL DEFAULT '',
            frequencia        TEXT    NOT NULL DEFAULT 'unico',
            debito_automatico INTEGER NOT NULL DEFAULT 0,
            status            TEXT    NOT NULL DEFAULT 'pendente',
            obs               TEXT    NOT NULL DEFAULT '',
            criado_em         TEXT    NOT NULL DEFAULT ''
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS precos_ativos (
            ticker      TEXT    NOT NULL,
            data        TEXT    NOT NULL,
            abertura    INTEGER NOT NULL DEFAULT 0,
            maxima      INTEGER NOT NULL DEFAULT 0,
            minima      INTEGER NOT NULL DEFAULT 0,
            fechamento  INTEGER NOT NULL DEFAULT 0,
            volume      INTEGER NOT NULL DEFAULT 0,
            fonte       TEXT    NOT NULL DEFAULT '',
            atualizado  TEXT    NOT NULL DEFAULT '',
            PRIMARY KEY (ticker, data)
        )
    )"));

    q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS cotacoes_tempo_real (
            ticker          TEXT    PRIMARY KEY,
            preco           INTEGER NOT NULL DEFAULT 0,
            variacao_pct    INTEGER NOT NULL DEFAULT 0,
            abertura        INTEGER NOT NULL DEFAULT 0,
            minima          INTEGER NOT NULL DEFAULT 0,
            maxima          INTEGER NOT NULL DEFAULT 0,
            volume          INTEGER NOT NULL DEFAULT 0,
            timestamp       TEXT    NOT NULL DEFAULT ''
        )
    )"));

    // Migration: add pagador columns to lancamentos_gastos (guard via PRAGMA).
    {
        QSqlQuery pq(m_db);
        pq.exec(QStringLiteral("PRAGMA table_info(lancamentos_gastos)"));
        bool hasPagador = false, hasPagadorId = false;
        while (pq.next()) {
            const QString col = pq.value(1).toString();
            if (col == QLatin1String("pagador"))    hasPagador   = true;
            if (col == QLatin1String("pagador_id")) hasPagadorId = true;
        }
        if (!hasPagador)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_gastos "
                "ADD COLUMN pagador TEXT NOT NULL DEFAULT ''"));
        if (!hasPagadorId)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_gastos "
                "ADD COLUMN pagador_id INTEGER "
                "REFERENCES pagadores(id) ON DELETE SET NULL"));
    }

    // Migration: add categoria_id to lancamentos_gastos
    {
        QSqlQuery pq(m_db);
        pq.exec(QStringLiteral("PRAGMA table_info(lancamentos_gastos)"));
        bool hasCatId = false;
        while (pq.next()) {
            if (pq.value(1).toString() == QLatin1String("categoria_id"))
                hasCatId = true;
        }
        if (!hasCatId)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_gastos "
                "ADD COLUMN categoria_id INTEGER "
                "REFERENCES categorias(id) ON DELETE SET NULL"));
    }

    // Migration: add categoria + categoria_id to lancamentos_entradas
    {
        QSqlQuery pq(m_db);
        pq.exec(QStringLiteral("PRAGMA table_info(lancamentos_entradas)"));
        bool hasCat = false, hasCatId = false;
        while (pq.next()) {
            const QString col = pq.value(1).toString();
            if (col == QLatin1String("categoria"))    hasCat   = true;
            if (col == QLatin1String("categoria_id")) hasCatId = true;
        }
        if (!hasCat)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_entradas "
                "ADD COLUMN categoria TEXT NOT NULL DEFAULT ''"));
        if (!hasCatId)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_entradas "
                "ADD COLUMN categoria_id INTEGER "
                "REFERENCES categorias(id) ON DELETE SET NULL"));
    }

    // Migration: add categoria + categoria_id to lancamentos_investimentos
    {
        QSqlQuery pq(m_db);
        pq.exec(QStringLiteral("PRAGMA table_info(lancamentos_investimentos)"));
        bool hasCat = false, hasCatId = false;
        while (pq.next()) {
            const QString col = pq.value(1).toString();
            if (col == QLatin1String("categoria"))    hasCat   = true;
            if (col == QLatin1String("categoria_id")) hasCatId = true;
        }
        if (!hasCat)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN categoria TEXT NOT NULL DEFAULT ''"));
        if (!hasCatId)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN categoria_id INTEGER "
                "REFERENCES categorias(id) ON DELETE SET NULL"));
    }

    // Migration #90: add receipt BLOB to 4 tables (anexo do comprovante).
    // Util para IRPF: associar foto/scan ao lancamento. BLOB nullable; UI
    // mostra thumbnail quando presente.
    const QStringList receiptTables = {
        QStringLiteral("lancamentos_entradas"),
        QStringLiteral("lancamentos_gastos"),
        QStringLiteral("compras"),
        QStringLiteral("lancamentos_investimentos"),
    };
    for (const QString &table : receiptTables) {
        QSqlQuery pq(m_db);
        pq.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table));
        bool hasReceipt = false;
        while (pq.next()) {
            if (pq.value(1).toString() == QLatin1String("receipt"))
                hasReceipt = true;
        }
        if (!hasReceipt)
            q.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN receipt BLOB")
                       .arg(table));
    }

    // Migration #48: ticker/quantidade/preco_unitario em lancamentos_investimentos
    // Permite tracking de ativos por papel (PETR4, MGLU3, BTC etc.) com qty e
    // preco unitario separados do valor total. quantidade aceita fracoes
    // (FII/cripto). preco_unitario em centavos como demais campos monetarios.
    {
        QSqlQuery pq(m_db);
        pq.exec(QStringLiteral("PRAGMA table_info(lancamentos_investimentos)"));
        bool hasTicker = false, hasQty = false, hasPU = false;
        while (pq.next()) {
            const QString col = pq.value(1).toString();
            if (col == QLatin1String("ticker"))          hasTicker = true;
            if (col == QLatin1String("quantidade"))      hasQty    = true;
            if (col == QLatin1String("preco_unitario"))  hasPU     = true;
        }
        if (!hasTicker)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN ticker TEXT NOT NULL DEFAULT ''"));
        if (!hasQty)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN quantidade REAL NOT NULL DEFAULT 0"));
        if (!hasPU)
            q.exec(QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN preco_unitario INTEGER NOT NULL DEFAULT 0"));
    }

    // #88: tabela fechamentos_mensais (snapshot imutavel mes contabil)
    if (!FechamentoMensalService::ensureSchema(m_db)) {
        qWarning() << "DatabaseManager: falha ao criar schema de fechamentos_mensais";
    }

    // #92: audit_log + triggers AFTER INSERT/UPDATE/DELETE em 7 tabelas
    if (!AuditLogService::ensureSchema(m_db)) {
        qWarning() << "DatabaseManager: falha ao criar schema de audit_log";
    }
}

bool DatabaseManager::encryptAndSave(const QString &srcDbPath,
                                     const QByteArray &key,
                                     const QString &destPath)
{
    QFile f(srcDbPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray plain = f.readAll();
    f.close();

    // Always write with the new format (AES-GCM / XChaCha20 + Argon2id key).
    SodiumCryptoService crypto;
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
                                     const QByteArray &keyNew,
                                     const QByteArray &keyLegacy)
{
    QFile f(encPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray cipher = f.readAll();
    f.close();

    QByteArray plain;
    try {
        if (SodiumCryptoService::isNewFormat(cipher)) {
            // New format: AES-GCM or XChaCha20 + Argon2id key.
            SodiumCryptoService crypto;
            plain = crypto.decrypt(cipher, keyNew);
        } else {
            // Legacy format: AES-256-CBC + PBKDF2 key.
            // Will be transparently re-encrypted on next save/close.
            if (keyLegacy.isEmpty()) {
                qWarning() << "DatabaseManager: legacy file detected but no legacy key provided";
                return false;
            }
            AesCryptoService legacyCrypto;
            plain = legacyCrypto.decrypt(cipher, keyLegacy);
            m_needsReencrypt = true;
            qDebug() << "DatabaseManager: legacy file detected — will migrate to new format on close";
        }
    } catch (const std::exception &e) {
        qWarning() << "Decrypt failed:" << e.what();
        return false;
    }

    QFile out(dbPath);
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(plain);
    return true;
}
