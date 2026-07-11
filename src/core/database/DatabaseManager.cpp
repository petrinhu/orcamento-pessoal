#include "DatabaseManager.h"
#include "core/crypto/AesCryptoService.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/settings/AppSettings.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QUuid>
#include <stdexcept>

#include <sys/stat.h>   // chmod — restringe o diretório de dados a 0700
#include <sodium.h>     // sodium_memzero — apaga a chave após o último uso

namespace {

// (#191/FIN-02) Executa um PRAGMA de setup checando o retorno; falha genuína
// (ex.: disco cheio, arquivo corrompido) fica logada em vez de passar em
// silêncio — antes os 3 PRAGMAs de open()/importFromFile() ignoravam bool.
void execPragmaComAviso(QSqlQuery &q, const QString &sql)
{
    if (!q.exec(sql))
        qWarning() << "DatabaseManager: PRAGMA falhou —" << sql
                   << "-" << q.lastError().text();
}

// (#203a/FIN-06) Executa um exec() de createSchema() (CREATE TABLE / ALTER
// TABLE / PRAGMA table_info) checando o retorno. As 16 tabelas base e as
// migrations incrementais são idempotentes (IF NOT EXISTS / guard via
// PRAGMA table_info), então uma falha isolada não trava o boot — mas antes
// os ~41 exec() de createSchema() ignoravam bool systematicamente, e uma
// falha genuína (disco cheio, arquivo corrompido, permissão) passava
// 100% em silêncio. Agora fica logada com o contexto (nome da
// tabela/coluna), preservando o comportamento auto-curativo mas tornando
// a falha observável.
bool execSchemaComAviso(QSqlQuery &q, const QString &sql, const char *contexto)
{
    if (!q.exec(sql)) {
        qWarning() << "DatabaseManager::createSchema:" << contexto
                   << "falhou -" << q.lastError().text();
        return false;
    }
    return true;
}

#ifndef QT_DEBUG
// (#181) Garante que a pasta-pai do caminho fixo ~/peubud/save/<slug>/
// exista e tenha permissões 0700. O `.db` em claro vive em data/ (CWD)
// durante a sessão; o cofre cifrado fica no caminho fixo.
void ensureSaveDirSecure(const QString &pbdPath)
{
    const QString dir = QFileInfo(pbdPath).absolutePath();
    QDir().mkpath(dir);
    // chmod 0700 é idempotente; best-effort (FS sem POSIX não bloqueia).
    ::chmod(QFile::encodeName(dir).constData(), S_IRWXU);
}

// (#181) Migração transparente: se o cofre no caminho fixo novo não existe
// mas existe no caminho legado (data/<slug>.pbd), copia para o novo destino
// e renomeia o antigo para <antigo>.bak — nunca deleta o original.
// Retorna true se o cofre novo foi criado por migração (ou já existia).
// Retorna false se nenhum cofre existe em nenhum lugar (novo usuário).
//
// TODO (ADR-001 §8 passos 6-7/D6, Fatia 5 — fora de escopo da Fatia 4):
// cofres single-user PRÉ-ADR-001 com username acentuado estão sob o slug
// ANTIGO (bug "João"->"joo" de UserSession::slug(), corrigido na Fatia 4 —
// ver AppSettings::slugify). `pbdPathLegado` aqui só cobre o path RELATIVO
// pré-#181 (`data/<slug>.pbd`); falta um 3º fallback checando o slug antigo
// (pré-decomposição NFD) no caminho FIXO (`savePathFixo`) para usernames
// com char fora de `[a-z0-9_ ]`. Depende de `LegacyVaultMigrationService`
// (ainda não implementado) para decidir QUAL dos dois slugs é a fonte de
// verdade quando ambos existem — não implementado aqui.
bool migrarCofreSeNecessario(const QString &pbdPathNovo, const QString &pbdPathLegado)
{
    if (QFile::exists(pbdPathNovo))
        return true; // caminho novo já tem cofre — nada a fazer

    if (!QFile::exists(pbdPathLegado))
        return false; // nenhum cofre existe — novo usuário, banco vazio

    // Cofre legado encontrado — copiar (não mover) para o novo destino.
    // SEGURANÇA: usar write atômico via arquivo .new para não deixar destino
    // corrompido se o processo for interrompido no meio.
    const QString tmpNovo = pbdPathNovo + QStringLiteral(".new");
    if (!QFile::copy(pbdPathLegado, tmpNovo)) {
        qWarning() << "DatabaseManager: falha ao copiar cofre legado para novo destino"
                   << pbdPathLegado << "->" << tmpNovo;
        return false;
    }
    if (!QFile::rename(tmpNovo, pbdPathNovo)) {
        qWarning() << "DatabaseManager: falha ao renomear cofre copiado"
                   << tmpNovo << "->" << pbdPathNovo;
        QFile::remove(tmpNovo);
        return false;
    }

    // Renomeia o antigo para .bak (mantém como backup — não deleta nunca).
    const QString bakPath = pbdPathLegado + QStringLiteral(".bak");
    if (!QFile::rename(pbdPathLegado, bakPath)) {
        // Renomear para .bak falhou (ex.: FS read-only, cross-device) —
        // deixa o original no lugar. Não é erro fatal: o cofre novo já está
        // no destino correto. Log de aviso para o dev.
        qWarning() << "DatabaseManager: não foi possível renomear legado para .bak"
                   << pbdPathLegado << "(cofre novo OK em" << pbdPathNovo << ")";
    } else {
        qDebug() << "DatabaseManager: cofre legado migrado para" << pbdPathNovo
                 << "| backup em" << bakPath;
    }

    return true;
}
#endif // QT_DEBUG

} // namespace

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
    m_slug        = slug;
    m_key         = keyNew;
    m_keyLegacy   = keyLegacy;
    m_needsReencrypt = false;
    // (#192/DIST-03/SEC-02) Local FIXO (QStandardPaths::AppDataLocation),
    // não mais "data/" relativo ao CWD do processo — o `.desktop` do pacote
    // não seta `Path=`, então o lançamento via menu tinha CWD imprevisível,
    // quebrando 100% na 1ª execução do pacote instalado. mkpath + chmod
    // 0700 do diretório são garantidos dentro de workingDbPath().
    m_dbPath      = AppSettings::workingDbPath(slug);

