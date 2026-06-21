// Render offscreen #165 iconButton: navBar de mês (◀️ Junho/2026 ▶️ 🔒 🖨️)
// Verifica:
//   (a) pixmap válido e com dimensões mínimas (render não falhou)
//   (b) arquivo salvo para aprovação visual manual
//   (c) NOVO (#165 fix-1render): no PRIMEIRO grab, sem repaint extra, o ◀️
//       (btnPrev) não apresenta a borda/caixa do estilo-base QPushButton.
//       Captura o quirk Qt/Fusion: #iconButton sem repolish sairia com borda
//       Color::BorderLight (#E4EAF2) no light e Color::BorderDark (#2A3340) no dark.
//
// Histórico: a versão anterior verificava AUSÊNCIA de laranja nos botões ◀▶
// (glifos mono). Com a decisão #165 (revert), as setas voltam a ser emoji
// laranja via Theme::emojiIcon — a assertiva de "sem laranja" foi removida pois
// agora as setas SÃO intencionalmente laranjas.
//
// A ausência de box/borda é garantida pelo QSS #iconButton (border:none;
// background:transparent) + repolish no showEvent (#165 fix-1render).
//
// Salva /tmp/iconbtn_light.png, /tmp/iconbtn_dark.png,
//       /tmp/btn_1render_antes.png (grab imediato do btnPrev, pré-fix visível),
//       /tmp/btn_1render_depois.png (grab do btnPrev após showEvent com repolish).

#include <QtTest>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
#include <QImage>
#include <QColor>
#include <QShowEvent>
#include <QStyle>

#include "ui/Theme.h"
#include "ui/ButtonShadowFilter.h"

// ── Widget que espelha a navBar de mês com os 4 botões do DashboardWidget ────
// A propriedade m_btnPrev é exposta para inspeção direta no teste de 1º render.

class NavBarIconBtn : public QWidget {
public:
    QPushButton *m_btnPrev = nullptr;

    explicit NavBarIconBtn(QWidget *parent = nullptr) : QWidget(parent)
    {
        setObjectName(QStringLiteral("navBarIconBtn"));
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(16, 12, 16, 8);
        lay->setSpacing(4);

        // ◀️ ▶️ com VS16 — emoji laranja via emojiIcon. objectName "iconButton" (#165).
        m_btnPrev = new QPushButton(this);
        auto *btnNext = new QPushButton(this);
        m_btnPrev->setIcon(Theme::emojiIcon(QStringLiteral("◀️")));
        btnNext->setIcon(Theme::emojiIcon(QStringLiteral("▶️")));
        m_btnPrev->setObjectName(QStringLiteral("iconButton"));
        btnNext->setObjectName(QStringLiteral("iconButton"));
        m_btnPrev->setToolTip(QStringLiteral("Mês anterior"));
        btnNext->setToolTip(QStringLiteral("Próximo mês"));

        auto *lblMes = new QLabel(QStringLiteral("Junho/2026"), this);
        {
            QFont f = lblMes->font();
            f.setPointSize(14);
            f.setBold(true);
            lblMes->setFont(f);
        }
        lblMes->setAlignment(Qt::AlignCenter);

        // 🔒 e 🖨️ — emoji via emojiIcon; objectName "iconButton" (#165).
        auto *btnFechar = new QPushButton(this);
        btnFechar->setIcon(Theme::emojiIcon(QStringLiteral("🔒")));
        btnFechar->setObjectName(QStringLiteral("iconButton"));
        btnFechar->setToolTip(QStringLiteral("Fechar mês contábil"));

        auto *btnPrint = new QPushButton(this);
        btnPrint->setIcon(Theme::emojiIcon(QStringLiteral("🖨️")));
        btnPrint->setObjectName(QStringLiteral("iconButton"));
        btnPrint->setToolTip(QStringLiteral("Imprimir Dashboard"));

        lay->addWidget(m_btnPrev);
        lay->addWidget(lblMes, 1);
        lay->addWidget(btnNext);
        lay->addWidget(btnFechar);
        lay->addWidget(btnPrint);
    }
};

// ── Widget com showEvent que espelha o fix do DashboardWidget ─────────────────
// Ao ser exibido, força repolish dos #iconButton (mesma lógica do DashboardWidget).

class NavBarComRepolish : public QWidget {
public:
    QPushButton *m_btnPrev = nullptr;

