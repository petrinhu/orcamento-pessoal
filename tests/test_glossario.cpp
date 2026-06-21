// PeuBud - GlossarioService TDD (#103, #104, #166, #187)

#include <QtTest/QtTest>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>
#include "core/settings/AppSettings.h"
#include "services/GlossarioService.h"
#include "ui/GlossarioDecorator.h"
#include "ui/Theme.h"

class TestGlossario : public QObject
{
    Q_OBJECT
private slots:
    void definicaoCetNaoVazia();
    void definicaoFireMencionaAposentadoria();
    void definicaoTermoDesconhecidoVazia();
    void microcopyCet();
    void labelColoquialFire();
    void labelColoquialTermoSemColoquialVazio();
    void termosCobreMinimo50Entradas();
    void primeiroTermoEncontradoCetEmFrase();
    void primeiroTermoEncontradoNenhumQuandoVazio();
    void caseInsensitive();
    void aplicarLabelsColoquiaisRenomeia();
    void aplicarLabelsRespeitaModoAvancado();
    void aplicarMicrocopiesAdicionaLabelCinza();
    void aplicarMicrocopiesIdempotente();

    // #187 — JCP IRRF 15% -> 17,5% (LC 224/2025, vigente jan/2026)
    // Precisao financeira: a definicao do JCP deve mencionar 17,5%.
    void jcpDefinicaoMenciona17v5Porcento();
    // Microcopy do JCP deve referenciar 17,5% (nao 15%).
    void jcpMicrocopyMenciona17v5();

    // #166 Peca 3 — dados do glossario: correcoes de precisao e termos novos
    // DRE deve ter UMA unica entrada no glossario (chave duplicada corrigida).
    void dreUmaEntradaApenas();
    // INSS: definicao NAO deve conter valor nominal de teto (ex: "908,85").
    void inssDefinicaoSemValorNominalDeTeto();
    // Termos novos obrigatorios (marcados com estrela na spec):
    void termoNovoProvento();
    void termoNovoDividendo();
    void termoNovoTicker();
    void termoNovoAporte();
    void termoNovoResgate();
    void termoNovoRendimento();
    void termoNovoCAGR();
    void termoNovoPayback();
    void termoNovoBonificacao();
    void termoNovoIRRF();
    void termoNovoIpca();       // IPCA+ (novo)
    void termoNovoLiquidez();   // Liquidez (novo)
    void termoNovoAcao();       // Acao (novo)
    void termoNovoIbovespa();   // Ibovespa (novo)
    void termoNovoCarteira();   // Carteira (novo)
    // Termos ja existentes: verificar que continuam presentes (regressao).
    void termoExistenteFgc();
    void termoExistenteCdi();

    // #166 Peca 3 — mecanismo: aplicarTooltips nao altera text() do widget
    void tooltipNaoDestruiTextoDoWidget();
    // aplicarTooltips e idempotente (2a chamada nao duplica/altera tooltip).
    void tooltipIdempotente();
    // aplicarTooltips seta tooltip quando termo encontrado.
    void tooltipSetadoQuandoTermoEncontrado();
    // QSS gerado contem regra de QToolTip (tooltip escuro).
    void qssContemRegraQToolTip();
};

void TestGlossario::definicaoCetNaoVazia()
{
    const QString d = GlossarioService::definicao(QStringLiteral("CET"));
    QVERIFY(!d.isEmpty());
    QVERIFY(d.contains(QStringLiteral("Custo Efetivo Total")));
}

void TestGlossario::definicaoFireMencionaAposentadoria()
{
    const QString d = GlossarioService::definicao(QStringLiteral("FIRE"));
    QVERIFY(d.toLower().contains(QStringLiteral("aposentadoria")));
}

void TestGlossario::definicaoTermoDesconhecidoVazia()
{
    QCOMPARE(GlossarioService::definicao(QStringLiteral("XYZQ123")), QString());
}

void TestGlossario::microcopyCet()
{
    const QString m = GlossarioService::microcopy(QStringLiteral("CET"));
    QVERIFY(!m.isEmpty());
    QVERIFY(m.startsWith(QStringLiteral("="))); // padrão microcopy
}

