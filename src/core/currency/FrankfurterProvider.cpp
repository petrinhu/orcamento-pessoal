#include "FrankfurterProvider.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

FrankfurterProvider::FrankfurterProvider(QObject *parent)
    : QObject(parent)
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
    // GET https://api.frankfurter.app/latest?from=BRL
    QUrl url(QStringLiteral("https://api.frankfurter.app/latest"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("from"), base);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("orcamento-pessoal/0.1"));

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
