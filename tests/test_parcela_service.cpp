// PeuBud - ParcelaService TDD (#18 suite).
// Foca em funcoes puras: CET, conversao moeda, parse/format, datas.
// Funcoes que dependem de DB (recalcularAposEdicao) ficam para integracao.

#include <QtTest/QtTest>
#include <QSettings>
#include <QTemporaryDir>

#include "core/settings/AppSettings.h"
#include "services/ParcelaService.h"
#include "models/Compra.h"

class TestParcelaService : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void cetFromJurosMensaisCompose();
    void jurosMensaisFromCetInverso();
    void formatarValorBRL();
    void parsearValorBRComDecimal();
    void parsearValorSoPontoGrupoDeMilhar();
    void simboloMoedaConhecidas();

    // ── #204 (Onda U, follow-up i18n do #197/I18N-01): formatarValor era
    // gêmeo locale-naive de uifmt::formatCentavosBRL, mas com moeda EXPLÍCITA
    // por chamador (Compra pode estar em moeda != moeda padrão do app) --
    // fix segue a MESMA aritmética inteira/QLocale, só o separador/posição
    // do sinal segue AppSettings::language() (não a moeda em si).
    void formatarValorPtBrGoldenComAgrupamentoDeMilhar();
    void formatarValorEnIntlUsdFormataComoDolar();
    void formatarValorEnIntlNegativoSinalAntesDoSimbolo();
    void formatarValorPtBrNegativoSinalDepoisDoSimbolo();
    void formatarValorMoedaExplicitaIndependeDaMoedaAtivaDoApp();

    // ── Amortizacao Price (#114) ─────────────────────────────────────────────
    void amortSaldoFinalZera();
    void amortSomaParcelasIgualTotalPago();
    void amortJurosDecrescentes();
    void amortPrimeiraMaisJurosQueUltima();
    void amortParcelaUnica();
    void amortSemJuros();
    void amortBordaInvalida();

    // ── resolverTaxa via SolveFor::Taxa (#114) ───────────────────────────────
    void resolverTaxaRecuperaTaxaConhecida();
    void resolverTaxaSemJurosRetornaZero();
    void resolverTaxaParcelaUnicaSemCrash();
    void resolverTaxaPmtZeroSemCrash();

    // ── dataParaParcela/gerarDatas (#199, Onda U, QA-04) ─────────────────────
    void dataParaParcelaViradaDeAno();
    void dataParaParcelaOffsetNegativo();
    void gerarDatasNGrandePrimeiraEUltima();

private:
    QTemporaryDir m_settingsDir;
};

// Helper: monta Compra para amortizacao. jurosMensais armazenado como % x 10000.
static Compra makeCompra(qint64 valorTotal, int numParcelas, double jurosMensaisPct)
{
    Compra c;
    c.valorTotal     = valorTotal;
    c.numParcelas    = numParcelas;
    c.parcelaInicial = 1;
    c.mesInicio      = 1;
    c.anoInicio      = 2025;
    c.jurosMensais   = static_cast<qint64>(std::llround(jurosMensaisPct * 10000.0));
    return c;
}

// Isola AppSettings (singleton sobre QSettings("petrinhu","PeuBud") -- MESMA
// config real do app instalado) num QTemporaryDir ANTES da 1a chamada de
// AppSettings::instance(). Registra os dois formatos (Ini/Native) porque
// QSettings(org,app) resolve para NativeFormat, não IniFormat -- ver
// comentário equivalente em test_locale_moeda.cpp (#197). Zero contato com
// ~/.config real.
void TestParcelaService::initTestCase()
{
    QVERIFY(m_settingsDir.isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
}

void TestParcelaService::cetFromJurosMensaisCompose()
{
    // 1% a.m. -> CET a.a. = (1.01)^12 - 1 = 12.6825%
    const double cet = ParcelaService::cetFromJurosMensais(1.0);
    QVERIFY2(qAbs(cet - 12.6825) < 1e-3,
        qPrintable(QStringLiteral("CET = %1, esperado ~12.6825").arg(cet)));
}

void TestParcelaService::jurosMensaisFromCetInverso()
{
    // CET 12.6825% -> mensal 1%
    const double mensal = ParcelaService::jurosMensaisFromCet(12.6825);
    QVERIFY2(qAbs(mensal - 1.0) < 1e-3,
        qPrintable(QStringLiteral("mensal = %1, esperado ~1.0").arg(mensal)));
}

void TestParcelaService::formatarValorBRL()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    // 123456 cents = R$ 1.234,56 (locale BR, agrupamento de milhar)
    const QString s = ParcelaService::formatarValor(123456,
                                                     QStringLiteral("BRL"));
    QCOMPARE(s, QStringLiteral("R$ 1.234,56"));
}