#ifndef QT_DEBUG
    // (#181) Caminho fixo do cofre: ~/peubud/save/<slug>/database.pbd
    const QString pbdPath    = AppSettings::savePathFixo(slug);
    const QString pbdLegado  = AppSettings::saveLegadoData(slug);

    // Garante que a pasta do cofre fixo existe e tem permissão 0700.
    ensureSaveDirSecure(pbdPath);

    // (#181) Migração transparente: se cofre novo ausente mas legado presente,
    // copia o legado para o novo destino (nunca deleta o original).
    migrarCofreSeNecessario(pbdPath, pbdLegado);

    // (#127c) Limpa `.db` em claro órfão de sessão anterior que não fechou
    // limpo (ex.: crash/kill — close() normal remove o `.db`). Em release a
    // fonte de verdade é sempre o `.pbd`; o `.db` é derivado, seguro descartar.
    if (QFile::exists(m_dbPath))
        QFile::remove(m_dbPath);

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
    // (#191/FIN-02) DELETE (não MEMORY): journal em disco, mas TRANSITÓRIO —
    // só existe durante uma transação aberta, é apagado no commit. Crash-safe
    // (recupera via rollback journal no próximo open), ao contrário de MEMORY
    // (journal em RAM — crash no meio de uma transação pode corromper o .db).
    // Preserva a intenção original de "mínimo resíduo persistente" do projeto
    // (o .db em claro já é sensível): ao contrário de WAL, não deixa um -wal
    // residente entre sessões. synchronous=NORMAL mantido (combo recomendado
    // pelo SQLite para DELETE/WAL).
    execPragmaComAviso(q, QStringLiteral("PRAGMA journal_mode = DELETE"));
    execPragmaComAviso(q, QStringLiteral("PRAGMA foreign_keys = ON"));
    execPragmaComAviso(q, QStringLiteral("PRAGMA synchronous = NORMAL"));

    createSchema();
    return true;
}

