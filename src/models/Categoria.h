#pragma once

#include <QString>

struct Categoria {
    int     id       = 0;
    QString nome;
    QString tipo;       // "gasto" | "entrada" | "investimento"
    int     paiId    = 0;   // 0 = categoria raiz
    QString paiNome;        // snapshot para exibição (não armazenado no BD)
};
