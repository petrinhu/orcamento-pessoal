// PeuBud - #199 (Onda U, QA-05): providers de mercado (BrapiProvider,
// OAuthDeviceFlow) compilados no app SEM suite de teste (gate de processo:
// nao fechar #45/#58 sem teste de contrato).
//
// A conversao JSON->struct de BrapiProvider e a classificacao de resposta
// de polling de OAuthDeviceFlow viviam inline dentro de lambdas conectadas a
// QNetworkReply::finished -- intestavel sem mockar rede. Extraidas (refactor
// comportamento-preservado, mesma logica) em metodos static puros:
//   BrapiProvider::parseQuoteJson / parseOhlcvJson
//   OAuthDeviceFlow::classifyPollResponse
// Este arquivo testa esses parsers com fixture JSON (resposta real brapi.dev
// / RFC 8628 sec 3.5), sem nenhuma chamada de rede.
//
// #203a (Onda U, SEC-03): BrapiProvider::buildQuoteUrl tambem extraido para
// provar o percent-encoding do ticker no path da URL.

#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include "services/BrapiProvider.h"
#include "services/OAuthDeviceFlow.h"
#include "models/PrecoAtivo.h"

class TestMarketProvidersContract : public QObject
{
    Q_OBJECT

private slots:
    // ── BrapiProvider::parseQuoteJson ────────────────────────────────────────

