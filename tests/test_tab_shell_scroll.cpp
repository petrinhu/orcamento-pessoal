// Regressão #188: conteúdo das abas deve rolar em janela pequena (não clipar).
//
// Verifica (TDD RED → GREEN):
//   (1) O TabShell contém um QScrollArea na sua hierarquia de widgets.
//   (2) O conteúdo passado via setContent() está *dentro* do QScrollArea
//       (não diretamente no TabShell).
//   (3) widgetResizable() do QScrollArea é true (condição para adaptar ao conteúdo).
//   (4) Render offscreen em tamanho pequeno (720×500) não é nulo —
//       smoke test de que o widget não crasha ao ser redimensionado abaixo
//       do tamanho mínimo do conteúdo.
//   (5) Callouts (helpDescBox, helpInstrBox) estão FORA do QScrollArea —
//       garantem que o título e instruções permanecem visíveis sem scroll.
//
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest>
#include <QApplication>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFrame>

#include "ui/widgets/TabShell.h"
#include "ui/Theme.h"
#include "ui/TabHelp.h"

// ── Helpers ────────────────────────────────────────────────────────────────────

// Procura recursivamente um QScrollArea na hierarquia de um widget.
static QScrollArea *encontrarScrollArea(QWidget *raiz)
{
    auto *sa = qobject_cast<QScrollArea *>(raiz);
    if (sa) return sa;
    for (QObject *child : raiz->children()) {
        if (auto *w = qobject_cast<QWidget *>(child)) {
            sa = encontrarScrollArea(w);
            if (sa) return sa;
        }
    }
    return nullptr;
}

// Verifica se um widget está dentro (descendente) de um QScrollArea.
static bool estaDentroDoScroll(QWidget *alvo, QScrollArea *scroll)
{
    if (!alvo || !scroll) return false;
    QWidget *parent = alvo->parentWidget();
    while (parent) {
        if (parent == scroll->widget()) return true;
        if (parent == scroll)          return true;
        parent = parent->parentWidget();
    }
    return false;
}

// Verifica se um widget com dado objectName está FORA do QScrollArea.
static bool estaForaDoScroll(QWidget *shell, const QString &objectName, QScrollArea *scroll)
{
    // Procura o widget pelo objectName na hierarquia do shell
    QWidget *alvo = shell->findChild<QWidget *>(objectName);
    if (!alvo) return true; // não existe = não está dentro do scroll
    // Se não é descendente do widget interno do scroll, está fora.
    return !estaDentroDoScroll(alvo, scroll);
}

// ── TestTabShellScroll ─────────────────────────────────────────────────────────

class TestTabShellScroll : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // (1) QScrollArea existe na hierarquia do TabShell
    void scrollArea_existe();

    // (2) Conteúdo (setContent) está dentro do QScrollArea
    void conteudo_estaDentroDoScroll();

    // (3) widgetResizable == true
    void scrollArea_widgetResizable();

    // (4) Render offscreen em tamanho pequeno não crasha
    void renderPequeno_naoNulo();

    // (5) Callouts helpDescBox e helpInstrBox estão fora do scroll
    void calloutDesc_estaForaDoScroll();
    void calloutInstr_estaForaDoScroll();
};

// ── initTestCase ──────────────────────────────────────────────────────────────

void TestTabShellScroll::initTestCase()
{
    Theme::setMode(Theme::ThemeMode::Light);
    Theme::aplicar();
}

// ── (1) QScrollArea existe ────────────────────────────────────────────────────

void TestTabShellScroll::scrollArea_existe()
{
    TabShell shell(QStringLiteral("Teste"));
    shell.resize(800, 600);
    shell.show();
    QApplication::processEvents();

    QScrollArea *sa = encontrarScrollArea(&shell);
    QVERIFY2(sa != nullptr, "TabShell nao contem nenhum QScrollArea — conteudo clipa em janela pequena (#188)");
}

// ── (2) Conteúdo está dentro do scroll ───────────────────────────────────────

