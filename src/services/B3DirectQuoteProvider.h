#ifndef B3DIRECTQUOTEPROVIDER_H
#define B3DIRECTQUOTEPROVIDER_H

#include "IMarketDataProvider.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QTimer>

class B3DirectQuoteProvider : public IMarketDataProvider {
    Q_OBJECT
public:
    explicit B3DirectQuoteProvider(QObject *parent = nullptr);
    ~B3DirectQuoteProvider() override;

    // IMarketDataProvider interface
    void fetchQuote(const QString &ticker, std::function<void(const CotacaoRT&)> callback) override;
    void fetchOHLCV(const QString &ticker,
                    const QDate &inicio,
                    const QDate &fim,
                    const QString &intervalo,
                    std::function<void(const QList<PrecoAtivo>&)> callback) override;
    void search(const QString &query,
                std::function<void(const QStringList&)> callback) override;
    QString name() const override;
    bool requiresAuth() const override;
    int typicalDelayMinutes() const override;

private slots:
    void onQuoteReplyFinished();
    void onOhlcvReplyFinished();
    void onSearchReplyFinished();

private:
    QNetworkAccessManager *m_networkManager;
    QMap<QNetworkReply*, std::function<void(const CotacaoRT&)>> m_quoteCallbacks;
    QMap<QNetworkReply*, std::function<void(const QList<PrecoAtivo>&)>> m_ohlcvCallbacks;
    QMap<QNetworkReply*, std::function<void(const QStringList&)>> m_searchCallbacks;
};

#endif // B3DIRECTQUOTEPROVIDER_H