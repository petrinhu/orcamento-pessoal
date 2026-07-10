#include "B3DirectQuoteProvider.h"
#include <QJsonArray>
#include <QDebug>
#include <QUrl>

B3DirectQuoteProvider::B3DirectQuoteProvider(QObject *parent)
    : IMarketDataProvider(parent),
      m_networkManager(new QNetworkAccessManager(this))
{
}

B3DirectQuoteProvider::~B3DirectQuoteProvider()
{
}

void B3DirectQuoteProvider::fetchQuote(const QString &ticker, std::function<void(const CotacaoRT&)> callback)
{
    // (#203a/SEC-03) ticker percent-encoded — ver comentário gêmeo em
    // BrapiProvider::fetchQuote.
    const QString encoded = QString::fromUtf8(QUrl::toPercentEncoding(ticker.toUpper()));
    QString urlStr = QString("https://cotacao.b3.com.br/mds/api/v1/instrumentQuotation/%1").arg(encoded);
    QUrl url(urlStr);
    
    QNetworkReply *reply = m_networkManager->get(QNetworkRequest(url));
    m_quoteCallbacks[reply] = callback;
    
    connect(reply, &QNetworkReply::finished, this, &B3DirectQuoteProvider::onQuoteReplyFinished);
}

void B3DirectQuoteProvider::fetchOHLCV(const QString &ticker,
                                       const QDate &inicio,
                                       const QDate &fim,
                                       const QString &intervalo,
                                       std::function<void(const QList<PrecoAtivo>&)> callback)
{
    // For now, we'll delegate to historical data (this would be implemented with COTAHIST or other providers)
    // For B3DirectQuoteProvider, we focus on real-time quotes
    QList<PrecoAtivo> emptyList;
    callback(emptyList);
}

void B3DirectQuoteProvider::search(const QString &query,
                                   std::function<void(const QStringList&)> callback)
{
    // B3 Direct doesn't provide search functionality via this endpoint
    // We could implement a basic search or delegate to another provider
    QStringList emptyList;
    callback(emptyList);
}

QString B3DirectQuoteProvider::name() const
{
    return "B3 Direto (tempo real)";
}

bool B3DirectQuoteProvider::requiresAuth() const
{
    return false;
}

int B3DirectQuoteProvider::typicalDelayMinutes() const
{
    return 0; // Real-time during market hours
}

void B3DirectQuoteProvider::onQuoteReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    auto it = m_quoteCallbacks.find(reply);
    if (it == m_quoteCallbacks.end()) {
        reply->deleteLater();
        return;
    }
    
    std::function<void(const CotacaoRT&)> callback = it.value();
    m_quoteCallbacks.erase(it);
    
    CotacaoRT result;
    result.ticker = "UNKNOWN";
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            
            // Extract data from B3 API response
            result.ticker = obj.value("ticker").toString().toUpper();
            result.preco = obj.value("curPrc").toVariant().toLongLong() * 100; // Convert to cents
            result.variacaoPct = obj.value("varPct").toVariant().toInt() * 100; // Convert to basis points
            result.abertura = obj.value("opnPrc").toVariant().toLongLong() * 100;
            result.minima = obj.value("minPrc").toVariant().toLongLong() * 100;
            result.maxima = obj.value("maxPrc").toVariant().toLongLong() * 100;
            result.volume = obj.value("totVol").toVariant().toLongLong();
            result.timestamp = QDateTime::currentDateTimeUtc();
        }
    } else {
        qDebug() << "Error fetching quote:" << reply->errorString();
    }
    
    callback(result);
    reply->deleteLater();
}

void B3DirectQuoteProvider::onOhlcvReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    auto it = m_ohlcvCallbacks.find(reply);
    if (it == m_ohlcvCallbacks.end()) {
        reply->deleteLater();
        return;
    }
    
    std::function<void(const QList<PrecoAtivo>&)> callback = it.value();
    m_ohlcvCallbacks.erase(it);
    
    QList<PrecoAtivo> result;
    callback(result);
    reply->deleteLater();
}

void B3DirectQuoteProvider::onSearchReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    auto it = m_searchCallbacks.find(reply);
    if (it == m_searchCallbacks.end()) {
        reply->deleteLater();
        return;
    }
    
    std::function<void(const QStringList&)> callback = it.value();
    m_searchCallbacks.erase(it);
    
    QStringList result;
    callback(result);
    reply->deleteLater();
}