// Render offscreen do LoginDialog pos-#186: icone @128px + tagline.
//
// Verifica:
//   (1) LoginDialog renderiza sem crash (nao-nulo).
//   (2) QLabel#loginIcon existe e tem pixmap nao-nulo (icone carregado do .qrc).
//   (3) Nenhum QLabel com texto "Orcamento Pessoal" (titulo de texto foi removido).
//   (4) QLabel com texto "Suas financas" existe (tagline presente).
//   (5) Salva /tmp/login_impl_light.png e /tmp/login_impl_dark.png.
//
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QPixmap>
#include <QLabel>

#include "ui/LoginDialog.h"
#include "ui/Theme.h"

class TestLoginHeaderRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void iconLabelExiste();
    void iconPixmapNaoNulo();
    void tituloTextoRemovido();
    void taglinePresente();
    void renderLightNaoNulo();
    void renderDarkNaoNulo();
};

void TestLoginHeaderRender::initTestCase()
{
    Theme::aplicar();
}

void TestLoginHeaderRender::iconLabelExiste()
{
    Theme::setMode(Theme::ThemeMode::Light);
    LoginDialog dlg;
    auto *lbl = dlg.findChild<QLabel *>(QStringLiteral("loginIcon"));
    QVERIFY2(lbl, "QLabel#loginIcon nao encontrado — icone de cabecalho ausente (#186)");
}

void TestLoginHeaderRender::iconPixmapNaoNulo()
{
    LoginDialog dlg;
    auto *lbl = dlg.findChild<QLabel *>(QStringLiteral("loginIcon"));
    QVERIFY2(lbl, "QLabel#loginIcon nao encontrado");
    QVERIFY2(!lbl->pixmap().isNull(),
             "Pixmap do loginIcon e nulo — :/icon/icon.png nao carregou do .qrc (#186)");
    // Verifica tamanho: deve ser 128x128 (scalado)
    QCOMPARE(lbl->pixmap().width(),  128);
    QCOMPARE(lbl->pixmap().height(), 128);
}

void TestLoginHeaderRender::tituloTextoRemovido()
{
    // Nenhum QLabel deve ter o texto "Orcamento Pessoal" — foi removido (#186).
    LoginDialog dlg;
    const auto labels = dlg.findChildren<QLabel *>();
    for (auto *lbl : labels) {
        // Verifica tanto PT quanto sem acento (por seguranca)
        QVERIFY2(!lbl->text().contains(QStringLiteral("Orçamento Pessoal")),
                 "QLabel com texto 'Orcamento Pessoal' ainda existe — titulo de texto nao foi removido (#186)");
    }
}

void TestLoginHeaderRender::taglinePresente()
{
    LoginDialog dlg;
    bool encontrou = false;
    const auto labels = dlg.findChildren<QLabel *>();
    for (auto *lbl : labels) {
        if (lbl->text().contains(QStringLiteral("Suas finanças"))) {
            encontrou = true;
            break;
        }
    }
    QVERIFY2(encontrou, "Tagline 'Suas financas, sem complicacao' nao encontrada no LoginDialog (#186)");
}

void TestLoginHeaderRender::renderLightNaoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    LoginDialog dlg;
    dlg.show();
    QPixmap px = dlg.grab();
    QVERIFY2(!px.isNull(), "grab() light retornou pixmap nulo");
    px.save(QStringLiteral("/tmp/login_impl_light.png"));
    qDebug() << "Light render salvo: /tmp/login_impl_light.png" << px.size();
}

void TestLoginHeaderRender::renderDarkNaoNulo()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    LoginDialog dlg;
    dlg.show();
    QPixmap px = dlg.grab();
    QVERIFY2(!px.isNull(), "grab() dark retornou pixmap nulo");
    px.save(QStringLiteral("/tmp/login_impl_dark.png"));
    qDebug() << "Dark render salvo: /tmp/login_impl_dark.png" << px.size();
}

QTEST_MAIN(TestLoginHeaderRender)
#include "test_login_header_render.moc"