    // Fixture: formato real de GET /api/quote/{ticker} (brapi.dev). Precisao
    // de centavos e o que mais importa (Prioridade #1 do projeto).
    void parseQuoteJsonConverteParaCentavosComPrecisao()
    {
        const QByteArray json = R"({
            "results": [{
                "symbol": "PETR4",
                "regularMarketPrice": 38.42,
                "regularMarketChangePercent": 1.35,
                "regularMarketOpen": 37.90,
                "regularMarketDayLow": 37.85,
                "regularMarketDayHigh": 38.50,
                "regularMarketVolume": 45123400,
                "regularMarketTime": "2025-03-14T18:00:00Z"
            }]
        })";

        const CotacaoRT rt = BrapiProvider::parseQuoteJson(QJsonDocument::fromJson(json));

        QCOMPARE(rt.ticker, QStringLiteral("PETR4"));
        QCOMPARE(rt.preco,    qint64(3842));   // R$ 38,42
        QCOMPARE(rt.abertura, qint64(3790));   // R$ 37,90
        QCOMPARE(rt.minima,   qint64(3785));
        QCOMPARE(rt.maxima,   qint64(3850));
        QCOMPARE(rt.variacaoPct, qint32(13500)); // 1,35% x 10000
        QCOMPARE(rt.volume, qint64(45123400));
    }

    void parseQuoteJsonResultsVazioDevolveTickerVazio()
    {
        const QByteArray json = R"({"results": []})";
        const CotacaoRT rt = BrapiProvider::parseQuoteJson(QJsonDocument::fromJson(json));
        QVERIFY(rt.ticker.isEmpty());
    }

    void parseQuoteJsonJsonInvalidoNaoCrasha()
    {
        const CotacaoRT rt = BrapiProvider::parseQuoteJson(QJsonDocument::fromJson(QByteArray("not json")));
        QVERIFY(rt.ticker.isEmpty());
    }

    // ── BrapiProvider::parseOhlcvJson ────────────────────────────────────────

    void parseOhlcvJsonConverteSerieHistoricaComPrecisao()
    {
        const QByteArray json = R"({
            "results": [{
                "historicalDataPrice": [
                    { "date": 1710374400, "open": 37.90, "high": 38.50, "low": 37.85, "close": 38.42, "volume": 45123400 },
                    { "date": 1710460800, "open": 38.42, "high": 38.90, "low": 38.10, "close": 38.75, "volume": 39876500 }
                ]
            }]
        })";

        const QList<PrecoAtivo> serie = BrapiProvider::parseOhlcvJson(
            QJsonDocument::fromJson(json), QStringLiteral("petr4"));

        QCOMPARE(serie.size(), 2);
        QCOMPARE(serie[0].ticker,     QStringLiteral("PETR4")); // toUpper()
        QCOMPARE(serie[0].fonte,      QStringLiteral("brapi"));
        QCOMPARE(serie[0].abertura,   qint64(3790));
        QCOMPARE(serie[0].maxima,     qint64(3850));
        QCOMPARE(serie[0].minima,     qint64(3785));
        QCOMPARE(serie[0].fechamento, qint64(3842));
        QCOMPARE(serie[0].volume,     qint64(45123400));
        QCOMPARE(serie[1].fechamento, qint64(3875));
    }

    void parseOhlcvJsonSemHistoricoDevolveListaVazia()
    {
        const QByteArray json = R"({"results": [{"historicalDataPrice": []}]})";
        const QList<PrecoAtivo> serie = BrapiProvider::parseOhlcvJson(
            QJsonDocument::fromJson(json), QStringLiteral("PETR4"));
        QVERIFY(serie.isEmpty());
    }

    // ── BrapiProvider::buildQuoteUrl (#203a/SEC-03) ──────────────────────────

    void buildQuoteUrlTickerNormalUppercaseNaMudaEstrutura()
    {
        const QUrl url = BrapiProvider::buildQuoteUrl(QStringLiteral("petr4"));
        QCOMPARE(url.toString(), QStringLiteral("https://brapi.dev/api/quote/PETR4"));
    }

    void buildQuoteUrlCaracterEspecialFicaPercentEncoded()
    {
        // "/" no ticker nao pode virar um segmento de path extra (path
        // traversal); espaco nao pode quebrar a URL.
        const QUrl url = BrapiProvider::buildQuoteUrl(QStringLiteral("PETR4/../x y"));
        const QString s = url.toString(QUrl::FullyEncoded);
        QVERIFY2(!s.contains(QStringLiteral("/../")),
                  qPrintable(QStringLiteral("URL nao deveria conter path traversal cru: ") + s));
        QVERIFY(s.startsWith(QStringLiteral("https://brapi.dev/api/quote/PETR4")));
        // decodifica de volta e confirma que o path original foi preservado
        // como UM segmento so (contagem de "/" apos o prefixo fixo == 0).
        const QString suffix = s.mid(QStringLiteral("https://brapi.dev/api/quote/").size());
        QVERIFY(!suffix.contains(QLatin1Char('/')));
    }

    // ── OAuthDeviceFlow::classifyPollResponse (RFC 8628 sec 3.5) ─────────────

    void classifyPollResponseAuthorizationPending()
    {
        const auto obj = QJsonDocument::fromJson(
            R"({"error":"authorization_pending"})").object();
        const auto res = OAuthDeviceFlow::classifyPollResponse(obj);
        QCOMPARE(res.outcome, OAuthPollOutcome::Pending);
    }

    void classifyPollResponseSlowDown()
    {
        const auto obj = QJsonDocument::fromJson(
            R"({"error":"slow_down"})").object();
        const auto res = OAuthDeviceFlow::classifyPollResponse(obj);
        QCOMPARE(res.outcome, OAuthPollOutcome::SlowDown);
    }

    void classifyPollResponseExpiredTokenETerminal()
    {
        const auto obj = QJsonDocument::fromJson(
            R"({"error":"expired_token"})").object();
        const auto res = OAuthDeviceFlow::classifyPollResponse(obj);
        QCOMPARE(res.outcome, OAuthPollOutcome::Terminal);
        QCOMPARE(res.error, QStringLiteral("expired_token"));
    }

    void classifyPollResponseAccessDeniedETerminal()
    {
        const auto obj = QJsonDocument::fromJson(
            R"({"error":"access_denied"})").object();
        const auto res = OAuthDeviceFlow::classifyPollResponse(obj);
        QCOMPARE(res.outcome, OAuthPollOutcome::Terminal);
    }

    void classifyPollResponseSucessoExtraiToken()
    {
        const auto obj = QJsonDocument::fromJson(R"({
            "access_token": "ghu_abc123",
            "token_type": "bearer",
            "refresh_token": "ghr_xyz789",
            "expires_in": 28800
        })").object();
        const auto res = OAuthDeviceFlow::classifyPollResponse(obj);
        QCOMPARE(res.outcome, OAuthPollOutcome::Success);
        QCOMPARE(res.accessToken,  QStringLiteral("ghu_abc123"));
        QCOMPARE(res.tokenType,    QStringLiteral("bearer"));
        QCOMPARE(res.refreshToken, QStringLiteral("ghr_xyz789"));
        QCOMPARE(res.expiresIn,    28800);
    }

    void classifyPollResponseErroDesconhecidoEUnknown()
    {
        const auto obj = QJsonDocument::fromJson(
            R"({"error":"algo_nunca_visto"})").object();
        const auto res = OAuthDeviceFlow::classifyPollResponse(obj);
        QCOMPARE(res.outcome, OAuthPollOutcome::Unknown);
        QCOMPARE(res.error, QStringLiteral("algo_nunca_visto"));
    }

    void classifyPollResponseTokenTypeDefaultBearer()
    {
        // token_type ausente -> default "Bearer" (RFC 8628 sec 3.5 exemplo).
        const auto obj = QJsonDocument::fromJson(
            R"({"access_token":"tok"})").object();
        const auto res = OAuthDeviceFlow::classifyPollResponse(obj);
        QCOMPARE(res.outcome, OAuthPollOutcome::Success);
        QCOMPARE(res.tokenType, QStringLiteral("Bearer"));
    }
};

QTEST_GUILESS_MAIN(TestMarketProvidersContract)
#include "test_market_providers_contract.moc"
