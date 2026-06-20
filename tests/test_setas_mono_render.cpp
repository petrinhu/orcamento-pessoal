// Render offscreen sanidade: navBar de mês com btnPrev/btnNext via emojiIcon.
//
// Histórico: este teste verificava que glifos ‹/› (mono) não geravam assinatura
// laranja. Com a decisão #165 (revert), as setas ◀️▶️ voltam a ser emoji laranja
// via Theme::emojiIcon — portanto a verificação de "sem laranja" não faz mais
// sentido e foi removida.
//
// O teste agora verifica apenas sanidade de render:
//   (a) pixmap válido e com dimensões mínimas
//   (b) arquivo /tmp/setas_fix.png salvo (aprovação visual manual)
//
// A ausência de box/borda é coberta por test_iconbtn_render (assertiva de
// pixels de cor sólida uniforme nas bordas do botão).

#include <QtTest>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
#include <QFont>

#include "ui/Theme.h"
#include "ui/ButtonShadowFilter.h"

// ── Widget que espelha a navBar de mês com emoji laranja ─────────────────────

class NavBarMes : public QWidget {
public:
    explicit NavBarMes(QWidget *parent = nullptr) : QWidget(parent)
    {
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(16, 12, 16, 8);
        lay->setSpacing(4);

        // ◀️ ▶️ com VS16 — emoji laranja via emojiIcon (padrão do app após #165).
        auto *btnPrev = new QPushButton(this);
        auto *btnNext = new QPushButton(this);
        btnPrev->setIcon(Theme::emojiIcon(QStringLiteral("◀️")));
        btnNext->setIcon(Theme::emojiIcon(QStringLiteral("▶️")));
        btnPrev->setObjectName(QStringLiteral("iconButton"));
        btnNext->setObjectName(QStringLiteral("iconButton"));

        auto *lblMes = new QLabel(QStringLiteral("junho 2026"), this);
        {
            QFont f = lblMes->font();
            f.setPointSize(14);
            f.setBold(true);
            lblMes->setFont(f);
        }
        lblMes->setAlignment(Qt::AlignCenter);

        lay->addWidget(btnPrev);
        lay->addWidget(lblMes, 1);
        lay->addWidget(btnNext);
    }
};

// ── Test class ───────────────────────────────────────────────────────────────

class TestSetasMonoRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void navBarRendersOk();
};

void TestSetasMonoRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

void TestSetasMonoRender::navBarRendersOk()
{
    // Monta a navBar em widget pai com tamanho suficiente para os botoes renderizarem.
    QWidget parent;
    parent.resize(400, 60);

    auto *lay = new QHBoxLayout(&parent);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(new NavBarMes(&parent));

    parent.show();
    parent.repaint();

    // Grab e sanidade basica.
    QPixmap px = parent.grab();
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo — render offscreen falhou");

    const bool saved = px.save(QStringLiteral("/tmp/setas_fix.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/setas_fix.png");

    QVERIFY2(px.width()  >= 400, "Largura do pixmap menor que 400px — inesperado");
    QVERIFY2(px.height() >= 60,  "Altura do pixmap menor que 60px — inesperado");

    qDebug("Render gravado em /tmp/setas_fix.png — "
           "setas ◀️▶️ emoji laranja via emojiIcon (#165 revert).");
}

QTEST_MAIN(TestSetasMonoRender)
#include "test_setas_mono_render.moc"
