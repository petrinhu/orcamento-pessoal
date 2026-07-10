// PeuBud - teste dos tokens de paleta do Theme (#164 redesign fix)
// Verifica que:
//   1. Os tokens da nova paleta azul-confiável retornam valores corretos.
//   2. O QSS gerado NÃO usa a cor negativa onde deveria usar borda (regressão
//      do bug: .arg(…,neg,brd,…) com posições trocadas tornava bordas vermelhas).
//   3. O QSS gerado usa a cor negativa APENAS em QLabel#error.
// Executar com QT_QPA_PLATFORM=offscreen.

#include <QtTest/QtTest>
#include <QApplication>
#include <QColor>
#include <cmath>

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

    // Gridline da tabela: transparente (sem grade vertical, #167 T1 §11) e
    // nunca a cor negativa.
    void gridline_transparenteNaoNegativa();

    // QMenuBar border-bottom deve ser borda, não negativa.
    void menuBarBorder_usaBordaNaoNegativa();

    // accHover e accPressed atualizados para tons coerentes com #3B82F6 (#164 fix).
    void accHover_tomClaroCoerente();
    void accPressed_tomEscuroCoerente();

    // ── Tokens de sidebar no QSS (#165) ─────────────────────────────────────
    // Garante que @@SIDEBAR1@@, @@SIDEBAR2@@, @@SIDEBARTXT@@ foram substituidos
    // pelos valores reais e que o seletor #sidebarNav esta presente no QSS.
    void sidebarNavBackground_contemSidebar1();
    void navItemAtivo_contemAccento();
    void navItemNormal_contemSidebarText();

    // ── Cue de glossário (#166 §9.6): cor visível e cinza (não-azul) ─────────
    // A cor do sublinhado deve ser claramente visível (alpha alto) e cinza —
    // NUNCA o azul accent (reservado a ações; competiria com os CTAs).
    void glossarioUnderline_alphaAlto_light();
    void glossarioUnderline_naoEhAccent_light();
    void glossarioUnderline_ehCinza_light();
    void glossarioUnderline_darkAware();

    // ── #198 (Onda U-4, A11Y-04): @@ACC_TEXT@@ — texto AA-safe sobre o acento ──
    // O acento cru (#3B82F6) reprova AA como TEXTO em vários fundos (hover de
    // botão, header hover, glifo de icon-button): 3.68:1 light / 4.26:1 dark
    // sobre @@SURF@@, abaixo do mínimo 4.5:1 (WCAG 1.4.3). Mesma classe de bug
    // já corrigida no eyebrow/número dos blocos de ajuda (#166).
    void acentoTexto_light();
    void acentoTexto_dark();
    void acentoCru_comoTexto_reprovaAA_regressao(); // ancora o "porquê" do fix
    void acentoTexto_comoTexto_passaAA_light();
    void acentoTexto_comoTexto_passaAA_dark();

    void pushButtonHover_usaAccTextNaoAccCru();
    void iconButtonHover_usaAccTextNaoAccCru();
    void iconButtonPressed_usaAccTextNaoAccCru();
    void headerSectionHover_usaAccTextNaoAccCru();

    // ── #198 (Onda U-4, A11Y-02): foco de teclado visível na sidebar ──────────
    // O fix #165 suprimia TODO indicador (outline+border:none) — item focado
    // via Tab ficava idêntico ao repouso, invisível para quem navega sem mouse
    // (WCAG 2.4.7). Fix: fundo sutil + borda 1px @@ACC@@ (não-texto, >=3:1).
    void navItemFoco_temIndicadorVisivel();
    void navItemFoco_contrasteBordaSobreNavy_AA();

    // ── A11Y-05: positivoTexto()/negativoTexto() — texto AA-safe de status ────
    // positivo()/negativo() crus reprovam AA como cor de TEXTO (2.20:1 e
    // 4.07/3.85:1 sobre @@SURF@@) — usados como setForeground em tabelas
    // (Metas, Planejamento) e color: (checklist de senha do LoginDialog).
    // Fills/badges/barras continuam com o token cru (regra de ouro preservada).
    void positivoTexto_light();
    void positivoTexto_dark();
    void negativoTexto_light();
    void negativoTexto_dark();
    void positivoCru_comoTexto_reprovaAA_regressao_light();
    void negativoCru_comoTexto_reprovaAA_regressao();
    void positivoTexto_comoTexto_passaAA_light_e_dark();
    void negativoTexto_comoTexto_passaAA_light_e_dark();
};

