// Render offscreen #165 iconButton: navBar de mês (◀️ Junho/2026 ▶️ 🔒 🖨️)
// Verifica:
//   (a) pixmap válido e com dimensões mínimas (render não falhou)
//   (b) arquivo salvo para aprovação visual manual
//
// Histórico: a versão anterior verificava AUSÊNCIA de laranja nos botões ◀▶
// (glifos mono). Com a decisão #165 (revert), as setas voltam a ser emoji
// laranja via Theme::emojiIcon — a assertiva de "sem laranja" foi removida pois
// agora as setas SÃO intencionalmente laranjas.
//
// A ausência de box/borda é garantida pelo QSS #iconButton (border:none;
// background:transparent) — verificado via aprovação visual de
// /tmp/iconbtn_light.png e /tmp/iconbtn_dark.png.
//
// Salva /tmp/iconbtn_light.png e /tmp/iconbtn_dark.png para inspeção visual.

#include <QtTest>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
#include <QImage>
#include <QColor>

#include "ui/Theme.h"
#include "ui/ButtonShadowFilter.h"

// ── Widget que espelha a navBar de mês com os 4 botões do DashboardWidget ────

class NavBarIconBtn : public QWidget {
public:
    explicit NavBarIconBtn(QWidget *parent = nullptr) : QWidget(parent)
    {
        setObjectName(QStringLiteral("navBarIconBtn"));
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(16, 12, 16, 8);
        lay->setSpacing(4);

        // ◀️ ▶️ com VS16 — emoji laranja via emojiIcon. objectName "iconButton" (#165).
        auto *btnPrev = new QPushButton(this);
        auto *btnNext = new QPushButton(this);
        btnPrev->setIcon(Theme::emojiIcon(QStringLiteral("◀️")));
        btnNext->setIcon(Theme::emojiIcon(QStringLiteral("▶️")));
        btnPrev->setObjectName(QStringLiteral("iconButton"));
        btnNext->setObjectName(QStringLiteral("iconButton"));
        btnPrev->setToolTip(QStringLiteral("Mês anterior"));
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

        lay->addWidget(btnPrev);
        lay->addWidget(lblMes, 1);
        lay->addWidget(btnNext);
        lay->addWidget(btnFechar);
        lay->addWidget(btnPrint);
    }
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

// ── Test class ────────────────────────────────────────────────────────────────

class TestIconBtnRender : public QObject {
    Q_OBJECT

private slots:
    void iconButtonLightRendersOk();
    void iconButtonDarkRendersOk();
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

QTEST_MAIN(TestIconBtnRender)
#include "test_iconbtn_render.moc"
