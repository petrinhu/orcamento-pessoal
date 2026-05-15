// PeuBud - GlossarioService TDD (#103, #104)

#include <QtTest/QtTest>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>
#include "core/settings/AppSettings.h"
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
    void aplicarLabelsColoquiaisRenomeia();
    void aplicarLabelsRespeitaModoAvancado();
    void aplicarMicrocopiesAdicionaLabelCinza();
    void aplicarMicrocopiesIdempotente();
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

void TestGlossario::aplicarLabelsColoquiaisRenomeia()
{
    AppSettings::instance().setModoIniciante(true);
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Calcular FIRE"), &root);
    GlossarioService::aplicarLabelsColoquiais(&root);
    QVERIFY(lbl->text().contains(QStringLiteral("Aposentadoria")));
    QVERIFY(lbl->text().contains(QStringLiteral("(FIRE)")));
}

void TestGlossario::aplicarLabelsRespeitaModoAvancado()
{
    AppSettings::instance().setModoIniciante(false);
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Calcular FIRE"), &root);
    GlossarioService::aplicarLabelsColoquiais(&root);
    QCOMPARE(lbl->text(), QStringLiteral("Calcular FIRE"));
    AppSettings::instance().setModoIniciante(true); // restaura default
}

void TestGlossario::aplicarMicrocopiesAdicionaLabelCinza()
{
    QWidget root;
    auto *form = new QFormLayout(&root);
    form->addRow(new QLabel(QStringLiteral("CET (% a.a.):"), &root),
                 new QLineEdit(&root));
    GlossarioService::aplicarMicrocopies(&root);
    // 1 linha original + 1 linha microcopy = 2.
    QCOMPARE(form->rowCount(), 2);
    auto *fieldItem = form->itemAt(1, QFormLayout::FieldRole);
    QVERIFY(fieldItem != nullptr);
    auto *micro = qobject_cast<QLabel*>(fieldItem->widget());
    QVERIFY(micro != nullptr);
    QVERIFY(micro->text().startsWith(QStringLiteral("=")));
    QVERIFY(micro->property("microcopy").toBool());
    QVERIFY(micro->styleSheet().contains(QStringLiteral("gray")));
}

void TestGlossario::aplicarMicrocopiesIdempotente()
{
    QWidget root;
    auto *form = new QFormLayout(&root);
    form->addRow(new QLabel(QStringLiteral("CET"), &root),
                 new QLineEdit(&root));
    GlossarioService::aplicarMicrocopies(&root);
    GlossarioService::aplicarMicrocopies(&root); // 2a chamada nao duplica
    QCOMPARE(form->rowCount(), 2);
}

QTEST_MAIN(TestGlossario)
#include "test_glossario.moc"
