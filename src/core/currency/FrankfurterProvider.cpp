#include "FrankfurterProvider.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <utility>

FrankfurterProvider::FrankfurterProvider(QObject *parent, QString baseUrl)
    : QObject(parent), m_baseUrl(std::move(baseUrl))
{}

bool FrankfurterProvider::isAvailable() const
{
    return !m_cache.isEmpty();
}

QMap<QString, double> FrankfurterProvider::fetchRates(const QString &base)
{
    const QDate today = QDate::currentDate();
    if (m_cachedBase == base && m_cacheDate == today && !m_cache.isEmpty())
        return m_cache;

    m_cache     = fetchSync(base);
    m_cachedBase = base;
    m_cacheDate  = today;
    return m_cache;
}

QMap<QString, double> FrankfurterProvider::fetchSync(const QString &base)
{
    // GET https://api.frankfurter.dev/v1/latest?from=BRL (ou base injetada em teste)
    QUrl url(m_baseUrl);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("from"), base);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("orcamento-pessoal/0.1"));
    // FIN-05: sem isso, um servidor que aceita a conexao e nunca responde
    // trava fetchSync() (e a GUI thread que a chama) para sempre.
    req.setTransferTimeout(FrankfurterProvider::kTransferTimeoutMs);

    QNetworkReply *reply = m_nam.get(req);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QMap<QString, double> rates;

    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "FrankfurterProvider:" << reply->errorString();
        reply->deleteLater();
        return rates;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    const QJsonObject ratesObj = doc.object().value(QStringLiteral("rates")).toObject();
    for (auto it = ratesObj.begin(); it != ratesObj.end(); ++it)
        rates[it.key()] = it.value().toDouble();

    // Add base → 1.0 for completeness
    rates[base] = 1.0;

    return rates;
}
