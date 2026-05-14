// PeuBud - SuporteResistencia TDD (#53)
// Service stateless puro - sem dependencia de DB.

#include <QtTest/QtTest>
#include "services/SuporteResistencia.h"

class TestSuporteResistencia : public QObject
{
    Q_OBJECT
private slots:
    void pivotClassico();
    void camarillaSimetrico();
    void fibonacciNiveisPadrao();
    void swingHighsPicosLocais();
    void swingLowsValesLocais();
    void seriesCurtaNaoCrash();
    void pivotRangeZero();
};

void TestSuporteResistencia::pivotClassico()
{
    SuporteResistencia::OHLC oh{100.0, 110.0, 95.0, 105.0};
    auto r = SuporteResistencia::pivot(oh);
    // PP = (110+95+105)/3 = 310/3 = 103.333...
    QVERIFY(qAbs(r.pp - 310.0 / 3.0) < 1e-9);
    // R1 = 2*PP - L = 2*103.333 - 95 = 111.666...
    QVERIFY(qAbs(r.r1 - (2.0 * 310.0 / 3.0 - 95.0)) < 1e-9);
    // S1 = 2*PP - H = 2*103.333 - 110 = 96.666...
    QVERIFY(qAbs(r.s1 - (2.0 * 310.0 / 3.0 - 110.0)) < 1e-9);
    // R2 = PP + (H-L) = 103.333 + 15 = 118.333
    QVERIFY(qAbs(r.r2 - (310.0 / 3.0 + 15.0)) < 1e-9);
    // S2 = PP - (H-L) = 103.333 - 15 = 88.333
    QVERIFY(qAbs(r.s2 - (310.0 / 3.0 - 15.0)) < 1e-9);
    // R3 = H + 2*(PP - L) = 110 + 2*(103.333 - 95) = 110 + 16.666 = 126.666
    QVERIFY(qAbs(r.r3 - (110.0 + 2.0 * (310.0 / 3.0 - 95.0))) < 1e-9);
    // S3 = L - 2*(H - PP) = 95 - 2*(110 - 103.333) = 95 - 13.333 = 81.666
    QVERIFY(qAbs(r.s3 - (95.0 - 2.0 * (110.0 - 310.0 / 3.0))) < 1e-9);
}

void TestSuporteResistencia::camarillaSimetrico()
{
    SuporteResistencia::OHLC oh{100.0, 110.0, 95.0, 105.0};
    auto r = SuporteResistencia::camarilla(oh);
    const double range11 = (110.0 - 95.0) * 1.1; // 16.5
    // R4 = C + range11/2 = 105 + 8.25 = 113.25
    QVERIFY(qAbs(r.r4 - (105.0 + range11 / 2.0)) < 1e-9);
    QCOMPARE(r.r4, 113.25);
    // R1 = C + range11/12
    QVERIFY(qAbs(r.r1 - (105.0 + range11 / 12.0)) < 1e-9);
    // S4 = C - range11/2 = 105 - 8.25 = 96.75
    QCOMPARE(r.s4, 96.75);
    // Simetria: distancia R4-C == C-S4
    QVERIFY(qAbs((r.r4 - 105.0) - (105.0 - r.s4)) < 1e-9);
    QVERIFY(qAbs((r.r1 - 105.0) - (105.0 - r.s1)) < 1e-9);
    QVERIFY(qAbs((r.r2 - 105.0) - (105.0 - r.s2)) < 1e-9);
    QVERIFY(qAbs((r.r3 - 105.0) - (105.0 - r.s3)) < 1e-9);
}

void TestSuporteResistencia::fibonacciNiveisPadrao()
{
    auto niveis = SuporteResistencia::fibonacci(100.0, 50.0);
    QCOMPARE(niveis.size(), 7);
    // 0% -> 50; 100% -> 100
    QCOMPARE(niveis[0].first, 0.0);
    QCOMPARE(niveis[0].second, 50.0);
    QCOMPARE(niveis[6].first, 100.0);
    QCOMPARE(niveis[6].second, 100.0);
    // 23.6 -> 50 + 50*0.236 = 61.8
    QVERIFY(qAbs(niveis[1].second - 61.8) < 1e-9);
    // 38.2 -> 50 + 50*0.382 = 69.1
    QVERIFY(qAbs(niveis[2].second - 69.1) < 1e-9);
    // 50 -> 75
    QVERIFY(qAbs(niveis[3].second - 75.0) < 1e-9);
    // 61.8 -> 50 + 50*0.618 = 80.9
    QVERIFY(qAbs(niveis[4].second - 80.9) < 1e-9);
    // 78.6 -> 50 + 50*0.786 = 89.3
    QVERIFY(qAbs(niveis[5].second - 89.3) < 1e-9);
}

void TestSuporteResistencia::swingHighsPicosLocais()
{
    QList<double> serie = {1, 2, 3, 2, 1, 2, 3, 4, 3, 2};
    auto idx = SuporteResistencia::swingHighs(serie, 2);
    // Pico em i=2 (valor 3, vizinhos [1,2] e [2,1])
    // Pico em i=7 (valor 4, vizinhos [2,3] e [3,2])
    QCOMPARE(idx.size(), 2);
    QCOMPARE(idx[0], 2);
    QCOMPARE(idx[1], 7);
}

void TestSuporteResistencia::swingLowsValesLocais()
{
    QList<double> serie = {3, 2, 1, 2, 3, 2, 1, 0, 1, 2};
    auto idx = SuporteResistencia::swingLows(serie, 2);
    // Vale em i=2 (valor 1, vizinhos [3,2] e [2,3])
    // Vale em i=7 (valor 0, vizinhos [2,1] e [1,2])
    QCOMPARE(idx.size(), 2);
    QCOMPARE(idx[0], 2);
    QCOMPARE(idx[1], 7);
}

void TestSuporteResistencia::seriesCurtaNaoCrash()
{
    QList<double> curta = {1, 2, 3};
    QCOMPARE(SuporteResistencia::swingHighs(curta, 2).size(), 0);
    QCOMPARE(SuporteResistencia::swingLows(curta, 2).size(), 0);
    QCOMPARE(SuporteResistencia::swingHighs({}, 5).size(), 0);
}

void TestSuporteResistencia::pivotRangeZero()
{
    // Quando H==L==C (sessao plana, ex: feriado) tudo colapsa em PP.
    SuporteResistencia::OHLC oh{100.0, 100.0, 100.0, 100.0};
    auto r = SuporteResistencia::pivot(oh);
    QCOMPARE(r.pp, 100.0);
    QCOMPARE(r.r1, 100.0);
    QCOMPARE(r.s1, 100.0);
    QCOMPARE(r.r2, 100.0);
    QCOMPARE(r.s2, 100.0);
}

QTEST_GUILESS_MAIN(TestSuporteResistencia)
#include "test_suporte_resistencia.moc"
