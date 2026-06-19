#pragma once

#include <QString>

// Classificacao de despesa da categoria (#118).
// Fixo  = gasto recorrente/previsivel (aluguel, assinatura, plano).
// Variavel = gasto eventual/discricionario (lazer, mercado avulso).
// Persistido como INTEGER (0|1) na coluna `fixo` de `categorias`.
// Default = Variavel: categorias antigas, sem classificacao, sao seguras
// como "variavel" (nao infla o calculo de despesa fixa do orcamento).
enum class TipoDespesa {
    Variavel = 0,
    Fixo     = 1,
};

inline int tipoDespesaParaInt(TipoDespesa t)
{
    return t == TipoDespesa::Fixo ? 1 : 0;
}

inline TipoDespesa tipoDespesaDeInt(int v)
{
    return v == 1 ? TipoDespesa::Fixo : TipoDespesa::Variavel;
}

inline QString tipoDespesaLabel(TipoDespesa t)
{
    return t == TipoDespesa::Fixo ? QStringLiteral("Fixo")
                                  : QStringLiteral("Variável");
}

struct Categoria {
    int         id          = 0;
    QString     nome;
    QString     tipo;       // "gasto" | "entrada" | "investimento"
    int         paiId       = 0;   // 0 = categoria raiz
    QString     paiNome;           // snapshot para exibição (não armazenado no BD)
    TipoDespesa tipoDespesa = TipoDespesa::Variavel; // #118: fixo vs variável
};