bool DatabaseManager::save()
{
#ifndef QT_DEBUG
    if (!m_db.isOpen()) return false;
    // (#181) Caminho fixo — ~/peubud/save/<slug>/database.pbd
    const QString pbdPath = AppSettings::savePathFixo(m_slug);
    return encryptAndSave(m_dbPath, m_key, pbdPath);
#else
    return true; // debug: SQLite persiste imediatamente
#endif
}

bool DatabaseManager::close()
{
    if (!m_db.isOpen()) {
        // (ADR-001 §8 passo 6/D4, Fatia 4) Nada a fechar, mas ainda reseta
        // o header — defensivo: evita vazamento entre sessões/testes se
        // setVaultHeader() foi chamado sem um open() bem-sucedido depois
        // (ex.: close() defensivo de cleanup() de teste no singleton).
        m_vaultHeader = VaultHeaderData{};
        m_vaultIsV2   = false;
        return true; // nada a fechar — não é falha
    }

    const QString dbPath = m_db.databaseName();
    m_db.close();
    m_db = QSqlDatabase();  // libera a referência antes de removeDatabase
    QSqlDatabase::removeDatabase(QStringLiteral("main"));

    bool ok = true;
#ifndef QT_DEBUG
    // (#181) Caminho fixo — ~/peubud/save/<slug>/database.pbd
    // (#191/FIN-02) encryptAndSave() roda PRAGMA integrity_check em dbPath
    // ANTES de cifrar; se o .db estiver corrompido, ABORTA e retorna false
    // sem tocar em pbdPath — o cofre bom anterior permanece intacto.
    const QString pbdPath = AppSettings::savePathFixo(m_slug);
    ok = encryptAndSave(dbPath, m_key, pbdPath);
    if (!ok) {
        qWarning() << "DatabaseManager::close: integrity_check reprovou o banco"
                      " em" << dbPath << "— save abortado, cofre anterior"
                      " preservado em" << pbdPath;
    }
#endif

    QFile::remove(dbPath);

    // (#128) A partir daqui a sessão está encerrada e a chave não é mais
    // necessária (a re-cifragem acima foi o último uso). Zera o material da
    // chave nesta cópia COW antes de soltá-la. close() só é chamado no fim do
    // app / destrutor — import/export NÃO passam por aqui, então não há risco
    // de zerar uma chave ainda em uso.
    if (!m_key.isEmpty())
        sodium_memzero(m_key.data(), static_cast<size_t>(m_key.size()));
    if (!m_keyLegacy.isEmpty())
        sodium_memzero(m_keyLegacy.data(), static_cast<size_t>(m_keyLegacy.size()));
    m_key.clear();
    m_keyLegacy.clear();

    // (ADR-001 §8 passo 6/D4, Fatia 4) Reseta o header v2 — cada sessão de
    // login é independente; sem isso, o header de UM usuário vazaria pro
    // open() do PRÓXIMO usuário na mesma execução do processo (singleton).
    m_vaultHeader = VaultHeaderData{};
    m_vaultIsV2   = false;

    return ok;
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
    ok = decryptAndLoad(srcPath, m_dbPath, m_key, m_keyLegacy);
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
    // (#191/FIN-02) Mesmo setup de open() — ver comentário lá para a
    // justificativa de DELETE (não MEMORY nem WAL).
    execPragmaComAviso(q, QStringLiteral("PRAGMA journal_mode = DELETE"));
    execPragmaComAviso(q, QStringLiteral("PRAGMA foreign_keys = ON"));
    execPragmaComAviso(q, QStringLiteral("PRAGMA synchronous = NORMAL"));
    return true;
}