// ── Helper de contraste WCAG (luminância relativa, mesma fórmula do spec) ──
// Independente do Theme — reimplementado aqui de propósito (oráculo externo
// ao código sob teste, evita "testar o próprio bug" caso Theme algum dia
// exponha um helper de contraste com o mesmo erro).
namespace {
qreal luminanciaRelativa(const QColor &c)
{
    auto lin = [](qreal ch) {
        return ch <= 0.03928 ? ch / 12.92 : std::pow((ch + 0.055) / 1.055, 2.4);
    };
    const qreal r = lin(c.redF());
    const qreal g = lin(c.greenF());
    const qreal b = lin(c.blueF());
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

qreal contraste(const QColor &a, const QColor &b)
{
    const qreal la = luminanciaRelativa(a);
    const qreal lb = luminanciaRelativa(b);
    const qreal maior = qMax(la, lb);
    const qreal menor = qMin(la, lb);
    return (maior + 0.05) / (menor + 0.05);
}
}

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

void TestThemeTokens::gridline_transparenteNaoNegativa()
{
    // #167 T1 (§11 manual de estilo): grade vertical removida — a única
    // divisória é a horizontal fina no item (border-bottom). gridline-color
    // é "transparent"; nunca a cor negativa (regressão do bug histórico de
    // troca posicional neg/brd).
    const QString qss = Theme::qssParaTeste();
    const QString corNegativa = Theme::negativo(); // #EA3943

    const QString prop = QStringLiteral("gridline-color:");
    const int posGl = qss.indexOf(prop);
    QVERIFY2(posGl >= 0, "Propriedade gridline-color ausente no QSS");

    const int posSemicolon = qss.indexOf(';', posGl);
    QVERIFY(posSemicolon > posGl);
    const QString valor = qss.mid(posGl, posSemicolon - posGl);

    QVERIFY2(valor.contains(QStringLiteral("transparent")),
             "gridline-color não está transparente — grade vertical ainda visível (#167 T1)");
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

// ── Tokens de sidebar no QSS (#165) ──────────────────────────────────────────

// Verifica que os tokens de sidebar foram substituidos corretamente no QSS e
// que o seletor raiz existe. NOTA (fix #165 paintEvent): o gradiente navy
// (@@SIDEBAR1@@/@@SIDEBAR2@@) foi movido para SidebarNav::paintEvent(), que e
// deterministico e independe de QSS/Wayland. O QSS nao contem mais o gradiente
// — contem apenas dimensoes, transparencias dos filhos e texto dos itens.
// Os tokens @@SIDEBAR1@@/@@SIDEBAR2@@ ainda sao substituidos em gerarQss()
// (chamada correta), mas o valor resultante nao precisa estar no QSS final.
void TestThemeTokens::sidebarNavBackground_contemSidebar1()
{
    const QString qss = Theme::qssParaTeste();

    // Token literal nao pode aparecer apos substituicao de tokens.
    // (gerarQss() substitui todos os @@TOKEN@@ — verificar que nenhum sobrou)
    QVERIFY2(!qss.contains(QStringLiteral("@@SIDEBAR1@@")),
             "Token @@SIDEBAR1@@ nao foi substituido no QSS");
    QVERIFY2(!qss.contains(QStringLiteral("@@SIDEBAR2@@")),
             "Token @@SIDEBAR2@@ nao foi substituido no QSS");
    QVERIFY2(!qss.contains(QStringLiteral("@@SIDEBARTXT@@")),
             "Token @@SIDEBARTXT@@ nao foi substituido no QSS");

    // O seletor do widget raiz deve existir (define min/max-width da coluna).
    QVERIFY2(qss.contains(QStringLiteral("QWidget#sidebarNav")),
             "Seletor QWidget#sidebarNav ausente no QSS");

    // O sidebarText (cor dos itens/nome) deve estar no QSS — e o unico token
    // de sidebar que aparece no QSS apos o fix paintEvent. O gradiente navy
    // agora vive no SidebarNav::paintEvent(), nao no QSS.
    QVERIFY2(qss.contains(Theme::sidebarText()),
             "Cor sidebarText ausente no QSS — token @@SIDEBARTXT@@ nao foi expandido");
}

// O bloco do item ativo [ativo="true"] deve conter o accent (#3B82F6).
void TestThemeTokens::navItemAtivo_contemAccento()
{
    const QString qss = Theme::qssParaTeste();
    const QString regra = QStringLiteral("[ativo=\"true\"]");
    const int posRegra = qss.indexOf(regra);
    QVERIFY2(posRegra >= 0, "Seletor [ativo=\"true\"] ausente no QSS");

    const int posAbre = qss.indexOf('{', posRegra);
    const int posFecha = qss.indexOf('}', posAbre);
    QVERIFY(posAbre >= 0 && posFecha > posAbre);
    const QString bloco = qss.mid(posAbre, posFecha - posAbre);

    QVERIFY2(bloco.contains(Theme::acento()),
             "Cor accent ausente no bloco do item ativo [ativo=\"true\"]");
}

// O bloco do navItem normal deve conter a cor de texto da sidebar (@@SIDEBARTXT@@).
void TestThemeTokens::navItemNormal_contemSidebarText()
{
    const QString qss = Theme::qssParaTeste();

    // Token literal nao pode aparecer (deve ter sido substituido).
    QVERIFY2(!qss.contains(QStringLiteral("@@SIDEBARTXT@@")),
             "Token @@SIDEBARTXT@@ nao foi substituido no QSS");

    // O valor real deve estar presente no QSS.
    QVERIFY2(qss.contains(Theme::sidebarText()),
             "Cor sidebarText ausente no QSS apos substituicao de @@SIDEBARTXT@@");
}

// ── Cue de glossário: cor do sublinhado (#166 §9.6) ──────────────────────────

void TestThemeTokens::glossarioUnderline_alphaAlto_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const QColor c = Theme::glossarioUnderline();
    // Visível: alpha >= 200 (~78%+). O valor antigo (140, ~55%) era imperceptível.
    QVERIFY2(c.alpha() >= 200,
        qPrintable(QStringLiteral("alpha do sublinhado deveria ser >= 200 (visível), got %1")
            .arg(c.alpha())));
}

void TestThemeTokens::glossarioUnderline_naoEhAccent_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const QColor c = Theme::glossarioUnderline();
    const QColor accent(Theme::acento()); // #3B82F6
    // NÃO pode ser o azul accent (reservado a ações).
    QVERIFY2(c.rgb() != accent.rgb(),
        "sublinhado de glossário NÃO pode ser o azul accent (reservado a CTAs)");
    // Robustez extra: não pode ser "muito azul" (blue dominante e saturado).
    // Cinza tem R≈G≈B; azul tem B >> R. Exigir que B não domine demais.
    QVERIFY2(c.blue() - c.red() < 60,
        qPrintable(QStringLiteral("cor parece azulada demais (R=%1 G=%2 B=%3) — deve ser cinza")
            .arg(c.red()).arg(c.green()).arg(c.blue())));
}

