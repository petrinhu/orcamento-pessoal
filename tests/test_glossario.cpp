// PeuBud - GlossarioService TDD (#103)

#include <QtTest/QtTest>
#include "services/GlossarioService.h"

class TestGlossario : public QObject
{
    Q_OBJECT
private slots:
    void definicaoCetNaoVazia();
    void definicaoFireMencionaAposentadoria();
    void definicaoTermoDesconhecidoVazia();
    void microcopyCet();
    void labelColoquialFire();
    void labelColoquialTermoSemColoquialVazio();
    void termosCobreMinimo50Entradas();
    void primeiroTermoEncontradoCetEmFrase();
    void primeiroTermoEncontradoNenhumQuandoVazio();
    void caseInsensitive();
};

void TestGlossario::definicaoCetNaoVazia()
{
    const QString d = GlossarioService::definicao(QStringLiteral("CET"));
    QVERIFY(!d.isEmpty());
    QVERIFY(d.contains(QStringLiteral("Custo Efetivo Total")));
}

void TestGlossario::definicaoFireMencionaAposentadoria()
{
    const QString d = GlossarioService::definicao(QStringLiteral("FIRE"));
    QVERIFY(d.toLower().contains(QStringLiteral("aposentadoria")));
}

void TestGlossario::definicaoTermoDesconhecidoVazia()
{
    QCOMPARE(GlossarioService::definicao(QStringLiteral("XYZQ123")), QString());
}

void TestGlossario::microcopyCet()
{
    const QString m = GlossarioService::microcopy(QStringLiteral("CET"));
    QVERIFY(!m.isEmpty());
    QVERIFY(m.startsWith(QStringLiteral("="))); // padrão microcopy
}

void TestGlossario::labelColoquialFire()
{
    const QString l = GlossarioService::labelColoquial(QStringLiteral("FIRE"));
    QVERIFY(l.contains(QStringLiteral("(FIRE)")));
    QVERIFY(l.toLower().contains(QStringLiteral("aposentadoria")));
}

void TestGlossario::labelColoquialTermoSemColoquialVazio()
{
    // SAC nao tem renomeacao coloquial (sigla ja conhecida)
    QCOMPARE(GlossarioService::labelColoquial(QStringLiteral("SAC")), QString());
}

void TestGlossario::termosCobreMinimo50Entradas()
{
    const auto t = GlossarioService::termos();
    QVERIFY2(t.size() >= 50,
        qPrintable(QStringLiteral("apenas %1 termos").arg(t.size())));
}

void TestGlossario::primeiroTermoEncontradoCetEmFrase()
{
    QCOMPARE(GlossarioService::primeiroTermoEncontrado(
                 QStringLiteral("Taxa CET ao ano")),
             QStringLiteral("CET"));
}

void TestGlossario::primeiroTermoEncontradoNenhumQuandoVazio()
{
    QCOMPARE(GlossarioService::primeiroTermoEncontrado(QString()), QString());
    QCOMPARE(GlossarioService::primeiroTermoEncontrado(
                 QStringLiteral("Olá mundo")), QString());
}

void TestGlossario::caseInsensitive()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("cet")).isEmpty());
    QVERIFY(!GlossarioService::definicao(QStringLiteral("Fire")).isEmpty());
    QVERIFY(!GlossarioService::definicao(QStringLiteral("ipca")).isEmpty());
}

QTEST_GUILESS_MAIN(TestGlossario)
#include "test_glossario.moc"
