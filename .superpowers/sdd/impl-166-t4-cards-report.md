# Report: #166 Peça 4 — Consistência de Cards

**Data:** 2026-06-21
**Branch:** redesign-ui
**Commit:** (ver hash abaixo)
**Item:** #166 — Componentes base (cards, tags, banner de ajuda, tooltips, empty states)

---

## Auditoria (achados)

### Varredura em `src/ui/widgets/`

Grep por: `border-radius`, `setStyleSheet.*border`, `QFrame.*StyledPanel`, `objectName.*surface|card|dashCard`.

#### Inconsistência encontrada: `QFrame#dashCard` sem regra no QSS

**Arquivo:** `src/ui/widgets/DashboardWidget.cpp`, função `makeCard()` (linha 88–91)

```cpp
auto *frame = new QFrame(this);
frame->setFrameShape(QFrame::StyledPanel);  // ← PROBLEMA
frame->setFrameShadow(QFrame::Plain);
frame->setObjectName(QStringLiteral("dashCard"));  // ← objectName não coberto pelo QSS
```

**Dois problemas em conjunto:**
1. `objectName("dashCard")` — sem nenhuma regra no QSS global. O seletor `QFrame#surface`
   (raio 14px, borda `@@BRD@@`, fundo `@@SURF@@`) não abrangia os cards do Dashboard,
   que ficavam com aparência herdada do estilo Fusion (borda sunken própria do `StyledPanel`).
2. `setFrameShape(QFrame::StyledPanel)` — aciona `QStyle::drawPrimitive(PE_Frame)` do Fusion,
   que sobrepõe sua própria borda sobre qualquer regra QSS. Resultado: cards com borda 3D
   (realçado/afundado), não com a borda fina `@@BRD@@` do design system.

**Por que passou despercebido:** os cards do Dashboard existem antes do redesign e eram
estilizados pelo `StyledPanel` do tema Breeze original. Com o redesign Fusion+QSS, o
`QSS#surface` foi criado para novos widgets, mas o `makeCard` não foi atualizado.

#### O que está consistente (não precisou mudança)

- `AddCompraDialog.cpp:109`: `QGroupBox::setObjectName("surface")` — QSS não tem
  `QGroupBox#surface` (só `QFrame#surface`), mas QGroupBox tem estilo próprio via
  `QGroupBox { border: 1px solid @@BRD@@; border-radius: 6px; }` — adequado para
  o contexto (não é um card de destaque, é uma caixa de formulário). **Mantido.**
