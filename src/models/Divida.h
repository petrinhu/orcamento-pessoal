#pragma once

#include <QString>
#include <QtTypes>

// Tipo: quem deve a quem.
// 'divida'     → o usuário deve a outra pessoa/instituição.
// 'emprestimo' → outra pessoa deve ao usuário.
enum class TipoDivida { Divida, Emprestimo };

inline QString tipoDividaParaTexto(TipoDivida t)
{
    return t == TipoDivida::Emprestimo
        ? QStringLiteral("emprestimo")
        : QStringLiteral("divida");
}

inline TipoDivida tipoDividaDeTexto(const QString &s)
{
    return s == QStringLiteral("emprestimo")
        ? TipoDivida::Emprestimo
        : TipoDivida::Divida;
}

inline QString tipoDividaLabel(TipoDivida t)
{
    return t == TipoDivida::Emprestimo
        ? QStringLiteral("Empréstimo")
        : QStringLiteral("Dívida");
}

// Status da dívida/empréstimo.
enum class StatusDivida { EmAndamento, Quitado, Atrasado };

inline QString statusDividaParaTexto(StatusDivida s)
{
    switch (s) {
    case StatusDivida::Quitado:    return QStringLiteral("quitado");
    case StatusDivida::Atrasado:   return QStringLiteral("atrasado");
    default:                       return QStringLiteral("em_andamento");
    }
}

inline StatusDivida statusDividaDeTexto(const QString &s)
{
    if (s == QStringLiteral("quitado"))  return StatusDivida::Quitado;
    if (s == QStringLiteral("atrasado")) return StatusDivida::Atrasado;
    return StatusDivida::EmAndamento;
}

inline QString statusDividaLabel(StatusDivida s)
{
    switch (s) {
    case StatusDivida::Quitado:  return QStringLiteral("Quitado");
    case StatusDivida::Atrasado: return QStringLiteral("Atrasado");
    default:                     return QStringLiteral("Em andamento");
}
}

struct Divida {
    int     id          = 0;
    QString tipo;                  // "divida" | "emprestimo"
    QString contraparte;           // nome do credor / devedor
    QString descricao;
    qint64  valorTotal  = 0;       // centavos
    qint64  valorPago   = 0;       // centavos
    int     jurosMensal = 0;       // % × 10000 (ex: 1,5% = 15000)
    QString dataInicio;            // ISO yyyy-MM-dd
    QString dataVenc;              // ISO yyyy-MM-dd (vazio = sem vencimento)
    QString status;                // "em_andamento" | "quitado" | "atrasado"
    QString obs;
    QString criadoEm;              // ISO 8601

    // Helpers
    qint64 saldo() const { return valorTotal - valorPago; }
};
