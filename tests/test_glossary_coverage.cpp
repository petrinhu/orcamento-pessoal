// PeuBud - Cobertura AMPLA do cue de glossario (#166 §9.6)
//
// GlossarioDecorator::aplicarTooltips agora instala o sublinhado pontilhado
// (via GlossaryUnderlineFilter) nos labels-chave curtos da MESMA varredura que
// ja seta os tooltips. Contrato:
//   (1) label-chave curto (texto ~= termo) recebe propriedade "glossarioUnderlined".
//   (2) prosa LONGA com termo no meio NAO recebe sublinhado (so tooltip).
//   (3) label sem termo nao recebe nada.
//   (4) idempotente: 2a chamada nao altera estado.
//   (5) nao-destrutivo: text() intacto em todos os casos.
//   (6) tooltip continua sendo setado (regressao do comportamento anterior).
//   (7) render: label decorado difere em pixel de label nao-decorado (pontilhado).
//
// Salva /tmp/glossary_coverage_light.png e _dark.png para aprovacao visual.
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QFormLayout>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QWidget>

#include "services/GlossarioService.h"
#include "ui/GlossarioDecorator.h"
#include "ui/Theme.h"

class TestGlossaryCoverage : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    // (1) label-chave curto recebe sublinhado
    void labelChaveCurtoRecebeSublinhado();
    void labelComDoisPontosRecebeSublinhado();
    void labelTermoComSufixoCurtoRecebeSublinhado();

    // (2) prosa longa NAO recebe sublinhado
    void prosaLongaNaoRecebeSublinhado();

    // (3) label sem termo nao recebe nada
    void labelSemTermoNaoRecebeSublinhado();

    // (4) idempotencia
    void idempotente();

    // (5) nao-destrutivo
    void naoDestrutivoTextIntacto();
    void prosaLongaTextIntacto();

    // (6) tooltip continua (regressao)
    void tooltipContinuaSetado();
    void prosaLongaAindaTemTooltip();

    // (7) render diff de pixel + PNGs
    void renderDecoradoDifereDeNaoDecorado();
    void renderCoberturaLightDark();
};

// ── helper ────────────────────────────────────────────────────────────────────

static QPixmap grabLabel(QLabel *lbl)
{
    lbl->setMinimumSize(120, 28);
    lbl->resize(120, 28);
    lbl->show();
    QApplication::processEvents();
    return lbl->grab();
}

static bool imagensDiferem(const QImage &a, const QImage &b)
{
    if (a.size() != b.size()) return true;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x)
            if (a.pixel(x, y) != b.pixel(x, y))
                return true;
    return false;
}

void TestGlossaryCoverage::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── (1) label-chave curto ─────────────────────────────────────────────────────

void TestGlossaryCoverage::labelChaveCurtoRecebeSublinhado()
{
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("CET"), &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QVERIFY2(lbl->property("glossarioUnderlined").toBool(),
             "label-chave curto 'CET' deveria receber sublinhado");
}

void TestGlossaryCoverage::labelComDoisPontosRecebeSublinhado()
{
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("CDI:"), &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QVERIFY(lbl->property("glossarioUnderlined").toBool());
}

void TestGlossaryCoverage::labelTermoComSufixoCurtoRecebeSublinhado()
{
    // "Taxa CDI" = 8 chars, <= 28 -> deve sublinhar
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Taxa CDI"), &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QVERIFY(lbl->property("glossarioUnderlined").toBool());
}

// ── (2) prosa longa ───────────────────────────────────────────────────────────

void TestGlossaryCoverage::prosaLongaNaoRecebeSublinhado()
{
    // Frase longa (> 28 chars) onde CET cobre < 55% -> so tooltip, sem sublinhado.
    QWidget root;
    auto *lbl = new QLabel(
        QStringLiteral("O CET considera juros, tarifas e impostos do financiamento"),
        &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QVERIFY2(!lbl->property("glossarioUnderlined").toBool(),
             "prosa longa NAO deve receber sublinhado (so tooltip)");
}

// ── (3) sem termo ─────────────────────────────────────────────────────────────

void TestGlossaryCoverage::labelSemTermoNaoRecebeSublinhado()
{
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Bom dia mundo"), &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QVERIFY(!lbl->property("glossarioUnderlined").toBool());
    QVERIFY(lbl->toolTip().isEmpty());
}

// ── (4) idempotencia ──────────────────────────────────────────────────────────

void TestGlossaryCoverage::idempotente()
{
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("PMT"), &root);
    GlossarioDecorator::aplicarTooltips(&root);
    const QString tip1 = lbl->toolTip();
    GlossarioDecorator::aplicarTooltips(&root); // 2a chamada
    QCOMPARE(lbl->toolTip(), tip1);
    QVERIFY(lbl->property("glossarioUnderlined").toBool());
}