// ESPEC GOLDEN pt-BR/BRL (#204): valor "antes" do fix já era "R$ 1234,56"
// (vírgula decimal correta, símbolo correto) -- o comportamento VISÍVEL de
// pt-BR não regride, só ganha o separador de milhar que faltava (mesma
// convenção que #197 já estabeleceu em uifmt::formatCentavosBRL). Idioma/
// moeda continuam pt-BR/BRL explicitamente para não depender de estado
// default do singleton.
void TestParcelaService::formatarValorPtBrGoldenComAgrupamentoDeMilhar()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);

    QCOMPARE(ParcelaService::formatarValor(123456789, QStringLiteral("BRL")),
              QStringLiteral("R$ 1.234.567,89"));
    QCOMPARE(ParcelaService::formatarValor(0, QStringLiteral("BRL")),
              QStringLiteral("R$ 0,00"));
}

// ESPEC RED->GREEN (#204): antes do fix, formatarValor SEMPRE usava vírgula
// decimal + símbolo-espaço-valor (padrão pt-BR hardcoded), mesmo com
// AppSettings::language()==EnIntl. EN + USD deve virar "$1,234.56" (ponto
// decimal, símbolo colado), igual ao gêmeo uifmt::formatCentavosBRL/#197.
void TestParcelaService::formatarValorEnIntlUsdFormataComoDolar()
{
    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);

    const QString out = ParcelaService::formatarValor(123456, QStringLiteral("USD"));
    QCOMPARE(out, QStringLiteral("$1,234.56"));
    QVERIFY2(!out.contains(QLatin1Char(',')) || out.indexOf(QLatin1Char(',')) < out.indexOf(QLatin1Char('.')),
        "regressao: virgula decimal pt-BR vazando em locale EN");
}

void TestParcelaService::formatarValorEnIntlNegativoSinalAntesDoSimbolo()
{
    AppSettings::instance().setLanguage(AppSettings::Language::EnIntl);
    QCOMPARE(ParcelaService::formatarValor(-50, QStringLiteral("USD")),
              QStringLiteral("-$0.50"));
}

void TestParcelaService::formatarValorPtBrNegativoSinalDepoisDoSimbolo()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    QCOMPARE(ParcelaService::formatarValor(-50, QStringLiteral("BRL")),
              QStringLiteral("R$ -0,50"));
}

// Diferença-chave vs. uifmt::formatCentavosBRL: a moeda de formatarValor é
// EXPLÍCITA por chamador (Compra.moeda pode divergir da moeda padrão ativa
// do app) -- só o idioma de exibição (separadores/posição do sinal) segue
// AppSettings::language(). Prova que a moeda ATIVA do app não vaza aqui.
void TestParcelaService::formatarValorMoedaExplicitaIndependeDaMoedaAtivaDoApp()
{
    AppSettings::instance().setLanguage(AppSettings::Language::PtBr);
    AppSettings::instance().setDefaultCurrency(QStringLiteral("EUR"));

    // Compra explicitamente em USD, mesmo com o app "ativo" em EUR.
    const QString out = ParcelaService::formatarValor(123456, QStringLiteral("USD"));
    QVERIFY2(out.startsWith(QStringLiteral("$")),
        qPrintable(QStringLiteral("esperado simbolo USD (explicito), obtido '%1'").arg(out)));
    QVERIFY(!out.contains(QStringLiteral("€")));
}

