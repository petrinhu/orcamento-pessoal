# Report: fix(#165) sidebar — scroll + labels sem clip + statusbar integrada

**Status:** entregue — aguarda verificação em device real (KDE/Wayland)
**Commit:** ver hash abaixo
**Branch:** redesign-ui
**Linha de teste:** `QT_QPA_PLATFORM=offscreen ctest --test-dir build` → 32/32 verde

---

## Bugs corrigidos

### Bug 1a — Clip horizontal ("Planejamento/Orçamento")

**Raiz:** `QPushButton#navItem` com `padding: 9px 11px` + ícone 16px + espaço Qt ≈ 4px +
texto "Planejamento/Orçamento" ~188px (Inter 13px) = ~226px necessários, mas sidebar
tinha `min/max-width: 230px` com `rootLayout.contentsMargins(8,12,8,8)` tirando 16px
laterais → apenas 214px disponíveis para o botão. Clip garantido.

**Correção:**
- `QWidget#sidebarNav`: `min-width/max-width: 230px → 252px` (22px de folga)
- `QPushButton#navItem padding`: `9px 11px → 9px 7px` (economiza 8px totais no botão)
- Total disponível para texto: ≈ 207px — suficiente para "Planejamento/Orçamento" ~188px com margem

**Confirmado em render:** `repro_full.png` — "Planejamento/Orçamento" aparece completo.

### Bug 1b — Clip vertical (itens somem ao reduzir janela)

**Raiz:** zona de itens era um `QWidget` no `rootLayout` sem `QScrollArea`. Quando a
janela encolhia, o layout simplesmente cortava os widgets que não cabiam.

**Correção:** a zona de itens (`mainWidget`) foi envolvida numa `QScrollArea` transparente:
- `setFrameShape(NoFrame)` — sem moldura visível
- `setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff)` — sem scroll horizontal
- `setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded)` — vertical só quando necessário
- `setWidgetResizable(true)` — mainWidget recebe a largura disponível
- `viewport()->setAttribute(WA_NoSystemBackground)` + `setAutoFillBackground(false)` —
  viewport transparente; navy do `paintEvent()` do `SidebarNav` transpassa sem interferência
- Scrollbar vertical estilizada no QSS: 4px, handle rgba(255,255,255,0.15), discreta sobre navy
- `addStretch(1)` permanente no fim do `m_mainLayout`; `addGrupo` e `registrarItem`
  inserem com `insertWidget(count()-1)` para manter stretch sempre último
- O rodapé (separador + footerItems + usuário) fica **fora** da QScrollArea no `rootLayout`,
  portanto sempre visível independente da altura da janela

**Confirmado em render:** `repro_short.png` — rodapé (Preferências/Ajuda/Tema/Petrus)
visível e fixo; itens "Planejar" e "Investir" além da viewport (alcançáveis por scroll).

### Bug 2 — Faixa clara da statusbar quebrando a continuidade navy

**Raiz:** `QStatusBar` no QSS tinha `background-color: @@BG@@` (= `#EEF3F9` em light mode),
criando faixa clara contrastante sob o navy da sidebar.

**Correção:** seletor `QStatusBar` movido para a seção sidebar no QSS e reestilizado:
- `background-color: @@SURF@@` (branco em light, `#1B2430` em dark) — combina com o tema
- `color: @@SUB@@` — texto discreto
- `border-top: 1px solid @@BRD@@` — separador sutil
- `QStatusBar QLabel { background: transparent }` — labels da statusbar sem fundo próprio
- O seletor antigo (`QStatusBar { background: @@BG@@ ... }`) foi removido para evitar
  duplicação e conflito de especificidade

**Confirmado em render:** `repro_full.png` — statusbar usa `@@SURF@@` (#FFFFFF light),
coerente com o tema. A transição navy → fundo do app não é mais "navy vs BG azulado"
gritante; ambos os tons são do mesmo sistema de cores.

---

## Arquivos modificados

| Arquivo | Mudança |
|---|---|
| `src/ui/widgets/SidebarNav.h` | forward declare `QScrollArea`; doc de layout atualizado |
| `src/ui/widgets/SidebarNav.cpp` | `#include <QScrollArea/QScrollBar>`; construtor refatorado com QScrollArea transparente; `addGrupo` e `registrarItem` usam `insertWidget(count()-1)` no mainLayout |
| `src/ui/Theme.cpp` | `sidebarNav min/max-width 230→252px`; `navItem padding 11px→7px`; QSS scrollarea/scrollbar na sidebar; QStatusBar reestilizado em `@@SURF@@` |
| `tests/test_sidebar_repro.cpp` | novo teste offscreen — grabs 1100×690 e 1100×470 + asserts |
| `tests/CMakeLists.txt` | registro de `test_sidebar_repro` |

---

## Renders

- `/tmp/repro_full.png` — 1100×690: todos os labels visíveis, "Planejamento/Orçamento" completo, statusbar coerente
- `/tmp/repro_short.png` — 1100×470: rodapé fixo visível, itens além da viewport ficam fora do scroll (comportamento correto)

---

## Gates

- `cmake --build build -j$(nproc)` — limpo (0 erros, warnings pré-existentes no MainWindow)
- `cmake --build build-rel -j$(nproc)` — limpo
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build` — 32/32 verde
