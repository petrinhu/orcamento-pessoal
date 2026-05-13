#ifndef IMARKETDATAPROVIDER_H
#define IMARKETDATAPROVIDER_H

#include <QObject>
#include <QString>
#include <QDate>
#include <QList>
#include <functional>
#include "models/PrecoAtivo.h"
#include "models/CotacaoRT.h"

// Interface para provedores de dados de mercado (cotações, OHLCV, busca, etc.)
class IMarketDataProvider : public QObject {
    Q_OBJECT
public:
    explicit IMarketDataProvider(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IMarketDataProvider() = default;

    // Busca cotação em tempo real de um ticker
    virtual void fetchQuote(const QString &ticker, std::function<void(const CotacaoRT&)> callback) = 0;

    // Busca série histórica OHLCV para um ticker no intervalo [inicio, fim]
    virtual void fetchOHLCV(const QString &ticker,
                            const QDate &inicio,
                            const QDate &fim,
                            const QString &intervalo, // ex: "1d", "1sem", "1mês"
                            std::function<void(const QList<PrecoAtivo>&)> callback) = 0;

    // Busca ativos por nome/ticker (autocomplete)
    virtual void search(const QString &query,
                        std::function<void(const QStringList&)> callback) = 0;

    // Retorna o nome do provedor (para exibição na UI)
    virtual QString name() const = 0;

    // Retorna se o provedor requer autenticação (para configuração)
    virtual bool requiresAuth() const { return false; }

    // Retorna delay típico em minutos (para aviso ao usuário)
    virtual int typicalDelayMinutes() const { return 0; }
};

#endif // IMARKETDATAPROVIDER_H