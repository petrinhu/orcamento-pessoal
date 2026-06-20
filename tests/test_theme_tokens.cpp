// PeuBud - teste dos tokens de paleta do Theme (#164 redesign fix)
// Verifica que:
//   1. Os tokens da nova paleta azul-confiável retornam valores corretos.
//   2. O QSS gerado NÃO usa a cor negativa onde deveria usar borda (regressão
//      do bug: .arg(…,neg,brd,…) com posições trocadas tornava bordas vermelhas).
//   3. O QSS gerado usa a cor negativa APENAS em QLabel#error.
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest/QtTest>
#include <QApplication>

#include "ui/Theme.h"

class TestThemeTokens : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // ── Tokens de paleta ────────────────────────────────────────────────────
    void acento_light();
    void positivo_light();
    void negativo_light();
    void borda_light();
    void dourado_light();
    void sidebar1_naoVazio();

    // Smoke test dark
    void acento_dark_naoVazio();

    // ── Regressão: cor negativa nunca em posição de borda (#164 fix) ────────
    // Garante que o esquema de tokens nomeados @@BRD@@ / @@NEG@@ está correto
    // e que uma futura edição do QSS não reintroduz a troca posicional.

    // QScrollBar::handle deve conter a cor de BORDA (neutra), não a negativa.
    void scrollbarHandle_usaBordaNaoNegativa();

    // QLabel#error deve conter a cor NEGATIVA (vermelho), não a de borda.
    void labelError_usaNegativaCorreto();

    // A cor negativa NÃO deve aparecer nas regras de borda dos inputs.
    void inputBorder_usaBordaNaoNegativa();

    // Gridline da tabela deve ser borda, não negativa.
    void gridline_usaBordaNaoNegativa();

    // QMenuBar border-bottom deve ser borda, não negativa.
    void menuBarBorder_usaBordaNaoNegativa();

    // accHover e accPressed atualizados para tons coerentes com #3B82F6 (#164 fix).
    void accHover_tomClaroCoerente();
    void accPressed_tomEscuroCoerente();
};

void TestThemeTokens::initTestCase()
{
    // Força Light para todos os testes desta suíte.
    // setMode não chama aplicar() (que precisaria de QApplication+Fusion),
    // apenas seta g_mode para que os getters e gerarQss() retornem valores Light.
    Theme::setMode(Theme::ThemeMode::Light);
}

// ── Tokens de paleta ─────────────────────────────────────────────────────────

void TestThemeTokens::acento_light()
{
    QCOMPARE(Theme::acento(), QStringLiteral("#3B82F6"));
}

void TestThemeTokens::positivo_light()
{
    QCOMPARE(Theme::positivo(), QStringLiteral("#16C784"));
}

void TestThemeTokens::negativo_light()
{
    QCOMPARE(Theme::negativo(), QStringLiteral("#EA3943"));
}

void TestThemeTokens::borda_light()
{
    QCOMPARE(Theme::borda(), QStringLiteral("#E4EAF2"));
}

void TestThemeTokens::dourado_light()
{
    QCOMPARE(Theme::dourado(), QStringLiteral("#E8A93B"));
}

void TestThemeTokens::sidebar1_naoVazio()
{
    QVERIFY(!Theme::sidebar1().isEmpty());
}

void TestThemeTokens::acento_dark_naoVazio()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    QVERIFY(!Theme::acento().isEmpty());
    Theme::setMode(Theme::ThemeMode::Light); // restaura para testes seguintes
}

// ── Regressão: cor negativa vs cor de borda no QSS ───────────────────────────

// Estratégia: gerar o QSS e verificar que a cor da borda (neutra) aparece nas
// regras de borda/scrollbar, e que a cor negativa (vermelha) aparece APENAS no
// seletor de erro — nunca nas posições de borda. Usa substrings literais do QSS
// para ser explícito e pegável por grep.

void TestThemeTokens::scrollbarHandle_usaBordaNaoNegativa()
{
    const QString qss = Theme::qssParaTeste();
    const QString corBorda   = Theme::borda();    // #E4EAF2 (light)
    const QString corNegativa = Theme::negativo(); // #EA3943

    // A regra do handle vertical deve conter a cor de borda
    const QString regra = QStringLiteral("QScrollBar::handle:vertical");
    const int posRegra = qss.indexOf(regra);
    QVERIFY2(posRegra >= 0, "Regra QScrollBar::handle:vertical ausente no QSS");

    // Extrai o bloco { … } após a regra (até a próxima '}')
    const int posAbre = qss.indexOf('{', posRegra);
    const int posFecha = qss.indexOf('}', posAbre);
    QVERIFY(posAbre >= 0 && posFecha > posAbre);
    const QString bloco = qss.mid(posAbre, posFecha - posAbre);

    QVERIFY2(bloco.contains(corBorda),
             qPrintable(QStringLiteral("ScrollBar handle não contém cor de borda (%1)").arg(corBorda)));
    QVERIFY2(!bloco.contains(corNegativa),
             qPrintable(QStringLiteral("ScrollBar handle contém cor negativa (%1) — bug de token!").arg(corNegativa)));
}

