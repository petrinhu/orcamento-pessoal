#include "LembreteRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

static Lembrete fromQuery(const QSqlQuery &q)
{
    Lembrete l;
    l.id              = q.value(QStringLiteral("id")).toInt();
    l.descricao       = q.value(QStringLiteral("descricao")).toString();
    l.valor           = q.value(QStringLiteral("valor")).toLongLong();
    l.dataVenc        = q.value(QStringLiteral("data_venc")).toString();
    l.frequencia      = q.value(QStringLiteral("frequencia")).toString();
    l.debitoAutomatico = q.value(QStringLiteral("debito_automatico")).toInt() != 0;
    l.status          = q.value(QStringLiteral("status")).toString();
    l.obs             = q.value(QStringLiteral("obs")).toString();
    l.criadoEm        = q.value(QStringLiteral("criado_em")).toString();
    return l;
}

bool LembreteRepository::insert(Lembrete &l)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO lembretes
            (descricao, valor, data_venc, frequencia,
             debito_automatico, status, obs, criado_em)
        VALUES
            (:descricao, :valor, :data_venc, :frequencia,
             :debito_automatico, :status, :obs, :criado_em)
    )"));
    q.bindValue(QStringLiteral(":descricao"),        l.descricao);
    q.bindValue(QStringLiteral(":valor"),             l.valor);
    q.bindValue(QStringLiteral(":data_venc"),         l.dataVenc);
    q.bindValue(QStringLiteral(":frequencia"),        l.frequencia);
    q.bindValue(QStringLiteral(":debito_automatico"), l.debitoAutomatico ? 1 : 0);
    q.bindValue(QStringLiteral(":status"),            l.status);
    q.bindValue(QStringLiteral(":obs"),               l.obs);
    q.bindValue(QStringLiteral(":criado_em"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "LembreteRepository::insert:" << q.lastError().text();
        return false;
    }
    l.id = q.lastInsertId().toInt();
    return true;
}

bool LembreteRepository::update(const Lembrete &l)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE lembretes SET
            descricao        = :descricao,
            valor            = :valor,
            data_venc        = :data_venc,
            frequencia       = :frequencia,
            debito_automatico = :debito_automatico,
            status           = :status,
            obs              = :obs
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":descricao"),        l.descricao);
    q.bindValue(QStringLiteral(":valor"),             l.valor);
    q.bindValue(QStringLiteral(":data_venc"),         l.dataVenc);
    q.bindValue(QStringLiteral(":frequencia"),        l.frequencia);
    q.bindValue(QStringLiteral(":debito_automatico"), l.debitoAutomatico ? 1 : 0);
    q.bindValue(QStringLiteral(":status"),            l.status);
    q.bindValue(QStringLiteral(":obs"),               l.obs);
    q.bindValue(QStringLiteral(":id"),                l.id);
    if (!q.exec()) {
        qWarning() << "LembreteRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool LembreteRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM lembretes WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "LembreteRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<Lembrete> LembreteRepository::findAll() const
{
    QSqlQuery q(db());
    QList<Lembrete> list;
    if (!q.exec(QStringLiteral(
            "SELECT * FROM lembretes ORDER BY data_venc ASC, id ASC"))) {
        qWarning() << "LembreteRepository::findAll:" << q.lastError().text();
        return list;
    }
    while (q.next())
        list << fromQuery(q);
    return list;
}

QList<Lembrete> LembreteRepository::findAtivos() const
{
    QSqlQuery q(db());
    QList<Lembrete> list;
    if (!q.exec(QStringLiteral(
            "SELECT * FROM lembretes "
            "WHERE status IN ('pendente','nao_pago') "
            "ORDER BY data_venc ASC, id ASC"))) {
        qWarning() << "LembreteRepository::findAtivos:" << q.lastError().text();
        return list;
    }
    while (q.next())
        list << fromQuery(q);
    return list;
}

std::optional<Lembrete> LembreteRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM lembretes WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "LembreteRepository::findById:" << q.lastError().text();
        return std::nullopt;
    }
    if (q.next())
        return fromQuery(q);
    return std::nullopt;
}
