#pragma once

#include <QString>
#include <QtTypes>

struct LancamentoEntrada {
    int     id         = 0;
    int     mes        = 1;    // 1–12
    int     ano        = 2025;
    int     dia        = 1;    // 1 quando usuário informou apenas MM/yyyy
    QString fonte;             // snapshot do nome da fonte
    int     fonteId     = 0;    // 0 = sem vínculo com fontes_renda
    QString categoria;          // snapshot do nome da categoria
    int     categoriaId = 0;    // FK → categorias(id); 0 = sem vínculo
    qint64  valorBruto  = 0;    // centavos
    qint64  irpf       = 0;    // centavos (IRPF retido na fonte)
    qint64  inss       = 0;    // centavos (INSS retido)
    // valorLiquido = valorBruto - irpf - inss  (calculado, não armazenado)
    QString obs;               // observação livre
    QString criadoEm;          // ISO 8601
};
