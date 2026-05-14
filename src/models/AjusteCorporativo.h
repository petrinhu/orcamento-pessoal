#pragma once

#include <QDate>
#include <QString>
#include <QtTypes>

// AjusteCorporativo (#65) - evento corporativo que afeta quantidade e/ou
// preco historico de um ativo. Persistido em ajustes_corporativos.
//
// Tipos:
//   "split"        - desdobramento (fator > 1). Multiplica qty, divide preco.
//   "inplit"       - grupamento (fator < 1). Divide qty, multiplica preco.
//   "bonificacao"  - bonus em acoes (% de novas acoes a custo zero).
//
// Aplicacao: ao computar valor historico de PrecoAtivo, multiplicar pelo
// fator cumulativo de todos os ajustes posteriores a aquela data.
struct AjusteCorporativo {
    qint64  id      = 0;
    QString ticker;
    QString tipo;        // split | inplit | bonificacao
    QDate   dataEx;      // data ex-evento (a partir desta data o ajuste vale)
    double  fator   = 1.0; // qty_nova = qty_antiga * fator;
                           // preco_novo = preco_antigo / fator
    QString observacao;
    QString criadoEm;
};
