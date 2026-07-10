#ifndef MARKETDATASERVICE_H
#define MARKETDATASERVICE_H

#include <QObject>
#include <QMap>
#include <QString>
#include <functional>
#include "IMarketDataProvider.h"
#include "models/PrecoAtivo.h" // declara CotacaoRT tambem

class MarketDataService : public QObject {
    Q_OBJECT
public:
    explicit MarketDataService(QObject *parent = nullptr);
    ~MarketDataService() override;

    // Register a provider with a name
    void registerProvider(const QString &name, IMarketDataProvider *provider);
    
    // Set the active provider by name
    bool setActiveProvider(const QString &name);
    
    // Get the currently active provider
    IMarketDataProvider* activeProvider() const;

    // UX-02: nome (chave) registrado do provider REALMENTE ativo agora —
    // não confundir com o que foi pedido em setActiveProvider(); se o pedido
    // falhar (nome não registrado), o ativo real continua sendo o anterior.
    // String vazia se nenhum provider está ativo.
    QString activeProviderName() const;

    // Get provider by name
    IMarketDataProvider* provider(const QString &name) const;
    
    // List all registered provider names
    QStringList providerNames() const;
    
    // Convenience methods that delegate to active provider
    void fetchQuote(const QString &ticker, std::function<void(const CotacaoRT&)> callback);
    void fetchOHLCV(const QString &ticker,
                    const QDate &inicio,
                    const QDate &fim,
                    const QString &intervalo,
                    std::function<void(const QList<PrecoAtivo>&)> callback);
    void search(const QString &query,
                std::function<void(const QStringList&)> callback);

signals:
    void activeProviderChanged(const QString &name);

private:
    QMap<QString, IMarketDataProvider*> m_providers;
    IMarketDataProvider *m_activeProvider = nullptr;
};

#endif // MARKETDATASERVICE_H