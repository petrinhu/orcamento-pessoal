#include "CompraRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

bool CompraRepository::insert(Compra &compra)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO compras
            (nome, valor_total, num_parcelas, parcela_inicial,
             mes_inicio, ano_inicio, juros_mensais, moeda, cartao_id, criado_em)
        VALUES
            (:nome, :valor_total, :num_parcelas, :parcela_inicial,
             :mes_inicio, :ano_inicio, :juros_mensais, :moeda, :cartao_id, :criado_em)
    )"));

    q.bindValue(QStringLiteral(":nome"),            compra.nome);
    q.bindValue(QStringLiteral(":valor_total"),     compra.valorTotal ? QVariant(compra.valorTotal) : QVariant());
    q.bindValue(QStringLiteral(":num_parcelas"),    compra.numParcelas);
    q.bindValue(QStringLiteral(":parcela_inicial"), compra.parcelaInicial);
    q.bindValue(QStringLiteral(":mes_inicio"),      compra.mesInicio);
    q.bindValue(QStringLiteral(":ano_inicio"),      compra.anoInicio);
    q.bindValue(QStringLiteral(":juros_mensais"),   compra.jurosMensais ? QVariant(compra.jurosMensais) : QVariant());
    q.bindValue(QStringLiteral(":moeda"),           compra.moeda);
    q.bindValue(QStringLiteral(":cartao_id"),       compra.cartaoId ? QVariant(compra.cartaoId) : QVariant());
    q.bindValue(QStringLiteral(":criado_em"),
                QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!q.exec()) {
        qWarning() << "CompraRepository::insert:" << q.lastError().text();
        return false;
    }
    compra.id = q.lastInsertId().toInt();
    return true;
}

bool CompraRepository::update(const Compra &compra)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE compras SET
            nome            = :nome,
            valor_total     = :valor_total,
            num_parcelas    = :num_parcelas,
            parcela_inicial = :parcela_inicial,
            mes_inicio      = :mes_inicio,
            ano_inicio      = :ano_inicio,
            juros_mensais   = :juros_mensais,
            moeda           = :moeda,
            cartao_id       = :cartao_id
        WHERE id = :id
    )"));

    q.bindValue(QStringLiteral(":nome"),            compra.nome);
    q.bindValue(QStringLiteral(":valor_total"),     compra.valorTotal ? QVariant(compra.valorTotal) : QVariant());
    q.bindValue(QStringLiteral(":num_parcelas"),    compra.numParcelas);
    q.bindValue(QStringLiteral(":parcela_inicial"), compra.parcelaInicial);
    q.bindValue(QStringLiteral(":mes_inicio"),      compra.mesInicio);
    q.bindValue(QStringLiteral(":ano_inicio"),      compra.anoInicio);
    q.bindValue(QStringLiteral(":juros_mensais"),   compra.jurosMensais ? QVariant(compra.jurosMensais) : QVariant());
    q.bindValue(QStringLiteral(":moeda"),           compra.moeda);
    q.bindValue(QStringLiteral(":cartao_id"),       compra.cartaoId ? QVariant(compra.cartaoId) : QVariant());
    q.bindValue(QStringLiteral(":id"),              compra.id);

    if (!q.exec()) {
        qWarning() << "CompraRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool CompraRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM compras WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "CompraRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

std::optional<Compra> CompraRepository::findById(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM compras WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}

QList<Compra> CompraRepository::findAll()
{
    QSqlQuery q(QStringLiteral("SELECT * FROM compras ORDER BY ano_inicio, mes_inicio, id"), db());
    QList<Compra> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

QList<Compra> CompraRepository::findByCartao(int cartaoId)
{
    QSqlQuery q(db());
    if (cartaoId == 0) {
        q.exec(QStringLiteral("SELECT * FROM compras ORDER BY ano_inicio, mes_inicio, id"));
    } else {
        q.prepare(QStringLiteral(
            "SELECT * FROM compras WHERE cartao_id = :cid ORDER BY ano_inicio, mes_inicio, id"));
        q.bindValue(QStringLiteral(":cid"), cartaoId);
        q.exec();
    }
    QList<Compra> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

Compra CompraRepository::fromQuery(const QSqlQuery &q)
{
    Compra c;
    c.id             = q.value(QStringLiteral("id")).toInt();
    c.nome           = q.value(QStringLiteral("nome")).toString();
    c.valorTotal     = q.value(QStringLiteral("valor_total")).toLongLong();
    c.numParcelas    = q.value(QStringLiteral("num_parcelas")).toInt();
    c.parcelaInicial = q.value(QStringLiteral("parcela_inicial")).toInt();
    c.mesInicio      = q.value(QStringLiteral("mes_inicio")).toInt();
    c.anoInicio      = q.value(QStringLiteral("ano_inicio")).toInt();
    c.jurosMensais   = q.value(QStringLiteral("juros_mensais")).toLongLong();
    c.moeda          = q.value(QStringLiteral("moeda")).toString();
    c.cartaoId       = q.value(QStringLiteral("cartao_id")).toInt();
    c.criadoEm       = q.value(QStringLiteral("criado_em")).toString();
    return c;
}
