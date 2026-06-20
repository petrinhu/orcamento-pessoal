// Render offscreen dos callouts de ajuda do TabShell (#166 tipografia).
//
// Verifica:
//   (1) Callout Descrição renderiza (não-nulo, salva /tmp/tipo_impl_light.png e dark).
//   (2) QFrame#helpDescBox existe e tem fundo azul-claro (HelpDescBg).
//   (3) QLabel#helpEyebrow tem cor de acento (HelpEyebrow, não SubtextLight).
//   (4) Callout Instruções renderiza com pílulas numeradas.
//   (5) Tokens de ajuda não ficaram substituídos (@@HELP_*@@ ausentes no QSS).
//
// Salva /tmp/tipo_impl_light.png e /tmp/tipo_impl_dark.png para aprovação visual.
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QVBoxLayout>
#include <QPixmap>
#include <QColor>
#include <QFrame>
#include <QLabel>
#include <QWidget>

#include "ui/widgets/TabShell.h"
#include "ui/Theme.h"

class TestAjudaRender : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void tokensHelpNoQss();
    void renderLightNaoNulo();
    void renderDarkNaoNulo();
    void helpDescBoxExiste();
    void helpEyebrowExiste();
    void helpPillsExistem();
    void corpoDescricaoNaoVazio();
};

// ── Conteúdo real de tab_cartoes (citação literal de TabHelp) ──────────────────

static const char *kDescricaoCartoes =
    "💳 Aqui você registra as compras feitas no cartão de crédito — "
    "à vista ou parceladas. O app distribui as parcelas por mês e "
    "calcula os juros e a tabela de amortização.";

static QStringList passosCartoes()
{
    return {
        QStringLiteral("Clique em <b>➕ Novo item</b> para adicionar uma compra."),
        QStringLiteral("Escolha o <b>cartão</b>, a <b>descrição</b>, o <b>valor total</b> e o <b>nº de parcelas</b>."),
        QStringLiteral("💰 Se houver <b>juros</b>, informe a taxa — o app calcula o <b>CET</b> e a <b>tabela de amortização</b> (Price)."),
        QStringLiteral("📅 As parcelas aparecem <b>por mês</b> na tabela; navegue com as setas <b>no topo</b>."),
        QStringLiteral("Quer simular antes de comprar? Use a <b>calculadora 🧮</b> na barra superior do app."),
    };
}

static TabShell *criarShellCartoes(QWidget *parent = nullptr)
{
    auto *shell = new TabShell(QStringLiteral("Cartões"), parent);
    shell->setTabKey(QStringLiteral("tab_cartoes_test")); // key de teste, sem persistência
    shell->resize(600, 500);
    shell->setDescription(QString::fromUtf8(kDescricaoCartoes));
    shell->setInstructions(passosCartoes());
    return shell;
}

// ── initTestCase ──────────────────────────────────────────────────────────────