void TestParcelaService::parsearValorBRComDecimal()
{
    // "R$ 1.234,56" -> 123456 cents
    const qint64 c = ParcelaService::parsearValor(
        QStringLiteral("R$ 1.234,56"));
    QCOMPARE(c, qint64(123456));

    // Sem prefixo
    QCOMPARE(ParcelaService::parsearValor(QStringLiteral("99,90")),
             qint64(9990));
}

// ESPEC RED->GREEN (#199/QA-03): gemeo de uifmt::parseCentavos. "1.234" (so
// ponto, sem virgula) DEVE ser 123400 centavos (R$ 1.234,00), nao 123
// centavos (R$ 1,23 -- bug original, toDouble("1.234")==1.234).
void TestParcelaService::parsearValorSoPontoGrupoDeMilhar()
{
    QCOMPARE(ParcelaService::parsearValor(QStringLiteral("1.234")), qint64(123400));
    QVERIFY2(ParcelaService::parsearValor(QStringLiteral("1.234")) != qint64(123),
        "regressao: '1.234' voltou a truncar para R$ 1,23");
    // Decimal com ponto (2 casas) continua decimal, sem regressao.
    QCOMPARE(ParcelaService::parsearValor(QStringLiteral("1234.56")), qint64(123456));
}

void TestParcelaService::simboloMoedaConhecidas()
{
    QCOMPARE(ParcelaService::simboloMoeda(QStringLiteral("BRL")),
             QStringLiteral("R$"));
    QCOMPARE(ParcelaService::simboloMoeda(QStringLiteral("USD")),
             QStringLiteral("$"));
    // EUR pode ser "€" ou simbolo similar
    const QString eur = ParcelaService::simboloMoeda(QStringLiteral("EUR"));
    QVERIFY(!eur.isEmpty());
}

// ── Amortizacao Price (ParcelaService::tabelaAmortizacao) ─────────────────────
// Esta implementacao ABSORVE o residuo na ultima parcela:
//   amortizacao da ultima linha = saldo remanescente (nao pmt - juros).
// Logo o saldo final deve fechar exatamente em 0 (sem residuo).

void TestParcelaService::amortSaldoFinalZera()
{
    // arrange: 10000,00 (1.000.000 cents) em 12x a 1% a.m.
    const Compra c = makeCompra(1000000, 12, 1.0);

    // act
    const QList<AmortizacaoRow> rows = ParcelaService::tabelaAmortizacao(c);

    // assert: 12 linhas; saldo apos ultima amortizacao = 0 exato.
    QCOMPARE(rows.size(), 12);
    qint64 saldo = c.valorTotal;
    for (const auto &r : rows) saldo -= r.amortizacao;
    QCOMPARE(saldo, qint64(0));
}

void TestParcelaService::amortSomaParcelasIgualTotalPago()
{
    // arrange
    const Compra c = makeCompra(1000000, 12, 1.0);

    // act
    const QList<AmortizacaoRow> rows = ParcelaService::tabelaAmortizacao(c);

    // assert: soma(juros) + soma(amortizacao) == soma(parcela);
    // soma(amortizacao) == valorTotal (todo o principal foi quitado).
    qint64 somaJuros = 0, somaAmort = 0, somaParcela = 0;
    for (const auto &r : rows) {
        somaJuros   += r.juros;
        somaAmort   += r.amortizacao;
        somaParcela += r.parcela;
        QCOMPARE(r.parcela, r.juros + r.amortizacao); // invariante por linha
    }
    QCOMPARE(somaAmort, c.valorTotal);
    QCOMPARE(somaParcela, somaJuros + somaAmort);
}

void TestParcelaService::amortJurosDecrescentes()
{
    // arrange
    const Compra c = makeCompra(1000000, 12, 1.0);

    // act
    const QList<AmortizacaoRow> rows = ParcelaService::tabelaAmortizacao(c);

    // assert: juros caem monotonicamente (saldo decrescente).
    for (int k = 1; k < rows.size(); ++k) {
        QVERIFY2(rows[k].juros < rows[k-1].juros,
            qPrintable(QStringLiteral("juros[%1]=%2 nao < juros[%3]=%4")
                .arg(k).arg(rows[k].juros).arg(k-1).arg(rows[k-1].juros)));
    }
}