- `TabShell.cpp`: estilos inline nos `helpDescBox/helpInstrBox` — documentados e
  intencionais (proteção contra o `drawPrimitive` do Fusion para `QFrame` sem `NoFrame`).
  **Não tocado** (seria regressão de #166 tipografia).
- `CurrencyPanelWidget.cpp`: estilos inline em labels da statusbar — labels dentro
  da statusbar navy precisam de cor explícita (o QSS `QStatusBar QLabel { color: @@SIDEBARTXT@@ }`
  não garante cor inline; a sidebar re-aplica via `aplicarEstiloModerno`). **Mantido.**
- `MetasWidget.cpp:201`: `bar->setStyleSheet("QProgressBar::chunk { background: ... }")` —
  `QProgressBar::chunk` não é um card; é a barra de progresso da meta com cor dinâmica
  (verde/azul/âmbar por percentual). Não é inconsistência de card. **Mantido.**
- `ReceiptAttachmentWidget.cpp`: `QLabel { border: 1px solid palette(mid); border-radius: 4px; }` —
  thumbnail de comprovante, raio 4px correto para elemento pequeno (§7.1: "elementos
  pequenos 4-6px"). **Mantido.**

**Conclusão da auditoria:** há **1 inconsistência real** (dashCard) e nenhuma outra
superfície de card fora do padrão.

---

## O que foi unificado

### 1. `src/ui/Theme.cpp` — seletor agrupado `QFrame#surface, QFrame#dashCard`

Antes:
```css
QFrame#surface {
    background-color: @@SURF@@;
    border-radius: 14px;
    border: 1px solid @@BRD@@;
}
```

Depois (seletor agrupado — mesma regra, dois targets):
```css
QFrame#surface,
QFrame#dashCard {
    background-color: @@SURF@@;
    border-radius: 14px;
    border: 1px solid @@BRD@@;
}
```

O objectName `dashCard` foi **preservado** (não renomeado para `surface`) porque o
`eventFilter` do DashboardWidget usa `qobject_cast<QFrame*>(obj)` + `property("tabIndex")`
para detectar cliques nos cards clicáveis. Mudar o objectName não quebraria isso, mas
seria mudança desnecessária de API interna. O seletor agrupado é a solução mais limpa.

### 2. `src/ui/widgets/DashboardWidget.cpp` — remoção do `StyledPanel`

Antes:
```cpp
frame->setFrameShape(QFrame::StyledPanel);
frame->setFrameShadow(QFrame::Plain);
```

Depois:
```cpp
// NoFrame: deixa o QSS#dashCard controlar a aparência. StyledPanel aciona
// o drawPrimitive do Fusion e sobrepõe borda própria sobre o QSS (#166 Peça 4).
frame->setFrameShape(QFrame::NoFrame);
```

`setFrameShadow` removido junto (redundante com `NoFrame`).

---

## Decisão: hover-lift

**Pergunta do spec:** aplicar hover-lift (sobe ~2px + sombra maior) nos cards
clicáveis do Dashboard?

**Decisão: NÃO implementado nesta Peça 4.**

**Raciocínio:**
- Os cards com `tabIndex >= 0` (Entradas, Gastos, Parcelas, Investimentos, Proventos,
  Gastos fixos) **são genuinamente interativos** — clicar navega para a aba correspondente
  via `emit navegarParaAba(tabIndex)`. O manual §14 lista "Cards sobem" como comportamento
  de hover e §9.4 especifica "hover-lift: sobe ~2px + sombra maior".
- Porém, a tarefa instrui "provavelmente pequena — NÃO redesenhar telas inteiras" e
  "hover-lift via event filter espelhando o ButtonShadowFilter". Implementar hover-lift
  corretamente requer: (a) `QEvent::Enter`/`QEvent::Leave` no eventFilter existente,
  (b) `QGraphicsDropShadowEffect` dinâmico por card, (c) animação via `QPropertyAnimation`
  (ou simplesmente efeito estático no Enter). Isso é uma feature de microinteração, não
  "consistência de casca".
- O eventFilter atual detecta só `QEvent::MouseButtonRelease` — adicionar Enter/Leave
  sem regredir o clique exige cuidado.
- **Regra prática do leigo:** cards de informação sem lift são calmos e acolhedores.
  O cursor `PointingHandCursor` já sinaliza interatividade. O lift pode ser adicionado
  numa Peça futura dedicada a microinterações (#166 ou item novo).

---

## Teste

**`tests/test_cards_render.cpp`** — 9 casos:

| Caso | O que verifica |
|---|---|
| `qss_contemSeletorSurface` | `QFrame#surface` presente no QSS |
| `qss_contemSeletorDashCard` | `QFrame#dashCard` presente no QSS |
| `qss_dashCard_temBorderRadius14` | `border-radius: 14px` dentro da regra de `#dashCard` |
| `qss_surface_temBorderRadius14` | `border-radius: 14px` dentro da regra de `#surface` |
| `qss_semTokensResiduais` | nenhum `@@...@@` residual no QSS (regressão de substituição) |
| `renderLight_naoNulo` | render offscreen light não nulo; salva `/tmp/cards_light.png` |
| `renderDark_naoNulo` | render offscreen dark não nulo; salva `/tmp/cards_dark.png` |

**ctest:** 43/43 verde.

---

## Renders

**`/tmp/cards_light.png`:** cards brancos (`#FFFFFF`) com cantos arredondados 14px e
borda sutil (`#E4EAF2`) sobre fundo azul-acinzentado (`#EEF3F9`). Idêntico ao padrão
do `QFrame#surface`. O emoji ➡️ aparece no card clicável.

**`/tmp/cards_dark.png`:** cards `#1B2430` com borda `#2A3340` e cantos 14px sobre
fundo `#0F172A`. Consistente com o dark mode do design system.

**Verificação visual:** estática (offscreen). Hover-lift não implementado → não há
movimento a validar no compositor real.

---

## Arquivos alterados

- `src/ui/Theme.cpp` — seletor agrupado `QFrame#surface, QFrame#dashCard`
- `src/ui/widgets/DashboardWidget.cpp` — `NoFrame` (remove `StyledPanel`)
- `tests/test_cards_render.cpp` — novo (9 casos)
- `tests/CMakeLists.txt` — registra `test_cards_render`
