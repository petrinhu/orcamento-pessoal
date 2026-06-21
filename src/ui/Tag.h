#pragma once

// Tag::Variante — enum das variantes de chip colorido (§9.5 manual de estilo).
//
// Semânticos (status):
//   Sucesso   → esmeralda  (#0D7A50/#D1FAE5 light, #34D399/#064E3B dark)
//   Perigo    → vermelho   (#9B1C1C/#FEE2E2 light, #FCA5A5/#7F1D1D dark)
//   Atencao   → dourado    (#92400E/#FEF3C7 light, #FCD34D/#78350F dark)
//   Info      → azul       (#1D4ED8/#DBEAFE light, #93C5FD/#1E3A5F dark)
//   Neutro    → slate      (#374151/#F1F5F9 light, #94A3B8/#1E293B dark)
//
// Tipo (fixo/variável):
//   Fixo      → azul (alias de Info)
//   Variavel  → roxo (alias de Roxo)
//
// Paleta categórica (gráficos, §5):
//   Azul      → azul categórico
//   Esmeralda → esmeralda categórica
//   Dourado   → dourado categórico (alias de Atencao)
//   Vermelho  → vermelho categórico (alias de Perigo)
//   Roxo      → roxo          (#5B21B6/#EDE9FE light, #A78BFA/#2E1065 dark)
//   Slate     → slate neutro  (alias de Neutro)
//
// Todos os pares fg/bg passam WCAG AA (≥4.5:1) em light e dark.
// Verificado em 2026-06-21:
//   sucesso:   4.73x / 5.06x
//   perigo:    6.67x / 5.28x
//   atencao:   6.37x / 6.29x
//   info:      5.49x / 6.38x
//   neutro:    9.41x / 5.71x
//   roxo:      7.57x / 5.60x

namespace Tag {

enum class Variante {
    // Semânticos
    Sucesso,
    Perigo,
    Atencao,
    Info,
    Neutro,
    // Tipo
    Fixo,       // alias para Info (azul)
    Variavel,   // alias para Roxo
    // Categóricos
    Azul,       // alias para Info
    Esmeralda,  // alias para Sucesso
    Dourado,    // alias para Atencao
    Vermelho,   // alias para Perigo
    Roxo,
    Slate,      // alias para Neutro
};

// Retorna a string usada como propriedade QSS "variante" para a variante dada.
// Usada internamente pelo Theme::criarTag e pode ser usada em testes de regressão.
inline const char* toQssKey(Variante v) {
    switch (v) {
        case Variante::Sucesso:
        case Variante::Esmeralda: return "sucesso";
        case Variante::Perigo:
        case Variante::Vermelho:  return "perigo";
        case Variante::Atencao:
        case Variante::Dourado:   return "atencao";
        case Variante::Info:
        case Variante::Fixo:
        case Variante::Azul:      return "info";
        case Variante::Neutro:
        case Variante::Slate:     return "neutro";
        case Variante::Variavel:
        case Variante::Roxo:      return "roxo";
    }
    return "neutro";
}

} // namespace Tag
