#pragma once

#include <QString>
#include <QtGlobal>

// Formatação/parse de valores monetários para a UI (centavos qint64).
// Consolida as cópias `parseCentavos`/`formatCentavos` antes espalhadas
// pelos widgets (#122). Convenção do projeto: valor sempre em centavos.
namespace uifmt {

// Converte texto digitado pelo usuário em centavos.
// Aceita "1.234,56" (pt-BR: ponto=milhar, vírgula=decimal),
// "1234.56" (decimal com ponto) e "1234" (inteiro). Ignora "R$" e espaços.
// Sinal negativo preservado. Texto inválido → 0.
// Usa std::llround (64-bit) para evitar overflow de qRound (32-bit).
qint64 parseCentavos(const QString &txt);

// Formata centavos SEM prefixo de moeda: ex. 123456 → "1234,56", -50 → "-0,50".
// (Não insere separador de milhar, preservando o visual histórico das telas.)
QString formatCentavos(qint64 cents);

// Formata centavos COM prefixo "R$ ": ex. 123456 → "R$ 1234,56".
QString formatCentavosBRL(qint64 cents);

} // namespace uifmt
