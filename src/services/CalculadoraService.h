#pragma once
#include <QList>
#include <QString>
#include <QtTypes>

// Utility class for the calculator: display formatting and value conversion.
// All methods are stateless and safe to call from any thread.
class CalculadoraService {
public:
    // Format a double for display: up to 10 decimal places, no trailing zeros.
    // Uses ',' as decimal separator (pt-BR).
    static QString formatarDisplay(double valor, int decimais = -1);

    // Parse a display string back to double.
    // Handles: "1.234,56" (thousands + decimal), "1234,56", "1234.56".
    static double parsearDisplay(const QString &texto);

    // Convert display value to centavos (rounds to 2 decimal places).
    static qint64 toCentavos(double valor);

    // Convert centavos to a double suitable for display.
    static double fromCentavos(qint64 centavos);

    // ── Wizard: Reserva de Emergência (FIN-04, auditoria 2026-06-22 §3.1) ────
    //
    // Extraído de CalculadoraDialog::buildTabWizards (wizard "Reserva de
    // Emergência", #61) — o cálculo era feito INTEIRAMENTE em `double`
    // (parseF/fmtF), incluindo multiplicações monetárias simples (NÃO
    // transcendentais, 100% inteirizáveis). Aqui: dinheiro SEMPRE em
    // centavos (qint64); a única fração double é a TAXA (delegada a
    // FiscalService/FinanceEngine), isolada e arredondada 1x na saída —
    // mesmo padrão "correto" já usado por ParcelaService::pmtPriceCentavos
    // (ver FIN-03/test_parcela_service_oracle.cpp).

    struct AplicacaoReserva {
        QString nome;                        // "Poupança" / "CDB 100% CDI" / "Tesouro Selic"
        double  taxaMensalLiquida = 0.0;      // fração decimal (0.005 = 0,5%) — display only
        qint64  mesesParaMeta     = 0;        // arredondado para cima (qtd. inteira de meses)
        qint64  rendaMensalPlenaCentavos = 0; // renda passiva se a meta fosse atingida hoje
    };

    struct ReservaEmergenciaResultado {
        qint64 metaCentavos   = 0; // despesas x meses de reserva
        qint64 gapCentavos    = 0; // max(0, meta - reservaAtual)
        bool   temComparativo = false; // selic e aporte informados (>0)
        QList<AplicacaoReserva> aplicacoes; // vazio se !temComparativo
    };

    // despesasMensaisCentavos: obrigatório (>0), senão resultado default (0).
    // mesesReserva: <=0 vira 6 (default do wizard). selicAnualFracao/
    // aporteMensalCentavos <=0 -> temComparativo=false (só meta/gap).
    [[nodiscard]] static ReservaEmergenciaResultado calcularReservaEmergencia(
        qint64 despesasMensaisCentavos,
        double mesesReserva,
        double selicAnualFracao,
        qint64 aporteMensalCentavos,
        qint64 reservaAtualCentavos);

    // ── Wizard: CLT vs PJ (FIN-04, auditoria 2026-06-22 §3.1) ─────────────────
    //
    // Extraído do wizard "CLT vs PJ" (#62). Achado da auditoria:
    // `double impPJ = pjFat * aliqPJ` — multiplicação monetária simples em
    // double, base de decisão financeira real. Aqui: bruto/faturamento
    // entram já em centavos (parse via uifmt::parseCentavos na UI, não
    // mais `QString::toDouble()` ad-hoc); INSS/IRPF (FiscalService) já são
    // qint64; a alíquota PJ (Simples/Lucro Presumido) é a única fração
    // double, multiplicada 1x contra o faturamento em centavos e
    // arredondada 1x.

    struct CltVsPjResultado {
        qint64 inssCentavos       = 0;
        qint64 irpfCentavos       = 0;
        qint64 liquidoCltCentavos = 0;
        double aliquotaPj         = 0.0; // fração decimal — display only
        qint64 impostosPjCentavos = 0;
        qint64 liquidoPjCentavos  = 0;
        qint64 diferencaCentavos  = 0; // liquidoPj - liquidoClt
        qint64 breakEvenCentavos  = 0; // faturamento PJ que iguala liq. CLT (aprox.)
    };

    // regimePj: 0 = Lucro Presumido (serviços); 1..5 = Simples Nacional
    // anexo I..V (RBT12 = faturamento x 12, calculado em inteiro).
    [[nodiscard]] static CltVsPjResultado calcularCltVsPj(
        qint64 brutoCltCentavos,
        qint64 faturamentoPjCentavos,
        int    regimePj);
};
