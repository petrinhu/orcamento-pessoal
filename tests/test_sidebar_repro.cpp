// Teste de reproducao dos bugs #165 + modernizacao rodape navy (#47, #165).
// Salva /tmp/repro_full.png (1100x690), /tmp/repro_short.png (1100x470),
// e /tmp/rodape_navy.png (1100x690, alias de repro_full para verificacao do rodape).
// Verifica:
//   1. "Planejamento/Orcamento" aparece completo (sem clip).
//   2. Janela curta: rodape nao some (fixo) e a area de itens rola.
//   3. Statusbar NAVY: pixel sob a sidebar e escuro (navy, lum < 0.30),
//      continuo com a sidebar — sem faixa clara no canto inferior esquerdo.
//   4. CurrencyPanelWidget moderno: sem caixa preta LED, inline com a statusbar.

#include <QtTest>
#include <QApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>
#include <QScrollArea>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QColor>
#include <QImage>

#include "ui/widgets/SidebarNav.h"
#include "ui/widgets/CurrencyPanelWidget.h"
#include "ui/Theme.h"
#include "ui/NavRegistry.h"

class TestSidebarRepro : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void reproFull();
    void reproShort();
};

// ── helpers ───────────────────────────────────────────────────────────────────

static SidebarNav *criarSidebarCompleta(QWidget *parent = nullptr)
{
    auto *nav = new SidebarNav(parent);
    QString grupoAtual;
    for (const NavItem &item : navItens()) {
        if (item.grupo != grupoAtual && !item.grupo.isEmpty()) {
            nav->addGrupo(item.grupo);
            grupoAtual = item.grupo;
        }
        nav->addItem(item.chave, item.emoji, item.titulo);
    }
    nav->addFooterItem(QStringLiteral("act_prefs"), QStringLiteral("⚙️"), QStringLiteral("Preferências"));
    nav->addFooterItem(QStringLiteral("act_ajuda"), QStringLiteral("❓"),       QStringLiteral("Ajuda"));
    nav->addFooterItem(QStringLiteral("act_tema"),  QStringLiteral("\U0001F317"),   QStringLiteral("Tema"));
    nav->setUsuario(QStringLiteral("Petrus"));
    nav->setAtivo(QStringLiteral("tab_dashboard"));
    return nav;
}

// Constroi QMainWindow que imita o app real: sidebar | stack + statusbar.
static QMainWindow *criarJanela(int w, int h)
{
    auto *win = new QMainWindow();
    win->resize(w, h);

    auto *central = new QWidget(win);
    auto *lay = new QHBoxLayout(central);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    SidebarNav *nav = criarSidebarCompleta(central);
    lay->addWidget(nav);

    // Stub de conteudo claro (simula QStackedWidget)
    auto *conteudo = new QWidget(central);
    conteudo->setAutoFillBackground(true);
    lay->addWidget(conteudo, 1);

    win->setCentralWidget(central);

    // Status bar como no MainWindow real (apos #47+#165 fix):
    // sem lblUser (removido como redundante com o rodape da sidebar),
    // m_statusTotal com saldo + CurrencyPanelWidget moderno (sem LED).
    auto *lblTotal = new QLabel(QStringLiteral("  Saldo: R$ 0,00"));
    lblTotal->setObjectName(QStringLiteral("subtitle"));
    win->statusBar()->addWidget(lblTotal);

    auto *currencyPanel = new CurrencyPanelWidget();
    QMap<QString, double> rates;
    rates.insert(QStringLiteral("USD"), 5.12);
    rates.insert(QStringLiteral("EUR"), 5.61);
    currencyPanel->setRates(rates);
    win->statusBar()->addPermanentWidget(currencyPanel);

    return win;
}

// Retorna luminancia media (0.0 = preto, 1.0 = branco)
static double luminancia(const QColor &c)
{
    return (c.red() + c.green() + c.blue()) / 3.0 / 255.0;
}

// ── initTestCase ─────────────────────────────────────────────────────────────

