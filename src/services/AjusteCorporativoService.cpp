#include "AjusteCorporativoService.h"
#include "core/database/DatabaseManager.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

bool AjusteCorporativoService::ensureSchema()
{
    QSqlQuery q(db());
    return q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS ajustes_corporativos (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            ticker     TEXT    NOT NULL,
            tipo       TEXT    NOT NULL DEFAULT 'split',
            data_ex    TEXT    NOT NULL,
            fator      REAL    NOT NULL DEFAULT 1.0,
            observacao TEXT    NOT NULL DEFAULT '',
            criado_em  TEXT    NOT NULL
        )
    )"));
}

static AjusteCorporativo fromQuery(const QSqlQuery &q)
{
    AjusteCorporativo a;
    a.id         = q.value(QStringLiteral("id")).toLongLong();
    a.ticker     = q.value(QStringLiteral("ticker")).toString();
    a.tipo       = q.value(QStringLiteral("tipo")).toString();
    a.dataEx     = QDate::fromString(
        q.value(QStringLiteral("data_ex")).toString(), Qt::ISODate);
    a.fator      = q.value(QStringLiteral("fator")).toDouble();
    a.observacao = q.value(QStringLiteral("observacao")).toString();
    a.criadoEm   = q.value(QStringLiteral("criado_em")).toString();
    return a;
}

bool AjusteCorporativoService::insert(AjusteCorporativo &a)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO ajustes_corporativos
            (ticker, tipo, data_ex, fator, observacao, criado_em)
        VALUES
            (:ticker, :tipo, :data, :fator, :obs, :criado)
    )"));
    q.bindValue(QStringLiteral(":ticker"), a.ticker.toUpper());
    q.bindValue(QStringLiteral(":tipo"),   a.tipo);
    q.bindValue(QStringLiteral(":data"),   a.dataEx.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":fator"),  a.fator);
    q.bindValue(QStringLiteral(":obs"),    a.observacao);
    q.bindValue(QStringLiteral(":criado"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "AjusteCorporativoService::insert:" << q.lastError().text();
        return false;
    }
    a.id = q.lastInsertId().toLongLong();
    return true;
}

bool AjusteCorporativoService::remove(qint64 id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM ajustes_corporativos WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

QList<AjusteCorporativo> AjusteCorporativoService::findByTicker(const QString &ticker)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM ajustes_corporativos WHERE ticker = :ticker "
        "ORDER BY data_ex ASC"));
    q.bindValue(QStringLiteral(":ticker"), ticker.toUpper());
    QList<AjusteCorporativo> list;
    if (q.exec()) while (q.next()) list.append(fromQuery(q));
    return list;
}

QList<AjusteCorporativo> AjusteCorporativoService::findAll()
{
    QSqlQuery q(QStringLiteral(
        "SELECT * FROM ajustes_corporativos ORDER BY ticker, data_ex"),
        db());
    QList<AjusteCorporativo> list;
    while (q.next()) list.append(fromQuery(q));
    return list;
}

// fatorCumulativo: definicao inline no header (testavel sem DB)
