#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

// #199 (QA-05): classificacao pura (sem rede/timer) da resposta de polling
// RFC 8628 sec 3.5 -- extraida de OAuthDeviceFlow::onPollReply para permitir
// teste de contrato com fixture JSON (authorization_pending/slow_down/
// expired_token/access_denied/sucesso), sem precisar de mock de rede.
enum class OAuthPollOutcome {
    Pending,        // authorization_pending -- continua polling
    SlowDown,       // slow_down -- aumenta intervalo em 5s
    Success,        // access_token presente
    Terminal,       // expired_token | access_denied -- aborta
    Unknown,        // resposta nao reconhecida -- aborta com o `error` cru
    NotJsonObject,  // corpo nao era objeto JSON valido
};

struct OAuthPollResult {
    OAuthPollOutcome outcome = OAuthPollOutcome::NotJsonObject;
    QString accessToken;
    QString tokenType;
    QString refreshToken;
    int     expiresIn = 0;
    QString error;     // codigo RFC 8628 cru (vazio se nao aplicavel)
};

// OAuthDeviceFlow (#83) - implementacao do RFC 8628 (Device Authorization Grant).
//
// Fluxo:
//   1. POST {deviceCodeUrl} body=client_id&scope  -> device_code, user_code,
//      verification_uri, expires_in, interval
//   2. UI mostra ao usuario o user_code + URL para autorizar no browser
//   3. Polling {accessTokenUrl} com device_code ate receber access_token
//      Trata responses: authorization_pending (continua), slow_down (+5s no
//      intervalo), expired_token (aborta), access_denied (aborta).
//
// Providers testados:
//   - GitHub Copilot: client_id "Iv1.b507a08c87ecfe98" (publico)
//     deviceCodeUrl: https://github.com/login/device/code
//     accessTokenUrl: https://github.com/login/oauth/access_token
//
// Template extraido de openclaw/extensions/github-copilot/login.ts (RFC 8628).
//
// Token NAO eh persistido por esta classe - quem chama deve salvar via
// SecretStoreService (libsecret -> KWallet/GNOME Keyring) apos receber o
// signal accessTokenReceived.
struct OAuthDeviceFlowConfig {
    QString clientId;
    QString scope;
    QUrl    deviceCodeUrl;
    QUrl    accessTokenUrl;
    // Header customizado para GitHub (Accept: application/json) e similares.
    bool    requireAcceptJson = true;
};

class OAuthDeviceFlow : public QObject
{
    Q_OBJECT
public:
    explicit OAuthDeviceFlow(QObject *parent = nullptr);
    ~OAuthDeviceFlow() override;

    // Inicia fluxo. Emite userCodeReceived em sucesso, flowError em falha.
    // Apos userCodeReceived, comeca polling automaticamente.
    void startFlow(const OAuthDeviceFlowConfig &cfg);

    // Aborta fluxo em andamento (cancela polling).
    void cancel();

    // #199 (QA-05): parser puro do corpo de resposta do polling (RFC 8628
    // sec 3.5). static + public para teste de contrato com fixture JSON.
    static OAuthPollResult classifyPollResponse(const QJsonObject &obj);

signals:
    // Emitido apos step 1 OK: UI deve mostrar userCode e abrir verificationUri.
    void userCodeReceived(const QString &userCode,
                          const QUrl    &verificationUri,
                          int            expiresInSeconds);

    // Emitido quando polling retorna token valido.
    void accessTokenReceived(const QString &accessToken,
                             const QString &tokenType,
                             const QString &refreshToken,
                             int            expiresInSeconds);

    // Emitido em qualquer falha terminal (expired, denied, network, parse).
    void flowError(const QString &reason);

private slots:
    void onDeviceCodeReply();
    void onPollTick();
    void onPollReply();

private:
    QNetworkAccessManager *m_nam      = nullptr;
    QTimer                *m_pollTimer = nullptr;
    OAuthDeviceFlowConfig  m_cfg;
    QString                m_deviceCode;
    int                    m_intervalMs = 5000;
    int                    m_elapsedMs  = 0;
    int                    m_expiresMs  = 0;
};
