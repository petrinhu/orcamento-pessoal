#include "FechamentoMensalService.h"

#include "models/ResumoMensal.h"
#include "services/ResumoService.h"

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
    // #144: a versao anterior fazia SQL cru com COLUNAS INEXISTENTES
    // (lancamentos_entradas.valor/.data, parcelas.vencimento,
    // lancamentos_investimentos.data) e o COALESCE(...,0) mascarava o erro,
    // produzindo SEMPRE zero. Alem disso somava entrada BRUTA, enquanto o
    // resto do app trabalha com LIQUIDO. A correcao delega ao ResumoService,
    // fonte unica ja testada (#116) que agrega por mes/ano com as colunas
    // reais (valor_bruto/irpf/inss, mes/ano/dia, dataParaParcela, etc.).
    //
    // O parametro `db` e mantido por compatibilidade de assinatura com os
    // chamadores (DashboardWidget). O ResumoService opera sobre o singleton
    // DatabaseManager — que e o MESMO banco que o DashboardWidget passa aqui
    // (DatabaseManager::instance().db()). Marcado como nao-usado de proposito.
    Q_UNUSED(db);

    const ResumoMensal rm = ResumoService().resumoDoMes(mes.month(), mes.year());

    FechamentoResumo r;
    r.entradasCentavos = rm.entradasLiquidas;  // LIQUIDO (bruto - irpf - inss)
    r.gastosCentavos   = rm.gastos;
    r.parcelasCentavos = rm.parcelas;

    // Escolha semantica (#144): investimentos = FLUXO DE CAIXA liquido do mes
    // (resgates - aportes), NAO patrimonio/posicao. Coerente com o card
    // Investimentos do Dashboard e com a correcao D0 do ResumoService
    // (rendimento/dividendo/JCP ficam em Proventos, fora do net). O calculo
    // antigo (quantidade*preco_unitario) media posicao e estava quebrado.
    r.investimentosCentavos = rm.investimentosNet;

    r.numLancamentos = rm.cntEntradas + rm.cntGastos + rm.cntParcelas;

    // saldoLiquido = entradas liquidas - gastos - parcelas (igual a rm.saldoLiquido).
    r.saldoLiquidoCentavos = rm.saldoLiquido;
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