// ── (5) nao-destrutivo ────────────────────────────────────────────────────────

void TestGlossaryCoverage::naoDestrutivoTextIntacto()
{
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("CET:"), &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QCOMPARE(lbl->text(), QStringLiteral("CET:"));
}

void TestGlossaryCoverage::prosaLongaTextIntacto()
{
    const QString prosa =
        QStringLiteral("O CET considera juros, tarifas e impostos do financiamento");
    QWidget root;
    auto *lbl = new QLabel(prosa, &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QCOMPARE(lbl->text(), prosa);
}

// ── (6) tooltip (regressao) ───────────────────────────────────────────────────

void TestGlossaryCoverage::tooltipContinuaSetado()
{
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("IOF"), &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QVERIFY2(!lbl->toolTip().isEmpty(),
             "tooltip deve continuar sendo setado (regressao)");
}

void TestGlossaryCoverage::prosaLongaAindaTemTooltip()
{
    // Mesmo sem sublinhado, prosa com termo deve ter tooltip.
    QWidget root;
    auto *lbl = new QLabel(
        QStringLiteral("O CET considera juros, tarifas e impostos do financiamento"),
        &root);
    GlossarioDecorator::aplicarTooltips(&root);
    QVERIFY2(!lbl->toolTip().isEmpty(),
             "prosa longa com termo deve ter tooltip (mesmo sem sublinhado)");
}

// ── (7) render ────────────────────────────────────────────────────────────────

void TestGlossaryCoverage::renderDecoradoDifereDeNaoDecorado()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    // Decorado via aplicarTooltips
    QWidget rootDec;
    auto *lblDec = new QLabel(QStringLiteral("CET:"), &rootDec);
    GlossarioDecorator::aplicarTooltips(&rootDec);
    QVERIFY(lblDec->property("glossarioUnderlined").toBool()); // pre-condicao

    // Nao-decorado (label cru)
    auto *lblPlain = new QLabel(QStringLiteral("CET:"));

    const QImage imgDec = grabLabel(lblDec).toImage();
    const QImage imgPlain = grabLabel(lblPlain).toImage();

    QVERIFY2(imagensDiferem(imgDec, imgPlain),
             "label decorado deve diferir em pixel do label cru (pontilhado ausente)");

    delete lblPlain;
}

void TestGlossaryCoverage::renderCoberturaLightDark()
{
    auto montar = [](QWidget *root) {
        auto *vbox = new QVBoxLayout(root);
        vbox->setContentsMargins(24, 24, 24, 24);
        vbox->setSpacing(10);
        auto *form = new QFormLayout;
        form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
        form->addRow(new QLabel(QStringLiteral("CET:")),   new QLabel(QStringLiteral("32,5%")));
        form->addRow(new QLabel(QStringLiteral("CDI:")),   new QLabel(QStringLiteral("100%")));
        form->addRow(new QLabel(QStringLiteral("SELIC:")), new QLabel(QStringLiteral("13,75%")));
        form->addRow(new QLabel(QStringLiteral("IOF:")),   new QLabel(QStringLiteral("0,38%")));
        form->addRow(new QLabel(QStringLiteral("DY:")),    new QLabel(QStringLiteral("8,2%")));
        form->addRow(new QLabel(QStringLiteral("Taxa CDB")), new QLabel(QStringLiteral("110%")));
        vbox->addLayout(form);
        // prosa: deve ficar SEM sublinhado
        auto *prosa = new QLabel(
            QStringLiteral("O CET inclui juros, tarifas e IOF do financiamento contratado"),
            root);
        prosa->setWordWrap(true);
        vbox->addWidget(prosa);
        root->setMinimumSize(420, 320);
    };

    // Light
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
    {
        QWidget root;
        root.setObjectName(QStringLiteral("surface"));
        montar(&root);
        GlossarioDecorator::aplicarTooltips(&root);
        root.show();
        QApplication::processEvents();
        const QPixmap pm = root.grab();
        QVERIFY(!pm.isNull());
        pm.save(QStringLiteral("/tmp/glossary_coverage_light.png"));
    }

    // Dark
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();
    {
        QWidget root;
        root.setObjectName(QStringLiteral("surface"));
        montar(&root);
        GlossarioDecorator::aplicarTooltips(&root);
        root.show();
        QApplication::processEvents();
        const QPixmap pm = root.grab();
        QVERIFY(!pm.isNull());
        pm.save(QStringLiteral("/tmp/glossary_coverage_dark.png"));
    }

    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

QTEST_MAIN(TestGlossaryCoverage)
#include "test_glossary_coverage.moc"