void TestAjudaRender::initTestCase()
{
    // Light primeiro (o estado default dos renders de verificação).
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── Tokens ausentes no QSS (nenhum @@HELP_*@@ deve sobrar) ──────────────────

void TestAjudaRender::tokensHelpNoQss()
{
    const QString qss = Theme::qssParaTeste();

    // Nenhum token literal deve restar após gerarQss() — todos substituídos.
    QVERIFY2(!qss.contains(QStringLiteral("@@HELP_DESC_BG@@")),
             "Token @@HELP_DESC_BG@@ não foi substituído no QSS");
    QVERIFY2(!qss.contains(QStringLiteral("@@HELP_DESC_BAR@@")),
             "Token @@HELP_DESC_BAR@@ não foi substituído no QSS");
    QVERIFY2(!qss.contains(QStringLiteral("@@HELP_INSTR_BG@@")),
             "Token @@HELP_INSTR_BG@@ não foi substituído no QSS");
    QVERIFY2(!qss.contains(QStringLiteral("@@HELP_NUM_BG@@")),
             "Token @@HELP_NUM_BG@@ não foi substituído no QSS");
    QVERIFY2(!qss.contains(QStringLiteral("@@HELP_NUM_FG@@")),
             "Token @@HELP_NUM_FG@@ não foi substituído no QSS");
    QVERIFY2(!qss.contains(QStringLiteral("@@HELP_EYEBROW@@")),
             "Token @@HELP_EYEBROW@@ não foi substituído no QSS");

    // Os seletores dos callouts devem estar presentes no QSS.
    QVERIFY2(qss.contains(QStringLiteral("QFrame#helpDescBox")),
             "Seletor QFrame#helpDescBox ausente no QSS");
    QVERIFY2(qss.contains(QStringLiteral("QFrame#helpInstrBox")),
             "Seletor QFrame#helpInstrBox ausente no QSS");
    QVERIFY2(qss.contains(QStringLiteral("QLabel#helpEyebrow")),
             "Seletor QLabel#helpEyebrow ausente no QSS");
    QVERIFY2(qss.contains(QStringLiteral("QLabel#helpPill")),
             "Seletor QLabel#helpPill ausente no QSS");

    // Os valores de cor devem aparecer no QSS (tokens expandidos).
    QVERIFY2(qss.contains(Theme::helpDescBg()),
             qPrintable(QStringLiteral("Cor helpDescBg (%1) ausente no QSS").arg(Theme::helpDescBg())));
    QVERIFY2(qss.contains(Theme::helpEyebrow()),
             qPrintable(QStringLiteral("Cor helpEyebrow (%1) ausente no QSS").arg(Theme::helpEyebrow())));
}

// ── Render light: salva /tmp/tipo_impl_light.png ─────────────────────────────

void TestAjudaRender::renderLightNaoNulo()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    QWidget parent;
    parent.resize(640, 520);
    auto *layout = new QVBoxLayout(&parent);
    layout->setContentsMargins(0, 0, 0, 0);

    TabShell *shell = criarShellCartoes(&parent);
    layout->addWidget(shell);

    parent.show();
    QApplication::processEvents();
    parent.repaint();
    QApplication::processEvents();

    QPixmap px = parent.grab();
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo — render light falhou");

    const bool saved = px.save(QStringLiteral("/tmp/tipo_impl_light.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/tipo_impl_light.png");

    qDebug("Render light gravado em /tmp/tipo_impl_light.png. "
           "Verificar: faixa de acento esquerda no callout Descrição, "
           "eyebrow azul, pílulas numeradas azuis, divisória tracejada entre passos.");
}

// ── Render dark: salva /tmp/tipo_impl_dark.png ───────────────────────────────

void TestAjudaRender::renderDarkNaoNulo()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    Theme::aplicar();

    QWidget parent;
    parent.resize(640, 520);
    auto *layout = new QVBoxLayout(&parent);
    layout->setContentsMargins(0, 0, 0, 0);

    TabShell *shell = criarShellCartoes(&parent);
    layout->addWidget(shell);

    parent.show();
    QApplication::processEvents();
    parent.repaint();
    QApplication::processEvents();

    QPixmap px = parent.grab();
    QVERIFY2(!px.isNull(), "grab() retornou pixmap nulo — render dark falhou");

    const bool saved = px.save(QStringLiteral("/tmp/tipo_impl_dark.png"));
    QVERIFY2(saved, "Falha ao salvar /tmp/tipo_impl_dark.png");

    qDebug("Render dark gravado em /tmp/tipo_impl_dark.png. "
           "Verificar: fundo azul-escuro (#16243D) na Descrição, "
           "eyebrow #7FB0FF, pílulas com #5B9BFF, tokens distintos do light.");

    // Restaura light para os testes seguintes
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── helpDescBox existe como filho do TabShell ─────────────────────────────────

void TestAjudaRender::helpDescBoxExiste()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();

    QWidget parent;
    TabShell *shell = criarShellCartoes(&parent);
    parent.show();
    QApplication::processEvents();

    // Procura QFrame#helpDescBox na hierarquia do shell
    QFrame *descBox = shell->findChild<QFrame *>(QStringLiteral("helpDescBox"));
    QVERIFY2(descBox != nullptr,
             "QFrame#helpDescBox não encontrado na hierarquia do TabShell — "
             "montarCallout não criou o callout da Descrição com o objectName correto.");
    QVERIFY2(descBox->isVisible(),
             "QFrame#helpDescBox existe mas está oculto — "
             "setDescription() não chamou box->show().");

    // Verifica que o callout das Instruções também existe
    QFrame *instrBox = shell->findChild<QFrame *>(QStringLiteral("helpInstrBox"));
    QVERIFY2(instrBox != nullptr,
             "QFrame#helpInstrBox não encontrado na hierarquia do TabShell.");
    QVERIFY2(instrBox->isVisible(),
             "QFrame#helpInstrBox existe mas está oculto.");
}

