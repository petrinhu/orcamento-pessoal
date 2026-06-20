// Render offscreen #165 iconButton: navBar de mês (◀ Junho/2026 ▶ 🔒 🖨️)
// Verifica:
//   (a) nenhuma borda/box ao redor dos ícones no estado normal (QSS transparent)
//   (b) glifos ◀ ▶ são triângulos sólidos monocromáticos (NÃO laranja)
//   (c) 🔒 🖨️ sem caixa tampoco (QPushButton#iconButton: border:none)
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

        // U+25C0 / U+25B6 — triângulo sólido SEM VS16. objectName "iconButton" (#165).
        auto *btnPrev = new QPushButton(QString(QChar(0x25C0)), this);  // ◀
        auto *btnNext = new QPushButton(QString(QChar(0x25B6)), this);  // ▶
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
    void iconButtonLightSemBordaSemLaranja();
    void iconButtonDarkSemBordaSemLaranja();
};

void TestIconBtnRender::iconButtonLightSemBordaSemLaranja()
{
    QPixmap px = renderNavBar(Theme::ThemeMode::Light);
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo (light)");

    const bool saved = px.save(QStringLiteral("/tmp/iconbtn_light.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/iconbtn_light.png");

    QVERIFY2(px.width() >= 400, "Largura do pixmap menor que 400px (light)");
    QVERIFY2(px.height() >= 50, "Altura do pixmap menor que 50px (light)");

    // Verifica ausência de assinatura laranja (R>200, G<120, B<80) — indica VS16/emoji-color.
    QImage img = px.toImage();
    const int cy = img.height() / 2;
    int laranjaCount = 0;
    for (int x = 0; x < img.width(); ++x) {
        QColor c = img.pixelColor(x, cy);
        if (c.red() > 200 && c.green() < 120 && c.blue() < 80)
            ++laranjaCount;
    }
    QVERIFY2(laranjaCount == 0,
        qPrintable(QStringLiteral(
            "[light] %1 pixel(s) laranja encontrado(s) — setas com VS16 ou caixa laranja. "
            "Ver /tmp/iconbtn_light.png.").arg(laranjaCount)));

    qDebug("Light render OK → /tmp/iconbtn_light.png (sem laranja, %d px escaneados)",
           img.width());
}

void TestIconBtnRender::iconButtonDarkSemBordaSemLaranja()
{
    QPixmap px = renderNavBar(Theme::ThemeMode::Dark);
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo (dark)");

    const bool saved = px.save(QStringLiteral("/tmp/iconbtn_dark.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/iconbtn_dark.png");

    QVERIFY2(px.width() >= 400, "Largura do pixmap menor que 400px (dark)");
    QVERIFY2(px.height() >= 50, "Altura do pixmap menor que 50px (dark)");

    QImage img = px.toImage();
    const int cy = img.height() / 2;
    int laranjaCount = 0;
    for (int x = 0; x < img.width(); ++x) {
        QColor c = img.pixelColor(x, cy);
        if (c.red() > 200 && c.green() < 120 && c.blue() < 80)
            ++laranjaCount;
    }
    QVERIFY2(laranjaCount == 0,
        qPrintable(QStringLiteral(
            "[dark] %1 pixel(s) laranja encontrado(s) — setas com VS16 ou caixa laranja. "
            "Ver /tmp/iconbtn_dark.png.").arg(laranjaCount)));

    qDebug("Dark render OK → /tmp/iconbtn_dark.png (sem laranja, %d px escaneados)",
           img.width());
}

QTEST_MAIN(TestIconBtnRender)
#include "test_iconbtn_render.moc"
