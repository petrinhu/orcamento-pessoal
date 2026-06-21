// PeuBud - teste de consistência de cards (#166 Peça 4)
//
// Verifica:
//   (1) O QSS contém a regra QFrame#dashCard (consistência com #surface).
//   (2) O QSS contém a regra QFrame#surface.
//   (3) Nenhum token @@...@@ residual no QSS (regressão de substituição).
//   (4) A regra de border-radius 14px está presente para #dashCard e #surface.
//   (5) Render offscreen light+dark de QFrame#dashCard não nulo (smoke de render).
//
// Os cards do Dashboard (makeCard) usam objectName="dashCard" preservando
// o eventFilter de clique (navegarParaAba). A regra QSS foi unificada com
// QFrame#surface via seletor agrupado (§9.4 manual de estilo, #166 Peça 4).
//
// Salva /tmp/cards_light.png e /tmp/cards_dark.png para aprovação visual.
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>

#include "ui/Theme.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

// Cria um card de demonstração com objectName "dashCard" (padrão DashboardWidget)
// e um segundo com "surface" (padrão genérico) para render comparativo.
static QWidget *criarDemoCards(QWidget *parent = nullptr)
{
    auto *root = new QWidget(parent);
    root->setObjectName(QStringLiteral("cardsDemo"));
    auto *lay  = new QHBoxLayout(root);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(12);

    // Card 1: dashCard (padrão DashboardWidget)
    auto *card1 = new QFrame(root);
    card1->setObjectName(QStringLiteral("dashCard"));
    card1->setFrameShape(QFrame::NoFrame); // NoFrame: QSS controla a borda
    card1->setMinimumSize(160, 100);
    auto *lay1 = new QVBoxLayout(card1);
    lay1->setContentsMargins(16, 12, 16, 12);
    auto *lbl1a = new QLabel(QStringLiteral("Entradas"), card1);
    lbl1a->setObjectName(QStringLiteral("subtitle"));
    auto *lbl1b = new QLabel(QStringLiteral("R$ 5.000,00"), card1);
    lay1->addWidget(lbl1a);
    lay1->addWidget(lbl1b);

    // Card 2: surface (genérico)
    auto *card2 = new QFrame(root);
    card2->setObjectName(QStringLiteral("surface"));
    card2->setFrameShape(QFrame::NoFrame);
    card2->setMinimumSize(160, 100);
    auto *lay2 = new QVBoxLayout(card2);
    lay2->setContentsMargins(16, 12, 16, 12);
    auto *lbl2a = new QLabel(QStringLiteral("Gastos"), card2);
    lbl2a->setObjectName(QStringLiteral("subtitle"));
    auto *lbl2b = new QLabel(QStringLiteral("R$ 2.300,00"), card2);
    lay2->addWidget(lbl2a);
    lay2->addWidget(lbl2b);

    // Card 3: dashCard clicável (simula card com tabIndex)
    auto *card3 = new QFrame(root);
    card3->setObjectName(QStringLiteral("dashCard"));
    card3->setFrameShape(QFrame::NoFrame);
    card3->setCursor(Qt::PointingHandCursor);
    card3->setMinimumSize(160, 100);
    auto *lay3 = new QVBoxLayout(card3);
    lay3->setContentsMargins(16, 12, 16, 12);
    auto *lbl3a = new QLabel(QStringLiteral("Saldo   ➡️"), card3);
    lbl3a->setObjectName(QStringLiteral("subtitle"));
    auto *lbl3b = new QLabel(QStringLiteral("R$ 2.700,00"), card3);
    lay3->addWidget(lbl3a);
    lay3->addWidget(lbl3b);

    lay->addWidget(card1);
    lay->addWidget(card2);
    lay->addWidget(card3);
    lay->addStretch();

    root->setMinimumSize(600, 140);
    return root;
}

// ── Classe de teste ───────────────────────────────────────────────────────────

class TestCardsRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // ── Contrato QSS ─────────────────────────────────────────────────────────
    void qss_contemSeletorSurface();
    void qss_contemSeletorDashCard();
    void qss_dashCard_temBorderRadius14();
    void qss_surface_temBorderRadius14();
    void qss_semTokensResiduais();

    // ── Render offscreen (smoke) ──────────────────────────────────────────────
    void renderLight_naoNulo();
    void renderDark_naoNulo();
};

// ── initTestCase ──────────────────────────────────────────────────────────────

void TestCardsRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── Contrato QSS ─────────────────────────────────────────────────────────────

void TestCardsRender::qss_contemSeletorSurface()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("QFrame#surface")),
             "Seletor QFrame#surface ausente no QSS — card padrao sem estilo");
}

void TestCardsRender::qss_contemSeletorDashCard()
{
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("QFrame#dashCard")),
             "Seletor QFrame#dashCard ausente no QSS — cards do Dashboard sem estilo (#166 Peca 4)");
}

void TestCardsRender::qss_dashCard_temBorderRadius14()
{
    // Verifica que a regra agrupada QFrame#surface, QFrame#dashCard contém
    // border-radius: 14px (§9.4 manual de estilo, #166 Peça 4).
    // Estratégia: confirmar que "border-radius: 14px" aparece dentro de 600
    // caracteres após o seletor "QFrame#dashCard" (janela ampla pra acomodar
    // comentários CMake-style no literal do QSS).
    const QString qss = Theme::qssParaTeste();
    const int posDashCard = qss.indexOf(QStringLiteral("QFrame#dashCard"));
    QVERIFY2(posDashCard != -1,
             "Seletor QFrame#dashCard ausente — nao ha onde verificar border-radius");
    const int pos14px = qss.indexOf(QStringLiteral("border-radius: 14px"), posDashCard);
    QVERIFY2(pos14px != -1 && (pos14px - posDashCard) < 600,
             "border-radius: 14px nao encontrado na regra de QFrame#dashCard — "
             "cards do Dashboard com raio errado (esperado 14px, §9.4)");
}

void TestCardsRender::qss_surface_temBorderRadius14()
{
    // Mesma verificação para QFrame#surface (seletor-irmão da regra agrupada).
    const QString qss = Theme::qssParaTeste();
    const int posSurface = qss.indexOf(QStringLiteral("QFrame#surface"));
    QVERIFY2(posSurface != -1,
             "Seletor QFrame#surface ausente — nao ha onde verificar border-radius");
    const int pos14px = qss.indexOf(QStringLiteral("border-radius: 14px"), posSurface);
    QVERIFY2(pos14px != -1 && (pos14px - posSurface) < 600,
             "border-radius: 14px nao encontrado na regra de QFrame#surface");
}

void TestCardsRender::qss_semTokensResiduais()
{
    const QString qss = Theme::qssParaTeste();
    // Nenhum token @@...@@ deve sobrar — regressao de substituicao de tokens.
    QVERIFY2(!qss.contains(QStringLiteral("@@")),
             "Token @@...@@ residual no QSS — gerarQss() nao substituiu todos");
}

// ── Render offscreen (smoke) ──────────────────────────────────────────────────

void TestCardsRender::renderLight_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    // Render de cards QFrame#dashCard + QFrame#surface lado a lado.
    auto *demo = criarDemoCards();
    demo->show();
    QApplication::processEvents();
    const QPixmap pm = demo->grab();
    QVERIFY2(!pm.isNull(), "Render light dos cards nulo — crash ou widget invisivel");
    pm.save(QStringLiteral("/tmp/cards_light.png"));
    delete demo;
}

void TestCardsRender::renderDark_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();

    auto *demo = criarDemoCards();
    demo->show();
    QApplication::processEvents();
    const QPixmap pm = demo->grab();
    QVERIFY2(!pm.isNull(), "Render dark dos cards nulo — crash ou widget invisivel");
    pm.save(QStringLiteral("/tmp/cards_dark.png"));
    delete demo;

    // Restaura light para nao poluir outros testes
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

QTEST_MAIN(TestCardsRender)
#include "test_cards_render.moc"
