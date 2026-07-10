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
// (Não insere separador de milhar, preservando o visual histórico das telas.
// Usado nos campos EDITÁVEIS — o texto volta para parseCentavos ao salvar —
// por isso fica fora do escopo do #197/I18N-01, que é sobre telas de DISPLAY.)
QString formatCentavos(qint64 cents);

// Formata centavos como valor monetário de DISPLAY, com símbolo + separadores
// locale-aware: pt-BR → "R$ 1.234,56"; en-US → "$1,234.56". Usa a moeda
// padrão ativa (AppSettings::defaultCurrency(), via ParcelaService::simboloMoeda)
// e o idioma ativo (AppSettings::language()) — NUNCA hardcoded.
//
// #197 (I18N-01, auditoria 2026-06-22 §3.8): antes concatenava "R$" + vírgula
// fixos, ignorando idioma/moeda (~24 call-sites → todos corrigidos de uma vez
// aqui, função central). Fix em aritmética INTEIRA (regra do projeto — sem
// double): agrupamento de milhar via QLocale::toString(qint64), separador
// decimal e posição do símbolo por switch(language()) (não QLocale::toCurrencyString,
// que exige double).
//
// Nome mantido por compatibilidade com os call-sites existentes (não é mais
// "sempre BRL" — formata a moeda ativa do app).
QString formatCentavosBRL(qint64 cents);

// Símbolo isolado da moeda ativa (ex.: título de eixo Y de gráfico, "R$"/"$").
// Equivalente a ParcelaService::simboloMoeda(AppSettings::defaultCurrency()).
QString simboloMoedaAtiva();

// ── Datas (#197, I18N-05) ────────────────────────────────────────────────────
// Formatos de exibição de data locale-aware — pt-BR usa dd/MM primeiro,
// en-US usa MM/dd primeiro (ordem de dia/mês é ambígua entre locales;
// dígitos sempre zero-padded, independente do QLocale::dateFormat() nativo,
// para preservar largura fixa em tabelas). NÃO usar em ImportService/
// ExportService (formato de arquivo/interop, corretamente fixo em dd/MM/yyyy).
QString dataDisplayFormat();          // "dd/MM/yyyy" pt-BR / "MM/dd/yyyy" en-US
QString dataDisplayFormatCurta();     // "dd/MM" pt-BR / "MM/dd" en-US (sem ano)
QString dataDisplayFormatAnoCurto();  // "dd/MM/yy" pt-BR / "MM/dd/yy" en-US
QString mesAnoDisplayFormat();        // "MM/yyyy" (ordem mês/ano não é ambígua)

} // namespace uifmt
