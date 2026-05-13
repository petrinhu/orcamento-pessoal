#include "TvmService.h"
#include "FinanceEngine.h"

#include <cmath>
#include <stdexcept>

// TvmService preserva a interface legada (TvmInput/TvmSolve) usada por
// CalculadoraDialog, mas o nucleo numerico (PMT/PV/FV/NPER/RATE) foi
// migrado para FinanceEngine via dedup #69. Aqui sobra apenas o adapter
// + a tabela de amortizacao Price (que segue logica de schedule, nao TVM).

TvmInput TvmService::resolver(TvmInput in, TvmSolve campo)
{
    const double r = in.i / 100.0; // FinanceEngine espera fracao decimal
    const int    n = static_cast<int>(std::round(in.n));

    switch (campo) {
    case TvmSolve::N: {
        in.n = FinanceEngine::nper(in.pv, in.pmt, in.fv, r, in.begin);
        break;
    }
    case TvmSolve::I: {
        if (n <= 0) throw std::domain_error("TvmService: n <= 0 ao resolver I");
        in.i = FinanceEngine::rate(in.pv, in.pmt, in.fv, n, in.begin) * 100.0;
        break;
    }
    case TvmSolve::PV: {
        if (n <= 0) throw std::domain_error("TvmService: n <= 0 ao resolver PV");
        in.pv = FinanceEngine::pv(in.pmt, in.fv, r, n, in.begin);
        break;
    }
    case TvmSolve::PMT: {
        if (n <= 0) throw std::domain_error("TvmService: n <= 0 ao resolver PMT");
        in.pmt = FinanceEngine::pmt(in.pv, in.fv, r, n, in.begin);
        break;
    }
    case TvmSolve::FV: {
        if (n <= 0) throw std::domain_error("TvmService: n <= 0 ao resolver FV");
        in.fv = FinanceEngine::fv(in.pv, in.pmt, r, n, in.begin);
        break;
    }
    }

    return in;
}

double TvmService::cetFromMensal(double iMensalPct)
{
    return ParcelaService::cetFromJurosMensais(iMensalPct);
}

double TvmService::mensalFromCet(double cetPct)
{
    return ParcelaService::jurosMensaisFromCet(cetPct);
}

QList<AmortizacaoRow> TvmService::amortizacao(const TvmInput &tvm, QDate dataInicio)
{
    // Price (SAF) schedule em END mode.
    // Para BEGIN mode o primeiro pagamento ocorre no periodo 0; modelamos
    // como END mas com datas shiftadas -1 (convencao HP-12C).
    const int N = static_cast<int>(std::round(tvm.n));
    if (N <= 0) return {};

    const double r   = tvm.i / 100.0;
    const double pmt = tvm.pmt; // signed: negativo = outflow
    double saldo      = tvm.pv;

    QList<AmortizacaoRow> rows;
    rows.reserve(N);

    for (int k = 1; k <= N; ++k) {
        AmortizacaoRow row;
        row.numero = k;
        row.data   = dataInicio.addMonths(k - 1 + (tvm.begin ? -1 : 0));

        const double juros = saldo * r;
        const double amort = (-pmt) - juros;
        saldo             -= amort;

        row.saldoDevedor = static_cast<qint64>(std::round(saldo * 100.0));
        row.juros        = static_cast<qint64>(std::round(juros * 100.0));
        row.amortizacao  = static_cast<qint64>(std::round(amort * 100.0));
        row.parcela      = static_cast<qint64>(std::round((-pmt) * 100.0));

        rows.append(row);
    }

    return rows;
}
