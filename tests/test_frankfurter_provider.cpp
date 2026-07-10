// PeuBud - FrankfurterProvider TDD (FIN-05, auditoria 2026-06-22 §3.1).
//
// fetchSync() rodava `QEventLoop loop; loop.exec();` SEM
// QNetworkRequest::setTransferTimeout() -- se o servidor aceita a conexao
// TCP mas nunca responde, a GUI thread que chama fetchRates() (refresh de
// ParcelasWidget) congela INDEFINIDAMENTE. Este teste sobe um QTcpServer
// local fake que aceita a conexao e propositalmente NUNCA escreve nada de
// volta, replicando o cenario de trava.
//
// Prova: com o timeout de FrankfurterProvider::kTransferTimeoutMs (5000ms)
// setado na QNetworkRequest, fetchRates() retorna (mapa vazio, erro de
// rede) num tempo LIMITADO -- nao trava para sempre. Sem o fix, este teste
// travaria o processo; por isso a suite registra TIMEOUT de ctest (ver
// tests/CMakeLists.txt) como watchdog de seguranca contra regressao futura
// (se alguem remover o setTransferTimeout, o teste falha por timeout do
// ctest em vez de travar a maquina de CI para sempre).

#include <QtTest/QtTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QElapsedTimer>

#include "core/currency/FrankfurterProvider.h"

class TestFrankfurterProvider : public QObject
{
    Q_OBJECT
private slots:
    void transferTimeoutConstanteEhRazoavel();
    void fetchSyncNaoTravaComServidorQueNuncaResponde();
};

// Estrutural: a constante que a auditoria pede (setTransferTimeout(5000))
// esta de fato definida e visivel/testavel (nao um magic number solto no
// .cpp) -- se alguem zerar/remover o valor, este teste acusa.
void TestFrankfurterProvider::transferTimeoutConstanteEhRazoavel()
{
    QCOMPARE(FrankfurterProvider::kTransferTimeoutMs, 5000);
    QVERIFY2(FrankfurterProvider::kTransferTimeoutMs > 0,
        "timeout precisa ser positivo -- 0/infinito e o bug original");
    QVERIFY2(FrankfurterProvider::kTransferTimeoutMs <= 10000,
        "timeout longo demais ainda deixa a GUI perceptivelmente travada");
}

// Comportamental: servidor fake aceita a conexao e nunca escreve nada.
// Antes do fix isso travava fetchSync() (e o teste) para sempre; com o
// fix, retorna dentro do timeout configurado + margem de rede.
void TestFrankfurterProvider::fetchSyncNaoTravaComServidorQueNuncaResponde()
{
    QTcpServer server;
    QVERIFY2(server.listen(QHostAddress::LocalHost),
        "nao conseguiu abrir porta local de teste");

    // Aceita a conexao mas NUNCA responde (simula servidor pendurado).
    QList<QTcpSocket *> conexoesAbertas;
    connect(&server, &QTcpServer::newConnection, &server, [&]() {
        while (QTcpSocket *sock = server.nextPendingConnection())
            conexoesAbertas.append(sock); // mantido vivo, nunca respondido
    });

    const QString baseUrl = QStringLiteral("http://127.0.0.1:%1/latest")
                                 .arg(server.serverPort());
    FrankfurterProvider provider(nullptr, baseUrl);

    QElapsedTimer cronometro;
    cronometro.start();

    const QMap<QString, double> rates = provider.fetchRates(QStringLiteral("BRL"));

    const qint64 decorridoMs = cronometro.elapsed();

    // Servidor nunca respondeu -> erro de rede -> mapa vazio (nao inventa taxa).
    QVERIFY2(rates.isEmpty(),
        "servidor nunca respondeu; fetchRates deveria devolver vazio, nao dado inventado");

    // Prova central do FIN-05: NAO travou indefinidamente. Damos margem
    // (timeout configurado + ~5s de folga para overhead de rede/CI) --
    // o que importa e' ser um numero FINITO e proximo do configurado, nao
    // um hang eterno (que so o watchdog de ctest interromperia).
    QVERIFY2(decorridoMs < FrankfurterProvider::kTransferTimeoutMs + 5000,
        qPrintable(QStringLiteral(
            "fetchSync demorou %1ms -- deveria abortar perto do timeout "
            "configurado (%2ms), nao travar indefinidamente")
            .arg(decorridoMs)
            .arg(FrankfurterProvider::kTransferTimeoutMs)));

    qDeleteAll(conexoesAbertas);
}

QTEST_GUILESS_MAIN(TestFrankfurterProvider)
#include "test_frankfurter_provider.moc"
