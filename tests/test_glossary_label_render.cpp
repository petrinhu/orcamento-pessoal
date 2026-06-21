// PeuBud - Render offscreen do GlossaryLabel (#166 Peca 3 §9.6)
//
// Verifica que o sublinhado pontilhado e pintado (diferenca de pixel entre
// GlossaryLabel e QLabel comum) e gera PNGs para aprovacao visual:
//   /tmp/glossary_cue_light.png
//   /tmp/glossary_cue_dark.png
//
// O pontilhado e desenhado pelo proprio widget (paintEvent com QPainter) —
// offscreen e fiel aqui, sem dependencia de compositor.
//
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QFormLayout>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>

#include "ui/Theme.h"
#include "ui/widgets/GlossaryLabel.h"

class TestGlossaryLabelRender : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    // Smoke: render nao-nulo em light
    void renderLight_naoNulo();
    // Smoke: render nao-nulo em dark
    void renderDark_naoNulo();
    // Diferenca de pixel: GlossaryLabel deve ser diferente de QLabel comum
    // (prova que o pontilhado realmente foi pintado).
    void glossaryLabel_difereDePlainLabel_light();
    void glossaryLabel_difereDePlainLabel_dark();
};

// ── Helper: demo widget ───────────────────────────────────────────────────────

static QWidget *criarDemoWidget(QWidget *parent = nullptr)
{
    auto *root = new QWidget(parent);
    root->setObjectName(QStringLiteral("surface"));
    auto *vbox = new QVBoxLayout(root);
    vbox->setContentsMargins(24, 24, 24, 24);
    vbox->setSpacing(12);

    // Titulo
    auto *titulo = new QLabel(QStringLiteral("Cue estatico de glossario (§9.6)"), root);
    titulo->setObjectName(QStringLiteral("subtitle"));
    vbox->addWidget(titulo);

    // Form layout simulando a CalculadoraDialog (Financeira)
    auto *form = new QFormLayout;
    form->setSpacing(8);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // Termos TVM com GlossaryLabel (curados)
    form->addRow(new GlossaryLabel(QStringLiteral("N:"),    root), new QLabel(QStringLiteral("—"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("i/mês:"),root), new QLabel(QStringLiteral("—"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("PV:"),   root), new QLabel(QStringLiteral("—"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("PMT:"),  root), new QLabel(QStringLiteral("—"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("FV:"),   root), new QLabel(QStringLiteral("—"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("CET:"),  root), new QLabel(QStringLiteral("—"), root));

    // Separador visual
    auto *sep = new QLabel(QStringLiteral("─────────────────────────────────"), root);
    sep->setObjectName(QStringLiteral("subtitle"));
    sep->setAlignment(Qt::AlignCenter);

    // Outros termos curados (investimentos)
    form->addRow(new QLabel(QStringLiteral(""), root), sep);
    form->addRow(new GlossaryLabel(QStringLiteral("CDI:"),  root), new QLabel(QStringLiteral("100%"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("SELIC:"),root), new QLabel(QStringLiteral("13,75%"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("IOF:"),  root), new QLabel(QStringLiteral("0%"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("IRRF:"), root), new QLabel(QStringLiteral("15%"), root));
    form->addRow(new GlossaryLabel(QStringLiteral("DY:"),   root), new QLabel(QStringLiteral("—"), root));

    vbox->addLayout(form);

    // Nota de rodape
    auto *nota = new QLabel(
        QStringLiteral("Termos sublinhados: passe o mouse para definicao"),
        root);
    nota->setObjectName(QStringLiteral("subtitle"));
    vbox->addWidget(nota);

    root->setMinimumSize(400, 360);
    return root;
}

// ── initTestCase ──────────────────────────────────────────────────────────────

void TestGlossaryLabelRender::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── Smoke de render ───────────────────────────────────────────────────────────

void TestGlossaryLabelRender::renderLight_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    auto *demo = criarDemoWidget();
    demo->show();
    QApplication::processEvents();
    const QPixmap pm = demo->grab();
    QVERIFY2(!pm.isNull(), "Render light nulo");
    pm.save(QStringLiteral("/tmp/glossary_cue_light.png"));
    delete demo;
}

void TestGlossaryLabelRender::renderDark_naoNulo()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();

    auto *demo = criarDemoWidget();
    demo->show();
    QApplication::processEvents();
    const QPixmap pm = demo->grab();
    QVERIFY2(!pm.isNull(), "Render dark nulo");
    pm.save(QStringLiteral("/tmp/glossary_cue_dark.png"));
    delete demo;

    // Restaura light
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── Diferenca de pixel (prova o pontilhado) ───────────────────────────────────
//
// Compara o render de GlossaryLabel("CET:") com QLabel("CET:") — devem diferir
// em pelo menos um pixel (o sublinhado pontilhado). Se forem identicos, o
// paintEvent nao esta pintando o pontilhado.

static QPixmap renderSingleLabel(QLabel *lbl)
{
    lbl->setMinimumSize(80, 28);
    lbl->resize(80, 28);
    lbl->show();
    QApplication::processEvents();
    return lbl->grab();
}

void TestGlossaryLabelRender::glossaryLabel_difereDePlainLabel_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    auto *glbl = new GlossaryLabel(QStringLiteral("CET:"));
    auto *plain = new QLabel(QStringLiteral("CET:"));

    const QPixmap pmGloss = renderSingleLabel(glbl);
    const QPixmap pmPlain = renderSingleLabel(plain);

    // Converte para imagem e compara os bits
    const QImage imgGloss = pmGloss.toImage();
    const QImage imgPlain = pmPlain.toImage();

    QCOMPARE(imgGloss.size(), imgPlain.size()); // pre-condicao

    bool diferente = false;
    for (int y = 0; y < imgGloss.height() && !diferente; ++y) {
        for (int x = 0; x < imgGloss.width() && !diferente; ++x) {
            if (imgGloss.pixel(x, y) != imgPlain.pixel(x, y))
                diferente = true;
        }
    }

    QVERIFY2(diferente,
        "GlossaryLabel deve diferir do QLabel comum (sublinhado pontilhado "
        "nao foi pintado — verificar paintEvent)");

    delete glbl;
    delete plain;
}

void TestGlossaryLabelRender::glossaryLabel_difereDePlainLabel_dark()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();

    auto *glbl = new GlossaryLabel(QStringLiteral("PMT:"));
    auto *plain = new QLabel(QStringLiteral("PMT:"));

    const QPixmap pmGloss = renderSingleLabel(glbl);
    const QPixmap pmPlain = renderSingleLabel(plain);

    const QImage imgGloss = pmGloss.toImage();
    const QImage imgPlain = pmPlain.toImage();

    QCOMPARE(imgGloss.size(), imgPlain.size());

    bool diferente = false;
    for (int y = 0; y < imgGloss.height() && !diferente; ++y) {
        for (int x = 0; x < imgGloss.width() && !diferente; ++x) {
            if (imgGloss.pixel(x, y) != imgPlain.pixel(x, y))
                diferente = true;
        }
    }

    QVERIFY2(diferente,
        "GlossaryLabel dark deve diferir do QLabel comum (pontilhado ausente)");

    delete glbl;
    delete plain;

    // Restaura light
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

QTEST_MAIN(TestGlossaryLabelRender)
#include "test_glossary_label_render.moc"
