// Teste TDD do SidebarNav (#165).
// Executa offscreen (sem display real). Cobre:
//   1. Sinal navegou emitido com a chave correta ao clicar o botao.
//   2. setAtivo() marca apenas o alvo; os demais ficam inativo.
#include <QtTest>
#include <QApplication>
#include <QSignalSpy>
#include <QPushButton>

#include "ui/widgets/SidebarNav.h"

class TestSidebarNav : public QObject {
    Q_OBJECT

private slots:
    void cliqueEmiteChaveCorreta();
    void setAtivoMarcaSoOAlvo();
    void footerItemERegistrado();
    void setUsuarioNaoQuebra();
};

void TestSidebarNav::cliqueEmiteChaveCorreta()
{
    SidebarNav nav;
    nav.addItem("tab_x", "🏠", "Início");
    nav.addItem("tab_y", "💵", "Entradas");

    QSignalSpy spy(&nav, &SidebarNav::navegou);
    QVERIFY(spy.isValid());

    // Localiza o botao de tab_x via acessor de teste e simula clique.
    QPushButton *btn = nav.itemParaTeste("tab_x");
    QVERIFY2(btn != nullptr, "Botao tab_x nao encontrado");
    btn->click();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), QString("tab_x"));
}

void TestSidebarNav::setAtivoMarcaSoOAlvo()
{
    SidebarNav nav;
    nav.addItem("tab_x", "🏠", "Início");
    nav.addItem("tab_y", "💵", "Entradas");
    nav.addItem("tab_z", "📊", "Resumo");

    nav.setAtivo("tab_y");

    QPushButton *btnX = nav.itemParaTeste("tab_x");
    QPushButton *btnY = nav.itemParaTeste("tab_y");
    QPushButton *btnZ = nav.itemParaTeste("tab_z");

    QVERIFY(btnX && btnY && btnZ);

    QCOMPARE(btnX->property("ativo").toBool(), false);
    QCOMPARE(btnY->property("ativo").toBool(), true);
    QCOMPARE(btnZ->property("ativo").toBool(), false);
}

void TestSidebarNav::footerItemERegistrado()
{
    SidebarNav nav;
    nav.addFooterItem("config", "⚙️", "Configurações");

    QPushButton *btn = nav.itemParaTeste("config");
    QVERIFY2(btn != nullptr, "Footer item 'config' nao registrado");
}

void TestSidebarNav::setUsuarioNaoQuebra()
{
    SidebarNav nav;

    // Nome vazio: avatar exibe "?" como fallback.
    nav.setUsuario("");
    QCOMPARE(nav.avatarTexto(), QStringLiteral("?"));

    // Nome preenchido: avatar exibe a inicial maiuscula.
    nav.setUsuario("Petrus");
    QCOMPARE(nav.avatarTexto(), QStringLiteral("P"));

    // Inicial minuscula deve ser convertida para maiuscula.
    nav.setUsuario("joana");
    QCOMPARE(nav.avatarTexto(), QStringLiteral("J"));
}

QTEST_MAIN(TestSidebarNav)
#include "test_sidebar_nav.moc"