void TestThemeTokens::glossarioUnderline_ehCinza_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const QColor c = Theme::glossarioUnderline();
    // Cinza-ardósia: saturação moderada/baixa. HSV saturation < ~45%.
    QVERIFY2(c.saturationF() < 0.45,
        qPrintable(QStringLiteral("saturação %1 alta demais — deve ser cinza dessaturado")
            .arg(c.saturationF())));
}

void TestThemeTokens::glossarioUnderline_darkAware()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const QColor light = Theme::glossarioUnderline();
    Theme::setMode(Theme::ThemeMode::Dark);
    const QColor dark = Theme::glossarioUnderline();
    Theme::setMode(Theme::ThemeMode::Light); // restaura

    // Light e dark devem diferir (tom apropriado a cada fundo).
    QVERIFY2(light.rgb() != dark.rgb(),
        "cor do sublinhado deve ser dark-aware (light != dark)");
    // No dark, deve ser mais CLARA (luminância maior) para destacar sobre fundo escuro.
    QVERIFY2(dark.lightnessF() > light.lightnessF(),
        "no dark, o sublinhado deve ser mais claro que no light (contraste com fundo)");
}

// ── #198 (A11Y-04): @@ACC_TEXT@@ ──────────────────────────────────────────

void TestThemeTokens::acentoTexto_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    QCOMPARE(Theme::acentoTexto(), QStringLiteral("#1D4ED8"));
}

