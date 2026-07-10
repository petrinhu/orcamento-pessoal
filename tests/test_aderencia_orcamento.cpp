// PeuBud - #203b (Onda U, A11Y-07): a coluna "%" do Planejamento sinalizava
// aderência (bem dentro/estourou) só por cor (verde/azul/vermelho) —
// daltônicos perdem a info (WCAG 1.4.1). Fix: sufixo textual redundante
// (" ✓"/" ⚠") além da cor, via aderencia::calcular/sufixo (puro, sem
// QApplication/DB — extraído de PlanejamentoWidget::populateTable).

#include <QtTest/QtTest>

#include "ui/AderenciaOrcamento.h"

class TestAderenciaOrcamento : public QObject
{
    Q_OBJECT

private slots:
    // ── Despesa ──────────────────────────────────────────────────────────────
    void despesaGastouMenos_ehBomComCheck();
    void despesaDentroDaFaixa_ehModeradoSemSufixo();
    void despesaEstourou_ehAlertaComAviso();

    // ── Receita (invertida) ──────────────────────────────────────────────────
    void receitaRecebeuMais_ehBomComCheck();
    void receitaDentroDaFaixa_ehModeradoSemSufixo();
    void receitaRecebeuMenos_ehAlertaComAviso();

    // ── Sem meta ─────────────────────────────────────────────────────────────
    void planZero_ehSemMetaSemSufixo();

    // ── WCAG 1.4.1: sufixos distintos por nível (não depende de cor) ─────────
    void sufixosSaoTextualmenteDistintosPorNivel();
};

// ── Despesa ──────────────────────────────────────────────────────────────────

void TestAderenciaOrcamento::despesaGastouMenos_ehBomComCheck()
{
    // real = 80% do plan (≤ 90%) → Bom
    const auto nivel = aderencia::calcular(800, 1000, /*isDespesa=*/true);
    QCOMPARE(nivel, aderencia::Nivel::Bom);
    QCOMPARE(aderencia::sufixo(nivel), QStringLiteral(" ✓"));
}

void TestAderenciaOrcamento::despesaDentroDaFaixa_ehModeradoSemSufixo()
{
    // real = 100% do plan (dentro de ±10%) → Moderado
    const auto nivel = aderencia::calcular(1000, 1000, /*isDespesa=*/true);
    QCOMPARE(nivel, aderencia::Nivel::Moderado);
    QVERIFY(aderencia::sufixo(nivel).isEmpty());
}

void TestAderenciaOrcamento::despesaEstourou_ehAlertaComAviso()
{
    // real = 150% do plan (> 110%) → Alerta
    const auto nivel = aderencia::calcular(1500, 1000, /*isDespesa=*/true);
    QCOMPARE(nivel, aderencia::Nivel::Alerta);
    QCOMPARE(aderencia::sufixo(nivel), QStringLiteral(" ⚠"));
}

// ── Receita (invertida) ───────────────────────────────────────────────────────

void TestAderenciaOrcamento::receitaRecebeuMais_ehBomComCheck()
{
    // real = 150% do plan (≥ 110%) → Bom (receita: mais é melhor)
    const auto nivel = aderencia::calcular(1500, 1000, /*isDespesa=*/false);
    QCOMPARE(nivel, aderencia::Nivel::Bom);
    QCOMPARE(aderencia::sufixo(nivel), QStringLiteral(" ✓"));
}

void TestAderenciaOrcamento::receitaDentroDaFaixa_ehModeradoSemSufixo()
{
    const auto nivel = aderencia::calcular(1000, 1000, /*isDespesa=*/false);
    QCOMPARE(nivel, aderencia::Nivel::Moderado);
    QVERIFY(aderencia::sufixo(nivel).isEmpty());
}

void TestAderenciaOrcamento::receitaRecebeuMenos_ehAlertaComAviso()
{
    // real = 80% do plan (≤ 90%) → Alerta (receita: menos é pior)
    const auto nivel = aderencia::calcular(800, 1000, /*isDespesa=*/false);
    QCOMPARE(nivel, aderencia::Nivel::Alerta);
    QCOMPARE(aderencia::sufixo(nivel), QStringLiteral(" ⚠"));
}

// ── Sem meta ───────────────────────────────────────────────────────────────────

void TestAderenciaOrcamento::planZero_ehSemMetaSemSufixo()
{
    const auto nivel = aderencia::calcular(500, 0, /*isDespesa=*/true);
    QCOMPARE(nivel, aderencia::Nivel::SemMeta);
    QVERIFY(aderencia::sufixo(nivel).isEmpty());
}

// ── WCAG 1.4.1 ───────────────────────────────────────────────────────────────

void TestAderenciaOrcamento::sufixosSaoTextualmenteDistintosPorNivel()
{
    // Bom e Alerta precisam ter sufixo NÃO-VAZIO e DIFERENTE entre si —
    // caso contrário a única diferença entre os estados seria a cor.
    const QString sufBom    = aderencia::sufixo(aderencia::Nivel::Bom);
    const QString sufAlerta = aderencia::sufixo(aderencia::Nivel::Alerta);
    QVERIFY(!sufBom.isEmpty());
    QVERIFY(!sufAlerta.isEmpty());
    QVERIFY(sufBom != sufAlerta);
}

QTEST_APPLESS_MAIN(TestAderenciaOrcamento)
#include "test_aderencia_orcamento.moc"
