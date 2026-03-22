#include "ParcelaRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

bool ParcelaRepository::insert(Parcela &p)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO parcelas (compra_id, numero, valor, data_fatura, modificada)
        VALUES (:compra_id, :numero, :valor, :data_fatura, :modificada)
    )"));
    q.bindValue(QStringLiteral(":compra_id"),   p.compraId);
    q.bindValue(QStringLiteral(":numero"),      p.numero);
    q.bindValue(QStringLiteral(":valor"),       p.valor);
    q.bindValue(QStringLiteral(":data_fatura"), p.dataFatura.isEmpty() ? QVariant() : QVariant(p.dataFatura));
    q.bindValue(QStringLiteral(":modificada"),  p.modificada ? 1 : 0);

    if (!q.exec()) {
        qWarning() << "ParcelaRepository::insert:" << q.lastError().text();
        return false;
    }
    p.id = q.lastInsertId().toInt();
    return true;
}

bool ParcelaRepository::update(const Parcela &p)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE parcelas SET
            valor       = :valor,
            data_fatura = :data_fatura,
            modificada  = :modificada
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":valor"),       p.valor);
    q.bindValue(QStringLiteral(":data_fatura"), p.dataFatura.isEmpty() ? QVariant() : QVariant(p.dataFatura));
    q.bindValue(QStringLiteral(":modificada"),  p.modificada ? 1 : 0);
    q.bindValue(QStringLiteral(":id"),          p.id);

    if (!q.exec()) {
        qWarning() << "ParcelaRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ParcelaRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM parcelas WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "ParcelaRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

std::optional<Parcela> ParcelaRepository::findById(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM parcelas WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}

QList<Parcela> ParcelaRepository::findAll()
{
    QSqlQuery q(QStringLiteral("SELECT * FROM parcelas ORDER BY compra_id, numero"), db());
    QList<Parcela> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

QList<Parcela> ParcelaRepository::findByCompra(int compraId)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM parcelas WHERE compra_id = :cid ORDER BY numero"));
    q.bindValue(QStringLiteral(":cid"), compraId);
    QList<Parcela> list;
    if (q.exec())
        while (q.next())
            list.append(fromQuery(q));
    return list;
}

bool ParcelaRepository::replaceAll(int compraId, const QList<Parcela> &parcelas)
{
    QSqlQuery del(db());
    del.prepare(QStringLiteral("DELETE FROM parcelas WHERE compra_id = :cid"));
    del.bindValue(QStringLiteral(":cid"), compraId);
    if (!del.exec()) return false;

    for (auto p : parcelas) {
        p.compraId = compraId;
        if (!insert(p)) return false;
    }
    return true;
}

Parcela ParcelaRepository::fromQuery(const QSqlQuery &q)
{
    Parcela p;
    p.id         = q.value(QStringLiteral("id")).toInt();
    p.compraId   = q.value(QStringLiteral("compra_id")).toInt();
    p.numero     = q.value(QStringLiteral("numero")).toInt();
    p.valor      = q.value(QStringLiteral("valor")).toLongLong();
    p.dataFatura = q.value(QStringLiteral("data_fatura")).toString();
    p.modificada = q.value(QStringLiteral("modificada")).toInt() != 0;
    return p;
}
