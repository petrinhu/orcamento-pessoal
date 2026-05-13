#include "LancamentoInvestimentoRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

static LancamentoInvestimento fromQuery(const QSqlQuery &q)
{
    LancamentoInvestimento l;
    l.id           = q.value(QStringLiteral("id")).toInt();
    l.mes          = q.value(QStringLiteral("mes")).toInt();
    l.ano          = q.value(QStringLiteral("ano")).toInt();
    l.dia          = q.value(QStringLiteral("dia")).toInt();
    l.nome         = q.value(QStringLiteral("nome")).toString();
    l.tipoAtivo    = q.value(QStringLiteral("tipo_ativo")).toString();
    l.tipoOperacao = q.value(QStringLiteral("tipo_operacao")).toString();
    l.valor        = q.value(QStringLiteral("valor")).toLongLong();
    l.categoria    = q.value(QStringLiteral("categoria")).toString();
    l.categoriaId  = q.value(QStringLiteral("categoria_id")).toInt();
    l.obs          = q.value(QStringLiteral("obs")).toString();
    l.criadoEm     = q.value(QStringLiteral("criado_em")).toString();
    l.receipt      = q.value(QStringLiteral("receipt")).toByteArray();
    l.ticker        = q.value(QStringLiteral("ticker")).toString();
    l.quantidade    = q.value(QStringLiteral("quantidade")).toDouble();
    l.precoUnitario = q.value(QStringLiteral("preco_unitario")).toLongLong();
    return l;
}

bool LancamentoInvestimentoRepository::insert(LancamentoInvestimento &l)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO lancamentos_investimentos
            (mes, ano, dia, nome, tipo_ativo, tipo_operacao, valor, categoria, categoria_id, obs, criado_em, receipt,
             ticker, quantidade, preco_unitario)
        VALUES
            (:mes, :ano, :dia, :nome, :tipo_ativo, :tipo_operacao, :valor, :categoria, :categoria_id, :obs, :criado_em, :receipt,
             :ticker, :quantidade, :preco_unitario)
    )"));
    q.bindValue(QStringLiteral(":mes"),           l.mes);
    q.bindValue(QStringLiteral(":ano"),           l.ano);
    q.bindValue(QStringLiteral(":dia"),           l.dia);
    q.bindValue(QStringLiteral(":nome"),          l.nome);
    q.bindValue(QStringLiteral(":tipo_ativo"),    l.tipoAtivo);
    q.bindValue(QStringLiteral(":tipo_operacao"), l.tipoOperacao);
    q.bindValue(QStringLiteral(":valor"),        l.valor);
    q.bindValue(QStringLiteral(":categoria"),    l.categoria);
    q.bindValue(QStringLiteral(":categoria_id"),
                l.categoriaId > 0 ? QVariant(l.categoriaId) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":obs"),          l.obs);
    q.bindValue(QStringLiteral(":criado_em"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":receipt"),      l.receipt.isEmpty() ? QVariant() : QVariant(l.receipt));
    q.bindValue(QStringLiteral(":ticker"),         l.ticker);
    q.bindValue(QStringLiteral(":quantidade"),     l.quantidade);
    q.bindValue(QStringLiteral(":preco_unitario"), l.precoUnitario);
    if (!q.exec()) {
        qWarning() << "LancamentoInvestimentoRepository::insert:" << q.lastError().text();
        return false;
    }
    l.id = q.lastInsertId().toInt();
    return true;
}

bool LancamentoInvestimentoRepository::update(const LancamentoInvestimento &l)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE lancamentos_investimentos SET
            mes            = :mes,
            ano            = :ano,
            dia            = :dia,
            nome           = :nome,
            tipo_ativo     = :tipo_ativo,
            tipo_operacao  = :tipo_operacao,
            valor          = :valor,
            categoria      = :categoria,
            categoria_id   = :categoria_id,
            obs            = :obs,
            receipt        = :receipt,
            ticker         = :ticker,
            quantidade     = :quantidade,
            preco_unitario = :preco_unitario
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":mes"),           l.mes);
    q.bindValue(QStringLiteral(":ano"),           l.ano);
    q.bindValue(QStringLiteral(":dia"),           l.dia);
    q.bindValue(QStringLiteral(":nome"),          l.nome);
    q.bindValue(QStringLiteral(":tipo_ativo"),    l.tipoAtivo);
    q.bindValue(QStringLiteral(":tipo_operacao"), l.tipoOperacao);
    q.bindValue(QStringLiteral(":valor"),        l.valor);
    q.bindValue(QStringLiteral(":categoria"),    l.categoria);
    q.bindValue(QStringLiteral(":categoria_id"),
                l.categoriaId > 0 ? QVariant(l.categoriaId) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":obs"),          l.obs);
    q.bindValue(QStringLiteral(":receipt"),      l.receipt.isEmpty() ? QVariant() : QVariant(l.receipt));
    q.bindValue(QStringLiteral(":ticker"),         l.ticker);
    q.bindValue(QStringLiteral(":quantidade"),     l.quantidade);
    q.bindValue(QStringLiteral(":preco_unitario"), l.precoUnitario);
    q.bindValue(QStringLiteral(":id"),            l.id);
    if (!q.exec()) {
        qWarning() << "LancamentoInvestimentoRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool LancamentoInvestimentoRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM lancamentos_investimentos WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "LancamentoInvestimentoRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<LancamentoInvestimento> LancamentoInvestimentoRepository::findAll() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT * FROM lancamentos_investimentos ORDER BY ano, mes, dia, id"), db());
    QList<LancamentoInvestimento> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

QList<LancamentoInvestimento> LancamentoInvestimentoRepository::findByMesAno(int mes, int ano) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM lancamentos_investimentos"
        " WHERE mes = :mes AND ano = :ano ORDER BY dia, id"));
    q.bindValue(QStringLiteral(":mes"), mes);
    q.bindValue(QStringLiteral(":ano"), ano);
    QList<LancamentoInvestimento> list;
    if (q.exec())
        while (q.next())
            list.append(fromQuery(q));
    return list;
}

std::optional<LancamentoInvestimento> LancamentoInvestimentoRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM lancamentos_investimentos WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}
