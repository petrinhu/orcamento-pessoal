// PeuBud - GlossarioService performance TDD (#194, Onda U PERF-01)
//
// Antes do fix: primeiroTermoEncontrado() ordenava as 92 chaves e compilava
// uma QRegularExpression NOVA por termo, TODA chamada (O(termos) regex
// compilations por chamada). Medido na auditoria: ~365ms so de glossario no
// buildUi (55% do startup), 1200-1700x mais lento que uma alternation-regex
// static+optimize().
//
// Este teste crava o orcamento: 1000 chamadas sem match (pior caso -- percorre
// a regex inteira sem casar) devem rodar em bem menos de 5ms no total.

#include <QtTest/QtTest>
#include <QElapsedTimer>
#include "services/GlossarioService.h"

class TestGlossarioPerf : public QObject
{
    Q_OBJECT
private slots:
    // Aquece qualquer inicializacao static (lazy-init da regex/tabela) antes
    // de medir, para o teste refletir custo de REGIME ESTAVEL (produção faz
    // milhares de chamadas na mesma sessão, não uma isolada).
    void milChamadasSemMatchAbaixoDe5ms();
};

void TestGlossarioPerf::milChamadasSemMatchAbaixoDe5ms()
{
    const QString semMatch = QStringLiteral(
        "Texto generico sem nenhuma sigla financeira cadastrada no glossario");

    // Aquecimento: garante que a regex static ja foi construida+optimize()
    // antes do cronometro comecar (o custo de 1x construcao nao é o que
    // queremos medir -- é o custo por-chamada em regime estavel).
    const QString aquecimento = GlossarioService::primeiroTermoEncontrado(semMatch);
    QVERIFY(aquecimento.isEmpty());

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < 1000; ++i) {
        const QString resultado = GlossarioService::primeiroTermoEncontrado(semMatch);
        QVERIFY(resultado.isEmpty());
    }
    const qint64 elapsedNs = timer.nsecsElapsed();
    const double elapsedMs = elapsedNs / 1'000'000.0;

    QVERIFY2(elapsedMs < 5.0,
        qPrintable(QStringLiteral("1000 chamadas sem match levaram %1ms (orcamento: <5ms)")
                       .arg(elapsedMs, 0, 'f', 3)));
}

QTEST_MAIN(TestGlossarioPerf)
#include "test_glossario_perf.moc"