void TestThemeTokens::labelError_usaNegativaCorreto()
{
    const QString qss = Theme::qssParaTeste();
    const QString corNegativa = Theme::negativo(); // #EA3943
    const QString corBorda    = Theme::borda();    // #E4EAF2

    // Localiza o bloco do QLabel#error
    const QString regra = QStringLiteral("QLabel#error");
    const int posRegra = qss.indexOf(regra);
    QVERIFY2(posRegra >= 0, "Regra QLabel#error ausente no QSS");

    const int posAbre = qss.indexOf('{', posRegra);
    const int posFecha = qss.indexOf('}', posAbre);
    QVERIFY(posAbre >= 0 && posFecha > posAbre);
    const QString bloco = qss.mid(posAbre, posFecha - posAbre);

    QVERIFY2(bloco.contains(corNegativa),
             qPrintable(QStringLiteral("QLabel#error não contém cor negativa (%1)").arg(corNegativa)));
    QVERIFY2(!bloco.contains(corBorda),
             qPrintable(QStringLiteral("QLabel#error contém cor de borda (%1) em vez da negativa!").arg(corBorda)));

    // A cor negativa deve aparecer em UMA única posição no QSS inteiro (só
    // QLabel#error). Pega regressão futura de @@NEG@@ vazando p/ outro seletor.
    QCOMPARE(qss.count(corNegativa), 1);
}

void TestThemeTokens::inputBorder_usaBordaNaoNegativa()
{
    const QString qss = Theme::qssParaTeste();
    const QString corBorda    = Theme::borda();    // #E4EAF2
    const QString corNegativa = Theme::negativo(); // #EA3943

    // Procura a regra de borda dos inputs
    const QString regra = QStringLiteral("QLineEdit, QSpinBox");
    const int posRegra = qss.indexOf(regra);
    QVERIFY2(posRegra >= 0, "Regra QLineEdit/QSpinBox ausente no QSS");

    const int posAbre = qss.indexOf('{', posRegra);
    const int posFecha = qss.indexOf('}', posAbre);
    QVERIFY(posAbre >= 0 && posFecha > posAbre);
    const QString bloco = qss.mid(posAbre, posFecha - posAbre);

    QVERIFY2(bloco.contains(corBorda),
             qPrintable(QStringLiteral("Input border não contém cor de borda (%1)").arg(corBorda)));
    QVERIFY2(!bloco.contains(corNegativa),
             qPrintable(QStringLiteral("Input border contém cor negativa (%1) — bug de token!").arg(corNegativa)));
}

void TestThemeTokens::gridline_usaBordaNaoNegativa()
{
    const QString qss = Theme::qssParaTeste();
    const QString corBorda    = Theme::borda();    // #E4EAF2
    const QString corNegativa = Theme::negativo(); // #EA3943

    // Localiza a propriedade gridline-color dentro do bloco QTableWidget
    const QString prop = QStringLiteral("gridline-color:");
    const int posGl = qss.indexOf(prop);
    QVERIFY2(posGl >= 0, "Propriedade gridline-color ausente no QSS");

    // Extrai o valor até o próximo ';'
    const int posSemicolon = qss.indexOf(';', posGl);
    QVERIFY(posSemicolon > posGl);
    const QString valor = qss.mid(posGl, posSemicolon - posGl);

    QVERIFY2(valor.contains(corBorda),
             qPrintable(QStringLiteral("gridline-color não usa cor de borda (%1)").arg(corBorda)));
    QVERIFY2(!valor.contains(corNegativa),
             qPrintable(QStringLiteral("gridline-color usa cor negativa (%1) — bug de token!").arg(corNegativa)));
}

void TestThemeTokens::menuBarBorder_usaBordaNaoNegativa()
{
    const QString qss = Theme::qssParaTeste();
    const QString corBorda    = Theme::borda();    // #E4EAF2
    const QString corNegativa = Theme::negativo(); // #EA3943

    // Extrai o bloco do QMenuBar (primeiro '{' após "QMenuBar {")
    const QString regra = QStringLiteral("QMenuBar {");
    const int posRegra = qss.indexOf(regra);
    QVERIFY2(posRegra >= 0, "Regra QMenuBar ausente no QSS");

    const int posAbre = qss.indexOf('{', posRegra);
    const int posFecha = qss.indexOf('}', posAbre);
    QVERIFY(posAbre >= 0 && posFecha > posAbre);
    const QString bloco = qss.mid(posAbre, posFecha - posAbre);

    QVERIFY2(bloco.contains(corBorda),
             qPrintable(QStringLiteral("QMenuBar border-bottom não usa cor de borda (%1)").arg(corBorda)));
    QVERIFY2(!bloco.contains(corNegativa),
             qPrintable(QStringLiteral("QMenuBar border-bottom usa cor negativa (%1) — bug de token!").arg(corNegativa)));
}

// ── Accent hover/pressed coerentes com #3B82F6 (#164 fix) ───────────────────

void TestThemeTokens::accHover_tomClaroCoerente()
{
    // Deve ser #60A5FA (blue-400) — mais claro que o accent base #3B82F6
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("#60A5FA")),
             "accHover (#60A5FA) ausente no QSS — verificar gerarQss()");
}

void TestThemeTokens::accPressed_tomEscuroCoerente()
{
    // Deve ser #2563EB (blue-600) — mais escuro que o accent base #3B82F6
    const QString qss = Theme::qssParaTeste();
    QVERIFY2(qss.contains(QStringLiteral("#2563EB")),
             "accPressed (#2563EB) ausente no QSS — verificar gerarQss()");
}

QTEST_MAIN(TestThemeTokens)
#include "test_theme_tokens.moc"