void TestGlossario::labelColoquialFire()
{
    const QString l = GlossarioService::labelColoquial(QStringLiteral("FIRE"));
    QVERIFY(l.contains(QStringLiteral("(FIRE)")));
    QVERIFY(l.toLower().contains(QStringLiteral("aposentadoria")));
}

void TestGlossario::labelColoquialTermoSemColoquialVazio()
{
    // SAC nao tem renomeacao coloquial (sigla ja conhecida)
    QCOMPARE(GlossarioService::labelColoquial(QStringLiteral("SAC")), QString());
}

void TestGlossario::termosCobreMinimo50Entradas()
{
    const auto t = GlossarioService::termos();
    QVERIFY2(t.size() >= 50,
        qPrintable(QStringLiteral("apenas %1 termos").arg(t.size())));
}

void TestGlossario::primeiroTermoEncontradoCetEmFrase()
{
    QCOMPARE(GlossarioService::primeiroTermoEncontrado(
                 QStringLiteral("Taxa CET ao ano")),
             QStringLiteral("CET"));
}

void TestGlossario::primeiroTermoEncontradoNenhumQuandoVazio()
{
    QCOMPARE(GlossarioService::primeiroTermoEncontrado(QString()), QString());
    QCOMPARE(GlossarioService::primeiroTermoEncontrado(
                 QStringLiteral("Olá mundo")), QString());
}

void TestGlossario::caseInsensitive()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("cet")).isEmpty());
    QVERIFY(!GlossarioService::definicao(QStringLiteral("Fire")).isEmpty());
    QVERIFY(!GlossarioService::definicao(QStringLiteral("ipca")).isEmpty());
}

void TestGlossario::aplicarLabelsColoquiaisRenomeia()
{
    AppSettings::instance().setModoIniciante(true);
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Calcular FIRE"), &root);
    GlossarioDecorator::aplicarLabelsColoquiais(&root);
    QVERIFY(lbl->text().contains(QStringLiteral("Aposentadoria")));
    QVERIFY(lbl->text().contains(QStringLiteral("(FIRE)")));
}

void TestGlossario::aplicarLabelsRespeitaModoAvancado()
{
    AppSettings::instance().setModoIniciante(false);
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Calcular FIRE"), &root);
    GlossarioDecorator::aplicarLabelsColoquiais(&root);
    QCOMPARE(lbl->text(), QStringLiteral("Calcular FIRE"));
    AppSettings::instance().setModoIniciante(true); // restaura default
}

void TestGlossario::aplicarMicrocopiesAdicionaLabelCinza()
{
    QWidget root;
    auto *form = new QFormLayout(&root);
    form->addRow(new QLabel(QStringLiteral("CET (% a.a.):"), &root),
                 new QLineEdit(&root));
    GlossarioDecorator::aplicarMicrocopies(&root);
    // 1 linha original + 1 linha microcopy = 2.
    QCOMPARE(form->rowCount(), 2);
    auto *fieldItem = form->itemAt(1, QFormLayout::FieldRole);
    QVERIFY(fieldItem != nullptr);
    auto *micro = qobject_cast<QLabel*>(fieldItem->widget());
    QVERIFY(micro != nullptr);
    QVERIFY(micro->text().startsWith(QStringLiteral("=")));
    QVERIFY(micro->property("microcopy").toBool());
    QVERIFY(micro->styleSheet().contains(QStringLiteral("gray")));
}

void TestGlossario::aplicarMicrocopiesIdempotente()
{
    QWidget root;
    auto *form = new QFormLayout(&root);
    form->addRow(new QLabel(QStringLiteral("CET"), &root),
                 new QLineEdit(&root));
    GlossarioDecorator::aplicarMicrocopies(&root);
    GlossarioDecorator::aplicarMicrocopies(&root); // 2a chamada nao duplica
    QCOMPARE(form->rowCount(), 2);
}

// ── #187 — JCP IRRF 15% -> 17,5% ─────────────────────────────────────────────

