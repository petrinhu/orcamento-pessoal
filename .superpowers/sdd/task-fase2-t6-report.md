# Task 6 — QSS sidebar navy (#165) — Relatório

**Data:** 2026-06-20
**Branch:** redesign-ui
**Escopo:** Fase 2, Task 6 — estilo QSS da sidebar (navy, grupos, pill ativo)

---

## QSS adicionado (Theme.cpp `gerarQss()`)

Bloco inserido após `QDialogButtonBox QPushButton`:

```
QWidget#sidebarNav { background-color: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 @@SIDEBAR1@@, stop:1 @@SIDEBAR2@@); min-width:230px; max-width:230px; }
QWidget#sidebarNav > QWidget, QWidget#sidebarNav QWidget#sidebarUsuario { background-color: transparent; }
QFrame#sidebarSep { background: rgba(255,255,255,0.08); max-height:1px; border:none; }
QWidget#sidebarNav QLabel#grupo { color: rgba(175,189,208,0.55); font-size:10px; font-weight:600; letter-spacing:1px; padding:12px 10px 4px; background:transparent; }
QWidget#sidebarNav QPushButton#navItem { color:@@SIDEBARTXT@@; background:transparent; border:none; text-align:left; padding:9px 11px; border-radius:9px; font-weight:500; font-size:13px; }
QWidget#sidebarNav QPushButton#navItem:hover { background:rgba(255,255,255,0.06); color:white; }
QWidget#sidebarNav QPushButton#navItem[ativo="true"] { background:@@ACC@@; color:white; font-weight:600; }
QWidget#sidebarNav QLabel#sidebarAvatar { background:@@ACC@@; color:white; border-radius:14px; font-weight:600; font-size:13px; }
QWidget#sidebarNav QLabel#sidebarNome { color:@@SIDEBARTXT@@; font-size:13px; font-weight:500; background:transparent; }
```

## Tokens adicionados

| Token         | Getter            | Valor (light=dark) |
|---------------|-------------------|--------------------|
| `@@SIDEBAR1@@`  | `Theme::sidebar1()` | `#1B2A41`        |
| `@@SIDEBAR2@@`  | `Theme::sidebar2()` | `#16233A`        |
| `@@SIDEBARTXT@@`| `Theme::sidebarText()` | `#AFBDD0`   |

Ordem de replace: tokens sidebar inseridos por último, após `@@HOV@@`. Tokens `@@ACC_HOVER@@` e `@@ACC_PRESSED@@` movidos antes de `@@ACC@@` para evitar replace parcial (embora `@@ACC@@` com delimitadores `@@` nunca conflite, a ordem mais segura foi mantida).

## Como o pill ativo funcionou

`setAtivo(chave)` já chamava `setProperty("ativo", bool)` + `style()->polish(btn)`. O QSS `[ativo="true"]` casa com `bool true` diretamente em Qt6 — sem necessidade de usar string `"true"`. Confirmado nos testes `setAtivoMarcaSoOAlvo` (propriedade bool) e `navItemAtivo_contemAccento` (valor accent no bloco do seletor).

## Mudanças em SidebarNav.cpp

1. `setObjectName(QStringLiteral("sidebarNav"))` — widget raiz — necessário para seletor QSS.
2. `setAutoFillBackground(true)` — sem isso, Fusion style não pinta `background-color` com gradiente em QWidget simples.
3. `usuarioWidget->setObjectName(QStringLiteral("sidebarUsuario"))` — permite regra transparente específica no QSS.
4. Removido `setStyleSheet(...)` inline do `m_avatarLabel` — conflitava com o QSS global; estilo movido para `Theme.cpp`.

## Resultado do screenshot offscreen

Executável temporário `/tmp/t6_sidebar_render2` compilado com `g++ -std=c++20` + pkg-config Qt6Widgets, usando os .o do `test_sidebar_nav`. `QT_QPA_PLATFORM=offscreen`, `nav.show()` + `app.processEvents()` antes de `grab()`.

Resultado: `/tmp/t6_sidebar.png` (240×600 px):
- Fundo navy com gradiente vertical (#1B2A41 → #16233A): OK
- Rótulos de grupo "DIA A DIA" / "PLANEJAR" em cinza-azulado dim (rgba 55%), uppercase via `toUpper()` no código: OK
- Item "Gastos" com pill azul (#3B82F6), texto branco, font-weight 600: OK (item ativo via setAtivo)
- Itens normais em #AFBDD0: OK
- Footer "Preferências" e "Ajuda" com mesma cor dos navItems: OK
- Avatar "P" (círculo accent) + nome "Petrus" sobre fundo transparente (gradiente navy visível): OK

## ctest

`100% tests passed, 0 tests failed out of 30` — incluindo `test_theme_tokens` (14 casos, 3 novos de sidebar) e `test_sidebar_nav` (4 casos).