void TestParcelaService::amortPrimeiraMaisJurosQueUltima()
{
    // arrange
    const Compra c = makeCompra(1000000, 12, 1.0);

    // act
    const QList<AmortizacaoRow> rows = ParcelaService::tabelaAmortizacao(c);

    // assert: 1a parcela = juros sobre saldo cheio = 1.000.000 * 1% = 10.000 cents
    QCOMPARE(rows.first().saldoDevedor, c.valorTotal);
    QCOMPARE(rows.first().juros, qint64(10000));
    QVERIFY(rows.first().juros > rows.last().juros);
}

void TestParcelaService::amortParcelaUnica()
{
    // arrange: n=1, 1000,00 a 2% a.m.
    const Compra c = makeCompra(100000, 1, 2.0);

    // act
    const QList<AmortizacaoRow> rows = ParcelaService::tabelaAmortizacao(c);

    // assert
    QCOMPARE(rows.size(), 1);
    QCOMPARE(rows.first().saldoDevedor, qint64(100000));
    QCOMPARE(rows.first().juros, qint64(2000));        // 1000 * 2%
    QCOMPARE(rows.first().amortizacao, qint64(100000)); // principal inteiro (ultima absorve)
    QCOMPARE(rows.first().parcela, qint64(102000));
}

void TestParcelaService::amortSemJuros()
{
    // arrange: sem juros, 1200,00 em 12x -> 100,00 cada, juros sempre 0.
    const Compra c = makeCompra(120000, 12, 0.0);

    // act
    const QList<AmortizacaoRow> rows = ParcelaService::tabelaAmortizacao(c);

    // assert
    QCOMPARE(rows.size(), 12);
    qint64 somaAmort = 0;
    for (const auto &r : rows) {
        QCOMPARE(r.juros, qint64(0));
        somaAmort += r.amortizacao;
    }
    QCOMPARE(somaAmort, c.valorTotal); // sem centavo perdido/criado
}

void TestParcelaService::amortBordaInvalida()
{
    // valorTotal <= 0 ou numParcelas <= 0 -> tabela vazia (sem crash).
    QVERIFY(ParcelaService::tabelaAmortizacao(makeCompra(0, 12, 1.0)).isEmpty());
    QVERIFY(ParcelaService::tabelaAmortizacao(makeCompra(100000, 0, 1.0)).isEmpty());
}

// ── resolverTaxa via recalcular(SolveFor::Taxa) ──────────────────────────────
// resolverTaxa e privado; exercitado pelo caminho publico recalcular().
// Newton-Raphson proprio do ParcelaService.

void TestParcelaService::resolverTaxaRecuperaTaxaConhecida()
{
    // arrange: gerar PMT a partir de taxa conhecida (1% a.m.), depois recuperar.
    // PV=10000,00, n=12, i=1% -> PMT (Price). Conferido em Excel:
    //   =PMT(0.01;12;-10000) = 888.49 -> 88849 cents.
    DadosFinanceiros d;
    d.valorTotal  = 1000000; // 10000,00
    d.numParcelas = 12;
    d.valorParcela = 88849;  // PMT @ 1% a.m.

    // act: resolver a taxa a partir de PV/PMT/n
    const DadosFinanceiros out = ParcelaService::recalcular(d, SolveFor::Taxa);

    // assert: recupera ~1% a.m. (tolerancia por arredondamento de PMT em cents)
    QVERIFY2(qAbs(out.jurosMensaisPct - 1.0) < 0.02,
        qPrintable(QStringLiteral("taxa = %1%% a.m., esperado ~1.0%%")
                       .arg(out.jurosMensaisPct)));
    // CET deve ser sincronizado e coerente com a mensal recuperada.
    QVERIFY2(qAbs(out.cetPct - ParcelaService::cetFromJurosMensais(out.jurosMensaisPct)) < 1e-6,
        qPrintable(QStringLiteral("CET=%1 nao coerente com mensal").arg(out.cetPct)));
}