void DatabaseManager::createSchema()
{
    QSqlQuery q(m_db);

    execSchemaComAviso(q, QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS cartoes (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            apelido TEXT NOT NULL UNIQUE
        )
    )"), "CREATE TABLE cartoes");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE compras");

    execSchemaComAviso(q, QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS parcelas (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            compra_id   INTEGER NOT NULL REFERENCES compras(id) ON DELETE CASCADE,
            numero      INTEGER NOT NULL,
            valor       INTEGER NOT NULL,
            data_fatura TEXT,
            modificada  INTEGER NOT NULL DEFAULT 0
        )
    )"), "CREATE TABLE parcelas");

    execSchemaComAviso(q, QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS fontes_renda (
            id   INTEGER PRIMARY KEY AUTOINCREMENT,
            nome TEXT    NOT NULL UNIQUE
        )
    )"), "CREATE TABLE fontes_renda");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE lancamentos_entradas");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE lancamentos_gastos");

    execSchemaComAviso(q, QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS orcamento_itens (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            nome           TEXT    NOT NULL,
            tipo           TEXT    NOT NULL DEFAULT 'despesa',
            valor_mensal   INTEGER NOT NULL DEFAULT 0,
            categoria_ref  TEXT    NOT NULL DEFAULT '',
            criado_em      TEXT    NOT NULL
        )
    )"), "CREATE TABLE orcamento_itens");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE lancamentos_investimentos");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE transacoes_recorrentes");

    execSchemaComAviso(q, QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS categorias (
            id      INTEGER PRIMARY KEY AUTOINCREMENT,
            nome    TEXT    NOT NULL,
            tipo    TEXT    NOT NULL DEFAULT 'gasto',
            pai_id  INTEGER REFERENCES categorias(id) ON DELETE SET NULL
        )
    )"), "CREATE TABLE categorias");

    execSchemaComAviso(q, QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS pagadores (
            id   INTEGER PRIMARY KEY AUTOINCREMENT,
            nome TEXT    NOT NULL UNIQUE,
            tipo TEXT    NOT NULL DEFAULT 'ambos',
            obs  TEXT    NOT NULL DEFAULT ''
        )
    )"), "CREATE TABLE pagadores");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE dividas");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE metas");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE lembretes");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE precos_ativos");

    execSchemaComAviso(q, QStringLiteral(R"(
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
    )"), "CREATE TABLE cotacoes_tempo_real");

    // Migration: add pagador columns to lancamentos_gastos (guard via PRAGMA).
    {
        QSqlQuery pq(m_db);
        execSchemaComAviso(pq, QStringLiteral("PRAGMA table_info(lancamentos_gastos)"),
                           "PRAGMA table_info(lancamentos_gastos) [pagador]");
        bool hasPagador = false, hasPagadorId = false;
        while (pq.next()) {
            const QString col = pq.value(1).toString();
            if (col == QLatin1String("pagador"))    hasPagador   = true;
            if (col == QLatin1String("pagador_id")) hasPagadorId = true;
        }
        if (!hasPagador)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_gastos "
                "ADD COLUMN pagador TEXT NOT NULL DEFAULT ''"),
                "ALTER lancamentos_gastos ADD pagador");
        if (!hasPagadorId)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_gastos "
                "ADD COLUMN pagador_id INTEGER "
                "REFERENCES pagadores(id) ON DELETE SET NULL"),
                "ALTER lancamentos_gastos ADD pagador_id");
    }

    // Migration: add categoria_id to lancamentos_gastos
    {
        QSqlQuery pq(m_db);
        execSchemaComAviso(pq, QStringLiteral("PRAGMA table_info(lancamentos_gastos)"),
                           "PRAGMA table_info(lancamentos_gastos) [categoria_id]");
        bool hasCatId = false;
        while (pq.next()) {
            if (pq.value(1).toString() == QLatin1String("categoria_id"))
                hasCatId = true;
        }
        if (!hasCatId)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_gastos "
                "ADD COLUMN categoria_id INTEGER "
                "REFERENCES categorias(id) ON DELETE SET NULL"),
                "ALTER lancamentos_gastos ADD categoria_id");
    }

    // Migration: add categoria + categoria_id to lancamentos_entradas
    {
        QSqlQuery pq(m_db);
        execSchemaComAviso(pq, QStringLiteral("PRAGMA table_info(lancamentos_entradas)"),
                           "PRAGMA table_info(lancamentos_entradas) [categoria]");
        bool hasCat = false, hasCatId = false;
        while (pq.next()) {
            const QString col = pq.value(1).toString();
            if (col == QLatin1String("categoria"))    hasCat   = true;
            if (col == QLatin1String("categoria_id")) hasCatId = true;
        }
        if (!hasCat)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_entradas "
                "ADD COLUMN categoria TEXT NOT NULL DEFAULT ''"),
                "ALTER lancamentos_entradas ADD categoria");
        if (!hasCatId)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_entradas "
                "ADD COLUMN categoria_id INTEGER "
                "REFERENCES categorias(id) ON DELETE SET NULL"),
                "ALTER lancamentos_entradas ADD categoria_id");
    }

    // Migration: add categoria + categoria_id to lancamentos_investimentos
    {
        QSqlQuery pq(m_db);
        execSchemaComAviso(pq, QStringLiteral("PRAGMA table_info(lancamentos_investimentos)"),
                           "PRAGMA table_info(lancamentos_investimentos) [categoria]");
        bool hasCat = false, hasCatId = false;
        while (pq.next()) {
            const QString col = pq.value(1).toString();
            if (col == QLatin1String("categoria"))    hasCat   = true;
            if (col == QLatin1String("categoria_id")) hasCatId = true;
        }
        if (!hasCat)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN categoria TEXT NOT NULL DEFAULT ''"),
                "ALTER lancamentos_investimentos ADD categoria");
        if (!hasCatId)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN categoria_id INTEGER "
                "REFERENCES categorias(id) ON DELETE SET NULL"),
                "ALTER lancamentos_investimentos ADD categoria_id");
    }

    // Migration #118: add fixo (classificacao fixo/variavel) a categorias.
    // 0 = variavel (default seguro), 1 = fixo. Categorias antigas, sem
    // classificacao, viram "variavel" e nao inflam a despesa fixa do orcamento.
    // Idempotente via PRAGMA table_info (mesmo padrao de #48/#90).
    {
        QSqlQuery pq(m_db);
        execSchemaComAviso(pq, QStringLiteral("PRAGMA table_info(categorias)"),
                           "PRAGMA table_info(categorias) [fixo]");
        bool hasFixo = false;
        while (pq.next()) {
            if (pq.value(1).toString() == QLatin1String("fixo"))
                hasFixo = true;
        }
        if (!hasFixo)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE categorias "
                "ADD COLUMN fixo INTEGER NOT NULL DEFAULT 0"),
                "ALTER categorias ADD fixo");
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
        execSchemaComAviso(pq, QStringLiteral("PRAGMA table_info(%1)").arg(table),
                           qPrintable(QStringLiteral("PRAGMA table_info(%1) [receipt]").arg(table)));
        bool hasReceipt = false;
        while (pq.next()) {
            if (pq.value(1).toString() == QLatin1String("receipt"))
                hasReceipt = true;
        }
        if (!hasReceipt)
            execSchemaComAviso(q, QStringLiteral("ALTER TABLE %1 ADD COLUMN receipt BLOB")
                       .arg(table),
                       qPrintable(QStringLiteral("ALTER %1 ADD receipt").arg(table)));
    }

    // Migration #48: ticker/quantidade/preco_unitario em lancamentos_investimentos
    // Permite tracking de ativos por papel (PETR4, MGLU3, BTC etc.) com qty e
    // preco unitario separados do valor total. quantidade aceita fracoes
    // (FII/cripto). preco_unitario em centavos como demais campos monetarios.
    {
        QSqlQuery pq(m_db);
        execSchemaComAviso(pq, QStringLiteral("PRAGMA table_info(lancamentos_investimentos)"),
                           "PRAGMA table_info(lancamentos_investimentos) [ticker/quantidade/preco_unitario]");
        bool hasTicker = false, hasQty = false, hasPU = false;
        while (pq.next()) {
            const QString col = pq.value(1).toString();
            if (col == QLatin1String("ticker"))          hasTicker = true;
            if (col == QLatin1String("quantidade"))      hasQty    = true;
            if (col == QLatin1String("preco_unitario"))  hasPU     = true;
        }
        if (!hasTicker)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN ticker TEXT NOT NULL DEFAULT ''"),
                "ALTER lancamentos_investimentos ADD ticker");
        if (!hasQty)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN quantidade REAL NOT NULL DEFAULT 0"),
                "ALTER lancamentos_investimentos ADD quantidade");
        if (!hasPU)
            execSchemaComAviso(q, QStringLiteral(
                "ALTER TABLE lancamentos_investimentos "
                "ADD COLUMN preco_unitario INTEGER NOT NULL DEFAULT 0"),
                "ALTER lancamentos_investimentos ADD preco_unitario");
    }

    // (#200/ARQ-01) Tabelas Mid/Backend (proventos_recebidos #66,
    // ajustes_corporativos #65, fechamentos_mensais #88, audit_log #92) não
    // são mais criadas por chamadas diretas a classes concretas aqui — o
    // Foundation não conhece mais FechamentoMensalService/AuditLogService/
    // ProventoRepository/AjusteCorporativoService. Os providers são
    // registrados no ponto de composição (AppBootstrap::
    // registerCoreSchemaProviders(), chamado em main.cpp e em cada suíte de
    // teste que abre o DB diretamente) via registerSchemaProvider(); aqui só
    // invocamos o que foi registrado, na ordem de registro.
    for (const auto &provider : schemaProviders()) {
        provider(m_db);
    }
}

