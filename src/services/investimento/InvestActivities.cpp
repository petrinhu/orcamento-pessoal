// PeuBud - implementacoes IInvestActivity (#89)

#include "services/investimento/InvestActivities.h"

namespace invest {

namespace {

// Helpers de validacao reutilizados pelas 10 activities.

inline bool tickerValido(const LancamentoInvestimento &l)
{
    return !l.ticker.trimmed().isEmpty();
}

inline bool quantidadeValida(const LancamentoInvestimento &l)
{
    return l.quantidade > 0.0;
}

inline bool precoValido(const LancamentoInvestimento &l)
{
    return l.precoUnitario > 0;
}

inline bool valorValido(const LancamentoInvestimento &l)
{
    return l.valor > 0;
}

} // namespace

// ── BuyShares ────────────────────────────────────────────────────────────────
QString BuySharesActivity::rawType() const { return QStringLiteral("buy"); }
IInvestActivity::Tipo BuySharesActivity::type() const { return Tipo::BuyShares; }
QString BuySharesActivity::label() const { return QStringLiteral("Compra"); }

IInvestActivity::CamposVisiveis BuySharesActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker = c.quantidade = c.precoUnitario = c.valor = true;
    c.jurosOuTaxa = false;
    c.fatorSplit  = false;
    return c;
}

QStringList BuySharesActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l))     erros << QStringLiteral("Ticker obrigatorio para compra.");
    if (!quantidadeValida(l)) erros << QStringLiteral("Quantidade deve ser > 0.");
    if (!precoValido(l))      erros << QStringLiteral("Preco unitario deve ser > 0.");
    return erros;
}

// ── SellShares ───────────────────────────────────────────────────────────────
QString SellSharesActivity::rawType() const { return QStringLiteral("sell"); }
IInvestActivity::Tipo SellSharesActivity::type() const { return Tipo::SellShares; }
QString SellSharesActivity::label() const { return QStringLiteral("Venda"); }

IInvestActivity::CamposVisiveis SellSharesActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker = c.quantidade = c.precoUnitario = c.valor = true;
    c.jurosOuTaxa = false;
    c.fatorSplit  = false;
    return c;
}

QStringList SellSharesActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l))     erros << QStringLiteral("Ticker obrigatorio para venda.");
    if (!quantidadeValida(l)) erros << QStringLiteral("Quantidade deve ser > 0.");
    return erros;
}

// ── Dividend ─────────────────────────────────────────────────────────────────
QString DividendActivity::rawType() const { return QStringLiteral("dividend"); }
IInvestActivity::Tipo DividendActivity::type() const { return Tipo::Dividend; }
QString DividendActivity::label() const { return QStringLiteral("Dividendo"); }

IInvestActivity::CamposVisiveis DividendActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker = true;
    c.quantidade    = false;
    c.precoUnitario = false;
    c.valor         = true;
    c.jurosOuTaxa = false;
    c.fatorSplit  = false;
    return c;
}

QStringList DividendActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l)) erros << QStringLiteral("Ticker obrigatorio para dividendo.");
    if (!valorValido(l))  erros << QStringLiteral("Valor do dividendo deve ser > 0.");
    return erros;
}

// ── ReinvestDividend ─────────────────────────────────────────────────────────
QString ReinvestDividendActivity::rawType() const { return QStringLiteral("reinvest"); }
IInvestActivity::Tipo ReinvestDividendActivity::type() const { return Tipo::ReinvestDividend; }
QString ReinvestDividendActivity::label() const { return QStringLiteral("Dividendo reinvestido (DRIP)"); }

IInvestActivity::CamposVisiveis ReinvestDividendActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker = c.quantidade = c.precoUnitario = c.valor = true;
    c.jurosOuTaxa = false;
    c.fatorSplit  = false;
    return c;
}

QStringList ReinvestDividendActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l))     erros << QStringLiteral("Ticker obrigatorio para DRIP.");
    if (!quantidadeValida(l)) erros << QStringLiteral("Quantidade reinvestida deve ser > 0.");
    if (!precoValido(l))      erros << QStringLiteral("Preco unitario deve ser > 0.");
    return erros;
}

// ── Yield ────────────────────────────────────────────────────────────────────
QString YieldActivity::rawType() const { return QStringLiteral("yield"); }
IInvestActivity::Tipo YieldActivity::type() const { return Tipo::Yield; }
QString YieldActivity::label() const { return QStringLiteral("Rendimento"); }

IInvestActivity::CamposVisiveis YieldActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker        = true;  // nome do ativo (ex: "CDB Nubank")
    c.quantidade    = false;
    c.precoUnitario = false;
    c.valor         = true;
    c.jurosOuTaxa   = true;
    c.fatorSplit    = false;
    return c;
}

