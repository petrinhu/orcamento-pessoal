// PeuBud - GlossaryLabel TDD (#166 Peca 3 §9.6)
//
// Contrato:
//   (a) text() NAO e alterado apos construcao — nao-destrutivo.
//   (b) tooltip e setado a partir de GlossarioService::definicao(termo).
//   (c) Constroi sem crash para termo cadastrado e para termo desconhecido.
//   (d) Cursor e WhatsThisCursor quando o termo tem definicao.
//   (e) Tooltip e VAZIO quando o termo nao existe no glossario.
//   (f) Termo com ":" final: lookup remove o ":" (ex.: "CET:" -> def de "CET").

#include <QtTest>
#include <QLabel>
#include <QWidget>

#include "services/GlossarioService.h"
#include "ui/widgets/GlossaryLabel.h"

class TestGlossaryLabel : public QObject
{
    Q_OBJECT
private slots:
    // (a) text() preservado — nao-destrutivo
    void textPreservado_termoCadastrado();
    void textPreservado_termoDesconhecido();

    // (b) tooltip vem do GlossarioService
    void tooltipSetadoParaTermoCadastrado();
    void tooltipMatchaDefinicaoDoServico();

    // (c) construcao sem crash
    void constroiSemCrash_termoCadastrado();
    void constroiSemCrash_termoDesconhecido();

    // (d) cursor correto
    void cursorWhatsThisQuandoTermoCadastrado();
    // Termo desconhecido nao deve mudar o cursor
    void cursorNaoMudaParaTermoDesconhecido();

    // (e) tooltip vazio para termo desconhecido
    void tooltipVazioParaTermoDesconhecido();

    // (f) ":" final e ignorado no lookup
    void doisPontosFinaisIgnoradosNoLookup();

    // A11Y-06 (WCAG 2.1.1/1.4.13): acessivel por teclado/leitor de tela
    void accessibleDescription_naoVaziaParaTermoCadastrado();
    void accessibleDescription_matchaTooltip();
    void accessibleDescription_vaziaParaTermoDesconhecido();
    void focavelPorTeclado_termoCadastrado();
    void naoFocavel_termoDesconhecido();
};

// ── (a) text() preservado ─────────────────────────────────────────────────────

void TestGlossaryLabel::textPreservado_termoCadastrado()
{
    const QString termo = QStringLiteral("CET");
    GlossaryLabel lbl(termo);
    QCOMPARE(lbl.text(), termo);
}

void TestGlossaryLabel::textPreservado_termoDesconhecido()
{
    const QString termo = QStringLiteral("XYZQ999");
    GlossaryLabel lbl(termo);
    QCOMPARE(lbl.text(), termo);
}

// ── (b) tooltip do GlossarioService ──────────────────────────────────────────

void TestGlossaryLabel::tooltipSetadoParaTermoCadastrado()
{
    GlossaryLabel lbl(QStringLiteral("CDI"));
    QVERIFY2(!lbl.toolTip().isEmpty(),
             "GlossaryLabel com termo 'CDI' deve ter tooltip nao-vazio");
}

void TestGlossaryLabel::tooltipMatchaDefinicaoDoServico()
{
    const QString termo = QStringLiteral("PMT");
    const QString defEsperada = GlossarioService::definicao(QStringLiteral("PMT"));
    QVERIFY(!defEsperada.isEmpty()); // pre-condicao: servico tem o termo

    GlossaryLabel lbl(termo);
    QCOMPARE(lbl.toolTip(), defEsperada);
}

// ── (c) construcao sem crash ──────────────────────────────────────────────────

void TestGlossaryLabel::constroiSemCrash_termoCadastrado()
{
    // Nao deve crashar; o simples fato de executar sem QFAIL ja valida isso.
    GlossaryLabel lbl(QStringLiteral("PV"));
    QVERIFY(!lbl.text().isEmpty());
}