std::vector<DatabaseManager::SchemaProvider> &DatabaseManager::schemaProviders()
{
    static std::vector<SchemaProvider> providers;
    return providers;
}

void DatabaseManager::registerSchemaProvider(SchemaProvider provider)
{
    schemaProviders().push_back(std::move(provider));
}

// (ADR-001 §8 passo 6/D4, Fatia 4) Ver ordering completo no .h. username
// vazio força v1 — reservado à migração de cofres legados (Fatia 5).
void DatabaseManager::setVaultHeader(const VaultHeaderData &header)
{
    m_vaultHeader = header;
    m_vaultIsV2   = !header.username.isEmpty();
}

// (ADR-001 §8 passos 6-7, Fatia 5) Ver justificativa completa no .h. Exige
// DB aberto — chamar DEPOIS de um open() bem-sucedido (a migração de cofre
// legado abre com a chave ANTIGA para decifrar, depois troca só a chave de
// saída via rekey() — nunca relê nem re-deriva o payload).
bool DatabaseManager::rekey(const QByteArray &newKey)
{
    if (!m_db.isOpen())
        return false;

    if (!m_key.isEmpty())
        sodium_memzero(m_key.data(), static_cast<size_t>(m_key.size()));
    m_key = newKey;

    if (!m_keyLegacy.isEmpty())
        sodium_memzero(m_keyLegacy.data(), static_cast<size_t>(m_keyLegacy.size()));
    m_keyLegacy.clear();

    m_needsReencrypt = false;
    return true;
}

