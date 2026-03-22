#pragma once

#include <QString>

struct Parcela {
    int     id         = 0;
    int     compraId   = 0;
    int     numero     = 1;       // número absoluto da parcela (1..N)
    qint64  valor      = 0;       // centavos
    QString dataFatura;           // dd/MM/yyyy; vazio = calculada
    bool    modificada = false;   // true = editada manualmente pelo usuário
};
