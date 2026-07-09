#pragma once

#include "IMarketDataProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>

// BrapiProvider (#58) - impl de IMarketDataProvider usando brapi.dev API.
//
// Endpoints cobertos:
//   - fetchQuote: GET /api/quote/{ticker}
//   - fetchOHLCV: GET /api/quote/{ticker}?range=...&interval=...
//   - search:     GET /api/quote/list?search=...
//
// Auth: Bearer token via AppSettings::brapiToken (preenchido em
// Preferencias -> Mercado, ou env BRAPI_TOKEN como fallback).
//
// Planos (limites do user importam):
//   Free: 15k req/mes, sandbox PETR4/MGLU3/VALE3/ITUB4
//   Startup: 150k, ativos hist 1y
//   Pro: 500k, ativos hist 10+y, dividendos completos
//
// Referencia completa: [[Resources/APIs/Brapi/brapi-api-reference]]
class BrapiProvider final : public IMarketDataProvider
{
    Q_OBJECT
public:
    explicit BrapiProvider(QObject *parent = nullptr);

    void fetchQuote(const QString &ticker,
                    std::function<void(const CotacaoRT&)> callback) override;

    void fetchOHLCV(const QString &ticker,
                    const QDate &inicio,
                    const QDate &fim,
                    const QString &intervalo,
                    std::function<void(const QList<PrecoAtivo>&)> callback) override;

    void search(const QString &query,
                std::function<void(const QStringList&)> callback) override;

    [[nodiscard]] QString name() const override { return QStringLiteral("Brapi"); }
    [[nodiscard]] bool    requiresAuth() const override { return true; }
    [[nodiscard]] int     typicalDelayMinutes() const override { return 5; }

    // Testa conectividade + autorizacao. Chama callback com (ok, mensagem).
    // Usa GET /api/quote/PETR4 (sandbox-friendly).
    void testarConexao(std::function<void(bool, const QString&)> callback);

    // #199 (QA-05): parsers JSON puros (sem rede), extraidos das lambdas de
    // fetchQuote/fetchOHLCV para permitir teste de contrato com fixture
    // (resposta real da brapi.dev), sem precisar mockar QNetworkAccessManager.
    // static + public: mesma logica de conversao usada em producao.
    static CotacaoRT parseQuoteJson(const QJsonDocument &doc);
    static QList<PrecoAtivo> parseOhlcvJson(const QJsonDocument &doc, const QString &ticker);

private:
    QString token() const;
    static QString apiBase() { return QStringLiteral("https://brapi.dev/api"); }

    QNetworkAccessManager *m_net = nullptr;
};
