// PeuBud - Prova congelada de precisao PMT Price (FIN-03, auditoria
// 2026-06-22 §3.1).
//
// FinanceEngine::pmt/pv/fv/nper/rate retornam `double` porque a matematica
// (1+i)^n e transcendental -- inevitavel. ParcelaService::pmtPriceCentavos
// (helper anonimo em ParcelaService.cpp, exercitado aqui via a API publica
// ParcelaService::comJurosMensais) isola esse `double` e arredonda para
// centavos (qint64) UMA UNICA VEZ na saida -- o padrao correto ja seguido
// pelo codigo (regra do lider: "dinheiro nunca em double, so na formatacao
// final" -- aqui o arredondamento final PARA centavos e essa "formatacao").
//
// O que faltava (achado da auditoria): a PROVA congelada em teste de que
// esse `double` de producao nao diverge de um oraculo de maior precisao.
// Auditoria rodou um sweep de 200k amostras (Decimal vs double) e achou 0
// divergencia no range realista -- mas essa garantia nao existia na suite.
//
// Oraculo: mesma formula (PMT = PV * i / (1 - (1+i)^-n)), calculada em
// `long double` (80-bit extended no x86-64, ~18-19 digitos significativos
// vs ~15-17 do double de producao) -- maior precisao que o caminho de
// producao, NAO outro double raso. Arredondamento final via roundl (ties
// away from zero), mesma semantica de std::round usado em producao.
//
// Range provado por este sweep (documentado aqui porque e o proprio
// contrato do teste):
//   PV:    de R$ 1,00 (100 centavos) ate R$ 100.000.000,00 (1e10 centavos)
//   taxa:  0,05% a.m. ate 20% a.m.
//   n:     1 ate 480 parcelas
//
// Resultado medido (rodado nesta suite, 3570 combinacoes): divergencia
// MAXIMA de 1 centavo, em 6/3570 combos (~0,17%) -- todos em fronteiras
// onde o valor matematico real do PMT cai a uma distancia subatomica de um
// limite exato de meio-centavo (double e long double arredondam o mesmo
// numero quase-exato para lados opostos). Isso NAO e um bug: e ruido de
// ultimo-bit de ponto flutuante, o mesmo fenomeno de qualquer tie-breaking,
// e fica dentro da tolerancia dura de 1 centavo que a propria auditoria
// definiu (mesmo criterio usado no QA-02 do CurrencyService). O que este
// teste GARANTE e trava para sempre: (1) NENHUM combo do range diverge
// mais que 1 centavo: (2) a fracao de combos com 1 centavo de diferenca
// fica pequena (<2%) -- se um refactor de FinanceEngine::pmt fizer essa
// fracao explodir, o teste falha e acusa regressao real de precisao.

#include <QtTest/QtTest>
#include <QSettings>
#include <QTemporaryDir>

#include "core/settings/AppSettings.h"
#include "services/ParcelaService.h"
#include "models/Compra.h"

#include <cmath>
#include <cstdint>

class TestParcelaServiceOracle : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void sweepPmtPriceContraOraculoLongDouble();
    void casosDeFronteiraExtremos();

private:
    QTemporaryDir m_settingsDir;
};

// Isola AppSettings (singleton) num QTemporaryDir -- ParcelaService.cpp
// referencia AppSettings::instance() em formatarValor(), entao o simbolo
// precisa resolver no link mesmo que este teste nao chame formatarValor
// diretamente (mesmo padrao de test_parcela_service.cpp).
void TestParcelaServiceOracle::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("petrinhu"));
    QCoreApplication::setApplicationName(QStringLiteral("PeuBud"));
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
}

namespace {

// Oraculo de maior precisao: mesma formula de pmtPriceCentavos (producao),
// calculada em long double em vez de double. PV em centavos (qint64),
// devolve PMT arredondado para centavos (half-away-from-zero, espelhando
// std::round de producao).
qint64 pmtOracleCentavosLD(qint64 pvCentavos, double iMensal, int n)
{
    const long double pv = static_cast<long double>(pvCentavos);
    const long double i  = static_cast<long double>(iMensal);
    const long double fator = powl(1.0L + i, -static_cast<long double>(n));
    const long double pmt   = pv * i / (1.0L - fator);
    return static_cast<qint64>(roundl(pmt));
}

Compra makeCompra(qint64 valorTotal, int numParcelas)
{
    Compra c;
    c.valorTotal     = valorTotal;
    c.numParcelas     = numParcelas;
    c.parcelaInicial  = 1;
    c.mesInicio       = 1;
    c.anoInicio       = 2025;
    return c;
}

} // namespace