void TestThemeTokens::acentoTexto_dark()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    QCOMPARE(Theme::acentoTexto(), QStringLiteral("#7FB0FF"));
    Theme::setMode(Theme::ThemeMode::Light); // restaura
}

// RED->GREEN anchor (#198): prova o "porquê" do fix — o acento cru, usado
// como TEXTO sobre a superfície do botão, reprova AA nos dois temas. Se este
// teste um dia passar a QVERIFY(...>= 4.5), a paleta mudou e o token
// @@ACC_TEXT@@ pode ter deixado de ser necessário (revisar a decisão).
void TestThemeTokens::acentoCru_comoTexto_reprovaAA_regressao()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const qreal cLight = contraste(QColor(Theme::acento()), QColor(Theme::surface()));
    QVERIFY2(cLight < 4.5,
        qPrintable(QStringLiteral("acento cru sobre surface (light) = %1:1 — esperava <4.5 (reprova AA)")
            .arg(cLight, 0, 'f', 2)));

    Theme::setMode(Theme::ThemeMode::Dark);
    const qreal cDark = contraste(QColor(Theme::acento()), QColor(Theme::surface()));
    Theme::setMode(Theme::ThemeMode::Light); // restaura
    QVERIFY2(cDark < 4.5,
        qPrintable(QStringLiteral("acento cru sobre surface (dark) = %1:1 — esperava <4.5 (reprova AA)")
            .arg(cDark, 0, 'f', 2)));
}

void TestThemeTokens::acentoTexto_comoTexto_passaAA_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const qreal c = contraste(QColor(Theme::acentoTexto()), QColor(Theme::surface()));
    QVERIFY2(c >= 4.5,
        qPrintable(QStringLiteral("@@ACC_TEXT@@ sobre surface (light) = %1:1 — esperava >=4.5 (WCAG AA texto)")
            .arg(c, 0, 'f', 2)));
}

void TestThemeTokens::acentoTexto_comoTexto_passaAA_dark()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    const qreal c = contraste(QColor(Theme::acentoTexto()), QColor(Theme::surface()));
    Theme::setMode(Theme::ThemeMode::Light); // restaura
    QVERIFY2(c >= 4.5,
        qPrintable(QStringLiteral("@@ACC_TEXT@@ sobre surface (dark) = %1:1 — esperava >=4.5 (WCAG AA texto)")
            .arg(c, 0, 'f', 2)));
}

// Helper local: extrai o bloco { … } da primeira ocorrência de `regra` no QSS.
static QString blocoDoQss(const QString &qss, const QString &regra)
{
    const int posRegra = qss.indexOf(regra);
    if (posRegra < 0) return QString();
    const int posAbre = qss.indexOf('{', posRegra);
    const int posFecha = qss.indexOf('}', posAbre);
    if (posAbre < 0 || posFecha <= posAbre) return QString();
    return qss.mid(posAbre, posFecha - posAbre);
}

void TestThemeTokens::pushButtonHover_usaAccTextNaoAccCru()
{
    const QString qss = Theme::qssParaTeste();
    const QString bloco = blocoDoQss(qss, QStringLiteral("QPushButton:hover"));
    QVERIFY2(!bloco.isEmpty(), "Regra QPushButton:hover ausente no QSS");

    QVERIFY2(bloco.contains(QStringLiteral(" color: %1;").arg(Theme::acentoTexto())),
        qPrintable(QStringLiteral("QPushButton:hover não usa @@ACC_TEXT@@ (%1) como color")
            .arg(Theme::acentoTexto())));
    QVERIFY2(!bloco.contains(QStringLiteral(" color: %1;").arg(Theme::acento())),
        qPrintable(QStringLiteral("QPushButton:hover ainda usa @@ACC@@ cru (%1) como color — bug #198!")
            .arg(Theme::acento())));
    // border-color: @@ACC@@ continua correto (não-texto, 3:1 basta).
    QVERIFY2(bloco.contains(QStringLiteral("border-color: %1;").arg(Theme::acento())),
        "QPushButton:hover perdeu border-color: @@ACC@@ (regressão não-relacionada)");
}

