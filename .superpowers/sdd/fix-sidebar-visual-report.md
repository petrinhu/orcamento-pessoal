# fix(#165): sidebar — remove drop-shadow dos itens + inset/hover/active como mockup

**Status:** implementado, ctest 36/36 verde, build-rel limpo.

## Bug 1 — Drop-shadow retangular nos navItems

**Causa raiz:** `ButtonShadowFilter` pulava botões `flat()`, e os navItems são criados com
`setFlat(true)`. Porém `QEvent::Polish` pode ser processado em janela que não passou pelo
layout ainda (ordem de eventos), e o check `isFlat()` pode não ser suficiente sozinho.
**Fix aplicado:** guard adicional em `ButtonShadowFilter::eventFilter`: qualquer
`QPushButton` com `objectName() == "navItem"` é descartado antes de instalar o
`QGraphicsDropShadowEffect`. Defense-in-depth: se por qualquer motivo o flat não estiver
setado no momento do Polish, o objectName garante que o filtro ainda pula.

**Teste:** `TestSidebarNav::navItemSemDropShadow()` — instancia SidebarNav com
`Theme::aplicar()` ativo, mostra o widget (dispara Polish/Show), verifica que
`btn->graphicsEffect() == nullptr` para todos os navItems (normais + footer).

## Bug 2a — Pill tocando borda direita / scrollbar sobrepondo pill

**Causa raiz:** `m_mainLayout` (dentro do QScrollArea/viewport) tinha
`contentsMargins(0, 0, 0, 0)`. O `rootLayout` tem 8px laterais mas o `mainWidget`
fica dentro do viewport do scroll area — a margem do rootLayout não chega até lá.
O scrollbar de 4px fica sobre o viewport, comendo espaço sem folga para o pill.

**Fix aplicado:** `m_mainLayout->setContentsMargins(8, 0, 8, 4)` — 8px cada lado dentro
do scroll area, garantindo folga entre o pill e as bordas/scrollbar.

**Confirmação por pixel (render `/tmp/sidebar_hover_fix.png`):**
- x=0..15: navy (folga esquerda ~16px = rootLayout 8 + mainLayout 8)
- x=16..~225: accent pill
- x=228..234: navy (folga direita ~8px antes do scrollbar)
- x=236..239: scrollbar (4px) — fora do pill
- x=244..251: navy do rootLayout
→ Cantos arredondados do pill visíveis nos 4 lados; sem toque nas bordas.

## Bug 2b — Hover fraco

`rgba(255,255,255,0.06)` → `rgba(255,255,255,0.10)` + `color: #ffffff` no QSS
`#navItem:hover`. Incremento de 4 pontos percentuais eleva perceptivelmente o contraste
sobre o navy sem perder a suavidade do design.

## Bug 2c — Foco residual

Adicionado `#navItem:focus { outline: none; border: none; }` no QSS. `NoFocusStyle`
já suprime o dotted-rect do Fusion, mas o seletor QSS fecha qualquer caminho alternativo
(estado de foco no Wayland, temas de acessibilidade).

## Render offscreen `/tmp/sidebar_hover_fix.png`

Gerado por `TestSidebarRender::pillAtivoComInset()` (test 32, Passed, 32ms).
Verificação visual confirmada:
- Nenhum box/sombra retangular visível sobre os itens
- Pill ativo (Gastos) com fundo accent e 4 cantos arredondados, com folga de ~8px
  de cada lado (navy visível entre pill e bordas laterais)
- Separação clara entre sidebar navy e área de conteúdo clara

## Commits e arquivos alterados

- `src/ui/ButtonShadowFilter.cpp` — guard objectName navItem
- `src/ui/widgets/SidebarNav.cpp` — `m_mainLayout` contentsMargins 8/0/8/4
- `src/ui/Theme.cpp` — hover 0.10, :focus sem outline, comentário pill ativo
- `tests/test_sidebar_nav.cpp` — slot `navItemSemDropShadow` + initTestCase
- `tests/test_sidebar_render.cpp` — slot `pillAtivoComInset` + include QPushButton

## ctest

```
100% tests passed, 0 tests failed out of 36
Total Test time (real) = 1.41 sec
```
