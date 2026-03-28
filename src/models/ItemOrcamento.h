#pragma once

#include <QString>
#include <QtTypes>

struct ItemOrcamento {
    int     id           = 0;
    QString nome;
    QString tipo;           // "receita" | "despesa"
    qint64  valorMensal  = 0;   // centavos — valor planejado por mês
    // Vínculo para cálculo automático do real:
    //   tipo=="despesa": categoria em lancamentos_gastos; "*" = todos os gastos
    //   tipo=="receita": fonte em lancamentos_entradas; "" ou "*" = todas as entradas
    QString categoriaRef;
    QString criadoEm;
};
