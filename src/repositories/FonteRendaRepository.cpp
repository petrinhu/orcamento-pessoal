#include "FonteRendaRepository.h"
#include "RepositoryUtils.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

bool FonteRendaRepository::insert(FonteRenda &f)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("INSERT INTO fontes_renda (nome) VALUES (:nome)"));
    q.bindValue(QStringLiteral(":nome"), naoNulo(f.nome));
    if (!q.exec()) {
        qWarning() << "FonteRendaRepository::insert:" << q.lastError().text();
        return false;
    }
    f.id = q.lastInsertId().toInt();
    return true;
}

bool FonteRendaRepository::update(const FonteRenda &f)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("UPDATE fontes_renda SET nome = :nome WHERE id = :id"));
    q.bindValue(QStringLiteral(":nome"), naoNulo(f.nome));
    q.bindValue(QStringLiteral(":id"),   f.id);
    if (!q.exec()) {
        qWarning() << "FonteRendaRepository::update:" << q.lastError().text();
        return false;
    }
    return true;
}

bool FonteRendaRepository::remove(int id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM fontes_renda WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        qWarning() << "FonteRendaRepository::remove:" << q.lastError().text();
        return false;
    }
    return true;
}

QList<FonteRenda> FonteRendaRepository::findAll() const
{
    QSqlQuery q(QStringLiteral("SELECT id, nome FROM fontes_renda ORDER BY nome"), db());
    QList<FonteRenda> list;
    while (q.next()) {
        FonteRenda f;
        f.id   = q.value(0).toInt();
        f.nome = q.value(1).toString();
        list.append(f);
    }
    return list;
}

std::optional<FonteRenda> FonteRendaRepository::findById(int id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT id, nome FROM fontes_renda WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next()) {
        FonteRenda f;
        f.id   = q.value(0).toInt();
        f.nome = q.value(1).toString();
        return f;
    }
    return std::nullopt;
}

FonteRenda FonteRendaRepository::findOrCreate(const QString &nome)
{
    const QString trimmed = nome.trimmed();
    // Try to find existing
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT id, nome FROM fontes_renda WHERE nome = :nome"));
    q.bindValue(QStringLiteral(":nome"), trimmed);
    if (q.exec() && q.next()) {
        FonteRenda f;
        f.id   = q.value(0).toInt();
        f.nome = q.value(1).toString();
        return f;
    }
    // Create new
    FonteRenda f;
    f.nome = trimmed;
    insert(f);
    return f;
}
