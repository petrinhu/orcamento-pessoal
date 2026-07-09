#include "OrcamentoRepository.h"
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

static ItemOrcamento fromQuery(const QSqlQuery &q)
{
    ItemOrcamento item;
    item.id           = q.value(QStringLiteral("id")).toInt();
    item.nome         = q.value(QStringLiteral("nome")).toString();
    item.tipo         = q.value(QStringLiteral("tipo")).toString();
    item.valorMensal  = q.value(QStringLiteral("valor_mensal")).toLongLong();
    item.categoriaRef = q.value(QStringLiteral("categoria_ref")).toString();
    item.criadoEm     = q.value(QStringLiteral("criado_em")).toString();
    return item;
}

bool OrcamentoRepository::insert(ItemOrcamento &item)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO orcamento_itens
            (nome, tipo, valor_mensal, categoria_ref, criado_em)
        VALUES
            (:nome, :tipo, :valor_mensal, :categoria_ref, :criado_em)
    )"));
    q.bindValue(QStringLiteral(":nome"),          naoNulo(item.nome));
    q.bindValue(QStringLiteral(":tipo"),          naoNulo(item.tipo));
    q.bindValue(QStringLiteral(":valor_mensal"),  item.valorMensal);
    q.bindValue(QStringLiteral(":categoria_ref"), naoNulo(item.categoriaRef));
    q.bindValue(QStringLiteral(":criado_em"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "OrcamentoRepository::insert:" << q.lastError().text();
        return false;
    }
    item.id = q.lastInsertId().toInt();
    return true;
}

bool OrcamentoRepository::update(const ItemOrcamento &item)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE orcamento_itens SET
            nome          = :nome,
            tipo          = :tipo,
            valor_mensal  = :valor_mensal,
            categoria_ref = :categoria_ref
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":nome"),          naoNulo(item.nome));
    q.bindValue(QStringLiteral(":tipo"),          naoNulo(item.tipo));
    q.bindValue(QStringLiteral(":valor_mensal"),  item.valorMensal);
    q.bindValue(QStringLiteral(":categoria_ref"), naoNulo(item.categoriaRef));
    q.bindValue(QStringLiteral(":id"),            item.id);
    if (!q.exec()) {
        qWarning() << "OrcamentoRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool OrcamentoRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM orcamento_itens WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "OrcamentoRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<ItemOrcamento> OrcamentoRepository::findAll() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT * FROM orcamento_itens"
        " ORDER BY CASE tipo WHEN 'receita' THEN 0 ELSE 1 END, nome COLLATE NOCASE"),
        db());
    QList<ItemOrcamento> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

std::optional<ItemOrcamento> OrcamentoRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM orcamento_itens WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}