void TestThemeTokens::iconButtonHover_usaAccTextNaoAccCru()
{
    const QString qss = Theme::qssParaTeste();
    const QString bloco = blocoDoQss(qss, QStringLiteral("QPushButton#iconButton:hover"));
    QVERIFY2(!bloco.isEmpty(), "Regra QPushButton#iconButton:hover ausente no QSS");

    QVERIFY2(bloco.contains(QStringLiteral(" color: %1;").arg(Theme::acentoTexto())),
        "iconButton:hover não usa @@ACC_TEXT@@ como color");
    QVERIFY2(!bloco.contains(QStringLiteral(" color: %1;").arg(Theme::acento())),
        "iconButton:hover ainda usa @@ACC@@ cru como color — bug #198!");
}

void TestThemeTokens::iconButtonPressed_usaAccTextNaoAccCru()
{
    const QString qss = Theme::qssParaTeste();
    const QString bloco = blocoDoQss(qss, QStringLiteral("QPushButton#iconButton:pressed"));
    QVERIFY2(!bloco.isEmpty(), "Regra QPushButton#iconButton:pressed ausente no QSS");

    QVERIFY2(bloco.contains(QStringLiteral(" color: %1;").arg(Theme::acentoTexto())),
        "iconButton:pressed não usa @@ACC_TEXT@@ como color");
    QVERIFY2(!bloco.contains(QStringLiteral(" color: %1;").arg(Theme::acento())),
        "iconButton:pressed ainda usa @@ACC@@ cru como color — bug #198!");
}

void TestThemeTokens::headerSectionHover_usaAccTextNaoAccCru()
{
    const QString qss = Theme::qssParaTeste();
    const QString bloco = blocoDoQss(qss, QStringLiteral("QHeaderView::section:hover"));
    QVERIFY2(!bloco.isEmpty(), "Regra QHeaderView::section:hover ausente no QSS");

    QVERIFY2(bloco.contains(QStringLiteral(" color: %1;").arg(Theme::acentoTexto())),
        "QHeaderView::section:hover não usa @@ACC_TEXT@@ como color");
    QVERIFY2(!bloco.contains(QStringLiteral(" color: %1;").arg(Theme::acento())),
        "QHeaderView::section:hover ainda usa @@ACC@@ cru como color — bug #198!");
}

// ── #198 (A11Y-02): foco visível na sidebar ─────────────────────────────────

void TestThemeTokens::navItemFoco_temIndicadorVisivel()
{
    const QString qss = Theme::qssParaTeste();
    const QString bloco = blocoDoQss(qss, QStringLiteral("QWidget#sidebarNav QPushButton#navItem:focus"));
    QVERIFY2(!bloco.isEmpty(), "Regra #navItem:focus ausente no QSS");

    QVERIFY2(!bloco.contains(QStringLiteral("outline: none")),
        "#navItem:focus ainda suprime outline sem substituto — regressão do bug #198 (A11Y-02)");
    QVERIFY2(bloco.contains(QStringLiteral("border: 1px solid %1;").arg(Theme::acento())),
        qPrintable(QStringLiteral("#navItem:focus sem borda indicadora %1 — foco de teclado invisível (WCAG 2.4.7)")
            .arg(Theme::acento())));
    QVERIFY2(bloco.contains(QStringLiteral("background:")),
        "#navItem:focus sem fundo diferenciado do repouso");
}

void TestThemeTokens::navItemFoco_contrasteBordaSobreNavy_AA()
{
    // Indicador NÃO-TEXTUAL (borda) — mínimo WCAG 1.4.11 é 3:1, não 4.5:1.
    const QColor acc(Theme::acento());
    const qreal c1 = contraste(acc, QColor(Theme::sidebar1()));
    const qreal c2 = contraste(acc, QColor(Theme::sidebar2()));
    QVERIFY2(c1 >= 3.0,
        qPrintable(QStringLiteral("borda de foco (%1) sobre sidebar1 (%2) = %3:1 — abaixo do mínimo 3:1 não-texto")
            .arg(Theme::acento(), Theme::sidebar1()).arg(c1, 0, 'f', 2)));
    QVERIFY2(c2 >= 3.0,
        qPrintable(QStringLiteral("borda de foco (%1) sobre sidebar2 (%2) = %3:1 — abaixo do mínimo 3:1 não-texto")
            .arg(Theme::acento(), Theme::sidebar2()).arg(c2, 0, 'f', 2)));
}

// ── A11Y-05: positivoTexto()/negativoTexto() ─────────────────────────────────