void TestGlossario::jcpDefinicaoMenciona17v5Porcento()
{
    const QString d = GlossarioService::definicao(QStringLiteral("JCP"));
    QVERIFY2(!d.isEmpty(), "JCP deve ter definicao cadastrada");
    // Aceita "17,5%" ou "17.5%" (ponto ou virgula decimal).
    const bool tem175 = d.contains(QStringLiteral("17,5")) ||
                        d.contains(QStringLiteral("17.5"));
    QVERIFY2(tem175,
        qPrintable(QStringLiteral("JCP definicao deve mencionar 17,5%% mas got: %1").arg(d)));
    // Nao deve mais mencionar 15% como aliquota (era o valor antigo).
    QVERIFY2(!d.contains(QStringLiteral("15%")),
        "JCP definicao nao deve mencionar 15% (aliquota revogada em jan/2026)");
}

void TestGlossario::jcpMicrocopyMenciona17v5()
{
    const QString m = GlossarioService::microcopy(QStringLiteral("JCP"));
    QVERIFY2(!m.isEmpty(), "JCP deve ter microcopy");
    const bool tem175 = m.contains(QStringLiteral("17,5")) ||
                        m.contains(QStringLiteral("17.5"));
    QVERIFY2(tem175,
        qPrintable(QStringLiteral("JCP microcopy deve mencionar 17,5%% mas got: %1").arg(m)));
}

// ── #166 Peca 3 — dados do glossario ─────────────────────────────────────────

void TestGlossario::dreUmaEntradaApenas()
{
    // QHash com chave duplicada: a segunda insercao sobrescreve a primeira de
    // forma nao-obvia. O contrato e: DRE tem exatamente uma definicao e ela
    // menciona "Resultado" (definicao canonica, Grupo 9 da spec).
    const QString d = GlossarioService::definicao(QStringLiteral("DRE"));
    QVERIFY2(!d.isEmpty(), "DRE deve ter definicao");
    // A definicao canonica (Grupo 9) menciona "Resultado".
    QVERIFY2(d.toLower().contains(QStringLiteral("resultado")),
        qPrintable(QStringLiteral("DRE: definicao esperada menciona Resultado, got: %1").arg(d)));
    // Nao deve ser a definicao do grupo "Outros" (relatorio anual de lucro) —
    // a chave duplicada foi eliminada. Verificar que e a definicao do Grupo 9
    // (menciona "receitas" E "despesas"), nao so "relatorio anual de lucro".
    QVERIFY2(d.toLower().contains(QStringLiteral("receita")),
        "DRE: definicao canonica (Grupo 9) deve mencionar 'receitas'");
}

void TestGlossario::inssDefinicaoSemValorNominalDeTeto()
{
    const QString d = GlossarioService::definicao(QStringLiteral("INSS"));
    QVERIFY2(!d.isEmpty(), "INSS deve ter definicao");
    // O teto do INSS e reajustado anualmente — nunca cravar valor nominal.
    // Verificar que a string nao contem o formato de valor em reais (R$ NNN).
    // Aceita "teto" em frase segura: "ate um teto mensal".
    QVERIFY2(!d.contains(QStringLiteral("908")),
        "INSS: definicao nao deve conter valor nominal do teto (ex: 908,85)");
    QVERIFY2(!d.contains(QStringLiteral("1.412")),
        "INSS: definicao nao deve conter valor nominal do teto");
    QVERIFY2(d.toLower().contains(QStringLiteral("teto")),
        "INSS: definicao deve mencionar 'teto' (frase segura)");
}

// Termos novos (estrela na spec) — verificar presenca e definicao nao-vazia

void TestGlossario::termoNovoProvento()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("PROVENTO")).isEmpty());
}

void TestGlossario::termoNovoDividendo()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("DIVIDENDO")).isEmpty());
}

void TestGlossario::termoNovoTicker()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("TICKER")).isEmpty());
}

void TestGlossario::termoNovoAporte()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("APORTE")).isEmpty());
}

void TestGlossario::termoNovoResgate()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("RESGATE")).isEmpty());
}

void TestGlossario::termoNovoRendimento()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("RENDIMENTO")).isEmpty());
}

void TestGlossario::termoNovoCAGR()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("CAGR")).isEmpty());
}

void TestGlossario::termoNovoPayback()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("PAYBACK")).isEmpty());
}

void TestGlossario::termoNovoBonificacao()
{
    // Chave sem acento (padrao do arquivo)
    QVERIFY(!GlossarioService::definicao(QStringLiteral("BONIFICACAO")).isEmpty());
}

