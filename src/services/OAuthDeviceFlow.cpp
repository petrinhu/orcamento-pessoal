#include "OAuthDeviceFlow.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>

OAuthDeviceFlow::OAuthDeviceFlow(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_pollTimer(new QTimer(this))
{
    m_pollTimer->setSingleShot(false);
    connect(m_pollTimer, &QTimer::timeout, this, &OAuthDeviceFlow::onPollTick);
}

OAuthDeviceFlow::~OAuthDeviceFlow() = default;

void OAuthDeviceFlow::cancel()
{
    m_pollTimer->stop();
    m_deviceCode.clear();
}

void OAuthDeviceFlow::startFlow(const OAuthDeviceFlowConfig &cfg)
{
    cancel();
    m_cfg        = cfg;
    m_elapsedMs  = 0;
    m_intervalMs = 5000;
    m_expiresMs  = 0;

    QNetworkRequest req(cfg.deviceCodeUrl);
    if (cfg.requireAcceptJson) {
        req.setRawHeader("Accept", "application/json");
    }
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    QUrlQuery body;
    body.addQueryItem(QStringLiteral("client_id"), cfg.clientId);
    if (!cfg.scope.isEmpty()) {
        body.addQueryItem(QStringLiteral("scope"), cfg.scope);
    }

    QNetworkReply *reply = m_nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &OAuthDeviceFlow::onDeviceCodeReply);
}

void OAuthDeviceFlow::onDeviceCodeReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit flowError(tr("Erro de rede no device-code: %1").arg(reply->errorString()));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit flowError(tr("Resposta device-code nao eh JSON objeto"));
        return;
    }
    const QJsonObject obj = doc.object();

    m_deviceCode = obj.value(QStringLiteral("device_code")).toString();
    if (m_deviceCode.isEmpty()) {
        const QString err = obj.value(QStringLiteral("error_description")).toString(
            obj.value(QStringLiteral("error")).toString(QStringLiteral("device_code ausente")));
        emit flowError(err);
        return;
    }

    const QString userCode    = obj.value(QStringLiteral("user_code")).toString();
    const QUrl    verifyUri   = QUrl(obj.value(QStringLiteral("verification_uri")).toString());
    const int     expiresIn   = obj.value(QStringLiteral("expires_in")).toInt(900);
    const int     intervalSec = obj.value(QStringLiteral("interval")).toInt(5);

    m_intervalMs = qMax(1000, intervalSec * 1000);
    m_expiresMs  = expiresIn * 1000;
    m_elapsedMs  = 0;

    emit userCodeReceived(userCode, verifyUri, expiresIn);
    m_pollTimer->start(m_intervalMs);
}

void OAuthDeviceFlow::onPollTick()
{
    m_elapsedMs += m_intervalMs;
    if (m_elapsedMs > m_expiresMs) {
        cancel();
        emit flowError(tr("Codigo expirado antes da autorizacao"));
        return;
    }

    QNetworkRequest req(m_cfg.accessTokenUrl);
    if (m_cfg.requireAcceptJson) {
        req.setRawHeader("Accept", "application/json");
    }
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QStringLiteral("application/x-www-form-urlencoded"));

    QUrlQuery body;
    body.addQueryItem(QStringLiteral("client_id"), m_cfg.clientId);
    body.addQueryItem(QStringLiteral("device_code"), m_deviceCode);
    body.addQueryItem(QStringLiteral("grant_type"),
                      QStringLiteral("urn:ietf:params:oauth:grant-type:device_code"));

    QNetworkReply *reply = m_nam->post(req, body.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, &OAuthDeviceFlow::onPollReply);
}

void OAuthDeviceFlow::onPollReply()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError
        && reply->error() != QNetworkReply::ContentAccessDenied
        && reply->error() != QNetworkReply::ContentOperationNotPermittedError) {
        // Erro de rede transitorio - apenas log, polling continua
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) return;
    const QJsonObject obj = doc.object();

    // Sucesso?
    const QString accessToken = obj.value(QStringLiteral("access_token")).toString();
    if (!accessToken.isEmpty()) {
        cancel();
        emit accessTokenReceived(
            accessToken,
            obj.value(QStringLiteral("token_type")).toString(QStringLiteral("Bearer")),
            obj.value(QStringLiteral("refresh_token")).toString(),
            obj.value(QStringLiteral("expires_in")).toInt(0));
        return;
    }

    // Erro? RFC 8628 codes:
    const QString err = obj.value(QStringLiteral("error")).toString();
    if (err == QStringLiteral("authorization_pending")) {
        return; // continuar polling no proximo tick
    }
    if (err == QStringLiteral("slow_down")) {
        // Aumenta intervalo em 5s (per RFC 8628 sec 3.5)
        m_intervalMs += 5000;
        m_pollTimer->setInterval(m_intervalMs);
        return;
    }
    if (err == QStringLiteral("expired_token")
        || err == QStringLiteral("access_denied")) {
        cancel();
        emit flowError(err);
        return;
    }
    // Erro desconhecido: cancela
    cancel();
    emit flowError(err.isEmpty() ? tr("Resposta inesperada do servidor") : err);
}
