#include "AuditLogService.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace {

// Tabelas financeiras chave + lista de colunas para json_object().
// Mantido propositalmente curto - cobertura completa fica em follow-up
// (cada tabela adicionada precisa ser revisada para nao logar campos sensiveis).
struct TabelaAudit {
    const char *tabela;
    const char *colunas; // CSV simples para json_object
};

// Colunas validadas 1:1 contra os CREATE TABLE reais em
// DatabaseManager::createSchema() (#142). Cada nome listado EXISTE no schema.
// Curadoria: nao logamos BLOBs (ex.: receipt) nem campos desnecessariamente
// sensiveis; mantemos id + os campos materiais de cada operacao.
constexpr TabelaAudit kTabelas[] = {
    { "lancamentos_entradas",      "id, fonte_id, valor_bruto, mes, ano" },
    { "lancamentos_gastos",        "id, valor, categoria, descricao, mes, ano" },
    // NOTE: o schema real de investimentos nao tem ticker/quantidade/
    // preco_unitario; usa nome/tipo_ativo/valor + mes/ano.
    { "lancamentos_investimentos", "id, nome, tipo_ativo, valor, mes, ano" },
    { "compras",                   "id, cartao_id, valor_total, num_parcelas, mes_inicio, ano_inicio" },
    { "parcelas",                  "id, valor, data_fatura, modificada" },
    { "dividas",                   "id, descricao, valor_total, valor_pago, status" },
    { "metas",                     "id, nome, valor_meta, valor_atual, data_prazo" },
};

QString geraJsonObject(const QString &alias, const QString &colunas)
{
    QString out = QStringLiteral("json_object(");
    const QStringList cols = colunas.split(QStringLiteral(", "));
    QStringList parts;
    parts.reserve(cols.size());
    for (const QString &c : cols) {
        parts << QStringLiteral("'%1', %2.%1").arg(c.trimmed(), alias);
    }
    out += parts.join(QStringLiteral(", "));
    out += QStringLiteral(")");
    return out;
}

QString geraTriggerInsert(const TabelaAudit &t)
{
    return QStringLiteral(
        "CREATE TRIGGER IF NOT EXISTS audit_%1_ins "
        "AFTER INSERT ON %1 BEGIN "
        "INSERT INTO audit_log(tabela, op, registro_id, dados_novos) "
        "VALUES ('%1', 'INSERT', NEW.id, %2); "
        "END;")
        .arg(QString::fromLatin1(t.tabela),
             geraJsonObject(QStringLiteral("NEW"),
                            QString::fromLatin1(t.colunas)));
}

QString geraTriggerUpdate(const TabelaAudit &t)
{
    return QStringLiteral(
        "CREATE TRIGGER IF NOT EXISTS audit_%1_upd "
        "AFTER UPDATE ON %1 BEGIN "
        "INSERT INTO audit_log(tabela, op, registro_id, dados_antigos, dados_novos) "
        "VALUES ('%1', 'UPDATE', NEW.id, %2, %3); "
        "END;")
        .arg(QString::fromLatin1(t.tabela),
             geraJsonObject(QStringLiteral("OLD"),
                            QString::fromLatin1(t.colunas)),
             geraJsonObject(QStringLiteral("NEW"),
                            QString::fromLatin1(t.colunas)));
}

QString geraTriggerDelete(const TabelaAudit &t)
{
    return QStringLiteral(
        "CREATE TRIGGER IF NOT EXISTS audit_%1_del "
        "AFTER DELETE ON %1 BEGIN "
        "INSERT INTO audit_log(tabela, op, registro_id, dados_antigos) "
        "VALUES ('%1', 'DELETE', OLD.id, %2); "
        "END;")
        .arg(QString::fromLatin1(t.tabela),
             geraJsonObject(QStringLiteral("OLD"),
                            QString::fromLatin1(t.colunas)));
}

bool execSql(QSqlDatabase &db, const QString &sql, QString *errorOut)
{
    QSqlQuery q(db);
    if (!q.exec(sql)) {
        if (errorOut != nullptr) *errorOut = q.lastError().text();
        return false;
    }
    return true;
}

