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
//
// #199 (QA-03, auditoria 2026-06-22 §3.4): "só ponto, sem vírgula" é
// ambíguo (decimal "12.5" vs. milhar "1.234"). Decisão de produto adotada
// (heurística de milhar, uma das 2 opções listadas na auditoria — a outra
// era validator/máscara "0,00" em ~10 diálogos, esforço bem maior): se o
// texto casa exatamente com grupos de 3 dígitos após ponto(s)
// (regex ^-?\d{1,3}(\.\d{3})+$, ex. "1.234", "12.345.678"), os pontos são
// separador de milhar — dinheiro nunca tem 3 casas decimais, então esse
// padrão nunca é ambíguo com decimal. Qualquer outro formato (ex. "12.5",
// "1234.56") continua sendo tratado como decimal (comportamento anterior).
qint64 parseCentavos(const QString &txt);

// Formata centavos SEM prefixo de moeda: ex. 123456 → "1234,56", -50 → "-0,50".
// (Não insere separador de milhar, preservando o visual histórico das telas.)
QString formatCentavos(qint64 cents);

// Formata centavos COM prefixo "R$ ": ex. 123456 → "R$ 1234,56".
QString formatCentavosBRL(qint64 cents);

} // namespace uifmt
