#include "PagadorRepository.h"
#include "RepositoryUtils.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

static Pagador fromQuery(const QSqlQuery &q)
{
    Pagador p;
    p.id   = q.value(QStringLiteral("id")).toInt();
    p.nome = q.value(QStringLiteral("nome")).toString();
    p.tipo = q.value(QStringLiteral("tipo")).toString();
    p.obs  = q.value(QStringLiteral("obs")).toString();
    return p;
}

bool PagadorRepository::insert(Pagador &p)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO pagadores (nome, tipo, obs)
        VALUES (:nome, :tipo, :obs)
    )"));
    q.bindValue(QStringLiteral(":nome"), naoNulo(p.nome));
    q.bindValue(QStringLiteral(":tipo"), naoNulo(p.tipo));
    q.bindValue(QStringLiteral(":obs"),  naoNulo(p.obs));
    if (!q.exec()) {
        qWarning() << "PagadorRepository::insert:" << q.lastError().text();
        return false;
    }
    p.id = q.lastInsertId().toInt();
    return true;
}

bool PagadorRepository::update(const Pagador &p)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE pagadores SET nome = :nome, tipo = :tipo, obs = :obs
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":nome"), naoNulo(p.nome));
    q.bindValue(QStringLiteral(":tipo"), naoNulo(p.tipo));
    q.bindValue(QStringLiteral(":obs"),  naoNulo(p.obs));
    q.bindValue(QStringLiteral(":id"),   p.id);
    if (!q.exec()) {
        qWarning() << "PagadorRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PagadorRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM pagadores WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "PagadorRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<Pagador> PagadorRepository::findAll() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT * FROM pagadores ORDER BY nome COLLATE NOCASE"), db());
    QList<Pagador> list;
    while (q.next())
        list.append(fromQuery(q));
    return list;
}

std::optional<Pagador> PagadorRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM pagadores WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next())
        return fromQuery(q);
    return std::nullopt;
}

int PagadorRepository::findOrCreate(const QString &nome, const QString &tipo)
{
    if (nome.isEmpty()) return 0;

    // Try to find existing (case-insensitive)
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT id FROM pagadores WHERE nome = :nome COLLATE NOCASE"));
    q.bindValue(QStringLiteral(":nome"), nome);
    if (q.exec() && q.next())
        return q.value(0).toInt();

    // Insert new
    Pagador p;
    p.nome = nome;
    p.tipo = tipo;
    insert(p);
    return p.id;
}

QStringList PagadorRepository::nomes() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT nome FROM pagadores ORDER BY nome COLLATE NOCASE"), db());
    QStringList list;
    while (q.next())
        list.append(q.value(0).toString());
    return list;
}