bool tabelaExiste(QSqlDatabase &db, const QString &nome)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=:n LIMIT 1"));
    q.bindValue(QStringLiteral(":n"), nome);
    if (!q.exec()) return false;
    return q.next();
}

}

bool AuditLogService::ensureSchema(QSqlDatabase &db, QString *errorOut)
{
    static const QString kCreateTable = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS audit_log("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  tabela TEXT NOT NULL,"
        "  op TEXT NOT NULL CHECK(op IN ('INSERT','UPDATE','DELETE')),"
        "  registro_id INTEGER,"
        "  dados_antigos TEXT,"
        "  dados_novos TEXT,"
        "  ts TEXT DEFAULT CURRENT_TIMESTAMP,"
        "  usuario TEXT"
        ")");
    if (!execSql(db, kCreateTable, errorOut)) return false;

    if (!execSql(db,
                 QStringLiteral("CREATE INDEX IF NOT EXISTS idx_audit_log_ts "
                                "ON audit_log(ts)"),
                 errorOut)) return false;

    if (!execSql(db,
                 QStringLiteral("CREATE INDEX IF NOT EXISTS idx_audit_log_tabela "
                                "ON audit_log(tabela)"),
                 errorOut)) return false;

    // Cria triggers somente em tabelas que existem (evita erro em DB jovem).
    //
    // (#142) Saneamento: bancos abertos pos-#92 podem ter triggers audit_*
    // quebrados (referenciando colunas inexistentes). CREATE TRIGGER IF NOT
    // EXISTS nao os corrige -- o trigger velho persiste e segue abortando todo
    // INSERT/UPDATE/DELETE. Por isso DROPamos cada trigger antes de recriar, de
    // forma que toda abertura do app reescreve os triggers para a versao
    // correta. Idempotente (DROP IF EXISTS + recriacao deterministica).
    for (const auto &t : kTabelas) {
        const QString tab = QString::fromLatin1(t.tabela);
        if (!tabelaExiste(db, tab)) continue;

        for (const char *suf : { "ins", "upd", "del" }) {
            const QString drop = QStringLiteral("DROP TRIGGER IF EXISTS audit_%1_%2")
                                     .arg(tab, QString::fromLatin1(suf));
            if (!execSql(db, drop, errorOut)) return false;
        }

        if (!execSql(db, geraTriggerInsert(t), errorOut)) return false;
        if (!execSql(db, geraTriggerUpdate(t), errorOut)) return false;
        if (!execSql(db, geraTriggerDelete(t), errorOut)) return false;
    }
    return true;
}

QList<AuditLogEntry> AuditLogService::query(QSqlDatabase &db,
                                              const QString  &tabela,
                                              const QDateTime &inicio,
                                              const QDateTime &fim,
                                              int             limite)
{
    QList<AuditLogEntry> out;

    QString sql = QStringLiteral(
        "SELECT id, tabela, op, registro_id, dados_antigos, dados_novos, ts, usuario "
        "FROM audit_log WHERE 1=1 ");
    if (!tabela.isEmpty())          sql += QStringLiteral("AND tabela = :tab ");
    if (inicio.isValid())           sql += QStringLiteral("AND ts >= :ini ");
    if (fim.isValid())              sql += QStringLiteral("AND ts <= :fim ");
    sql += QStringLiteral("ORDER BY id DESC LIMIT :lim");

    QSqlQuery q(db);
    q.prepare(sql);
    if (!tabela.isEmpty())  q.bindValue(QStringLiteral(":tab"), tabela);
    if (inicio.isValid())   q.bindValue(QStringLiteral(":ini"), inicio.toString(Qt::ISODate));
    if (fim.isValid())      q.bindValue(QStringLiteral(":fim"), fim.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":lim"), limite);

    if (!q.exec()) return out;
    while (q.next()) {
        AuditLogEntry e;
        e.id           = q.value(0).toLongLong();
        e.tabela       = q.value(1).toString();
        e.op           = q.value(2).toString();
        e.registroId   = q.value(3).toLongLong();
        e.dadosAntigos = q.value(4).toString();
        e.dadosNovos   = q.value(5).toString();
        e.timestamp    = QDateTime::fromString(q.value(6).toString(), Qt::ISODate);
        e.usuario      = q.value(7).toString();
        out.append(e);
    }
    return out;
}