QStringList YieldActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l)) erros << QStringLiteral("Identificacao do ativo obrigatoria.");
    if (!valorValido(l))  erros << QStringLiteral("Valor do rendimento deve ser > 0.");
    return erros;
}

// ── AddShares ────────────────────────────────────────────────────────────────
QString AddSharesActivity::rawType() const { return QStringLiteral("add"); }
IInvestActivity::Tipo AddSharesActivity::type() const { return Tipo::AddShares; }
QString AddSharesActivity::label() const { return QStringLiteral("Entrada de cotas (sem custo)"); }

IInvestActivity::CamposVisiveis AddSharesActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker        = true;
    c.quantidade    = true;
    c.precoUnitario = false;  // sem custo
    c.valor         = false;
    c.jurosOuTaxa = false;
    c.fatorSplit  = false;
    return c;
}

QStringList AddSharesActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l))     erros << QStringLiteral("Ticker obrigatorio.");
    if (!quantidadeValida(l)) erros << QStringLiteral("Quantidade deve ser > 0.");
    return erros;
}

// ── RemoveShares ─────────────────────────────────────────────────────────────
QString RemoveSharesActivity::rawType() const { return QStringLiteral("remove"); }
IInvestActivity::Tipo RemoveSharesActivity::type() const { return Tipo::RemoveShares; }
QString RemoveSharesActivity::label() const { return QStringLiteral("Saida de cotas (sem custo)"); }

IInvestActivity::CamposVisiveis RemoveSharesActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker        = true;
    c.quantidade    = true;
    c.precoUnitario = false;
    c.valor         = false;
    c.jurosOuTaxa = false;
    c.fatorSplit  = false;
    return c;
}

QStringList RemoveSharesActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l))     erros << QStringLiteral("Ticker obrigatorio.");
    if (!quantidadeValida(l)) erros << QStringLiteral("Quantidade deve ser > 0.");
    return erros;
}

// ── SplitShares ──────────────────────────────────────────────────────────────
QString SplitSharesActivity::rawType() const { return QStringLiteral("split"); }
IInvestActivity::Tipo SplitSharesActivity::type() const { return Tipo::SplitShares; }
QString SplitSharesActivity::label() const { return QStringLiteral("Desdobramento (Split)"); }

IInvestActivity::CamposVisiveis SplitSharesActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker        = true;
    c.quantidade    = false;
    c.precoUnitario = false;
    c.valor         = false;
    c.jurosOuTaxa   = false;
    c.fatorSplit    = true;
    return c;
}

QStringList SplitSharesActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l)) erros << QStringLiteral("Ticker obrigatorio para split.");
    // O fator e armazenado na quantidade quando esta activity esta ativa
    // (campo dedicado fica para a futura migracao do model). Validar > 0.
    if (l.quantidade <= 0.0)
        erros << QStringLiteral("Fator de desdobramento deve ser > 0.");
    return erros;
}

// ── InterestIncome (JCP) ─────────────────────────────────────────────────────
QString InterestIncomeActivity::rawType() const { return QStringLiteral("interest"); }
IInvestActivity::Tipo InterestIncomeActivity::type() const { return Tipo::InterestIncome; }
QString InterestIncomeActivity::label() const { return QStringLiteral("Juros sobre capital proprio"); }

IInvestActivity::CamposVisiveis InterestIncomeActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker        = true;
    c.quantidade    = false;
    c.precoUnitario = false;
    c.valor         = true;
    c.jurosOuTaxa   = true;
    c.fatorSplit    = false;
    return c;
}

QStringList InterestIncomeActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l)) erros << QStringLiteral("Ticker obrigatorio para JCP.");
    if (!valorValido(l))  erros << QStringLiteral("Valor de JCP deve ser > 0.");
    return erros;
}

// ── Bonificacao ──────────────────────────────────────────────────────────────
QString BonificacaoActivity::rawType() const { return QStringLiteral("bonus"); }
IInvestActivity::Tipo BonificacaoActivity::type() const { return Tipo::Bonificacao; }
QString BonificacaoActivity::label() const { return QStringLiteral("Bonificacao em acoes"); }

IInvestActivity::CamposVisiveis BonificacaoActivity::camposVisiveis() const
{
    CamposVisiveis c;
    c.ticker        = true;
    c.quantidade    = true;
    c.precoUnitario = false;
    c.valor         = false;
    c.jurosOuTaxa = false;
    c.fatorSplit  = false;
    return c;
}

QStringList BonificacaoActivity::consistencyCheck(const LancamentoInvestimento &l) const
{
    QStringList erros;
    if (!tickerValido(l))     erros << QStringLiteral("Ticker obrigatorio para bonificacao.");
    if (!quantidadeValida(l)) erros << QStringLiteral("Quantidade bonificada deve ser > 0.");
    return erros;
}

} // namespace invest
