// Render offscreen do SidebarNav em contexto real (#165 fix paintEvent).
//
// O test anterior renderizava a sidebar ISOLADA, o que mascarava o bug:
// o stylesheet global (aplicado ao qApp) faz QWidget filhos herdar
// background-color: @@BG@@ (claro) e pintarem por cima do navy.
//
// Este teste renderiza a sidebar num contexto que imita o MainWindow:
//   QApplication + Theme::aplicar() (stylesheet global ativo, crucial)
//   + QWidget parent (900x560) com QHBoxLayout
//   + SidebarNav ao lado de QWidget claro (simula area de conteudo)
//
// Assertions verificam que o pixel central da sidebar e dark (navy),
// nao claro (fundo app default #EEF3F9) — mesmo com stylesheet global ativo.
//
// Salva /tmp/sidebar_real_ctx.png para inspecao manual.

#include <QtTest>
#include <QApplication>
#include <QHBoxLayout>
#include <QPixmap>
#include <QColor>
#include <QWidget>

#include "ui/widgets/SidebarNav.h"
#include "ui/Theme.h"
#include "ui/NavRegistry.h"

class TestSidebarRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void fundoNavyContextoReal();
    void textoSidebarLegivel();
};

// ── helper ──────────────────────────────────────────────────────────────────

static SidebarNav *criarNavCompleto(QWidget *parent = nullptr)
{
    auto *nav = new SidebarNav(parent);

    // Popula com itens representativos (espelha NavRegistry::todos()).
    nav->addGrupo("Principal");
    nav->addItem("tab_gastos",     "\xF0\x9F\x92\xB3", "Gastos");
    nav->addItem("tab_entradas",   "\xF0\x9F\x92\xB5", "Entradas");
    nav->addItem("tab_resumo",     "\xF0\x9F\x93\x8A", "Resumo");
    nav->addGrupo("Investimentos");
    nav->addItem("tab_investimentos", "\xF0\x9F\x93\x88", "Investimentos");
    nav->addItem("tab_metas",      "\xF0\x9F\x8E\xAF", "Metas");
    nav->addGrupo("Ferramentas");
    nav->addItem("tab_calculadora","\xF0\x9F\xA7\xAE", "Calculadora");
    nav->addGrupo("Relatorios");
    nav->addItem("tab_relatorios", "\xF0\x9F\x93\x91", "Relatorios");

    nav->addFooterItem("tab_prefs",  "\xE2\x9A\x99", "Preferencias");
    nav->addFooterItem("tab_ajuda",  "\xE2\x9D\x93", "Ajuda");

    nav->setUsuario("Petrus");
    nav->setAtivo("tab_gastos");

    return nav;
}

// Retorna true se a cor e "escura" (luminancia < 0.25, ou seja, navy/dark).
// Navy #1B2A41 tem R=27,G=42,B=65 -> media ~45 -> 45/255 ~0.176 -> escuro.
// Fundo app #EEF3F9 tem R=238,G=243,B=249 -> media ~243 -> 0.953 -> claro.
static bool isEscuro(const QColor &c)
{
    const double lum = (c.red() + c.green() + c.blue()) / 3.0 / 255.0;
    return lum < 0.25;
}

// ── initTestCase ──────────────────────────────────────────────────────────────

void TestSidebarRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    // aplicar() define Fusion style + palette global + stylesheet global.
    // E CRUCIAL chamar aqui: e exatamente o contexto do app real que causava
    // o bug (stylesheet global sobrescreve background dos filhos QWidget).
    Theme::aplicar();
}

// ── Teste principal: contexto real (parent + layout + conteudo claro ao lado) ─

void TestSidebarRender::fundoNavyContextoReal()
{
    // Monta o layout que imita MainWindow: sidebar | conteudo claro.
    QWidget parent;
    parent.resize(900, 560);
    auto *hbox = new QHBoxLayout(&parent);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    SidebarNav *nav = criarNavCompleto(&parent);
    hbox->addWidget(nav);

    // Widget de conteudo claro ao lado: simula o painel central do MainWindow.
    auto *conteudo = new QWidget(&parent);
    conteudo->setAutoFillBackground(true);  // pinta de BG claro (como na realidade)
    hbox->addWidget(conteudo, 1);

    parent.show();
    parent.repaint();

    // Grab do widget PARENT (contexto completo): a sidebar ocupa os primeiros
    // 230px da largura (min-width/max-width 230px definido no QSS).
    QPixmap px = parent.grab();
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo — render falhou");

    const bool saved = px.save(QStringLiteral("/tmp/sidebar_real_ctx.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/sidebar_real_ctx.png");

    // Pixel central da sidebar: x=115 (dentro dos 230px), y=280 (metade de 560).
    const QImage img = px.toImage();
    const QColor pixelSidebar = img.pixelColor(115, 280);

    // Pixel no conteudo (x=500, y=280): deve ser CLARO (BG do app).
    const QColor pixelConteudo = img.pixelColor(500, 280);

    QVERIFY2(isEscuro(pixelSidebar),
             qPrintable(QStringLiteral(
                 "FAIL: pixel central da sidebar nao eh navy com stylesheet global ativo. "
                 "Cor obtida: %1 (R=%2 G=%3 B=%4). "
                 "Causa provavel: filho QWidget da sidebar recebe background claro do QSS global "
                 "e pinta por cima do paintEvent do pai.")
                 .arg(pixelSidebar.name())
                 .arg(pixelSidebar.red())
                 .arg(pixelSidebar.green())
                 .arg(pixelSidebar.blue())));

    // Verificacao complementar: o conteudo claro ao lado NAO deve ser navy.
    // (garante que a sidebar nao vazou cor para o vizinho — sanidade do layout)
    QVERIFY2(!isEscuro(pixelConteudo),
             qPrintable(QStringLiteral(
                 "FAIL: pixel do conteudo (x=500) esta escuro — sidebar vazou cor para vizinho? "
                 "Cor: %1 (R=%2 G=%3 B=%4).")
                 .arg(pixelConteudo.name())
                 .arg(pixelConteudo.red())
                 .arg(pixelConteudo.green())
                 .arg(pixelConteudo.blue())));
}

// ── Texto legivel: sidebarText (#D6DEEA) deve ter contraste suficiente sobre navy ─

void TestSidebarRender::textoSidebarLegivel()
{
    // Verifica via tokens que sidebarText tem luminancia > 0.5 (claro sobre navy).
    // Nao e um pixel test — e um teste de contrato dos tokens:
    // sidebarText DEVE ser claro; se um refactoring trocar o valor, o teste falha.
    const QColor sidebarTxt(Theme::sidebarText());
    const double lum = (sidebarTxt.red() + sidebarTxt.green() + sidebarTxt.blue()) / 3.0 / 255.0;
    QVERIFY2(lum > 0.5,
             qPrintable(QStringLiteral(
                 "FAIL: Theme::sidebarText() nao eh suficientemente claro para ser legivel "
                 "sobre o fundo navy. Valor: %1, luminancia: %2 (esperado > 0.5). "
                 "Texto claro sobre navy escuro e requisito de contraste WCAG.")
                 .arg(sidebarTxt.name())
                 .arg(lum)));
}

QTEST_MAIN(TestSidebarRender)
#include "test_sidebar_render.moc"