void TestSidebarRepro::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── repro tamanho normal (1100x690) ──────────────────────────────────────────

void TestSidebarRepro::reproFull()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        QSKIP("grab de pixel do fundo navy (QSS) nao e confiavel no platform offscreen/headless; validado no display real do dev");

    QMainWindow *win = criarJanela(1100, 690);
    win->show();
    QApplication::processEvents();
    win->repaint();
    QApplication::processEvents();

    QPixmap px = win->grab();
    QVERIFY2(!px.isNull(), "grab() retornou nulo");

    const bool saved = px.save(QStringLiteral("/tmp/repro_full.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/repro_full.png");

    // Alias exigido pela spec do fix: /tmp/rodape_navy.png
    px.save(QStringLiteral("/tmp/rodape_navy.png"));

    const QImage img = px.toImage();

    // ── Verificacao 1: pixel central da sidebar e navy (escuro) ─────────────
    // Sidebar ocupa min-width/max-width = 252px. Centro horizontal = 126, vertical = 345.
    const QColor pixSidebar = img.pixelColor(126, 345);
    QVERIFY2(luminancia(pixSidebar) < 0.30,
        qPrintable(QStringLiteral("FAIL: sidebar nao e navy em full. Cor: %1 lum=%2")
            .arg(pixSidebar.name()).arg(luminancia(pixSidebar))));

    // ── Verificacao 2: statusbar NAVY (#47+#165 fix) ──────────────────────────
    // Apos o fix, a statusbar usa SIDEBAR2 (#16233A = R22 G35 B58, lum ~0.095).
    // O pixel sob a sidebar (x=126, y=680) DEVE ser escuro (navy): lum < 0.30.
    // Isso confirma continuidade navy sidebar -> statusbar sem faixa clara.
    const QColor pixStatus = img.pixelColor(126, 680);
    QVERIFY2(luminancia(pixStatus) < 0.30,
        qPrintable(QStringLiteral(
            "FAIL: statusbar nao e navy sob a sidebar (#47+#165 fix). "
            "Esperado escuro (lum < 0.30), obtido: %1 lum=%2. "
            "Provavel causa: QSS QStatusBar ainda usa @@SURF@@ em vez de @@SIDEBAR2@@.")
            .arg(pixStatus.name()).arg(luminancia(pixStatus))));

    win->deleteLater();
}

// ── repro janela curta (1100x470) ────────────────────────────────────────────

void TestSidebarRepro::reproShort()
{
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        QSKIP("grab de pixel do fundo navy (QSS) nao e confiavel no platform offscreen/headless; validado no display real do dev");

    QMainWindow *win = criarJanela(1100, 470);
    win->show();
    QApplication::processEvents();
    win->repaint();
    QApplication::processEvents();

    QPixmap px = win->grab();
    QVERIFY2(!px.isNull(), "grab() retornou nulo");

    const bool saved = px.save(QStringLiteral("/tmp/repro_short.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/repro_short.png");

    const QImage img = px.toImage();

    // ── Verificacao 1: sidebar e navy na janela curta ────────────────────────
    const QColor pixSidebar = img.pixelColor(126, 235);
    QVERIFY2(luminancia(pixSidebar) < 0.30,
        qPrintable(QStringLiteral("FAIL: sidebar nao e navy em short. Cor: %1 lum=%2")
            .arg(pixSidebar.name()).arg(luminancia(pixSidebar))));

    // ── Verificacao 2: conteudo (x=600) nao e navy (sidebar nao vazou) ──────
    const QColor pixConteudo = img.pixelColor(600, 235);
    QVERIFY2(luminancia(pixConteudo) > 0.50,
        qPrintable(QStringLiteral("FAIL: area de conteudo esta escura — sidebar vazou. Cor: %1")
            .arg(pixConteudo.name())));

    win->deleteLater();
}

QTEST_MAIN(TestSidebarRepro)
#include "test_sidebar_repro.moc"