void TestGlossario::termoNovoIRRF()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("IRRF")).isEmpty());
}

void TestGlossario::termoNovoIpca()
{
    // IPCA+ (IPCA mais taxa fixa)
    const QString d = GlossarioService::definicao(QStringLiteral("IPCA+"));
    QVERIFY2(!d.isEmpty(), "IPCA+ (novo) deve ter definicao");
}

void TestGlossario::termoNovoLiquidez()
{
    const QString d = GlossarioService::definicao(QStringLiteral("LIQUIDEZ"));
    QVERIFY2(!d.isEmpty(), "LIQUIDEZ (novo) deve ter definicao");
}

void TestGlossario::termoNovoAcao()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("ACAO")).isEmpty());
}

void TestGlossario::termoNovoIbovespa()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("IBOVESPA")).isEmpty());
}

void TestGlossario::termoNovoCarteira()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("CARTEIRA")).isEmpty());
}

// Termos existentes: regressao (nao devem desaparecer com a atualizacao)

void TestGlossario::termoExistenteFgc()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("FGC")).isEmpty());
}

void TestGlossario::termoExistenteCdi()
{
    QVERIFY(!GlossarioService::definicao(QStringLiteral("CDI")).isEmpty());
}

// ── #166 Peca 3 — mecanismo GlossarioDecorator ───────────────────────────────

void TestGlossario::tooltipNaoDestruiTextoDoWidget()
{
    // Regressao do bug de sobrescrever: aplicarTooltips NAO deve alterar text().
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Taxa CET"), &root);
    const QString textoOriginal = lbl->text();
    GlossarioDecorator::aplicarTooltips(&root);
    QCOMPARE(lbl->text(), textoOriginal);
}

void TestGlossario::tooltipIdempotente()
{
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Sobre CET e IOF"), &root);
    GlossarioDecorator::aplicarTooltips(&root);
    const QString tooltipApos1 = lbl->toolTip();
    GlossarioDecorator::aplicarTooltips(&root); // 2a chamada
    QCOMPARE(lbl->toolTip(), tooltipApos1);
}

void TestGlossario::tooltipSetadoQuandoTermoEncontrado()
{
    QWidget root;
    auto *lbl = new QLabel(QStringLiteral("Aliquota IRRF"), &root);
    QVERIFY(lbl->toolTip().isEmpty()); // precondition
    GlossarioDecorator::aplicarTooltips(&root);
    QVERIFY2(!lbl->toolTip().isEmpty(),
        "IRRF no texto do label deve gerar tooltip nao-vazio");
}

void TestGlossario::qssContemRegraQToolTip()
{
    // Verifica que o QSS global contem uma regra de QToolTip com fundo escuro
    // (tooltip escuro conforme §9.6 do manual de estilo, #166 Peca 3).
    // Theme::qssParaTeste() nao precisa de QApplication — gera o QSS puro.
    const QString qss = Theme::qssParaTeste();

    // 1. Regra QToolTip presente
    QVERIFY2(qss.contains(QStringLiteral("QToolTip")),
        "QSS deve conter regra QToolTip (tooltip escuro §9.6)");

    // 2. A regra inteira (ate o fechamento da chave) contem background-color.
    // Extraimos um trecho maior para cobrir a regra completa (~10 linhas x 40 chars).
    const int idx = qss.indexOf(QStringLiteral("QToolTip"));
    QVERIFY2(idx >= 0, "QToolTip nao encontrado no QSS (inconsistencia interna)");
    // 600 chars cobre a regra QToolTip inteira (8 propriedades, ~480 chars)
    const QString trecho = qss.mid(idx, 600);
    QVERIFY2(trecho.contains(QStringLiteral("background-color")),
        qPrintable(QStringLiteral("Regra QToolTip deve definir background-color. Trecho: %1").arg(trecho.left(300))));

    // 3. Fundo e escuro (navy da sidebar: #1B2A41)
    QVERIFY2(trecho.contains(QStringLiteral("#1B2A41")) ||
             trecho.toLower().contains(QStringLiteral("#1b2a41")),
        "Tooltip deve usar fundo navy escuro #1B2A41 (contraste 9.07:1 WCAG AA)");
}

QTEST_MAIN(TestGlossario)
#include "test_glossario.moc"
