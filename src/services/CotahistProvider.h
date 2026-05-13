#ifndef COTAHISTPROVIDER_H
#define COTAHISTPROVIDER_H

#include "IMarketDataProvider.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QList>
#include <functional>
#include "models/PrecoAtivo.h"
#include "repositories/PriceRepository.h"

class CotahistProvider : public IMarketDataProvider {
    Q_OBJECT
public:
    explicit CotahistProvider(QObject *parent = nullptr);
    ~CotahistProvider() override;

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

public slots:
    void downloadHistoricalData(int years = 5);

private slots:
    void onDownloadFinished();
    void onHistoricalDataReady();
    void onError(QNetworkReply::NetworkError code);

private:
    void downloadAndExtractZip(const QUrl &url, const QString &destPath);
    void processCotahistFile(const QString &filePath, 
                            std::function<void(const QList<PrecoAtivo>&)> callback);
    PrecoAtivo parseCotahistLine(const QString &line);
    bool isValidRecord(const QString &line) const;

    QNetworkAccessManager *m_networkManager;
    QString m_downloadPath;
    QMap<QString, QNetworkReply*> m_activeDownloads;
    PriceRepository m_repo;
};

#endif // COTAHISTPROVIDER_H
