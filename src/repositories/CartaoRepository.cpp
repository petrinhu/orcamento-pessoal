#include "CartaoRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

bool CartaoRepository::insert(Cartao &c)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("INSERT INTO cartoes (apelido) VALUES (:ap)"));
    q.bindValue(QStringLiteral(":ap"), c.apelido);
    if (!q.exec()) {
        qWarning() << "CartaoRepository::insert:" << q.lastError().text();
        return false;
    }
    c.id = q.lastInsertId().toInt();
    return true;
}

bool CartaoRepository::update(const Cartao &c)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("UPDATE cartoes SET apelido = :ap WHERE id = :id"));
    q.bindValue(QStringLiteral(":ap"), c.apelido);
    q.bindValue(QStringLiteral(":id"), c.id);
    if (!q.exec()) {
        qWarning() << "CartaoRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool CartaoRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM cartoes WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "CartaoRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<Cartao> CartaoRepository::findAll() const
{
    QSqlQuery q(QStringLiteral("SELECT id, apelido FROM cartoes ORDER BY apelido"), db());
    QList<Cartao> list;
    while (q.next()) {
        Cartao c;
        c.id      = q.value(0).toInt();
        c.apelido = q.value(1).toString();
        list.append(c);
    }
    return list;
}

std::optional<Cartao> CartaoRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT id, apelido FROM cartoes WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next()) {
        Cartao c;
        c.id      = q.value(0).toInt();
        c.apelido = q.value(1).toString();
        return c;
    }
    return std::nullopt;
}