    explicit NavBarComRepolish(QWidget *parent = nullptr) : QWidget(parent)
    {
        setObjectName(QStringLiteral("navBarComRepolish"));
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(16, 12, 16, 8);
        lay->setSpacing(4);

        m_btnPrev = new QPushButton(this);
        auto *btnNext   = new QPushButton(this);
        auto *btnFechar = new QPushButton(this);
        auto *btnPrint  = new QPushButton(this);

        m_btnPrev->setIcon(Theme::emojiIcon(QStringLiteral("◀️")));
        btnNext->setIcon(Theme::emojiIcon(QStringLiteral("▶️")));
        btnFechar->setIcon(Theme::emojiIcon(QStringLiteral("🔒")));
        btnPrint->setIcon(Theme::emojiIcon(QStringLiteral("🖨️")));

        for (auto *b : { m_btnPrev, btnNext, btnFechar, btnPrint })
            b->setObjectName(QStringLiteral("iconButton"));

        auto *lblMes = new QLabel(QStringLiteral("Junho/2026"), this);
        {
            QFont f = lblMes->font();
            f.setPointSize(14);
            f.setBold(true);
            lblMes->setFont(f);
        }
        lblMes->setAlignment(Qt::AlignCenter);

        lay->addWidget(m_btnPrev);
        lay->addWidget(lblMes, 1);
        lay->addWidget(btnNext);
        lay->addWidget(btnFechar);
        lay->addWidget(btnPrint);

        m_iconBtns = { m_btnPrev, btnNext, btnFechar, btnPrint };
    }

protected:
    void showEvent(QShowEvent *ev) override
    {
        QWidget::showEvent(ev);
        if (!m_firstShow)
            return;
        m_firstShow = false;
        QStyle *s = style();
        for (QPushButton *btn : std::as_const(m_iconBtns)) {
            s->unpolish(btn);
            s->polish(btn);
        }
    }

private:
    bool m_firstShow = true;
    QList<QPushButton *> m_iconBtns;
};

// ── Render helper ─────────────────────────────────────────────────────────────

