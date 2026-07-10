#include "CalculadoraService.h"
#include "services/FiscalService.h"
#include "services/FinanceEngine.h"

#include <algorithm>
#include <cmath>

QString CalculadoraService::formatarDisplay(double valor, int decimais)
{
    if (decimais >= 0) {
        return QString::number(valor, 'f', decimais)
            .replace(QLatin1Char('.'), QLatin1Char(','));
    }
    // Auto: up to 10 decimal places, trim trailing zeros
    QString s = QString::number(valor, 'f', 10);
    if (s.contains(QLatin1Char('.'))) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    }
    return s.replace(QLatin1Char('.'), QLatin1Char(','));
}

double CalculadoraService::parsearDisplay(const QString &texto)
{
    QString s = texto.trimmed();
    // Remove currency symbols / percent signs
    s.remove(QLatin1Char('R')).remove(QLatin1Char('$')).remove(QLatin1Char('%'));
    s = s.trimmed();
    // If string contains comma: comma is decimal, dots are thousands separators
    if (s.contains(QLatin1Char(','))) {
        s.remove(QLatin1Char('.'));          // remove thousands separators
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    bool ok = false;
    const double v = s.toDouble(&ok);
    return ok ? v : 0.0;
}

qint64 CalculadoraService::toCentavos(double valor)
{
    return static_cast<qint64>(std::round(valor * 100.0));
}

double CalculadoraService::fromCentavos(qint64 centavos)
{
    return centavos / 100.0;
}

// ── FIN-04: Reserva de Emergência ────────────────────────────────────────────

CalculadoraService::ReservaEmergenciaResultado
CalculadoraService::calcularReservaEmergencia(qint64 despesasMensaisCentavos,
                                               double mesesReserva,
                                               double selicAnualFracao,
                                               qint64 aporteMensalCentavos,
                                               qint64 reservaAtualCentavos)
{
    ReservaEmergenciaResultado out;
    if (despesasMensaisCentavos <= 0) return out;

    const double meses = (mesesReserva <= 0.0) ? 6.0 : mesesReserva;

    // meta = despesas x meses — único ponto de multiplicação double x
    // centavos (meses pode ser fracionário no wizard), arredondado 1x.
    out.metaCentavos = static_cast<qint64>(
        std::llround(static_cast<double>(despesasMensaisCentavos) * meses));
    out.gapCentavos = std::max<qint64>(0, out.metaCentavos - reservaAtualCentavos);

    out.temComparativo = (selicAnualFracao > 0.0 && aporteMensalCentavos > 0);
    if (!out.temComparativo) return out;

    // Taxas mensais líquidas (assumindo >720d -> IR 15% em CDB/Tesouro),
    // delegadas a FiscalService/FinanceEngine (única fonte de frações).
    const double mPoup    = FiscalService::poupancaMensal(selicAnualFracao);
    const double mBruta   = FinanceEngine::taxaEquivalente(selicAnualFracao, 1, 12);
    const double irRF     = FiscalService::irRendaFixaPorDias(721); // 15%
    const double mCDB     = mBruta * (1.0 - irRF);
    const double mTesouro = mCDB; // mesma alíquota; custódia 0,2% a.a. simplificada como 0

    // tempoAcumular: contagem de PERÍODOS (não dinheiro) — nper() delega ao
    // FinanceEngine (mesmo caminho transcendental provado em FIN-03); i<=0
    // usa divisão inteira exata em centavos (100% inteirizável, sem double).
    auto tempoAcumular = [&](double i) -> qint64 {
        if (i <= 0.0) {
            if (aporteMensalCentavos <= 0) return 0;
            return (out.gapCentavos + aporteMensalCentavos - 1) / aporteMensalCentavos;
        }
        // pv=0, pmt=-aporte, fv=gap — unidades homogêneas (centavos), o
        // resultado (nº de períodos) independe de escala.
        const double n = FinanceEngine::nper(
            0.0, -static_cast<double>(aporteMensalCentavos),
            static_cast<double>(out.gapCentavos), i);
        return static_cast<qint64>(std::ceil(n));
    };
    // rendaMensal: meta x taxa — único ponto de multiplicação double x
    // centavos por aplicação, arredondado 1x.
    auto rendaMensalCentavos = [&](double i) -> qint64 {
        return static_cast<qint64>(
            std::llround(static_cast<double>(out.metaCentavos) * i));
    };

    auto addLinha = [&](const QString &nome, double taxa) {
        AplicacaoReserva a;
        a.nome                     = nome;
        a.taxaMensalLiquida        = taxa;
        a.mesesParaMeta            = tempoAcumular(taxa);
        a.rendaMensalPlenaCentavos = rendaMensalCentavos(taxa);
        out.aplicacoes.append(a);
    };
    addLinha(QStringLiteral("Poupança"),      mPoup);
    addLinha(QStringLiteral("CDB 100% CDI"),  mCDB);
    addLinha(QStringLiteral("Tesouro Selic"), mTesouro);

    return out;
}

// ── FIN-04: CLT vs PJ ────────────────────────────────────────────────────────

CalculadoraService::CltVsPjResultado
CalculadoraService::calcularCltVsPj(qint64 brutoCltCentavos,
                                     qint64 faturamentoPjCentavos,
                                     int    regimePj)
{
    CltVsPjResultado out;
    if (brutoCltCentavos <= 0 || faturamentoPjCentavos <= 0) return out;

    // CLT — INSS/IRPF já vêm de FiscalService em centavos (qint64 puro).
    out.inssCentavos       = FiscalService::inssMensal2024(brutoCltCentavos);
    const qint64 baseIrpf  = brutoCltCentavos - out.inssCentavos;
    out.irpfCentavos       = FiscalService::irpfMensal2024(baseIrpf);
    out.liquidoCltCentavos = brutoCltCentavos - out.inssCentavos - out.irpfCentavos;

    // PJ — alíquota é a ÚNICA fração double (Simples Nacional ou Lucro
    // Presumido); RBT12 = faturamento x 12 é multiplicação inteira exata
    // (sem risco de precisão, nenhum double envolvido).
    if (regimePj == 0) {
        out.aliquotaPj = FiscalService::lucroPresumidoServicosAliquota();
    } else {
        const qint64 rbt12 = faturamentoPjCentavos * 12;
        out.aliquotaPj = FiscalService::simplesAliquotaEfetiva(regimePj, rbt12);
    }

    // impostosPj = faturamento(centavos) x alíquota(double) — único ponto
    // de multiplicação monetária em double, arredondado 1x (FIN-04: antes
    // essa conta inteira — faturamento incluído — rodava em double).
    out.impostosPjCentavos = static_cast<qint64>(
        std::llround(static_cast<double>(faturamentoPjCentavos) * out.aliquotaPj));
    out.liquidoPjCentavos  = faturamentoPjCentavos - out.impostosPjCentavos;
    out.diferencaCentavos  = out.liquidoPjCentavos - out.liquidoCltCentavos;

    // Break-even (aproximação, mesma premissa da UI original: alíquota
    // constante — no Simples real, RBT12 cresce com o faturamento, então o
    // ponto real é maior; suficiente para orientação).
    // pjFat * (1 - aliq) = liqCLT  ->  pjFat = liqCLT / (1 - aliq)
    if (out.aliquotaPj < 1.0) {
        out.breakEvenCentavos = static_cast<qint64>(std::llround(
            static_cast<double>(out.liquidoCltCentavos) / (1.0 - out.aliquotaPj)));
    }

    return out;
}
