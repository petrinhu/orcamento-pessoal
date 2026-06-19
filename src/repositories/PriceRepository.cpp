#include "PriceRepository.h"
#include "core/database/DatabaseManager.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QDebug>

static QSqlDatabase db()
{
    return DatabaseManager::instance().db();
}

// ── helpers ───────────────────────────────────────────────────────────────────

static PrecoAtivo precoFromQuery(const QSqlQuery &q)
{
    PrecoAtivo p;
    p.ticker     = q.value(QStringLiteral("ticker")).toString();
    p.data       = QDate::fromString(
                       q.value(QStringLiteral("data")).toString(),
                       Qt::ISODate);
    p.abertura   = q.value(QStringLiteral("abertura")).toLongLong();
    p.maxima     = q.value(QStringLiteral("maxima")).toLongLong();
    p.minima     = q.value(QStringLiteral("minima")).toLongLong();
    p.fechamento = q.value(QStringLiteral("fechamento")).toLongLong();
    p.volume     = q.value(QStringLiteral("volume")).toLongLong();
    p.fonte      = q.value(QStringLiteral("fonte")).toString();
    p.atualizado = QDateTime::fromString(
                       q.value(QStringLiteral("atualizado")).toString(),
                       Qt::ISODate);
    return p;
}

static CotacaoRT rtFromQuery(const QSqlQuery &q)
{
    CotacaoRT c;
    c.ticker      = q.value(QStringLiteral("ticker")).toString();
    c.preco       = q.value(QStringLiteral("preco")).toLongLong();
    c.variacaoPct = q.value(QStringLiteral("variacao_pct")).toInt();
    c.abertura    = q.value(QStringLiteral("abertura")).toLongLong();
    c.minima      = q.value(QStringLiteral("minima")).toLongLong();
    c.maxima      = q.value(QStringLiteral("maxima")).toLongLong();
    c.volume      = q.value(QStringLiteral("volume")).toLongLong();
    c.timestamp   = QDateTime::fromString(
                        q.value(QStringLiteral("timestamp")).toString(),
                        Qt::ISODate);
    return c;
}

// ── OHLCV histórico ───────────────────────────────────────────────────────────

bool PriceRepository::upsert(const PrecoAtivo &p)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT OR REPLACE INTO precos_ativos
            (ticker, data, abertura, maxima, minima, fechamento, volume, fonte, atualizado)
        VALUES
            (:ticker, :data, :abertura, :maxima, :minima, :fechamento, :volume, :fonte, :atualizado)
    )"));
    q.bindValue(QStringLiteral(":ticker"),     p.ticker);
    q.bindValue(QStringLiteral(":data"),       p.data.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":abertura"),   p.abertura);
    q.bindValue(QStringLiteral(":maxima"),     p.maxima);
    q.bindValue(QStringLiteral(":minima"),     p.minima);
    q.bindValue(QStringLiteral(":fechamento"), p.fechamento);
    q.bindValue(QStringLiteral(":volume"),     p.volume);
    q.bindValue(QStringLiteral(":fonte"),      p.fonte);
    q.bindValue(QStringLiteral(":atualizado"), p.atualizado.toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "PriceRepository::upsert:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PriceRepository::upsertBatch(const QList<PrecoAtivo> &lista)
{
    if (lista.isEmpty())
        return true;

    QSqlDatabase d = db();
    if (!d.transaction()) {
        qWarning() << "PriceRepository::upsertBatch: não foi possível iniciar transação";
        return false;
    }

    QSqlQuery q(d);
    q.prepare(QStringLiteral(R"(
        INSERT OR REPLACE INTO precos_ativos
            (ticker, data, abertura, maxima, minima, fechamento, volume, fonte, atualizado)
        VALUES
            (:ticker, :data, :abertura, :maxima, :minima, :fechamento, :volume, :fonte, :atualizado)
    )"));

    for (const PrecoAtivo &p : lista) {
        q.bindValue(QStringLiteral(":ticker"),     p.ticker);
        q.bindValue(QStringLiteral(":data"),       p.data.toString(Qt::ISODate));
        q.bindValue(QStringLiteral(":abertura"),   p.abertura);
        q.bindValue(QStringLiteral(":maxima"),     p.maxima);
        q.bindValue(QStringLiteral(":minima"),     p.minima);
        q.bindValue(QStringLiteral(":fechamento"), p.fechamento);
        q.bindValue(QStringLiteral(":volume"),     p.volume);
        q.bindValue(QStringLiteral(":fonte"),      p.fonte);
        q.bindValue(QStringLiteral(":atualizado"), p.atualizado.toString(Qt::ISODate));
        if (!q.exec()) {
            qWarning() << "PriceRepository::upsertBatch:" << q.lastError().text();
            d.rollback();
            return false;
        }
    }

    if (!d.commit()) {
        qWarning() << "PriceRepository::upsertBatch: commit falhou";
        d.rollback();
        return false;
    }
    return true;
}

std::optional<PrecoAtivo> PriceRepository::findLatest(const QString &ticker) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM precos_ativos "
        "WHERE ticker = :ticker "
        "ORDER BY data DESC LIMIT 1"));
    q.bindValue(QStringLiteral(":ticker"), ticker);
    if (!q.exec()) {
        qWarning() << "PriceRepository::findLatest:" << q.lastError().text();
        return std::nullopt;
    }
    if (q.next())
        return precoFromQuery(q);
    return std::nullopt;
}