// (#191/FIN-02) Verifica a integridade de um arquivo SQLite via
// `PRAGMA integrity_check`, usando uma conexão QSQLITE própria e
// somente-leitura — não depende de m_db estar aberto (funciona chamado
// antes OU depois de m_db.close(), e não interfere com sessões vivas de
// outra conexão apontando para o mesmo arquivo). Nome de conexão único
// (QUuid) evita colidir com "main" ou com chamadas concorrentes/repetidas
// (ex.: em testes).
bool DatabaseManager::checkFileIntegrity(const QString &dbFilePath)
{
    if (!QFile::exists(dbFilePath)) {
        qWarning() << "DatabaseManager::checkFileIntegrity: arquivo não encontrado —"
                   << dbFilePath;
        return false;
    }

    const QString connName = QStringLiteral("integrity_check_%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    bool ok = false;
    {
        QSqlDatabase check = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        check.setDatabaseName(dbFilePath);
        // Somente-leitura: um arquivo que não é SQLite válido (ou corrompido a
        // ponto de não ter cabeçalho reconhecível) não deve ser silenciosamente
        // "reparado" ou ter um DB novo criado por cima ao abrir.
        check.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));

        if (!check.open()) {
            qWarning() << "DatabaseManager::checkFileIntegrity: falha ao abrir"
                       << dbFilePath << "-" << check.lastError().text();
        } else {
            QSqlQuery q(check);
            if (!q.exec(QStringLiteral("PRAGMA integrity_check"))) {
                qWarning() << "DatabaseManager::checkFileIntegrity:"
                              " PRAGMA integrity_check falhou —" << q.lastError().text();
            } else if (!q.next()) {
                qWarning() << "DatabaseManager::checkFileIntegrity:"
                              " integrity_check sem resultado (arquivo vazio/inválido)";
            } else {
                const QString resultado = q.value(0).toString();
                ok = (resultado == QLatin1String("ok"));
                if (!ok)
                    qWarning() << "DatabaseManager::checkFileIntegrity:"
                                  " banco corrompido —" << resultado;
            }
            check.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    return ok;
}

bool DatabaseManager::encryptAndSave(const QString &srcDbPath,
                                     const QByteArray &key,
                                     const QString &destPath)
{
    // (#191/FIN-02) GATE único: os 3 chamadores (save/close/exportToFile)
    // passam por aqui. Se o .db em claro estiver corrompido, ABORTA antes de
    // ler+cifrar — nunca sobrescreve o último .pbd bom com lixo. O chamador
    // recebe false e decide como alertar o usuário; nada é escrito em destPath.
    if (!checkFileIntegrity(srcDbPath)) {
        qWarning() << "DatabaseManager::encryptAndSave: integrity_check reprovou"
                   << srcDbPath << "— save ABORTADO, preservando" << destPath;
        return false;
    }

    QFile f(srcDbPath);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray plain = f.readAll();
    f.close();

    // Always write with the new format (AES-GCM / XChaCha20 + Argon2id key).
    SodiumCryptoService crypto;
    QByteArray cipher = crypto.encrypt(plain, key);

    // (ADR-001 §8 passo 6/D4, Fatia 4) Envelopa em `.pbd` v2 quando há um
    // header primed nesta sessão (setVaultHeader() foi chamado — bootstrap
    // de cofre novo, ou o header já veio restaurado de um open() v2
    // anterior). D1: o `.pbd` v2 é a ÚNICA fonte de verdade dos wraps de K
    // de QUALQUER conta (admin e comum) — save()/close()/exportToFile()
    // herdam o envelope de graça por passarem todos por aqui.
    if (m_vaultIsV2)
        cipher = VaultFile::frameV2(m_vaultHeader, cipher);

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
    const QByteArray fileBytes = f.readAll();
    f.close();

    // (ADR-001 §8 passo 6/D4, Fatia 4) Separa o header v2 (se houver) do
    // innerBlob v1 verbatim ANTES de decidir novo-vs-legado — o caminho de
    // decrypt abaixo (SodiumCryptoService::isNewFormat/decrypt +
    // AesCryptoService legado) NÃO MUDA, só passa a operar sobre `cipher`
    // (que é o arquivo inteiro no v1/legado, ou só o innerBlob no v2).
    QByteArray cipher = fileBytes;
    if (VaultFile::isV2(fileBytes)) {
        const VaultFile::ParseResult parsed = VaultFile::parseV2(fileBytes);
        if (!parsed.ok) {
            qWarning() << "DatabaseManager::decryptAndLoad: .pbd v2 malformado/corrompido"
                          " (magic ok, parse falhou) —" << encPath;
            return false; // fail-closed (D4) — nunca tenta decifrar lixo
        }
        m_vaultHeader = parsed.header;
        m_vaultIsV2   = true;
        cipher        = parsed.innerBlob;
    } else {
        // V1 pós-migração OU legado AES-CBC: reseta o header — o
        // DatabaseManager é singleton reusado entre logins na mesma
        // execução do processo; sem este reset, o header de uma sessão v2
        // anterior vazaria para este login v1/legado.
        m_vaultHeader = VaultHeaderData{};
        m_vaultIsV2   = false;
    }

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
    out.close();

    // (#127a) Restringe o `.db` em claro a 0600 (somente o dono lê/escreve).
    // Best-effort; complementa o 0700 do diretório.
    out.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}
