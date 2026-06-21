// #165 iconButton — contrato do FocusVisibleFilter + smoke de render da navBar.
//
// LIÇÃO (#165, depois de 2 fixes falhos): a "caixa no 1º render" do ◀️ NÃO é a
// borda do estilo-base QPushButton, nem tamanho errado, nem sombra. É o estado
// :focus do #iconButton — o botão, sendo o 1º widget focável da janela, recebe
// foco AUTOMÁTICO (Qt::ActiveWindowFocusReason) quando a janela fica ativa, e a
// regra QSS `QPushButton#iconButton:focus { border:1px solid <accent>; background:
// <hover>; }` pinta a caixa azul — antes de qualquer interação.
//
// Esse bug só aparece com COMPOSITOR REAL (KDE/Wayland) atribuindo foco à janela.
// O offscreen NÃO o reproduz, e QWidget::grab() renderiza o botão sem o estado de
// foco da janela. Logo, um teste de PIXEL offscreen é MENTIROSO para este bug:
// fica "verde" sem provar nada (foi o que mascarou os 2 fixes anteriores — eles
// faziam repolish, que reaplica o QSS mas não remove o foco).
//
// O teste que VALE aqui é o de LÓGICA DE FOCO do FocusVisibleFilter, que emula
// `:focus-visible`: o anel só aparece para foco de TECLADO (Tab/Backtab/Shortcut).
// QFocusEvent com reason específico é event-logic pura — roda headless e prova o
// contrato sem depender de compositor:
//   - reason=ActiveWindow em #iconButton  -> filtro chama clearFocus()  (sem anel)
//   - reason=Tab          em #iconButton  -> foco mantido               (anel)
//   - reason=ActiveWindow em widget comum -> filtro NÃO interfere
//
// A verificação visual definitiva (◀️ sem caixa no 1º render) é feita FORA do
// ctest, por screenshot do compositor real (spectacle) — ver report fix #165.
//
// Os grabs offscreen ficam apenas como SMOKE de render (não falhar / dimensões),
// salvos em /tmp/iconbtn_light.png e /tmp/iconbtn_dark.png p/ inspeção manual.

#include <QtTest>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
#include <QFocusEvent>
#include <QEvent>

#include "ui/Theme.h"
#include "ui/FocusVisibleFilter.h"

// ── navBar de mês com os 4 botões #iconButton (espelha o DashboardWidget) ────
// Sem showEvent/repolish: o fix correto vive no FocusVisibleFilter global, não
// num override por-widget (o repolish foi removido — era código morto).

class NavBarIconBtn : public QWidget {
public:
    QPushButton *m_btnPrev = nullptr;

    explicit NavBarIconBtn(QWidget *parent = nullptr) : QWidget(parent)
    {
        setObjectName(QStringLiteral("navBarIconBtn"));
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(16, 12, 16, 8);
        lay->setSpacing(4);

        m_btnPrev = new QPushButton(this);
        auto *btnNext = new QPushButton(this);
        m_btnPrev->setIcon(Theme::emojiIcon(QStringLiteral("◀️")));
        btnNext->setIcon(Theme::emojiIcon(QStringLiteral("▶️")));
        m_btnPrev->setObjectName(QStringLiteral("iconButton"));
        btnNext->setObjectName(QStringLiteral("iconButton"));

        auto *lblMes = new QLabel(QStringLiteral("Junho/2026"), this);
        {
            QFont f = lblMes->font();
            f.setPointSize(14);
            f.setBold(true);
            lblMes->setFont(f);
        }
        lblMes->setAlignment(Qt::AlignCenter);

        auto *btnFechar = new QPushButton(this);
        btnFechar->setIcon(Theme::emojiIcon(QStringLiteral("\U0001F512")));
        btnFechar->setObjectName(QStringLiteral("iconButton"));

        auto *btnPrint = new QPushButton(this);
        btnPrint->setIcon(Theme::emojiIcon(QStringLiteral("\U0001F5A8️")));
        btnPrint->setObjectName(QStringLiteral("iconButton"));

        lay->addWidget(m_btnPrev);
        lay->addWidget(lblMes, 1);
        lay->addWidget(btnNext);
        lay->addWidget(btnFechar);
        lay->addWidget(btnPrint);
    }
};

// ── Render helper (smoke) ─────────────────────────────────────────────────────

static QPixmap renderNavBar(Theme::ThemeMode mode)
{
    Theme::setMode(mode);
    Theme::aplicar();

    QWidget parent;
    const QString bgColor = (mode == Theme::ThemeMode::Dark)
        ? QStringLiteral("#0F172A")
        : QStringLiteral("#EEF3F9");
    parent.setStyleSheet(QStringLiteral("background: %1;").arg(bgColor));
    parent.resize(500, 60);

    auto *lay = new QHBoxLayout(&parent);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(new NavBarIconBtn(&parent));

    parent.show();
    parent.repaint();

    return parent.grab();
}

// ── Test class ────────────────────────────────────────────────────────────────

class TestIconBtnRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // Smoke de render (não falhar / dimensões) — NÃO prova ausência de caixa.
    void iconButtonLightRendersOk();
    void iconButtonDarkRendersOk();

    // Contrato do FocusVisibleFilter (o que de fato corrige o bug #165):
    void focoAutomaticoLimpaAnelNoIconButton();   // ActiveWindow -> clearFocus
    void focoDeTecladoMantemAnelNoIconButton();   // Tab/Backtab/Shortcut -> mantém
    void focoAutomaticoNaoMexeEmWidgetComum();    // não-iconButton -> intacto
};

void TestIconBtnRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar(); // instala FocusVisibleFilter no qApp (idempotente)
}

void TestIconBtnRender::iconButtonLightRendersOk()
{
    QPixmap px = renderNavBar(Theme::ThemeMode::Light);
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo (light)");
    QVERIFY2(px.save(QStringLiteral("/tmp/iconbtn_light.png")),
             "Falha ao salvar /tmp/iconbtn_light.png");
    QVERIFY2(px.width()  >= 400, "Largura do pixmap menor que 400px (light)");
    QVERIFY2(px.height() >= 50,  "Altura do pixmap menor que 50px (light)");
}

void TestIconBtnRender::iconButtonDarkRendersOk()
{
    QPixmap px = renderNavBar(Theme::ThemeMode::Dark);
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo (dark)");
    QVERIFY2(px.save(QStringLiteral("/tmp/iconbtn_dark.png")),
             "Falha ao salvar /tmp/iconbtn_dark.png");
    QVERIFY2(px.width()  >= 400, "Largura do pixmap menor que 400px (dark)");
    QVERIFY2(px.height() >= 50,  "Altura do pixmap menor que 50px (dark)");
}

// ── Contrato 1: foco automático (ActiveWindow) é suprimido no #iconButton ─────
// Reproduz a CAUSA RAIZ do bug em event-logic pura: ao receber FocusIn com
// Qt::ActiveWindowFocusReason (o foco que o Qt dá ao 1º focável ao ativar a
// janela), o FocusVisibleFilter deve chamar clearFocus() — o botão fica sem
// foco, logo a regra :focus não casa e a caixa não é pintada.
void TestIconBtnRender::focoAutomaticoLimpaAnelNoIconButton()
{
    QWidget host;
    auto *btn = new QPushButton(&host);
    btn->setObjectName(QStringLiteral("iconButton"));
    btn->setFocusPolicy(Qt::StrongFocus);
    auto *outro = new QPushButton(&host); // alvo de foco inicial neutro
    outro->setObjectName(QStringLiteral("primary"));
    auto *lay = new QHBoxLayout(&host);
    lay->addWidget(outro);
    lay->addWidget(btn);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    // Dá foco ao botão e simula a origem "janela ficou ativa".
    btn->setFocus(Qt::ActiveWindowFocusReason);
    // setFocus já gera o FocusIn com esse reason; o filtro global deve ter agido.
    QApplication::processEvents();

    QVERIFY2(!btn->hasFocus(),
             "FocusVisibleFilter NÃO limpou o foco automático (ActiveWindow) do "
             "#iconButton — a caixa :focus apareceria no 1º render.");
}

// ── Contrato 2: foco de teclado preserva o anel (a11y) ───────────────────────
// Tab/Backtab/Shortcut são interação real do usuário: o anel DEVE aparecer.
// O filtro não pode roubar esse foco, senão a navegação por teclado quebra.
void TestIconBtnRender::focoDeTecladoMantemAnelNoIconButton()
{
    for (Qt::FocusReason r : { Qt::TabFocusReason,
                               Qt::BacktabFocusReason,
                               Qt::ShortcutFocusReason }) {
        QWidget host;
        auto *btn = new QPushButton(&host);
        btn->setObjectName(QStringLiteral("iconButton"));
        btn->setFocusPolicy(Qt::StrongFocus);
        auto *lay = new QHBoxLayout(&host);
        lay->addWidget(btn);
        host.show();
        QVERIFY(QTest::qWaitForWindowExposed(&host));

        btn->setFocus(r);
        QApplication::processEvents();

        QVERIFY2(btn->hasFocus(),
                 qPrintable(QStringLiteral(
                     "FocusVisibleFilter removeu indevidamente o foco de TECLADO "
                     "(reason=%1) do #iconButton — quebraria a navegação por Tab "
                     "(acessibilidade).").arg(int(r))));
    }
}

// ── Contrato 3: o filtro só age em #iconButton ───────────────────────────────
// Um widget comum (sem objectName "iconButton") que recebe foco automático deve
// manter o foco — o filtro é cirúrgico, não global a todos os widgets.
void TestIconBtnRender::focoAutomaticoNaoMexeEmWidgetComum()
{
    QWidget host;
    auto *btn = new QPushButton(&host);
    btn->setObjectName(QStringLiteral("primary")); // NÃO é iconButton
    btn->setFocusPolicy(Qt::StrongFocus);
    auto *lay = new QHBoxLayout(&host);
    lay->addWidget(btn);
    host.show();
    QVERIFY(QTest::qWaitForWindowExposed(&host));

    btn->setFocus(Qt::ActiveWindowFocusReason);
    QApplication::processEvents();

    QVERIFY2(btn->hasFocus(),
             "FocusVisibleFilter interferiu num widget que NÃO é #iconButton — "
             "o filtro deve ser cirúrgico (só #iconButton).");
}

QTEST_MAIN(TestIconBtnRender)
#include "test_iconbtn_render.moc"
