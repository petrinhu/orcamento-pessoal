#pragma once

#include <QByteArray>
#include <QString>
#include <QtTypes>

// ── Tipo de ativo ─────────────────────────────────────────────────────────────

enum class TipoAtivo {
    Acao, Fii, Cdb, LciLca, Tesouro, Fundo, Poupanca, Outro
};

inline QString tipoAtivoParaTexto(TipoAtivo t)
{
    switch (t) {
    case TipoAtivo::Acao:     return QStringLiteral("acao");
    case TipoAtivo::Fii:      return QStringLiteral("fii");
    case TipoAtivo::Cdb:      return QStringLiteral("cdb");
    case TipoAtivo::LciLca:   return QStringLiteral("lci_lca");
    case TipoAtivo::Tesouro:  return QStringLiteral("tesouro");
    case TipoAtivo::Fundo:    return QStringLiteral("fundo");
    case TipoAtivo::Poupanca: return QStringLiteral("poupanca");
    default:                  return QStringLiteral("outro");
    }
}

inline TipoAtivo tipoAtivoDeTexto(const QString &s)
{
    if (s == QStringLiteral("acao"))     return TipoAtivo::Acao;
    if (s == QStringLiteral("fii"))      return TipoAtivo::Fii;
    if (s == QStringLiteral("cdb"))      return TipoAtivo::Cdb;
    if (s == QStringLiteral("lci_lca"))  return TipoAtivo::LciLca;
    if (s == QStringLiteral("tesouro"))  return TipoAtivo::Tesouro;
    if (s == QStringLiteral("fundo"))    return TipoAtivo::Fundo;
    if (s == QStringLiteral("poupanca")) return TipoAtivo::Poupanca;
    return TipoAtivo::Outro;
}

inline QString tipoAtivoLabel(TipoAtivo t)
{
    switch (t) {
    case TipoAtivo::Acao:     return QStringLiteral("Ação");
    case TipoAtivo::Fii:      return QStringLiteral("FII");
    case TipoAtivo::Cdb:      return QStringLiteral("CDB");
    case TipoAtivo::LciLca:   return QStringLiteral("LCI/LCA");
    case TipoAtivo::Tesouro:  return QStringLiteral("Tesouro");
    case TipoAtivo::Fundo:    return QStringLiteral("Fundo");
    case TipoAtivo::Poupanca: return QStringLiteral("Poupança");
    default:                  return QStringLiteral("Outro");
    }
}

inline QString tipoAtivoLabel(const QString &raw)
{
    return tipoAtivoLabel(tipoAtivoDeTexto(raw));
}

// ── Tipo de operação ──────────────────────────────────────────────────────────

enum class TipoOperacaoInv {
    Aporte, Resgate, Rendimento, Dividendo
};

inline QString tipoOpInvParaTexto(TipoOperacaoInv t)
{
    switch (t) {
    case TipoOperacaoInv::Aporte:     return QStringLiteral("aporte");
    case TipoOperacaoInv::Resgate:    return QStringLiteral("resgate");
    case TipoOperacaoInv::Rendimento: return QStringLiteral("rendimento");
    default:                          return QStringLiteral("dividendo");
    }
}

inline TipoOperacaoInv tipoOpInvDeTexto(const QString &s)
{
    if (s == QStringLiteral("aporte"))     return TipoOperacaoInv::Aporte;
    if (s == QStringLiteral("resgate"))    return TipoOperacaoInv::Resgate;
    if (s == QStringLiteral("rendimento")) return TipoOperacaoInv::Rendimento;
    return TipoOperacaoInv::Dividendo;
}

inline QString tipoOpInvLabel(TipoOperacaoInv t)
{
    switch (t) {
    case TipoOperacaoInv::Aporte:     return QStringLiteral("Aporte");
    case TipoOperacaoInv::Resgate:    return QStringLiteral("Resgate");
    case TipoOperacaoInv::Rendimento: return QStringLiteral("Rendimento");
    default:                          return QStringLiteral("Dividendo");
    }
}

inline QString tipoOpInvLabel(const QString &raw)
{
    return tipoOpInvLabel(tipoOpInvDeTexto(raw));
}

// ── Model ─────────────────────────────────────────────────────────────────────

struct LancamentoInvestimento {
    int        id           = 0;
    int        mes          = 1;     // 1–12
    int        ano          = 2025;
    int        dia          = 1;
    QString    nome;                 // nome do ativo: "PETR4", "CDB Nubank"
    QString    tipoAtivo;            // raw: "acao"|"fii"|"cdb"|"lci_lca"|"tesouro"|"fundo"|"poupanca"|"outro"
    QString    tipoOperacao;         // raw: "aporte"|"resgate"|"rendimento"|"dividendo"
    qint64     valor        = 0;     // centavos (sempre positivo; direção dada por tipoOperacao)
    QString    categoria;            // snapshot do nome da categoria
    int        categoriaId  = 0;    // FK → categorias(id); 0 = sem vínculo
    QString    obs;
    QString    criadoEm;
    QByteArray receipt;             // anexo do comprovante (#90); vazio = sem anexo
};

// Valor com sinal para cálculo do saldo líquido:
// aporte = saída de caixa (negativo); resgate/rendimento/dividendo = entrada (positivo).
inline qint64 valorSignadoInv(const LancamentoInvestimento &l)
{
    return (l.tipoOperacao == QStringLiteral("aporte")) ? -l.valor : l.valor;
}