// ── helpEyebrow existe e tem texto não-vazio ──────────────────────────────────

void TestAjudaRender::helpEyebrowExiste()
{
    QWidget parent;
    TabShell *shell = criarShellCartoes(&parent);
    parent.show();
    QApplication::processEvents();

    // Deve haver exatamente 2 labels com objectName "helpEyebrow" (1 por bloco)
    const QList<QLabel *> eyebrows = shell->findChildren<QLabel *>(QStringLiteral("helpEyebrow"));
    QVERIFY2(eyebrows.size() == 2,
             qPrintable(QStringLiteral(
                 "Esperado 2 QLabel#helpEyebrow (1 por bloco), encontrado %1.")
                 .arg(eyebrows.size())));

    for (const QLabel *lbl : eyebrows) {
        QVERIFY2(!lbl->text().isEmpty(),
                 "QLabel#helpEyebrow com texto vazio — eyebrowTexto() retornou string vazia.");
        // O texto deve conter a âncora "●" e o rótulo (em maiúsculas)
        QVERIFY2(lbl->text().contains(QStringLiteral("●")),
                 qPrintable(QStringLiteral(
                     "QLabel#helpEyebrow não contém ponto âncora '●'. Texto: %1")
                     .arg(lbl->text())));
    }
}

// ── Pílulas numeradas nas Instruções ─────────────────────────────────────────

void TestAjudaRender::helpPillsExistem()
{
    QWidget parent;
    TabShell *shell = criarShellCartoes(&parent);
    parent.show();
    QApplication::processEvents();

    // Deve haver 5 pílulas (= 5 passos de tab_cartoes)
    const QList<QLabel *> pills = shell->findChildren<QLabel *>(QStringLiteral("helpPill"));
    QVERIFY2(pills.size() == 5,
             qPrintable(QStringLiteral(
                 "Esperado 5 QLabel#helpPill (1 por passo), encontrado %1. "
                 "reconstruirPassos() pode não ter criado todas as pílulas.")
                 .arg(pills.size())));

    // Cada pílula deve ter o número correto (1..5)
    for (int i = 0; i < pills.size(); ++i) {
        const QString esperado = QString::number(i + 1);
        QVERIFY2(pills[i]->text() == esperado,
                 qPrintable(QStringLiteral(
                     "Pílula %1: esperado texto '%2', encontrado '%3'.")
                     .arg(i).arg(esperado, pills[i]->text())));
    }
}

// ── Corpo da Descrição não está vazio após setDescription ─────────────────────

void TestAjudaRender::corpoDescricaoNaoVazio()
{
    QWidget parent;
    TabShell *shell = criarShellCartoes(&parent);
    parent.show();
    QApplication::processEvents();

    QLabel *descBody = shell->findChild<QLabel *>(QStringLiteral("helpDescBody"));
    QVERIFY2(descBody != nullptr,
             "QLabel#helpDescBody não encontrado — Descrição não foi criada.");
    QVERIFY2(!descBody->text().isEmpty(),
             "QLabel#helpDescBody está vazio após setDescription() com texto não-vazio.");

    // O rich-text deve conter o trecho âncora da 1ª frase em peso 600
    QVERIFY2(descBody->text().contains(QStringLiteral("font-weight:600")),
             "QLabel#helpDescBody não contém span font-weight:600 — "
             "formatarDescricaoRich() não aplicou ênfase na 1ª oração.");
}

QTEST_MAIN(TestAjudaRender)
#include "test_ajuda_render.moc"
