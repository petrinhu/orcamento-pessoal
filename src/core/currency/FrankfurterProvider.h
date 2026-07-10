#pragma once

#include "ICurrencyProvider.h"
#include <QDateTime>
#include <QNetworkAccessManager>

// Fetches ECB exchange rates from api.frankfurter.dev (free, no API key).
// Rates are cached for the calendar day.
//
// FIN-05 (auditoria 2026-06-22 §3.1): fetchSync() rodava QEventLoop::exec()
// sem setTransferTimeout() -- se o servidor aceitasse a conexao TCP e nunca
// respondesse, a GUI thread congelava INDEFINIDAMENTE (chamado no refresh de
// ParcelasWidget). Fix: timeout explicito de 5s na QNetworkRequest (Qt
// dispara QNetworkReply::TransferTimeoutError e encerra o loop.exec() local
// mesmo aninhado, pois o QTimer roda no mesmo thread). Mover para
// assincrono/thread fica como follow-up (nao e necessario para resolver o
// travamento, que e o bug critico).
class FrankfurterProvider final : public QObject, public ICurrencyProvider {
    Q_OBJECT
public:
    // baseUrl: injetavel para teste (servidor fake local); producao usa o
    // default (api.frankfurter.dev direto -- evita depender do redirect 301
    // de api.frankfurter.app, hoje descontinuado em favor do .dev).
    explicit FrankfurterProvider(
        QObject *parent = nullptr,
        QString baseUrl = QStringLiteral("https://api.frankfurter.dev/v1/latest"));

    QMap<QString, double> fetchRates(const QString &baseCurrency = QStringLiteral("BRL")) override;
    bool isAvailable() const override;

    // Timeout de transferencia (ms) aplicado a toda requisicao de fetchSync().
    // Exposto só para prova em teste (test_frankfurter_provider.cpp).
    static constexpr int kTransferTimeoutMs = 5000;

private:
    QNetworkAccessManager  m_nam;
    QMap<QString, double>  m_cache;
    QString                m_cachedBase;
    QDate                  m_cacheDate;
    QString                m_baseUrl;

    QMap<QString, double> fetchSync(const QString &base);
};