void TestParcelaService::resolverTaxaSemJurosRetornaZero()
{
    // PMT * n == PV (sem juros) -> taxa 0.
    DadosFinanceiros d;
    d.valorTotal   = 120000; // 1200,00
    d.numParcelas  = 12;
    d.valorParcela = 10000;  // 100,00 * 12 == 1200,00

    const DadosFinanceiros out = ParcelaService::recalcular(d, SolveFor::Taxa);

    QVERIFY2(qAbs(out.jurosMensaisPct) < 1e-6,
        qPrintable(QStringLiteral("taxa = %1%%, esperado 0").arg(out.jurosMensaisPct)));
}

void TestParcelaService::resolverTaxaParcelaUnicaSemCrash()
{
    // borda n=1: PV=1000, PMT=1020 -> i = 2% a.m. Sem crash, valor plausivel.
    DadosFinanceiros d;
    d.valorTotal   = 100000;
    d.numParcelas  = 1;
    d.valorParcela = 102000;

    const DadosFinanceiros out = ParcelaService::recalcular(d, SolveFor::Taxa);

    QVERIFY2(out.jurosMensaisPct >= 0.0 && out.jurosMensaisPct < 100.0,
        qPrintable(QStringLiteral("taxa n=1 = %1%% (esperado ~2%%, sem crash)")
                       .arg(out.jurosMensaisPct)));
}

void TestParcelaService::resolverTaxaPmtZeroSemCrash()
{
    // borda PMT=0: resolverTaxa retorna 0 internamente (pmt<=0). Sem crash.
    DadosFinanceiros d;
    d.valorTotal   = 100000;
    d.numParcelas  = 12;
    d.valorParcela = 0;

    // recalcular nao resolve Taxa quando valorParcela<=0 (guard); permanece 0.
    const DadosFinanceiros out = ParcelaService::recalcular(d, SolveFor::Taxa);
    QCOMPARE(out.jurosMensaisPct, 0.0);
}

// ── dataParaParcela/gerarDatas (#199, Onda U, QA-04) ─────────────────────────
//
// dataParaParcela(c, numero): offset = numero - parcelaInicial; mes/ano
// normalizados por while-loop (aceita offset negativo e mes fora de 1..12
// antes de normalizar). Sem teste ate #199 -- casos derivados a mao
// (conferidos via oraculo Python, mesmo algoritmo de normalizacao).

void TestParcelaService::dataParaParcelaViradaDeAno()
{
    // parcelaInicial=1, mesInicio=nov/2025. Parcela numero 3 (offset=+2)
    // cai em nov+2 = jan/2026 -- virada de ano.
    Compra c = makeCompra(120000, 12, 0.0);
    c.parcelaInicial = 1;
    c.mesInicio       = 11;
    c.anoInicio        = 2025;

    const QDate d = ParcelaService::dataParaParcela(c, 3);
    QCOMPARE(d, QDate(2026, 1, 1));
}

void TestParcelaService::dataParaParcelaOffsetNegativo()
{
    // Compra ja na 5a parcela (mar/2025) quando cadastrada retroativamente.
    // Parcela numero 1 (a primeira, ja paga) tem offset = 1-5 = -4:
    // mar/2025 - 4 meses = nov/2024.
    Compra c = makeCompra(500000, 10, 0.0);
    c.parcelaInicial = 5;
    c.mesInicio       = 3;
    c.anoInicio        = 2025;

    const QDate d = ParcelaService::dataParaParcela(c, 1);
    QCOMPARE(d, QDate(2024, 11, 1));
}

void TestParcelaService::gerarDatasNGrandePrimeiraEUltima()
{
    // N=420 (35 anos de parcelas) -- normalizacao nao pode estourar/travar.
    Compra c = makeCompra(42000000, 420, 0.0);
    c.parcelaInicial = 1;
    c.mesInicio       = 1;
    c.anoInicio        = 2025;

    const QList<QDate> datas = ParcelaService::gerarDatas(c);
    QCOMPARE(datas.size(), 420);
    QCOMPARE(datas.first(), QDate(2025, 1, 1));
    QCOMPARE(datas.last(),  QDate(2059, 12, 1));
}

QTEST_GUILESS_MAIN(TestParcelaService)
#include "test_parcela_service.moc"
