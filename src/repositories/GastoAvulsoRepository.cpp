#include "GastoAvulsoRepository.h"
#include "RepositoryUtils.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

static GastoAvulso fromQuery(const QSqlQuery &q)
{
    GastoAvulso g;
    g.id        = q.value(QStringLiteral("id")).toInt();
    g.mes       = q.value(QStringLiteral("mes")).toInt();
    g.ano       = q.value(QStringLiteral("ano")).toInt();
    g.dia       = q.value(QStringLiteral("dia")).toInt();
    g.descricao = q.value(QStringLiteral("descricao")).toString();
    g.categoria = q.value(QStringLiteral("categoria")).toString();
    g.tipoPgto  = q.value(QStringLiteral("tipo_pgto")).toString();
    g.valor     = q.value(QStringLiteral("valor")).toLongLong();
    g.obs       = q.value(QStringLiteral("obs")).toString();
    g.pagador     = q.value(QStringLiteral("pagador")).toString();
    g.pagadorId   = q.value(QStringLiteral("pagador_id")).toInt();
    g.categoriaId = q.value(QStringLiteral("categoria_id")).toInt();
    g.criadoEm    = q.value(QStringLiteral("criado_em")).toString();
    g.receipt     = q.value(QStringLiteral("receipt")).toByteArray();
    return g;
}

bool GastoAvulsoRepository::insert(GastoAvulso &g)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO lancamentos_gastos
            (mes, ano, dia, descricao, categoria, categoria_id, tipo_pgto, valor, obs,
             pagador, pagador_id, criado_em, receipt)
        VALUES
            (:mes, :ano, :dia, :descricao, :categoria, :categoria_id, :tipo_pgto, :valor, :obs,
             :pagador, :pagador_id, :criado_em, :receipt)
    )"));
    q.bindValue(QStringLiteral(":mes"),        g.mes);
    q.bindValue(QStringLiteral(":ano"),        g.ano);
    q.bindValue(QStringLiteral(":dia"),        g.dia);
    q.bindValue(QStringLiteral(":descricao"),  naoNulo(g.descricao));
    q.bindValue(QStringLiteral(":categoria"),    naoNulo(g.categoria));
    q.bindValue(QStringLiteral(":categoria_id"),
                g.categoriaId > 0 ? QVariant(g.categoriaId) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":tipo_pgto"),  naoNulo(g.tipoPgto));
    q.bindValue(QStringLiteral(":valor"),      g.valor);
    q.bindValue(QStringLiteral(":obs"),        naoNulo(g.obs));
    q.bindValue(QStringLiteral(":pagador"),    naoNulo(g.pagador));
    q.bindValue(QStringLiteral(":pagador_id"),
                g.pagadorId > 0 ? QVariant(g.pagadorId) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":criado_em"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":receipt"),    g.receipt.isEmpty() ? QVariant() : QVariant(g.receipt));
    if (!q.exec()) {
        qWarning() << "GastoAvulsoRepository::insert:" << q.lastError().text();
        return false;
    }
    g.id = q.lastInsertId().toInt();
    return true;
}

bool GastoAvulsoRepository::update(const GastoAvulso &g)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE lancamentos_gastos SET
            mes          = :mes,
            ano          = :ano,
            dia          = :dia,
            descricao    = :descricao,
            categoria    = :categoria,
            categoria_id = :categoria_id,
            tipo_pgto    = :tipo_pgto,
            valor        = :valor,
            obs          = :obs,
            pagador      = :pagador,
            pagador_id   = :pagador_id,
            receipt      = :receipt
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":mes"),        g.mes);
    q.bindValue(QStringLiteral(":ano"),        g.ano);
    q.bindValue(QStringLiteral(":dia"),        g.dia);
    q.bindValue(QStringLiteral(":descricao"),  naoNulo(g.descricao));
    q.bindValue(QStringLiteral(":categoria"),    naoNulo(g.categoria));
    q.bindValue(QStringLiteral(":categoria_id"),
                g.categoriaId > 0 ? QVariant(g.categoriaId) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":tipo_pgto"),  naoNulo(g.tipoPgto));
    q.bindValue(QStringLiteral(":valor"),      g.valor);
    q.bindValue(QStringLiteral(":obs"),        naoNulo(g.obs));
    q.bindValue(QStringLiteral(":pagador"),    naoNulo(g.pagador));
    q.bindValue(QStringLiteral(":pagador_id"),
                g.pagadorId > 0 ? QVariant(g.pagadorId) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":receipt"),    g.receipt.isEmpty() ? QVariant() : QVariant(g.receipt));
    q.bindValue(QStringLiteral(":id"),         g.id);
    if (!q.exec()) {
        qWarning() << "GastoAvulsoRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool GastoAvulsoRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM lancamentos_gastos WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "GastoAvulsoRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<GastoAvulso> GastoAvulsoRepository::findAll() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT * FROM lancamentos_gastos ORDER BY ano, mes, dia, id"), db());
    QList<GastoAvulso> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

QList<GastoAvulso> GastoAvulsoRepository::findByMesAno(int mes, int ano) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM lancamentos_gastos WHERE mes = :mes AND ano = :ano ORDER BY dia, id"));
    q.bindValue(QStringLiteral(":mes"), mes);
    q.bindValue(QStringLiteral(":ano"), ano);
    QList<GastoAvulso> list;
    if (q.exec())
        while (q.next())
            list.append(fromQuery(q));
    return list;
}

std::optional<GastoAvulso> GastoAvulsoRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM lancamentos_gastos WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}

QStringList GastoAvulsoRepository::categorias() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT DISTINCT categoria FROM lancamentos_gastos"
        " WHERE categoria != '' ORDER BY categoria"), db());
    QStringList list;
    while (q.next())
        list.append(q.value(0).toString());
    return list;
}
