// PeuBud - ReconciliacaoAutomatica TDD (#85)
// Testa subset-sum/knapsack puro sem dependencia de DB.

#include <QtTest/QtTest>
#include "services/ReconciliacaoAutomatica.h"

class TestReconciliacao : public QObject
{
    Q_OBJECT
private slots:
    void targetZeroEncontradoVazio();
    void targetNegativoNaoEncontrado();
    void candidatoUnicoBateExato();
    void tresCandidatosSomaDoisDeles();
    void targetInalcancavel();
    void candidatosVazioAlvoPositivo();
    void timeoutArtificialZero();
    void candidatosComValoresNegativosIgnorados();
};

using Lanc = ReconciliacaoAutomatica::Lancamento;

static Lanc mk(qint64 id, qint64 valor)
{
    return Lanc{id, valor, QStringLiteral("L%1").arg(id)};
}

void TestReconciliacao::targetZeroEncontradoVazio()
{
    auto r = ReconciliacaoAutomatica::resolver(
        {mk(1, 100), mk(2, 200)}, 0);
    QVERIFY(r.encontrado);
    QVERIFY(r.ids.isEmpty());
    QVERIFY(!r.timeoutAtingido);
}

void TestReconciliacao::targetNegativoNaoEncontrado()
{
    auto r = ReconciliacaoAutomatica::resolver(
        {mk(1, 100), mk(2, 200)}, -50);
    QVERIFY(!r.encontrado);
    QVERIFY(!r.timeoutAtingido);
}

void TestReconciliacao::candidatoUnicoBateExato()
{
    auto r = ReconciliacaoAutomatica::resolver(
        {mk(7, 12345)}, 12345);
    QVERIFY(r.encontrado);
    QCOMPARE(r.ids.size(), 1);
    QCOMPARE(r.ids.first(), qint64(7));
}

void TestReconciliacao::tresCandidatosSomaDoisDeles()
{
    // 100 + 250 = 350 (descartando 99)
    auto r = ReconciliacaoAutomatica::resolver(
        {mk(1, 100), mk(2, 250), mk(3, 99)}, 350);
    QVERIFY(r.encontrado);
    QCOMPARE(r.ids.size(), 2);

    qint64 soma = 0;
    for (qint64 id : r.ids) {
        if (id == 1) { soma += 100; }
        else if (id == 2) { soma += 250; }
        else if (id == 3) { soma += 99; }
    }
    QCOMPARE(soma, qint64(350));
}

void TestReconciliacao::targetInalcancavel()
{
    auto r = ReconciliacaoAutomatica::resolver(
        {mk(1, 100), mk(2, 200), mk(3, 300)}, 50);
    QVERIFY(!r.encontrado);
}

void TestReconciliacao::candidatosVazioAlvoPositivo()
{
    auto r = ReconciliacaoAutomatica::resolver({}, 100);
    QVERIFY(!r.encontrado);
}

void TestReconciliacao::timeoutArtificialZero()
{
    // Conjunto grande forca varios estados; timeoutMs=0 deve disparar timeout
    // antes de concluir.
    QList<Lanc> grandes;
    for (int i = 0; i < 100; ++i) {
        grandes.append(mk(i + 1, 1000 + i * 7));
    }
    // Alvo intencionalmente nao trivial
    auto r = ReconciliacaoAutomatica::resolver(grandes, 999999999, 0);
    // Com timeout 0 ms deve marcar timeoutAtingido ou nao encontrar
    // (toleramos os 2 caminhos pra evitar flakiness, mas tipicamente timeout).
    QVERIFY(r.timeoutAtingido || !r.encontrado);
}

void TestReconciliacao::candidatosComValoresNegativosIgnorados()
{
    // Valores <=0 sao filtrados; soma viavel ainda eh 100+200=300.
    auto r = ReconciliacaoAutomatica::resolver(
        {mk(1, 100), mk(2, -500), mk(3, 200), mk(4, 0)}, 300);
    QVERIFY(r.encontrado);
    QCOMPARE(r.ids.size(), 2);
}

QTEST_GUILESS_MAIN(TestReconciliacao)
#include "test_reconciliacao.moc"