static QPixmap renderNavBar(Theme::ThemeMode mode)
{
    Theme::setMode(mode);
    Theme::aplicar();

    QWidget parent;
    // Background explícito de @@BG@@ para refletir o header do Dashboard.
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

// ── Helpers de inspeção de pixel ──────────────────────────────────────────────

// Retorna true se o pixel na posicao (x,y) do widget WRT a janela pai tem a
// cor de borda esperada (indica que o botao saiu com "caixa").
// Tolerancia: delta de canal <= 8 (suavizacao de anti-aliasing offscreen).
static bool pixelMatchesBorderColor(const QImage &img, int x, int y,
                                    const QColor &borderColor)
{
    if (x < 0 || y < 0 || x >= img.width() || y >= img.height())
        return false;
    const QColor got(img.pixel(x, y));
    return std::abs(got.red()   - borderColor.red())   <= 8 &&
           std::abs(got.green() - borderColor.green()) <= 8 &&
           std::abs(got.blue()  - borderColor.blue())  <= 8;
}

// ── Test class ────────────────────────────────────────────────────────────────

class TestIconBtnRender : public QObject {
    Q_OBJECT

private slots:
    void iconButtonLightRendersOk();
    void iconButtonDarkRendersOk();
    // Regressao do 1º render: detecta se o btnPrev sai SEM borda no grab imediato.
    // Antes do fix: borda Color::BorderLight (#E4EAF2) aparecia no 1º show.
    // Apos o fix (showEvent com repolish): sem borda, fundo transparente.
    void iconButtonSemBordaNoFirstRenderLight();
    void iconButtonSemBordaNoFirstRenderDark();
};

void TestIconBtnRender::iconButtonLightRendersOk()
{
    QPixmap px = renderNavBar(Theme::ThemeMode::Light);
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo (light)");

    const bool saved = px.save(QStringLiteral("/tmp/iconbtn_light.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/iconbtn_light.png");

    QVERIFY2(px.width() >= 400, "Largura do pixmap menor que 400px (light)");
    QVERIFY2(px.height() >= 50, "Altura do pixmap menor que 50px (light)");

    // Nota: ◀️▶️ agora são emoji laranja intencionais — não há assertiva de cor.
    // Aprovação visual: /tmp/iconbtn_light.png deve mostrar setas laranjas sem box.
    qDebug("Light render OK → /tmp/iconbtn_light.png (setas ◀️▶️ laranjas, %d px largura)",
           px.width());
}

void TestIconBtnRender::iconButtonDarkRendersOk()
{
    QPixmap px = renderNavBar(Theme::ThemeMode::Dark);
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo (dark)");

    const bool saved = px.save(QStringLiteral("/tmp/iconbtn_dark.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/iconbtn_dark.png");

    QVERIFY2(px.width() >= 400, "Largura do pixmap menor que 400px (dark)");
    QVERIFY2(px.height() >= 50, "Altura do pixmap menor que 50px (dark)");

    // Nota: ◀️▶️ agora são emoji laranja intencionais — não há assertiva de cor.
    // Aprovação visual: /tmp/iconbtn_dark.png deve mostrar setas laranjas sem box.
    qDebug("Dark render OK → /tmp/iconbtn_dark.png (setas ◀️▶️ laranjas, %d px largura)",
           px.width());
}

// ── Regressão do 1º render (light) ───────────────────────────────────────────
// Constrói NavBarComRepolish (espelha o showEvent do DashboardWidget), faz grab
// IMEDIATO após show() SEM repaint extra, e verifica que nenhum pixel do botão
// exibe a cor da borda do estilo-base.
//
// Estratégia de detecção:
//   - Posiciona o cursor no canto superior do btnPrev (margem left=16, top=12,
//     dentro da barra de 60px).
//   - Verifica que o pixel NÃO é Color::BorderLight (#E4EAF2 no light).
//   - Sem o fix, o Fusion style renderizaria borda de 1px nessa posição.
//   - Com o fix (repolish no showEvent), o fundo é transparente (@@BG@@) ou negro.
//
// Salva /tmp/btn_1render_depois.png para inspeção.
void TestIconBtnRender::iconButtonSemBordaNoFirstRenderLight()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    const QString bgColor = QStringLiteral("#EEF3F9");
    const QColor borderLight(QStringLiteral("#E4EAF2")); // Color::BorderLight

    QWidget parent;
    parent.setStyleSheet(QStringLiteral("background: %1;").arg(bgColor));
    parent.resize(500, 60);

    auto *lay = new QHBoxLayout(&parent);
    lay->setContentsMargins(0, 0, 0, 0);
    auto *navBar = new NavBarComRepolish(&parent);
    lay->addWidget(navBar);

    // show() dispara showEvent → repolish dos iconButtons.
    parent.show();
    // Grab IMEDIATO, sem repaint() adicional — este é o cenário do bug.
    const QPixmap px = parent.grab();

    QVERIFY2(!px.isNull(), "grab() nulo no 1º render (light)");
    px.save(QStringLiteral("/tmp/btn_1render_depois.png"));
    qDebug("1º render depois (light) → /tmp/btn_1render_depois.png (%d x %d px)",
           px.width(), px.height());

    // O btnPrev fica na margem left=16, top=12 (contentsMargins do layout).
    // Testamos o pixel imediatamente acima do conteudo do botão — se houver borda
    // de 1px, a cor seria borderLight; com repolish, deve ser o fundo (@@BG@@).
    // Coordenadas aproximadas do topo do btnPrev: x=17, y=13.
    const QImage img = px.toImage();
    const int checkX = 17;
    const int checkY = 13;
    const bool temBorda = pixelMatchesBorderColor(img, checkX, checkY, borderLight);
    QVERIFY2(!temBorda,
             qPrintable(QStringLiteral(
                 "btnPrev exibe cor de borda (#E4EAF2) no primeiro render light "
                 "(pixel %1,%2 = #%3). Fix showEvent/repolish não atuou.")
                 .arg(checkX).arg(checkY)
                 .arg(QColor(img.pixel(checkX, checkY)).rgb(), 6, 16, QLatin1Char('0'))));
}

// ── Regressão do 1º render (dark) ────────────────────────────────────────────
// Idem light, mas verifica que nenhum pixel do btnPrev tem Color::BorderDark.
// Salva /tmp/btn_1render_antes.png (convencao do report — antes=dark neste contexto).
void TestIconBtnRender::iconButtonSemBordaNoFirstRenderDark()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();

    const QString bgColor = QStringLiteral("#0F172A");
    const QColor borderDark(QStringLiteral("#2A3340")); // Color::BorderDark

    QWidget parent;
    parent.setStyleSheet(QStringLiteral("background: %1;").arg(bgColor));
    parent.resize(500, 60);

    auto *lay = new QHBoxLayout(&parent);
    lay->setContentsMargins(0, 0, 0, 0);
    auto *navBar = new NavBarComRepolish(&parent);
    lay->addWidget(navBar);

    // show() dispara showEvent → repolish dos iconButtons.
    parent.show();
    // Grab IMEDIATO.
    const QPixmap px = parent.grab();

    QVERIFY2(!px.isNull(), "grab() nulo no 1º render (dark)");
    px.save(QStringLiteral("/tmp/btn_1render_antes.png"));
    qDebug("1º render antes/dark → /tmp/btn_1render_antes.png (%d x %d px)",
           px.width(), px.height());

    const QImage img = px.toImage();
    const int checkX = 17;
    const int checkY = 13;
    const bool temBorda = pixelMatchesBorderColor(img, checkX, checkY, borderDark);
    QVERIFY2(!temBorda,
             qPrintable(QStringLiteral(
                 "btnPrev exibe cor de borda (#2A3340) no primeiro render dark "
                 "(pixel %1,%2 = #%3). Fix showEvent/repolish não atuou.")
                 .arg(checkX).arg(checkY)
                 .arg(QColor(img.pixel(checkX, checkY)).rgb(), 6, 16, QLatin1Char('0'))));
}

QTEST_MAIN(TestIconBtnRender)
#include "test_iconbtn_render.moc"