void TestGlossaryLabel::constroiSemCrash_termoDesconhecido()
{
    GlossaryLabel lbl(QStringLiteral("ZZZUNKNOWN123"));
    QCOMPARE(lbl.text(), QStringLiteral("ZZZUNKNOWN123"));
}

// ── (d) cursor ────────────────────────────────────────────────────────────────

void TestGlossaryLabel::cursorWhatsThisQuandoTermoCadastrado()
{
    GlossaryLabel lbl(QStringLiteral("FV"));
    QCOMPARE(lbl.cursor().shape(), Qt::WhatsThisCursor);
}

void TestGlossaryLabel::cursorNaoMudaParaTermoDesconhecido()
{
    GlossaryLabel lbl(QStringLiteral("XYZQ999"));
    // Cursor padrao do QLabel e Arrow (nao WhatsThis)
    QVERIFY(lbl.cursor().shape() != Qt::WhatsThisCursor);
}

// ── (e) tooltip vazio para desconhecido ──────────────────────────────────────

void TestGlossaryLabel::tooltipVazioParaTermoDesconhecido()
{
    GlossaryLabel lbl(QStringLiteral("XYZQ999"));
    QVERIFY2(lbl.toolTip().isEmpty(),
             "Termo desconhecido nao deve gerar tooltip");
}

// ── (f) ":" final ignorado ────────────────────────────────────────────────────

void TestGlossaryLabel::doisPontosFinaisIgnoradosNoLookup()
{
    // "CET:" deve ter o mesmo tooltip que "CET"
    GlossaryLabel lblComDoisPontos(QStringLiteral("CET:"));
    GlossaryLabel lblSemDoisPontos(QStringLiteral("CET"));

    // Ambos devem ter tooltip nao-vazio
    QVERIFY(!lblComDoisPontos.toolTip().isEmpty());
    // E devem ser iguais (mesmo lookup)
    QCOMPARE(lblComDoisPontos.toolTip(), lblSemDoisPontos.toolTip());
    // text() preservado (com ":" no primeiro)
    QCOMPARE(lblComDoisPontos.text(), QStringLiteral("CET:"));
    QCOMPARE(lblSemDoisPontos.text(), QStringLiteral("CET"));
}

// ── A11Y-06: acessivel por teclado/leitor de tela ────────────────────────────

void TestGlossaryLabel::accessibleDescription_naoVaziaParaTermoCadastrado()
{
    GlossaryLabel lbl(QStringLiteral("CET"));
    QVERIFY2(!lbl.accessibleDescription().isEmpty(),
        "GlossaryLabel com termo cadastrado deve ter accessibleDescription "
        "nao-vazia (leitor de tela precisa ler a definicao sem depender de hover)");
}

void TestGlossaryLabel::accessibleDescription_matchaTooltip()
{
    GlossaryLabel lbl(QStringLiteral("CDI"));
    QCOMPARE(lbl.accessibleDescription(), lbl.toolTip());
}

void TestGlossaryLabel::accessibleDescription_vaziaParaTermoDesconhecido()
{
    GlossaryLabel lbl(QStringLiteral("XYZQ999"));
    QVERIFY(lbl.accessibleDescription().isEmpty());
}

void TestGlossaryLabel::focavelPorTeclado_termoCadastrado()
{
    GlossaryLabel lbl(QStringLiteral("PV"));
    // TabFocus: alcancavel por Tab (WCAG 2.1.1), sem virar alvo de clique
    // (QLabel nao e um controle interativo de mouse).
    QCOMPARE(lbl.focusPolicy(), Qt::TabFocus);
}

void TestGlossaryLabel::naoFocavel_termoDesconhecido()
{
    // Termo sem definicao: nao decora, nao entra na tab order (minima
    // poluicao — so labels com conteudo real ficam focaveis).
    GlossaryLabel lbl(QStringLiteral("ZZZUNKNOWN123"));
    QCOMPARE(lbl.focusPolicy(), Qt::NoFocus);
}

QTEST_MAIN(TestGlossaryLabel)
#include "test_glossary_label.moc"
