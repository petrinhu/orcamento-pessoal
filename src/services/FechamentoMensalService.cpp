#include "FechamentoMensalService.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString periodoStr(const QDate &mes)
{
    return mes.toString(QStringLiteral("yyyy-MM"));
}

QDate primeiroDia(const QDate &mes)
{
    return QDate(mes.year(), mes.month(), 1);
}

QDate ultimoDia(const QDate &mes)
{
    return QDate(mes.year(), mes.month(), mes.daysInMonth());
}

qint64 somaCentavos(QSqlDatabase &db, const QString &sql,
                     const QDate &ini, const QDate &fim)
{
    QSqlQuery q(db);
    q.prepare(sql);
    q.bindValue(QStringLiteral(":ini"), ini.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":fim"), fim.toString(Qt::ISODate));
    if (!q.exec() || !q.next()) return 0;
    return q.value(0).toLongLong();
}

int contaLancamentos(QSqlDatabase &db, const QString &sql,
                      const QDate &ini, const QDate &fim)
{
    QSqlQuery q(db);
    q.prepare(sql);
    q.bindValue(QStringLiteral(":ini"), ini.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":fim"), fim.toString(Qt::ISODate));
    if (!q.exec() || !q.next()) return 0;
    return q.value(0).toInt();
}

}

bool FechamentoMensalService::ensureSchema(QSqlDatabase &db, QString *errorOut)
{
    QSqlQuery q(db);
    const QString sql = QStringLiteral(
        "CREATE TABLE IF NOT EXISTS fechamentos_mensais("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  periodo TEXT NOT NULL UNIQUE,"
        "  dados_json TEXT NOT NULL,"
        "  criado_em TEXT DEFAULT CURRENT_TIMESTAMP,"
        "  fechado_por TEXT"
        ")");
    if (!q.exec(sql)) {
        if (errorOut != nullptr) *errorOut = q.lastError().text();
        return false;
    }
    if (!q.exec(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_fechamentos_periodo "
            "ON fechamentos_mensais(periodo)"))) {
        if (errorOut != nullptr) *errorOut = q.lastError().text();
        return false;
    }
    return true;
}

FechamentoResumo FechamentoMensalService::simulate(QSqlDatabase &db,
                                                     const QDate  &mes)
{
    FechamentoResumo r;
    const QDate ini = primeiroDia(mes);
    const QDate fim = ultimoDia(mes);

    r.entradasCentavos = somaCentavos(db,
        QStringLiteral("SELECT COALESCE(SUM(valor), 0) FROM lancamentos_entradas "
                       "WHERE data BETWEEN :ini AND :fim"),
        ini, fim);

    r.gastosCentavos = somaCentavos(db,
        QStringLiteral("SELECT COALESCE(SUM(valor), 0) FROM lancamentos_gastos "
                       "WHERE data BETWEEN :ini AND :fim"),
        ini, fim);

    r.parcelasCentavos = somaCentavos(db,
        QStringLiteral("SELECT COALESCE(SUM(valor), 0) FROM parcelas "
                       "WHERE vencimento BETWEEN :ini AND :fim"),
        ini, fim);

    r.investimentosCentavos = somaCentavos(db,
        QStringLiteral("SELECT COALESCE(SUM(quantidade * preco_unitario), 0) "
                       "FROM lancamentos_investimentos "
                       "WHERE data BETWEEN :ini AND :fim"),
        ini, fim);

    r.numLancamentos =
          contaLancamentos(db, QStringLiteral("SELECT COUNT(*) FROM lancamentos_entradas WHERE data BETWEEN :ini AND :fim"), ini, fim)
        + contaLancamentos(db, QStringLiteral("SELECT COUNT(*) FROM lancamentos_gastos    WHERE data BETWEEN :ini AND :fim"), ini, fim)
        + contaLancamentos(db, QStringLiteral("SELECT COUNT(*) FROM parcelas              WHERE vencimento BETWEEN :ini AND :fim"), ini, fim);

    r.saldoLiquidoCentavos = r.entradasCentavos
                             - r.gastosCentavos
                             - r.parcelasCentavos;
    return r;
}

std::optional<FechamentoMensal> FechamentoMensalService::close(
    QSqlDatabase  &db,
    const QDate   &mes,
    const QString &fechadoPor)
{
    const QString per = periodoStr(mes);

    if (findByPeriod(db, mes).has_value()) {
        return std::nullopt;
    }

    const FechamentoResumo r = simulate(db, mes);
    QJsonObject obj;
    obj.insert(QStringLiteral("entradas_centavos"),     r.entradasCentavos);
    obj.insert(QStringLiteral("gastos_centavos"),       r.gastosCentavos);
    obj.insert(QStringLiteral("parcelas_centavos"),     r.parcelasCentavos);
    obj.insert(QStringLiteral("investimentos_centavos"), r.investimentosCentavos);
    obj.insert(QStringLiteral("saldo_liquido_centavos"), r.saldoLiquidoCentavos);
    obj.insert(QStringLiteral("num_lancamentos"),       r.numLancamentos);
    const QString json = QJsonDocument(obj).toJson(QJsonDocument::Compact);

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO fechamentos_mensais(periodo, dados_json, fechado_por) "
        "VALUES(:per, :json, :user)"));
    q.bindValue(QStringLiteral(":per"),  per);
    q.bindValue(QStringLiteral(":json"), json);
    q.bindValue(QStringLiteral(":user"), fechadoPor);
    if (!q.exec()) return std::nullopt;

    FechamentoMensal f;
    f.id         = q.lastInsertId().toLongLong();
    f.periodo    = per;
    f.dadosJson  = json;
    f.criadoEm   = QDateTime::currentDateTime();
    f.fechadoPor = fechadoPor;
    return f;
}

bool FechamentoMensalService::reopen(QSqlDatabase &db, const QDate &mes)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "DELETE FROM fechamentos_mensais WHERE periodo = :per"));
    q.bindValue(QStringLiteral(":per"), periodoStr(mes));
    return q.exec();
}

std::optional<FechamentoMensal> FechamentoMensalService::findByPeriod(
    QSqlDatabase &db, const QDate &mes)
{
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, periodo, dados_json, criado_em, fechado_por "
        "FROM fechamentos_mensais WHERE periodo = :per LIMIT 1"));
    q.bindValue(QStringLiteral(":per"), periodoStr(mes));
    if (!q.exec() || !q.next()) return std::nullopt;
    FechamentoMensal f;
    f.id         = q.value(0).toLongLong();
    f.periodo    = q.value(1).toString();
    f.dadosJson  = q.value(2).toString();
    f.criadoEm   = QDateTime::fromString(q.value(3).toString(), Qt::ISODate);
    f.fechadoPor = q.value(4).toString();
    return f;
}
