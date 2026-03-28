#pragma once

#include <QString>
#include <QDate>
#include <QtTypes>

struct TransacaoRecorrente {
    int     id        = 0;
    QString descricao;
    QString tipo;           // "entrada" | "gasto"
    qint64  valor     = 0;  // centavos

    // só para tipo == "entrada"
    int     fonteId   = 0;
    QString fonte;          // snapshot do nome

    // só para tipo == "gasto"
    QString categoria;
    QString tipoPgto;       // raw: "pix"|"debito"|"dinheiro"|"boleto"|"transferencia"|"fixo"|"outro"

    QString obs;

    // recorrência
    QString frequencia;     // "semanal" | "quinzenal" | "mensal" | "anual"
    QDate   dataInicio;
    QDate   dataFim;        // isNull() = sem limite
    bool    ativo        = true;
    QDate   ultimoLancamento; // isNull() = nunca gerado

    QString criadoEm;
};
