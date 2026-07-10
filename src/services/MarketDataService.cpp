#include "MarketDataService.h"
#include <QDebug>

MarketDataService::MarketDataService(QObject *parent)
    : QObject(parent)
{
}

MarketDataService::~MarketDataService()
{
    // Clean up providers
    qDeleteAll(m_providers);
    m_providers.clear();
}

void MarketDataService::registerProvider(const QString &name, IMarketDataProvider *provider)
{
    if (provider) {
        m_providers[name] = provider;
        // If this is the first provider, set it as active
        if (!m_activeProvider) {
            m_activeProvider = provider;
            emit activeProviderChanged(name);
        }
    }
}

bool MarketDataService::setActiveProvider(const QString &name)
{
    if (m_providers.contains(name)) {
        m_activeProvider = m_providers[name];
        emit activeProviderChanged(name);
        return true;
    }
    return false;
}

IMarketDataProvider* MarketDataService::activeProvider() const
{
    return m_activeProvider;
}

QString MarketDataService::activeProviderName() const
{
    if (!m_activeProvider) return QString();
    return m_providers.key(m_activeProvider, QString());
}

IMarketDataProvider* MarketDataService::provider(const QString &name) const
{
    return m_providers.value(name, nullptr);
}

QStringList MarketDataService::providerNames() const
{
    return m_providers.keys();
}

void MarketDataService::fetchQuote(const QString &ticker, std::function<void(const CotacaoRT&)> callback)
{
    if (m_activeProvider) {
        m_activeProvider->fetchQuote(ticker, callback);
    } else {
        // No active provider - call callback with empty result
        CotacaoRT empty;
        callback(empty);
    }
}

void MarketDataService::fetchOHLCV(const QString &ticker,
                                   const QDate &inicio,
                                   const QDate &fim,
                                   const QString &intervalo,
                                   std::function<void(const QList<PrecoAtivo>&)> callback)
{
    if (m_activeProvider) {
        m_activeProvider->fetchOHLCV(ticker, inicio, fim, intervalo, callback);
    } else {
        // No active provider - call callback with empty result
        QList<PrecoAtivo> empty;
        callback(empty);
    }
}

void MarketDataService::search(const QString &query,
                               std::function<void(const QStringList&)> callback)
{
    if (m_activeProvider) {
        m_activeProvider->search(query, callback);
    } else {
        // No active provider - call callback with empty result
        QStringList empty;
        callback(empty);
    }
}