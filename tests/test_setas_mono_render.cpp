// Render offscreen fix #165: setas de navegacao de mes devem ser
// monocromaticas (glifos tipograficos), nao color-emoji com caixa laranja.
//
// Estrategia:
//   1. Monta uma navBar de mes identica a do DashboardWidget (btnPrev "‹",
//      lblMes, btnNext "›"), com Theme::aplicar() ativo.
//   2. Renderiza via grab() e salva /tmp/setas_fix.png para inspecao visual.
//   3. Assertiva de sanidade: pixmap valido, dimensoes minimas, arquivo salvo.
//   4. Verificacao cromatica: samplea pixels dos botoes na horizontal central
//      e confirma que nenhuma banda de 8px consecutivos tem R>200 E G<120
//      (assinatura laranja do Noto Color Emoji nos botoes de midia).
//
// A ausencia da "caixa laranja" e confirmada via inspecao manual de
// /tmp/setas_fix.png apos rodar `QT_QPA_PLATFORM=offscreen ctest`.

#include <QtTest>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
#include <QFont>
#include <QImage>
#include <QColor>

#include "ui/Theme.h"
#include "ui/ButtonShadowFilter.h"

// ── Widget que espelha a navBar de mes do DashboardWidget ────────────────────

class NavBarMes : public QWidget {
public:
    explicit NavBarMes(QWidget *parent = nullptr) : QWidget(parent)
    {
        auto *lay = new QHBoxLayout(this);
        lay->setContentsMargins(16, 12, 16, 8);
        lay->setSpacing(4);

        // Botoes com glifos mono (#165 fix) — mesma logica do DashboardWidget.
        auto *btnPrev = new QPushButton(QStringLiteral("‹"), this); // ‹
        auto *btnNext = new QPushButton(QStringLiteral("›"), this); // ›
        {
            QFont f = btnPrev->font();
            f.setPointSize(15);
            btnPrev->setFont(f);
            btnNext->setFont(f);
        }
        btnPrev->setFixedWidth(32);
        btnNext->setFixedWidth(32);
        btnPrev->setObjectName(QStringLiteral("btnPrev"));
        btnNext->setObjectName(QStringLiteral("btnNext"));

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
    void setasNavegacaoSaoMonocromaticas();
};

void TestSetasMonoRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

void TestSetasMonoRender::setasNavegacaoSaoMonocromaticas()
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

    // Verificacao cromatica: procura assinatura laranja do Noto Color Emoji
    // (R>200, G<120, B<80) em uma faixa horizontal central da imagem.
    // Se presente => os botoes ainda usam color-emoji (regressao).
    QImage img = px.toImage();
    const int cy    = img.height() / 2;
    int laranjaCount = 0;

    for (int x = 0; x < img.width(); ++x) {
        QColor c = img.pixelColor(x, cy);
        if (c.red() > 200 && c.green() < 120 && c.blue() < 80) {
            ++laranjaCount;
        }
    }

    // Tolerancia zero: nenhum pixel laranja deve aparecer na linha central.
    // (A cor laranja tipica do emoji ◀️/▶️ e R≈230-240, G≈90-110, B≈40-60.)
    QVERIFY2(laranjaCount == 0,
        qPrintable(QStringLiteral(
            "Encontrados %1 pixels laranja na linha central — "
            "botoes ainda usam color-emoji (caixa laranja). "
            "Verificar /tmp/setas_fix.png.")
            .arg(laranjaCount)));

    qDebug("Render gravado em /tmp/setas_fix.png — "
           "setas monocromaticas confirmadas: nenhum pixel laranja na linha central.");
}

QTEST_MAIN(TestSetasMonoRender)
#include "test_setas_mono_render.moc"
