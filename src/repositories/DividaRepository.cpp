#include "DividaRepository.h"
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

static Divida fromQuery(const QSqlQuery &q)
{
    Divida d;
    d.id          = q.value(QStringLiteral("id")).toInt();
    d.tipo        = q.value(QStringLiteral("tipo")).toString();
    d.contraparte = q.value(QStringLiteral("contraparte")).toString();
    d.descricao   = q.value(QStringLiteral("descricao")).toString();
    d.valorTotal  = q.value(QStringLiteral("valor_total")).toLongLong();
    d.valorPago   = q.value(QStringLiteral("valor_pago")).toLongLong();
    d.jurosMensal = q.value(QStringLiteral("juros_mensal")).toInt();
    d.dataInicio  = q.value(QStringLiteral("data_inicio")).toString();
    d.dataVenc    = q.value(QStringLiteral("data_venc")).toString();
    d.status      = q.value(QStringLiteral("status")).toString();
    d.obs         = q.value(QStringLiteral("obs")).toString();
    d.criadoEm   = q.value(QStringLiteral("criado_em")).toString();
    return d;
}

bool DividaRepository::insert(Divida &d)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO dividas
            (tipo, contraparte, descricao, valor_total, valor_pago,
             juros_mensal, data_inicio, data_venc, status, obs, criado_em)
        VALUES
            (:tipo, :contraparte, :descricao, :valor_total, :valor_pago,
             :juros_mensal, :data_inicio, :data_venc, :status, :obs, :criado_em)
    )"));
    q.bindValue(QStringLiteral(":tipo"),         naoNulo(d.tipo));
    q.bindValue(QStringLiteral(":contraparte"),  naoNulo(d.contraparte));
    q.bindValue(QStringLiteral(":descricao"),    naoNulo(d.descricao));
    q.bindValue(QStringLiteral(":valor_total"),  d.valorTotal);
    q.bindValue(QStringLiteral(":valor_pago"),   d.valorPago);
    q.bindValue(QStringLiteral(":juros_mensal"), d.jurosMensal);
    q.bindValue(QStringLiteral(":data_inicio"),  naoNulo(d.dataInicio));
    q.bindValue(QStringLiteral(":data_venc"),    naoNulo(d.dataVenc));
    q.bindValue(QStringLiteral(":status"),       naoNulo(d.status));
    q.bindValue(QStringLiteral(":obs"),          naoNulo(d.obs));
    q.bindValue(QStringLiteral(":criado_em"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "DividaRepository::insert:" << q.lastError().text();
        return false;
    }
    d.id = q.lastInsertId().toInt();
    return true;
}

bool DividaRepository::update(const Divida &d)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE dividas SET
            tipo         = :tipo,
            contraparte  = :contraparte,
            descricao    = :descricao,
            valor_total  = :valor_total,
            valor_pago   = :valor_pago,
            juros_mensal = :juros_mensal,
            data_inicio  = :data_inicio,
            data_venc    = :data_venc,
            status       = :status,
            obs          = :obs
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":tipo"),         naoNulo(d.tipo));
    q.bindValue(QStringLiteral(":contraparte"),  naoNulo(d.contraparte));
    q.bindValue(QStringLiteral(":descricao"),    naoNulo(d.descricao));
    q.bindValue(QStringLiteral(":valor_total"),  d.valorTotal);
    q.bindValue(QStringLiteral(":valor_pago"),   d.valorPago);
    q.bindValue(QStringLiteral(":juros_mensal"), d.jurosMensal);
    q.bindValue(QStringLiteral(":data_inicio"),  naoNulo(d.dataInicio));
    q.bindValue(QStringLiteral(":data_venc"),    naoNulo(d.dataVenc));
    q.bindValue(QStringLiteral(":status"),       naoNulo(d.status));
    q.bindValue(QStringLiteral(":obs"),          naoNulo(d.obs));
    q.bindValue(QStringLiteral(":id"),           d.id);
    if (!q.exec()) {
        qWarning() << "DividaRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool DividaRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM dividas WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "DividaRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<Divida> DividaRepository::findAll() const
{
    QSqlQuery q(db());
    q.exec(QStringLiteral(
        "SELECT * FROM dividas ORDER BY data_inicio DESC, id DESC"));
    QList<Divida> list;
    while (q.next())
        list << fromQuery(q);
    return list;
}

QList<Divida> DividaRepository::findByStatus(const QString &status) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM dividas WHERE status = :status ORDER BY data_inicio DESC"));
    q.bindValue(QStringLiteral(":status"), status);
    QList<Divida> list;
    if (!q.exec()) {
        qWarning() << "DividaRepository::findByStatus:" << q.lastError().text();
        return list;
    }
    while (q.next())
        list << fromQuery(q);
    return list;
}

std::optional<Divida> DividaRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM dividas WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "DividaRepository::findById:" << q.lastError().text();
        return std::nullopt;
    }
    if (q.next())
        return fromQuery(q);
    return std::nullopt;
}