void TestThemeTokens::positivoTexto_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    QCOMPARE(Theme::positivoTexto(), QStringLiteral("#0D7A50"));
}

void TestThemeTokens::positivoTexto_dark()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    QCOMPARE(Theme::positivoTexto(), QStringLiteral("#34D399"));
    Theme::setMode(Theme::ThemeMode::Light); // restaura
}

void TestThemeTokens::negativoTexto_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    QCOMPARE(Theme::negativoTexto(), QStringLiteral("#9B1C1C"));
}

void TestThemeTokens::negativoTexto_dark()
{
    Theme::setMode(Theme::ThemeMode::Dark);
    QCOMPARE(Theme::negativoTexto(), QStringLiteral("#FCA5A5"));
    Theme::setMode(Theme::ThemeMode::Light); // restaura
}

// RED->GREEN anchor (A11Y-05): prova o "porquê" do fix — positivo()/negativo()
// crus, usados como TEXTO sobre @@SURF@@, reprovam AA. Se este teste um dia
// passar a QVERIFY(...>= 4.5), a paleta mudou e as variantes *Texto() podem
// ter deixado de ser necessárias (revisar a decisão A11Y-05).
void TestThemeTokens::positivoCru_comoTexto_reprovaAA_regressao_light()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const qreal c = contraste(QColor(Theme::positivo()), QColor(Theme::surface()));
    QVERIFY2(c < 4.5,
        qPrintable(QStringLiteral("positivo() cru sobre surface (light) = %1:1 — esperava <4.5 (reprova AA)")
            .arg(c, 0, 'f', 2)));
}

void TestThemeTokens::negativoCru_comoTexto_reprovaAA_regressao()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const qreal cLight = contraste(QColor(Theme::negativo()), QColor(Theme::surface()));
    QVERIFY2(cLight < 4.5,
        qPrintable(QStringLiteral("negativo() cru sobre surface (light) = %1:1 — esperava <4.5 (reprova AA)")
            .arg(cLight, 0, 'f', 2)));

    Theme::setMode(Theme::ThemeMode::Dark);
    const qreal cDark = contraste(QColor(Theme::negativo()), QColor(Theme::surface()));
    Theme::setMode(Theme::ThemeMode::Light); // restaura
    QVERIFY2(cDark < 4.5,
        qPrintable(QStringLiteral("negativo() cru sobre surface (dark) = %1:1 — esperava <4.5 (reprova AA)")
            .arg(cDark, 0, 'f', 2)));
}

void TestThemeTokens::positivoTexto_comoTexto_passaAA_light_e_dark()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const qreal cLight = contraste(QColor(Theme::positivoTexto()), QColor(Theme::surface()));
    QVERIFY2(cLight >= 4.5,
        qPrintable(QStringLiteral("positivoTexto() sobre surface (light) = %1:1 — esperava >=4.5")
            .arg(cLight, 0, 'f', 2)));

    Theme::setMode(Theme::ThemeMode::Dark);
    const qreal cDark = contraste(QColor(Theme::positivoTexto()), QColor(Theme::surface()));
    Theme::setMode(Theme::ThemeMode::Light); // restaura
    QVERIFY2(cDark >= 4.5,
        qPrintable(QStringLiteral("positivoTexto() sobre surface (dark) = %1:1 — esperava >=4.5")
            .arg(cDark, 0, 'f', 2)));
}

void TestThemeTokens::negativoTexto_comoTexto_passaAA_light_e_dark()
{
    Theme::setMode(Theme::ThemeMode::Light);
    const qreal cLight = contraste(QColor(Theme::negativoTexto()), QColor(Theme::surface()));
    QVERIFY2(cLight >= 4.5,
        qPrintable(QStringLiteral("negativoTexto() sobre surface (light) = %1:1 — esperava >=4.5")
            .arg(cLight, 0, 'f', 2)));

    Theme::setMode(Theme::ThemeMode::Dark);
    const qreal cDark = contraste(QColor(Theme::negativoTexto()), QColor(Theme::surface()));
    Theme::setMode(Theme::ThemeMode::Light); // restaura
    QVERIFY2(cDark >= 4.5,
        qPrintable(QStringLiteral("negativoTexto() sobre surface (dark) = %1:1 — esperava >=4.5")
            .arg(cDark, 0, 'f', 2)));
}

QTEST_MAIN(TestThemeTokens)
#include "test_theme_tokens.moc"
