# fix-iconbtn-1render-report.md

**Status:** CONCLUÍDO
**Branch:** redesign-ui
**Escopo do ID:** #165 (botões-ícone sem borda)

---

## 1. Solução escolhida

Override de `showEvent` no `DashboardWidget`, com repolish explícito dos 4 botões `#iconButton` da navBar **apenas na primeira exibição** (flag `m_firstShow`).

**Por que esta abordagem:**
- Cirúrgica: atua exatamente no momento em que o Qt expõe o widget ao display — o ponto correto para forçar a reavaliação da cascata QSS.
- Sem `singleShot(0)` (hack frágil de timer): o `showEvent` é síncrono e determinístico.
- Custo zero em shows subsequentes (`m_firstShow = false` após o primeiro ciclo).
- `m_navIconBtns` (lista dos 4 botões guardada no `buildUi`) evita `findChildren` a cada show — sem varredura da árvore de widgets.
- O ciclo `unpolish → polish` é a API canônica do Qt para forçar reavaliação de estilo.

---

## 2. Escopo da correção

**Apenas a navBar do Dashboard (`DashboardWidget`).**

O quirk afeta widgets cujo objectName é setado antes do primeiro show E cujo parent ainda não foi exibido. No PeuBud, apenas o `DashboardWidget` tem `#iconButton` em posição que exibiu o bug (confirmado visualmente). Outros `#iconButton` (ex.: `QToolButton#iconButton` dos toggles do `TabShell`) são criados em widgets que já passaram pelo polish antes do `DashboardWidget` ser adicionado ao layout — portanto não sofrem o mesmo quirk.

Se outro widget sofrer o mesmo sintoma no futuro, o padrão `showEvent + repolish dos m_navIconBtns` é o template a seguir.

---

## 3. Arquivos alterados

| Arquivo | Tipo | O que mudou |
|---|---|---|
| `src/ui/widgets/DashboardWidget.h` | Header | `+showEvent` override; `+m_firstShow`; `+m_navIconBtns`; `+QList<QPushButton*>` include |
| `src/ui/widgets/DashboardWidget.cpp` | Impl | `+QShowEvent` include; registo de `m_navIconBtns` no `buildUi()`; implementação de `showEvent` |
| `tests/test_iconbtn_render.cpp` | Teste | `+NavBarComRepolish` (espelha fix); `+iconButtonSemBordaNoFirstRenderLight/Dark` (2 casos de regressão do 1º render) |

---

## 4. Teste de 1 linha

```
QT_QPA_PLATFORM=offscreen ctest --test-dir build -R test_iconbtn_render
```

Resultado: **6/6 PASS** (4 casos originais + 2 novos de regressão do 1º render).

---

## 5. Descrição dos dois renders

### Antes do fix (comportamento bugado — sem showEvent/repolish)
O `QPushButton#iconButton` era polished pelo Fusion style usando a regra-base `QPushButton { border: 1px solid @@BRD@@; background: @@SURF@@ }` porque a regra `#iconButton` (por objectName) não era aplicada no primeiro polish. Resultado: `btnPrev` (◀️) saia com borda visível de 1px + fundo `@@SURF@@` — a "caixa". Os outros botões da navBar tinham o mesmo potencial mas o `btnPrev`, sendo o primeiro filho criado no layout, era o mais afetado de forma consistente. O bug desaparecia após qualquer evento que disparasse unpolish/polish (resize, trocar de aba, maximizar).

Pixel (17,13) no render dark sem fix: teria sido `#2A3340` (Color::BorderDark) — cor de borda.

### Depois do fix (com showEvent + repolish)
O `showEvent` dispara no primeiro `show()` e executa `style()->unpolish(btn); style()->polish(btn)` para cada um dos 4 botões da navBar. O Fusion style reavalia a cascata QSS e a regra `QPushButton#iconButton { background: transparent; border: none }` vence corretamente.

- **1render_depois.png (light):** pixel (17,13) = RGB(238,243,249) = `#EEF3F9` (fundo `@@BG@@`). Sem borda.
- **1render_antes.png (dark):** pixel (17,13) = RGB(15,23,42) = `#0F172A` (fundo `@@BG@@` dark). Sem borda.

Ambos confirmam que nenhum botão exibe caixa no primeiro render.

---

## 6. Gates

- `cmake --build build -j$(nproc)`: LIMPO (warnings nodiscard no MainWindow.cpp são pré-existentes)
- `cmake --build build-rel -j$(nproc)`: LIMPO
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build`: **40/40 PASS**