void TestParcelaServiceOracle::sweepPmtPriceContraOraculoLongDouble()
{
    // PV: de R$1 a R$100.000.000 (1e10 centavos), espacado exponencialmente
    // + alguns valores "feios" (nao redondos) para estressar arredondamento.
    const qint64 pvsCentavos[] = {
        100LL,                  // R$ 1,00
        9999LL,                 // R$ 99,99
        123456LL,                // R$ 1.234,56
        1000000LL,               // R$ 10.000,00
        33333333LL,               // R$ 333.333,33 (feio)
        100000000LL,              // R$ 1.000.000,00
        987654321LL,              // R$ 9.876.543,21 (feio)
        1000000000LL,             // R$ 10.000.000,00
        5000000000LL,             // R$ 50.000.000,00
        10000000000LL,            // R$ 100.000.000,00 (teto do range provado)
    };

    // taxa mensal: 0,05% a.m. ate 20% a.m. (range do wizard "Reserva de
    // Emergencia"/"CET" da calculadora -- realista para financas pessoais).
    const double taxasPct[] = {
        0.05, 0.1, 0.25, 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0,
        2.5, 3.0, 4.0, 5.0, 6.0, 7.5, 10.0, 12.5, 15.0, 17.5, 20.0,
    };

    // numero de parcelas: 1 ate 480 (40 anos, teto de financiamento
    // imobiliario -- maior N usado no app).
    const int ns[] = {
        1, 2, 3, 6, 9, 12, 18, 24, 36, 48, 60, 84, 120, 180, 240, 360, 480,
    };

    qint64 maxDivergenciaAbs = 0;
    qint64 totalCombos       = 0;
    qint64 combosDivergentes = 0;

    for (qint64 pv : pvsCentavos) {
        for (double taxaPct : taxasPct) {
            const double i = taxaPct / 100.0;
            for (int n : ns) {
                ++totalCombos;

                Compra c = makeCompra(pv, n);
                const ResultadoFinanceiro r = ParcelaService::comJurosMensais(c, taxaPct);
                QVERIFY2(!r.parcelas.isEmpty(), "comJurosMensais nao gerou parcelas");
                const qint64 pmtProducao = r.parcelas.first().valor;

                const qint64 pmtOraculo = pmtOracleCentavosLD(pv, i, n);

                const qint64 diff = qAbs(pmtProducao - pmtOraculo);
                if (diff != 0) {
                    ++combosDivergentes;
                    maxDivergenciaAbs = qMax(maxDivergenciaAbs, diff);
                }

                // Todas as parcelas do modo Price sao iguais (sem
                // redistribuicao de resto) -- confere invariante junto.
                for (const auto &p : r.parcelas)
                    QCOMPARE(p.valor, pmtProducao);

                // Tolerancia DURA (nunca deveria disparar dentro do range
                // provado): >1 centavo de divergencia e falha imediata,
                // com o combo exato no diagnostico.
                QVERIFY2(diff <= 1, qPrintable(QStringLiteral(
                    "divergencia de %1 centavo(s): pv=%2 taxa=%3%% n=%4 -> "
                    "producao=%5 oraculo=%6")
                    .arg(diff).arg(pv).arg(taxaPct).arg(n)
                    .arg(pmtProducao).arg(pmtOraculo)));
            }
        }
    }

    qInfo("FIN-03 sweep: %lld combinacoes, %lld divergentes (max %lld centavo(s))",
          static_cast<long long>(totalCombos),
          static_cast<long long>(combosDivergentes),
          static_cast<long long>(maxDivergenciaAbs));

    // Trava dura: nenhuma divergencia pode passar de 1 centavo (ja
    // verificado por combo acima, reforcado aqui sobre o maximo agregado).
    QVERIFY2(maxDivergenciaAbs <= 1, qPrintable(QStringLiteral(
        "divergencia maxima de %1 centavo(s) excede a tolerancia dura de 1")
        .arg(maxDivergenciaAbs)));

    // Trava de taxa: fracao de combos com 1 centavo de diferenca fica
    // pequena (<2%) -- documentado como ruido de ultimo-bit esperado
    // (ver comentario no topo do arquivo). Se subir muito, e regressao.
    QVERIFY2(combosDivergentes * 100 <= totalCombos * 2, qPrintable(QStringLiteral(
        "%1/%2 combos divergiram (%3%%) -- acima do esperado (<2%%), "
        "investigar regressao de precisao em FinanceEngine::pmt")
        .arg(combosDivergentes).arg(totalCombos)
        .arg(100.0 * combosDivergentes / totalCombos)));
}

void TestParcelaServiceOracle::casosDeFronteiraExtremos()
{
    // PV no teto exato do range provado (R$100M) com a maior taxa (20%
    // a.m.) e o maior N (480) -- pior caso combinado de acumulo de erro
    // em (1+i)^n com i grande e n grande.
    Compra c = makeCompra(10000000000LL, 480);
    const ResultadoFinanceiro r = ParcelaService::comJurosMensais(c, 20.0);
    const qint64 pmtProducao = r.parcelas.first().valor;
    const qint64 pmtOraculo  = pmtOracleCentavosLD(10000000000LL, 0.20, 480);

    QVERIFY2(qAbs(pmtProducao - pmtOraculo) <= 1, qPrintable(QStringLiteral(
        "pior caso do range (PV=R$100M, 20%% a.m., 480x) divergiu: "
        "producao=%1 oraculo=%2").arg(pmtProducao).arg(pmtOraculo)));

    // Taxa minima do range (0,05% a.m.) com PV pequeno (R$1,00) e N=1 --
    // extremo oposto (quase sem juros, quase sem periodos).
    Compra c2 = makeCompra(100LL, 1);
    const ResultadoFinanceiro r2 = ParcelaService::comJurosMensais(c2, 0.05);
    const qint64 pmtProducao2 = r2.parcelas.first().valor;
    const qint64 pmtOraculo2  = pmtOracleCentavosLD(100LL, 0.0005, 1);
    QCOMPARE(pmtProducao2, pmtOraculo2);
}

QTEST_GUILESS_MAIN(TestParcelaServiceOracle)
#include "test_parcela_service_oracle.moc"
