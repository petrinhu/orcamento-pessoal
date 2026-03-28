#include "CategoriaRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

// ── helpers ──────────────────────────────────────────────────────────────────

static Categoria fromQuery(const QSqlQuery &q)
{
    Categoria c;
    c.id      = q.value(QStringLiteral("id")).toInt();
    c.nome    = q.value(QStringLiteral("nome")).toString();
    c.tipo    = q.value(QStringLiteral("tipo")).toString();
    c.paiId   = q.value(QStringLiteral("pai_id")).toInt();
    return c;
}

// ── CRUD ─────────────────────────────────────────────────────────────────────

int CategoriaRepository::insert(const Categoria &c)
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "INSERT INTO categorias (nome, tipo, pai_id) VALUES (:nome, :tipo, :pai_id)"));
    q.bindValue(QStringLiteral(":nome"),   c.nome);
    q.bindValue(QStringLiteral(":tipo"),   c.tipo);
    if (c.paiId > 0)
        q.bindValue(QStringLiteral(":pai_id"), c.paiId);
    else
        q.bindValue(QStringLiteral(":pai_id"), QVariant(QMetaType::fromType<int>()));

    if (!q.exec()) {
        qWarning() << "CategoriaRepository::insert:" << q.lastError().text();
        return 0;
    }
    return q.lastInsertId().toInt();
}

bool CategoriaRepository::update(const Categoria &c)
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "UPDATE categorias SET nome=:nome, tipo=:tipo, pai_id=:pai_id WHERE id=:id"));
    q.bindValue(QStringLiteral(":nome"),   c.nome);
    q.bindValue(QStringLiteral(":tipo"),   c.tipo);
    if (c.paiId > 0)
        q.bindValue(QStringLiteral(":pai_id"), c.paiId);
    else
        q.bindValue(QStringLiteral(":pai_id"), QVariant(QMetaType::fromType<int>()));
    q.bindValue(QStringLiteral(":id"),     c.id);

    if (!q.exec()) {
        qWarning() << "CategoriaRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool CategoriaRepository::remove(int id)
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral("DELETE FROM categorias WHERE id=:id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "CategoriaRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

// ── Queries ───────────────────────────────────────────────────────────────────

QList<Categoria> CategoriaRepository::findAll() const
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "SELECT c.id, c.nome, c.tipo, c.pai_id, p.nome AS pai_nome "
        "FROM categorias c LEFT JOIN categorias p ON c.pai_id = p.id "
        "ORDER BY c.tipo, p.nome NULLS FIRST, c.nome"));
    QList<Categoria> list;
    if (!q.exec()) {
        qWarning() << "CategoriaRepository::findAll:" << q.lastError().text();
        return list;
    }
    while (q.next()) {
        Categoria c = fromQuery(q);
        c.paiNome = q.value(QStringLiteral("pai_nome")).toString();
        list.append(c);
    }
    return list;
}

QList<Categoria> CategoriaRepository::findByTipo(const QString &tipo) const
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "SELECT c.id, c.nome, c.tipo, c.pai_id, p.nome AS pai_nome "
        "FROM categorias c LEFT JOIN categorias p ON c.pai_id = p.id "
        "WHERE c.tipo=:tipo "
        "ORDER BY p.nome NULLS FIRST, c.nome"));
    q.bindValue(QStringLiteral(":tipo"), tipo);
    QList<Categoria> list;
    if (!q.exec()) {
        qWarning() << "CategoriaRepository::findByTipo:" << q.lastError().text();
        return list;
    }
    while (q.next()) {
        Categoria c = fromQuery(q);
        c.paiNome = q.value(QStringLiteral("pai_nome")).toString();
        list.append(c);
    }
    return list;
}

Categoria CategoriaRepository::findById(int id) const
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "SELECT c.id, c.nome, c.tipo, c.pai_id, p.nome AS pai_nome "
        "FROM categorias c LEFT JOIN categorias p ON c.pai_id = p.id "
        "WHERE c.id=:id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next()) {
        Categoria c = fromQuery(q);
        c.paiNome = q.value(QStringLiteral("pai_nome")).toString();
        return c;
    }
    return {};
}

int CategoriaRepository::findOrCreate(const QString &nome, const QString &tipo, int paiId)
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "SELECT id FROM categorias "
        "WHERE lower(nome)=lower(:nome) AND tipo=:tipo AND "
        "COALESCE(pai_id,0)=:pai_id"));
    q.bindValue(QStringLiteral(":nome"),   nome);
    q.bindValue(QStringLiteral(":tipo"),   tipo);
    q.bindValue(QStringLiteral(":pai_id"), paiId);
    if (q.exec() && q.next())
        return q.value(0).toInt();

    Categoria c;
    c.nome  = nome;
    c.tipo  = tipo;
    c.paiId = paiId;
    return insert(c);
}

QStringList CategoriaRepository::nomes(const QString &tipo) const
{
    const auto list = findByTipo(tipo);
    QStringList result;
    for (const auto &c : list) {
        if (c.paiId == 0)
            result.append(c.nome);
        else
            result.append(c.paiNome + QStringLiteral(" > ") + c.nome);
    }
    return result;
}