void TestTabShellScroll::conteudo_estaDentroDoScroll()
{
    TabShell shell(QStringLiteral("Teste"));

    auto *conteudo = new QLabel(QStringLiteral("widget de conteudo"));
    conteudo->setObjectName(QStringLiteral("conteudoTeste"));
    shell.setContent(conteudo);

    shell.resize(800, 600);
    shell.show();
    QApplication::processEvents();

    QScrollArea *sa = encontrarScrollArea(&shell);
    QVERIFY2(sa != nullptr, "QScrollArea nao encontrado");
    QVERIFY2(estaDentroDoScroll(conteudo, sa),
             "Conteudo (setContent) nao esta dentro do QScrollArea (#188)");
}

// ── (3) widgetResizable == true ───────────────────────────────────────────────

void TestTabShellScroll::scrollArea_widgetResizable()
{
    TabShell shell(QStringLiteral("Teste"));
    shell.resize(800, 600);
    shell.show();
    QApplication::processEvents();

    QScrollArea *sa = encontrarScrollArea(&shell);
    QVERIFY2(sa != nullptr, "QScrollArea nao encontrado");
    QVERIFY2(sa->widgetResizable(),
             "QScrollArea::widgetResizable() deve ser true para adaptar o conteudo (#188)");
}

// ── (4) Render offscreen em tamanho pequeno ───────────────────────────────────

void TestTabShellScroll::renderPequeno_naoNulo()
{
    TabShell shell(QStringLiteral("Planejamento/Orcamento"));
    shell.setTabKey(QStringLiteral("tab_planejamento"));
    shell.applyHelp();

    // Conteúdo simulado: label longo que normalmente causaria clip
    auto *conteudo = new QWidget;
    auto *lay = new QVBoxLayout(conteudo);
    auto *lbl = new QLabel(
        QStringLiteral("Nenhum item no orcamento — Planeje quanto quer gastar em cada categoria do mes e veja se esta dentro do combinado."),
        conteudo);
    lbl->setWordWrap(true);
    lay->addWidget(lbl);
    shell.setContent(conteudo);

    // Redimensiona para tamanho pequeno (replica o cenário do bug #188)
    shell.resize(720, 500);
    shell.show();
    QApplication::processEvents();

    const QPixmap pm = shell.grab();
    pm.save(QStringLiteral("/tmp/tab_shell_scroll_pequeno.png"));
    QVERIFY2(!pm.isNull(), "Render em tamanho pequeno retornou pixmap nulo (#188)");
}

// ── (5) Callouts fora do scroll ───────────────────────────────────────────────

void TestTabShellScroll::calloutDesc_estaForaDoScroll()
{
    TabShell shell(QStringLiteral("Teste"));
    shell.setTabKey(QStringLiteral("tab_planejamento"));
    shell.applyHelp();
    shell.resize(800, 600);
    shell.show();
    QApplication::processEvents();

    QScrollArea *sa = encontrarScrollArea(&shell);
    QVERIFY2(sa != nullptr, "QScrollArea nao encontrado");

    // helpDescBox deve estar fora do QScrollArea (sempre visivel sem scroll)
    QVERIFY2(estaForaDoScroll(&shell, QStringLiteral("helpDescBox"), sa),
             "Callout de descricao (helpDescBox) esta dentro do QScrollArea — deve ficar fora (#188)");
}

void TestTabShellScroll::calloutInstr_estaForaDoScroll()
{
    TabShell shell(QStringLiteral("Teste"));
    shell.setTabKey(QStringLiteral("tab_planejamento"));
    shell.applyHelp();
    shell.resize(800, 600);
    shell.show();
    QApplication::processEvents();

    QScrollArea *sa = encontrarScrollArea(&shell);
    QVERIFY2(sa != nullptr, "QScrollArea nao encontrado");

    // helpInstrBox deve estar fora do QScrollArea
    QVERIFY2(estaForaDoScroll(&shell, QStringLiteral("helpInstrBox"), sa),
             "Callout de instrucoes (helpInstrBox) esta dentro do QScrollArea — deve ficar fora (#188)");
}

QTEST_MAIN(TestTabShellScroll)
#include "test_tab_shell_scroll.moc"
