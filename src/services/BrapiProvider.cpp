#include "BrapiProvider.h"
#include "core/settings/AppSettings.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <cmath>

BrapiProvider::BrapiProvider(QObject *parent)
    : IMarketDataProvider(parent)
    , m_net(new QNetworkAccessManager(this))
{}

QString BrapiProvider::token() const
{
    QString t = AppSettings::instance().brapiToken();
    if (t.isEmpty()) t = QString::fromUtf8(qgetenv("BRAPI_TOKEN"));
    return t;
}

static QNetworkRequest makeRequest(const QUrl &url, const QString &bearer)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/json"));
    req.setRawHeader("User-Agent", "PeuBud/0.1.0");
    if (!bearer.isEmpty()) {
        req.setRawHeader("Authorization",
                          QByteArray("Bearer ") + bearer.toUtf8());
    }
    return req;
}

CotacaoRT BrapiProvider::parseQuoteJson(const QJsonDocument &doc)
{
    CotacaoRT rt;
    rt.ticker = QString();
    const auto results = doc.object().value(QStringLiteral("results")).toArray();
    if (!results.isEmpty()) {
        const auto r = results.first().toObject();
        rt.ticker = r.value(QStringLiteral("symbol")).toString();
        rt.preco       = static_cast<qint64>(std::round(
            r.value(QStringLiteral("regularMarketPrice")).toDouble() * 100.0));
        rt.variacaoPct = static_cast<qint32>(std::round(
            r.value(QStringLiteral("regularMarketChangePercent")).toDouble() * 10000.0));
        rt.abertura    = static_cast<qint64>(std::round(
            r.value(QStringLiteral("regularMarketOpen")).toDouble() * 100.0));
        rt.minima      = static_cast<qint64>(std::round(
            r.value(QStringLiteral("regularMarketDayLow")).toDouble() * 100.0));
        rt.maxima      = static_cast<qint64>(std::round(
            r.value(QStringLiteral("regularMarketDayHigh")).toDouble() * 100.0));
        rt.volume = static_cast<qint64>(
            r.value(QStringLiteral("regularMarketVolume")).toDouble());
        rt.timestamp = QDateTime::fromString(
            r.value(QStringLiteral("regularMarketTime")).toString(), Qt::ISODate);
    }
    return rt;
}

void BrapiProvider::fetchQuote(const QString &ticker,
                                std::function<void(const CotacaoRT&)> callback)
{
    QUrl url(apiBase() + QStringLiteral("/quote/") + ticker.toUpper());
    auto *reply = m_net->get(makeRequest(url, token()));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        CotacaoRT rt;
        rt.ticker = QString();
        if (reply->error() == QNetworkReply::NoError) {
            rt = parseQuoteJson(QJsonDocument::fromJson(reply->readAll()));
        }
        reply->deleteLater();
        callback(rt);
    });
}

QList<PrecoAtivo> BrapiProvider::parseOhlcvJson(const QJsonDocument &doc, const QString &ticker)
{
    QList<PrecoAtivo> list;
    const auto results = doc.object().value(QStringLiteral("results")).toArray();
    if (!results.isEmpty()) {
        const auto hist = results.first().toObject()
                            .value(QStringLiteral("historicalDataPrice")).toArray();
        for (const auto &v : hist) {
            const auto o = v.toObject();
            PrecoAtivo p;
            p.ticker  = ticker.toUpper();
            p.fonte   = QStringLiteral("brapi");
            const qint64 ts = static_cast<qint64>(
                o.value(QStringLiteral("date")).toDouble());
            p.data    = QDate::fromString(
                QDateTime::fromSecsSinceEpoch(ts).toString(QStringLiteral("yyyy-MM-dd")),
                QStringLiteral("yyyy-MM-dd"));
            p.abertura = static_cast<qint64>(std::round(
                o.value(QStringLiteral("open")).toDouble() * 100.0));
            p.maxima   = static_cast<qint64>(std::round(
                o.value(QStringLiteral("high")).toDouble() * 100.0));
            p.minima   = static_cast<qint64>(std::round(
                o.value(QStringLiteral("low")).toDouble() * 100.0));
            p.fechamento = static_cast<qint64>(std::round(
                o.value(QStringLiteral("close")).toDouble() * 100.0));
            p.volume = static_cast<qint64>(
                o.value(QStringLiteral("volume")).toDouble());
            list.append(p);
        }
    }
    return list;
}

void BrapiProvider::fetchOHLCV(const QString &ticker,
                                const QDate &inicio,
                                const QDate &fim,
                                const QString &intervalo,
                                std::function<void(const QList<PrecoAtivo>&)> callback)
{
    QUrl url(apiBase() + QStringLiteral("/quote/") + ticker.toUpper());
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("interval"), intervalo);
    if (inicio.isValid())
        q.addQueryItem(QStringLiteral("startDate"), inicio.toString(Qt::ISODate));
    if (fim.isValid())
        q.addQueryItem(QStringLiteral("endDate"), fim.toString(Qt::ISODate));
    url.setQuery(q);

    auto *reply = m_net->get(makeRequest(url, token()));
    connect(reply, &QNetworkReply::finished, this, [reply, ticker, callback]() {
        QList<PrecoAtivo> list;
        if (reply->error() == QNetworkReply::NoError) {
            list = parseOhlcvJson(QJsonDocument::fromJson(reply->readAll()), ticker);
        }
        reply->deleteLater();
        callback(list);
    });
}

void BrapiProvider::search(const QString &query,
                            std::function<void(const QStringList&)> callback)
{
    QUrl url(apiBase() + QStringLiteral("/quote/list"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("search"), query);
    q.addQueryItem(QStringLiteral("limit"), QStringLiteral("20"));
    url.setQuery(q);

    auto *reply = m_net->get(makeRequest(url, token()));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        QStringList tickers;
        if (reply->error() == QNetworkReply::NoError) {
            const auto doc = QJsonDocument::fromJson(reply->readAll());
            const auto stocks = doc.object().value(QStringLiteral("stocks")).toArray();
            for (const auto &v : stocks) {
                tickers.append(v.toObject().value(QStringLiteral("stock")).toString());
            }
        }
        reply->deleteLater();
        callback(tickers);
    });
}

void BrapiProvider::testarConexao(std::function<void(bool, const QString&)> callback)
{
    QUrl url(apiBase() + QStringLiteral("/quote/PETR4"));
    auto *reply = m_net->get(makeRequest(url, token()));
    connect(reply, &QNetworkReply::finished, this, [reply, callback]() {
        const int httpCode = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QNetworkReply::NetworkError err = reply->error();
        QString msg;
        bool ok = false;
        if (err == QNetworkReply::NoError && httpCode == 200) {
            ok  = true;
            msg = QStringLiteral("Conexao OK. Token valido.");
        } else if (httpCode == 401) {
            msg = QStringLiteral("Token invalido (HTTP 401).");
        } else if (httpCode == 403) {
            msg = QStringLiteral("Plano nao autorizado para este recurso (HTTP 403).");
        } else if (httpCode == 429) {
            msg = QStringLiteral("Limite de requisicoes excedido (HTTP 429).");
        } else {
            msg = QStringLiteral("Falha: %1 (HTTP %2)")
                    .arg(reply->errorString()).arg(httpCode);
        }
        reply->deleteLater();
        callback(ok, msg);
    });
}
