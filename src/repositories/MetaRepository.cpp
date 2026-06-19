#include "MetaRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

static Meta fromQuery(const QSqlQuery &q)
{
    Meta m;
    m.id         = q.value(QStringLiteral("id")).toInt();
    m.nome       = q.value(QStringLiteral("nome")).toString();
    m.categoria  = q.value(QStringLiteral("categoria")).toString();
    m.valorMeta  = q.value(QStringLiteral("valor_meta")).toLongLong();
    m.valorAtual = q.value(QStringLiteral("valor_atual")).toLongLong();
    m.dataInicio = q.value(QStringLiteral("data_inicio")).toString();
    m.dataPrazo  = q.value(QStringLiteral("data_prazo")).toString();
    m.status     = q.value(QStringLiteral("status")).toString();
    m.obs        = q.value(QStringLiteral("obs")).toString();
    m.criadoEm  = q.value(QStringLiteral("criado_em")).toString();
    return m;
}

bool MetaRepository::insert(Meta &m)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO metas
            (nome, categoria, valor_meta, valor_atual,
             data_inicio, data_prazo, status, obs, criado_em)
        VALUES
            (:nome, :categoria, :valor_meta, :valor_atual,
             :data_inicio, :data_prazo, :status, :obs, :criado_em)
    )"));
    q.bindValue(QStringLiteral(":nome"),        m.nome);
    q.bindValue(QStringLiteral(":categoria"),   m.categoria);
    q.bindValue(QStringLiteral(":valor_meta"),  m.valorMeta);
    q.bindValue(QStringLiteral(":valor_atual"), m.valorAtual);
    q.bindValue(QStringLiteral(":data_inicio"), m.dataInicio);
    q.bindValue(QStringLiteral(":data_prazo"),  m.dataPrazo);
    q.bindValue(QStringLiteral(":status"),      m.status);
    q.bindValue(QStringLiteral(":obs"),         m.obs);
    q.bindValue(QStringLiteral(":criado_em"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "MetaRepository::insert:" << q.lastError().text();
        return false;
    }
    m.id = q.lastInsertId().toInt();
    return true;
}

bool MetaRepository::update(const Meta &m)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE metas SET
            nome        = :nome,
            categoria   = :categoria,
            valor_meta  = :valor_meta,
            valor_atual = :valor_atual,
            data_inicio = :data_inicio,
            data_prazo  = :data_prazo,
            status      = :status,
            obs         = :obs
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":nome"),        m.nome);
    q.bindValue(QStringLiteral(":categoria"),   m.categoria);
    q.bindValue(QStringLiteral(":valor_meta"),  m.valorMeta);
    q.bindValue(QStringLiteral(":valor_atual"), m.valorAtual);
    q.bindValue(QStringLiteral(":data_inicio"), m.dataInicio);
    q.bindValue(QStringLiteral(":data_prazo"),  m.dataPrazo);
    q.bindValue(QStringLiteral(":status"),      m.status);
    q.bindValue(QStringLiteral(":obs"),         m.obs);
    q.bindValue(QStringLiteral(":id"),          m.id);
    if (!q.exec()) {
        qWarning() << "MetaRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool MetaRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM metas WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "MetaRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<Meta> MetaRepository::findAll() const
{
    QSqlQuery q(db());
    QList<Meta> list;
    if (!q.exec(QStringLiteral(
            "SELECT * FROM metas ORDER BY status ASC, data_prazo ASC, id ASC"))) {
        qWarning() << "MetaRepository::findAll:" << q.lastError().text();
        return list;
    }
    while (q.next())
        list << fromQuery(q);
    return list;
}

std::optional<Meta> MetaRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM metas WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "MetaRepository::findById:" << q.lastError().text();
        return std::nullopt;
    }
    if (q.next())
        return fromQuery(q);
    return std::nullopt;
}
