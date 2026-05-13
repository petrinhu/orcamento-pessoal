#pragma once

#include <QString>
#include <QDateTime>

// FechamentoMensal (#88) - snapshot imutavel de um mes fechado.
// Persistido em fechamentos_mensais(periodo PK, dados_json, criado_em,
// fechado_por). dados_json eh estrutura agregada (entradas, gastos,
// investimentos, parcelas, saldo) - protegida contra edicoes posteriores.
struct FechamentoMensal {
    qint64    id           = 0;
    QString   periodo;          // YYYY-MM (ex: "2026-05")
    QString   dadosJson;        // JSON serializado da agregacao
    QDateTime criadoEm;
    QString   fechadoPor;
};
