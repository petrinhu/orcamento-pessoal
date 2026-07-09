#include "ProventoRepository.h"
#include "RepositoryUtils.h"
#include "core/database/DatabaseManager.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

bool ProventoRepository::ensureSchema()
{
    QSqlQuery q(db());
    return q.exec(QStringLiteral(R"(
        CREATE TABLE IF NOT EXISTS proventos_recebidos (
            id              INTEGER PRIMARY KEY AUTOINCREMENT,
            ticker          TEXT    NOT NULL,
            tipo            TEXT    NOT NULL DEFAULT 'dividendo',
            data_evento     TEXT    NOT NULL,
            valor_bruto     INTEGER NOT NULL DEFAULT 0,
            valor_liquido   INTEGER NOT NULL DEFAULT 0,
            por_acao        REAL    NOT NULL DEFAULT 0,
            quantidade      REAL    NOT NULL DEFAULT 0,
            observacao      TEXT    NOT NULL DEFAULT '',
            criado_em       TEXT    NOT NULL
        )
    )"));
}

static Provento fromQuery(const QSqlQuery &q)
{
    Provento p;
    p.id               = q.value(QStringLiteral("id")).toLongLong();
    p.ticker           = q.value(QStringLiteral("ticker")).toString();
    p.tipo             = q.value(QStringLiteral("tipo")).toString();
    p.dataEvento       = QDate::fromString(
        q.value(QStringLiteral("data_evento")).toString(), Qt::ISODate);
    p.valorBrutoCent   = q.value(QStringLiteral("valor_bruto")).toLongLong();
    p.valorLiquidoCent = q.value(QStringLiteral("valor_liquido")).toLongLong();
    p.porAcao          = q.value(QStringLiteral("por_acao")).toDouble();
    p.quantidade       = q.value(QStringLiteral("quantidade")).toDouble();
    p.observacao       = q.value(QStringLiteral("observacao")).toString();
    p.criadoEm         = q.value(QStringLiteral("criado_em")).toString();
    return p;
}

bool ProventoRepository::insert(Provento &p)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT INTO proventos_recebidos
            (ticker, tipo, data_evento, valor_bruto, valor_liquido,
             por_acao, quantidade, observacao, criado_em)
        VALUES
            (:ticker, :tipo, :data, :bruto, :liquido,
             :poracao, :qty, :obs, :criado)
    )"));
    q.bindValue(QStringLiteral(":ticker"),  naoNulo(p.ticker.toUpper()));
    q.bindValue(QStringLiteral(":tipo"),    naoNulo(p.tipo));
    q.bindValue(QStringLiteral(":data"),    naoNulo(p.dataEvento.toString(Qt::ISODate)));
    q.bindValue(QStringLiteral(":bruto"),   p.valorBrutoCent);
    q.bindValue(QStringLiteral(":liquido"), p.valorLiquidoCent);
    q.bindValue(QStringLiteral(":poracao"), p.porAcao);
    q.bindValue(QStringLiteral(":qty"),     p.quantidade);
    q.bindValue(QStringLiteral(":obs"),     naoNulo(p.observacao));
    q.bindValue(QStringLiteral(":criado"),
                QDateTime::currentDateTime().toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "ProventoRepository::insert:" << q.lastError().text();
        return false;
    }
    p.id = q.lastInsertId().toLongLong();
    return true;
}

bool ProventoRepository::update(const Provento &p)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        UPDATE proventos_recebidos SET
            ticker = :ticker, tipo = :tipo, data_evento = :data,
            valor_bruto = :bruto, valor_liquido = :liquido,
            por_acao = :poracao, quantidade = :qty, observacao = :obs
        WHERE id = :id
    )"));
    q.bindValue(QStringLiteral(":ticker"),  naoNulo(p.ticker.toUpper()));
    q.bindValue(QStringLiteral(":tipo"),    naoNulo(p.tipo));
    q.bindValue(QStringLiteral(":data"),    naoNulo(p.dataEvento.toString(Qt::ISODate)));
    q.bindValue(QStringLiteral(":bruto"),   p.valorBrutoCent);
    q.bindValue(QStringLiteral(":liquido"), p.valorLiquidoCent);
    q.bindValue(QStringLiteral(":poracao"), p.porAcao);
    q.bindValue(QStringLiteral(":qty"),     p.quantidade);
    q.bindValue(QStringLiteral(":obs"),     naoNulo(p.observacao));
    q.bindValue(QStringLiteral(":id"),      p.id);
    return q.exec();
}

bool ProventoRepository::remove(qint64 id)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("DELETE FROM proventos_recebidos WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}

std::optional<Provento> ProventoRepository::findById(qint64 id) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral("SELECT * FROM proventos_recebidos WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (q.exec() && q.next()) return fromQuery(q);
    return std::nullopt;
}

QList<Provento> ProventoRepository::findAll() const
{
    QSqlQuery q(QStringLiteral(
        "SELECT * FROM proventos_recebidos ORDER BY data_evento DESC, id DESC"),
        db());
    QList<Provento> list;
    while (q.next()) list.append(fromQuery(q));
    return list;
}

QList<Provento> ProventoRepository::findByTicker(const QString &ticker) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM proventos_recebidos WHERE ticker = :ticker "
        "ORDER BY data_evento DESC"));
    q.bindValue(QStringLiteral(":ticker"), ticker.toUpper());
    QList<Provento> list;
    if (q.exec()) while (q.next()) list.append(fromQuery(q));
    return list;
}

QList<Provento> ProventoRepository::findByPeriod(const QDate &inicio,
                                                  const QDate &fim) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM proventos_recebidos "
        "WHERE data_evento BETWEEN :ini AND :fim "
        "ORDER BY data_evento DESC"));
    q.bindValue(QStringLiteral(":ini"), inicio.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":fim"), fim.toString(Qt::ISODate));
    QList<Provento> list;
    if (q.exec()) while (q.next()) list.append(fromQuery(q));
    return list;
}

qint64 ProventoRepository::totalLiquidoAno(int ano) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(valor_liquido), 0) FROM proventos_recebidos "
        "WHERE strftime('%Y', data_evento) = :ano"));
    q.bindValue(QStringLiteral(":ano"), QString::number(ano));
    if (q.exec() && q.next()) return q.value(0).toLongLong();
    return 0;
}

double ProventoRepository::dy12m(const QString &ticker,
                                  qint64 cotacaoAtualCent,
                                  double quantidade,
                                  const QDate &refData) const
{
    if (cotacaoAtualCent <= 0 || quantidade <= 0.0) return 0.0;

    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(valor_liquido), 0) FROM proventos_recebidos "
        "WHERE ticker = :ticker AND data_evento >= :inicio"));
    q.bindValue(QStringLiteral(":ticker"), ticker.toUpper());
    q.bindValue(QStringLiteral(":inicio"), refData.addYears(-1).toString(Qt::ISODate));
    if (!q.exec() || !q.next()) return 0.0;

    const double recebido12mCent = static_cast<double>(q.value(0).toLongLong());
    const double valorInvestido  = quantidade * static_cast<double>(cotacaoAtualCent);
    if (valorInvestido == 0.0) return 0.0;
    return recebido12mCent / valorInvestido;
}
