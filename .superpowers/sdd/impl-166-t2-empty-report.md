# Report — Peca 2 do #166: Empty States Guiados (12 telas)

**Data:** 2026-06-21
**Branch:** redesign-ui
**Status:** implementado, aguardando verificacao visual

---

## O que foi feito

### Componente reutilizavel: `EmptyStateWidget`

Novo componente `src/ui/widgets/EmptyStateWidget.{h,cpp}` com:

- Emoji 64px via `Theme::emojiIcon(emoji, 64)` — objectName `emptyStateEmoji`
- Titulo SemiBold 16px — objectName `emptyStateTitle` (estilizado via QSS com token `@@TXT@@`)
- Frase Regular 13px, wordWrap, maxWidth 480px — objectName `emptyStateFrase` (token `@@SUB@@`)
- CTA opcional: objectName `primary`, icone `➕`, minWidth 160px — ausente quando `ctaLabel` esta vazio
- Layout VBox com stretch top/bottom para centralizacao vertical

Acessores publicos: `ctaButton()`, `emojiLabel()`, `titleLabel()`, `phraseLabel()`.

### QSS adicionado em `Theme.cpp`

Tres blocos novos no literal de QSS:

```css
QLabel#emptyStateTitle  { font-size: 16px; font-weight: 600; color: @@TXT@@; ... }
QLabel#emptyStateFrase  { font-size: 13px; font-weight: 400; color: @@SUB@@; ... }
QLabel#emptyStateEmoji  { background-color: transparent; }
```

### 12 telas modificadas

| Widget | Emoji | CTA | Tecnica de conexao |
|---|---|---|---|
| ParcelasWidget | 💳 | Novo item | `connect(ctaBtn, clicked, this, &ParcelasWidget::onNovaCompra)` |
| EntradasWidget | 💵 | Novo lancamento | signal→signal: `connect(ctaBtn, clicked, btnNova, click)` |
| GastosWidget | 🧾 | Novo gasto | signal→signal: `connect(ctaBtn, clicked, btnNova, click)` |
| MetasWidget | 🎯 | Nova meta | `connect(ctaBtn, clicked, this, &MetasWidget::onNova)` |
| InvestimentosWidget | 📈 | Novo investimento | signal→signal: `connect(ctaBtn, clicked, btnNovo, click)` |
| PlanejamentoWidget | 📋 | Novo item | signal→signal: `connect(ctaBtn, clicked, btnNovo, click)` |
| RecorrenciaWidget | 🔄 | Adicionar | signal→signal: `connect(ctaBtn, clicked, btnAdd, click)` |
| DividasWidget | 💸 | Nova divida | `connect(ctaBtn, clicked, this, &DividasWidget::onNova)` |
| LembretesWidget | 🔔 | Novo | `connect(ctaBtn, clicked, this, &LembretesWidget::onNovo)` |
| ProventosWidget | 💰 | Novo provento | `connect(ctaBtn, clicked, this, &ProventosWidget::onNovo)` |
| TendenciasWidget | 📊 | — (sem CTA) | stack alternado em `recarregar()` quando `total == 0` |
| BolsaWidget | 🏦 | — (sem CTA) | stack alternado em `onBuscar()` quando `results.isEmpty()` |

**Pattern do `QStackedWidget`:** index 0 = EmptyStateWidget, index 1 = tabela/chart. A alternancia e feita no metodo de carregamento de dados de cada widget.

### CMakeLists.txt

Adicionado `src/ui/widgets/EmptyStateWidget.cpp` ao target `orcamento-pessoal` (link error corrigido apos 1a build).

---

## TDD

- Teste escrito em `tests/test_empty_state_render.cpp` (41 casos)
- Registrado em `tests/CMakeLists.txt` com `QT_QPA_PLATFORM=offscreen`
- **Red → Green:** componente implementado, testes validam API, copy exata e QSS

---

## Build e testes

```
cmake --build build -j$(nproc)       → [100%] Built target orcamento-pessoal  (LIMPO)
QT_QPA_PLATFORM=offscreen ctest      → 100% tests passed, 0 tests failed out of 44
```

---

## Renders gerados

| Arquivo | Descricao |
|---|---|
| `/tmp/empty_entradas_light.png` | Entradas, tema claro — emoji 💵, CTA "Novo lancamento" |
| `/tmp/empty_entradas_dark.png`  | Entradas, tema escuro — estrutura identica, fundo #1B1E20 |
| `/tmp/empty_metas_light.png`    | Metas, tema claro — emoji 🎯, CTA "Nova meta" |
| `/tmp/empty_tendencias_light.png` | Tendencias, tema claro — emoji 📊, SEM CTA |

Os renders confirmam: emoji centralizado, tipografia correta, CTA presente apenas onde previsto, dark mode com tokens corretos.

---

## Arquivos alterados

- `src/ui/widgets/EmptyStateWidget.h` (NOVO)
- `src/ui/widgets/EmptyStateWidget.cpp` (NOVO)
- `src/ui/Theme.cpp` (QSS rules adicionadas)
- `src/ui/widgets/ParcelasWidget.cpp`
- `src/ui/widgets/EntradasWidget.cpp`
- `src/ui/widgets/GastosWidget.cpp`
- `src/ui/widgets/MetasWidget.cpp`
- `src/ui/widgets/InvestimentosWidget.cpp`
- `src/ui/widgets/PlanejamentoWidget.cpp`
- `src/ui/widgets/RecorrenciaWidget.{h,cpp}`
- `src/ui/widgets/DividasWidget.cpp`
- `src/ui/widgets/LembretesWidget.cpp`
- `src/ui/widgets/ProventosWidget.{h,cpp}`
- `src/ui/widgets/TendenciasWidget.{h,cpp}`
- `src/ui/widgets/BolsaWidget.{h,cpp}`
- `tests/test_empty_state_render.cpp` (NOVO)
- `tests/CMakeLists.txt`
- `CMakeLists.txt` (raiz)
