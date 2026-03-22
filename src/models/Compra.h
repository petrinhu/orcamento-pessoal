#pragma once

#include <QString>

struct Compra {
    int     id            = 0;
    QString nome;
    qint64  valorTotal    = 0;    // centavos; 0 = não informado
    int     numParcelas   = 1;
    int     parcelaInicial = 1;   // ex: já é a 4ª de 10
    int     mesInicio     = 1;    // 1–12, mês da parcelaInicial
    int     anoInicio     = 2025;
    qint64  jurosMensais  = 0;    // % × 10000; 0 = sem juros
    QString moeda         = QStringLiteral("BRL");
    int     cartaoId      = 0;    // 0 = sem cartão
    QString criadoEm;             // ISO 8601
};