std::optional<PrecoAtivo> PriceRepository::findByTickerAndDate(
    const QString &ticker, const QDate &data) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM precos_ativos "
        "WHERE ticker = :ticker AND data = :data"));
    q.bindValue(QStringLiteral(":ticker"), ticker);
    q.bindValue(QStringLiteral(":data"),   data.toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "PriceRepository::findByTickerAndDate:" << q.lastError().text();
        return std::nullopt;
    }
    if (q.next())
        return precoFromQuery(q);
    return std::nullopt;
}

QList<PrecoAtivo> PriceRepository::findByTickerRange(
    const QString &ticker, const QDate &inicio, const QDate &fim) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM precos_ativos "
        "WHERE ticker = :ticker AND data BETWEEN :inicio AND :fim "
        "ORDER BY data ASC"));
    q.bindValue(QStringLiteral(":ticker"), ticker);
    q.bindValue(QStringLiteral(":inicio"), inicio.toString(Qt::ISODate));
    q.bindValue(QStringLiteral(":fim"),    fim.toString(Qt::ISODate));
    QList<PrecoAtivo> list;
    if (!q.exec()) {
        qWarning() << "PriceRepository::findByTickerRange:" << q.lastError().text();
        return list;
    }
    while (q.next())
        list << precoFromQuery(q);
    return list;
}

QStringList PriceRepository::tickersComHistorico() const
{
    QSqlQuery q(db());
    QStringList list;
    if (!q.exec(QStringLiteral(
            "SELECT DISTINCT ticker FROM precos_ativos ORDER BY ticker ASC"))) {
        qWarning() << "PriceRepository::tickersComHistorico:" << q.lastError().text();
        return list;
    }
    while (q.next())
        list << q.value(0).toString();
    return list;
}

int PriceRepository::countByTicker(const QString &ticker) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM precos_ativos WHERE ticker = :ticker"));
    q.bindValue(QStringLiteral(":ticker"), ticker);
    if (!q.exec()) {
        qWarning() << "PriceRepository::countByTicker:" << q.lastError().text();
        return 0;
    }
    if (q.next())
        return q.value(0).toInt();
    return 0;
}

bool PriceRepository::removeByTicker(const QString &ticker)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "DELETE FROM precos_ativos WHERE ticker = :ticker"));
    q.bindValue(QStringLiteral(":ticker"), ticker);
    if (!q.exec()) {
        qWarning() << "PriceRepository::removeByTicker:" << q.lastError().text();
        return false;
    }
    return true;
}

bool PriceRepository::removeAll()
{
    QSqlQuery q(db());
    if (!q.exec(QStringLiteral("DELETE FROM precos_ativos"))) {
        qWarning() << "PriceRepository::removeAll:" << q.lastError().text();
        return false;
    }
    return true;
}

// ── Cotação em tempo real ─────────────────────────────────────────────────────

bool PriceRepository::upsertRT(const CotacaoRT &c)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(R"(
        INSERT OR REPLACE INTO cotacoes_tempo_real
            (ticker, preco, variacao_pct, abertura, minima, maxima, volume, timestamp)
        VALUES
            (:ticker, :preco, :variacao_pct, :abertura, :minima, :maxima, :volume, :timestamp)
    )"));
    q.bindValue(QStringLiteral(":ticker"),      c.ticker);
    q.bindValue(QStringLiteral(":preco"),       c.preco);
    q.bindValue(QStringLiteral(":variacao_pct"),c.variacaoPct);
    q.bindValue(QStringLiteral(":abertura"),    c.abertura);
    q.bindValue(QStringLiteral(":minima"),      c.minima);
    q.bindValue(QStringLiteral(":maxima"),      c.maxima);
    q.bindValue(QStringLiteral(":volume"),      c.volume);
    q.bindValue(QStringLiteral(":timestamp"),   c.timestamp.toString(Qt::ISODate));
    if (!q.exec()) {
        qWarning() << "PriceRepository::upsertRT:" << q.lastError().text();
        return false;
    }
    return true;
}

std::optional<CotacaoRT> PriceRepository::findRT(const QString &ticker) const
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "SELECT * FROM cotacoes_tempo_real WHERE ticker = :ticker"));
    q.bindValue(QStringLiteral(":ticker"), ticker);
    if (!q.exec()) {
        qWarning() << "PriceRepository::findRT:" << q.lastError().text();
        return std::nullopt;
    }
    if (q.next())
        return rtFromQuery(q);
    return std::nullopt;
}

bool PriceRepository::removeRT(const QString &ticker)
{
    QSqlQuery q(db());
    q.prepare(QStringLiteral(
        "DELETE FROM cotacoes_tempo_real WHERE ticker = :ticker"));
    q.bindValue(QStringLiteral(":ticker"), ticker);
    if (!q.exec()) {
        qWarning() << "PriceRepository::removeRT:" << q.lastError().text();
        return false;
    }
    return true;
}
