# Relatório — Fase 1 Redesign UI: Paleta Azul-Confiável (#164) + Fix Bug Tokens QSS

**Branch:** redesign-ui
**Data:** 2026-06-20
**Status:** DONE (inclui fix crítico de tokens 2026-06-20)

---

## O que mudou

### `src/ui/Theme.h`

- Comentário de cabeçalho atualizado: "Paleta azul-confiável (redesign Fase 1, #164)"
- Adicionados no `namespace Color`:
  - `Surface2Light` / `Surface2Dark`
  - `DouradoLight` / `DouradoDark`
  - `Sidebar1Light` / `Sidebar1Dark`
  - `Sidebar2Light` / `Sidebar2Dark`
  - `SidebarTextLight` / `SidebarTextDark`
- Adicionados os getters públicos: `subtexto()`, `bg2()`, `dourado()`, `sidebar1()`, `sidebar2()`, `sidebarText()`

### `src/ui/Theme.cpp`

#### Paleta Light (substituição completa dos valores Breeze)

| Token | Antigo (Breeze) | Novo (azul-confiável) |
|---|---|---|
| BgLight | `#EFF0F1` | `#EEF3F9` |
| SurfaceLight | `#FFFFFF` | `#FFFFFF` (inalterado) |
| BorderLight | `#BDC3C7` | `#E4EAF2` |
| TextLight | `#232629` | `#15233B` |
| SubtextLight | `#7F8C8D` | `#67768D` |
| AccentLight | `#3DAEE9` | `#3B82F6` |
| PositiveLight | `#27AE60` | `#16C784` |
| NegativeLight | `#DA4453` | `#EA3943` |
| **Surface2Light** | (novo) | `#F7FAFD` |
| **DouradoLight** | (novo) | `#E8A93B` |
| **Sidebar1Light** | (novo) | `#1B2A41` |
| **Sidebar2Light** | (novo) | `#16233A` |
| **SidebarTextLight** | (novo) | `#AFBDD0` |

#### Paleta Dark

| Token | Antigo (Breeze) | Novo |
|---|---|---|
| BgDark | `#31363B` | `#0F172A` |
| SurfaceDark | `#1B1E20` | `#1B2430` |
| BorderDark | `#575B5F` | `#2A3340` |
| TextDark | `#EFF0F1` | `#EAF0F8` |
| SubtextDark | `#7F8C8D` | `#8A97A8` |
| AccentDark | `#3DAEE9` | `#3B82F6` |
| PositiveDark | `#27AE60` | `#16C784` |
| NegativeDark | `#DA4453` | `#EA3943` |
| **Surface2Dark** | (novo) | `#131A24` |
| **DouradoDark** | (novo) | `#E8A93B` |
| **Sidebar1Dark** | (novo) | `#1B2A41` (mesmo navy do light — sidebar não inverte) |
| **Sidebar2Dark** | (novo) | `#16233A` |
| **SidebarTextDark** | (novo) | `#AFBDD0` |

#### Getters adicionados (seguindo padrão dos existentes)

```cpp
QString subtexto()   { return isDark() ? Color::SubtextDark  : Color::SubtextLight;     }
QString bg2()        { return isDark() ? Color::Surface2Dark  : Color::Surface2Light;    }
QString dourado()    { return isDark() ? Color::DouradoDark   : Color::DouradoLight;     }
QString sidebar1()   { return isDark() ? Color::Sidebar1Dark  : Color::Sidebar1Light;    }
QString sidebar2()   { return isDark() ? Color::Sidebar2Dark  : Color::Sidebar2Light;    }
QString sidebarText(){ return isDark() ? Color::SidebarTextDark : Color::SidebarTextLight;}
```

#### QSS — o que mudou

- `QFrame#surface`: `border-radius` de 8px → **14px** (cards)
- `QPushButton` (default e `#primary`): `border-radius` de 6px → **9px** (botões/inputs)
- `QLineEdit`, `QSpinBox`, `QDoubleSpinBox`, `QComboBox`, `QDateEdit`: `border-radius` de 6px → **9px**
- Hover alternado (`alt`): usa `Color::Surface2Light` em light (era `#F5F5F5`); `#171F2D` em dark
- Hover de itens (`hov`): `#DCE8FA` em light (era `#D0E9F6`); `#222D3D` em dark
- Todas as referências a `%7` (que era `brd`) foram deslocadas: a posição do `brd` no `arg()` passou de %7 para %6, e `alt` para %7. O `neg` continuou na posição %6 mas não é usado no QSS por template (é substituído via `neg` var internamente se necessário)
- O mecanismo `@@ACC_HOVER@@` / `@@ACC_PRESSED@@` e o `multiArg` `%1..%9` permanecem intactos
- `setMode()` agora guarda `qApp` antes de chamar `setPalette/setStyleSheet` para compatibilidade com testes offscreen sem QApplication

### `tests/test_theme_tokens.cpp` (novo)

Testa em modo Light (via `setMode(Light)`):
- `acento() == "#3B82F6"`
- `positivo() == "#16C784"`
- `negativo() == "#EA3943"`
- `dourado() == "#E8A93B"`
- `sidebar1()` não vazio
- Dark smoke test: `acento()` dark não vazio

### `tests/CMakeLists.txt`

Adicionado:
```cmake
peubud_add_test(test_theme_tokens
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp)
set_tests_properties(test_theme_tokens PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
```

---

## Resultado do build e teste

- `cmake --build build -j$(nproc)`: limpo, sem warnings
- `test_theme_tokens`: **8/8 PASS** (3ms)
- `ctest --test-dir build`: **28/28 PASS** (1.05s total; `test_theme_tokens` é o #27)

---

---

## Fix crítico de tokens QSS (2026-06-20)

### Causa raiz

Em `gerarQss()`, o `.arg(txt, bg, surf, acc, sub, neg, brd, alt, hov)` posicionava:

| Posição | Variável injetada | Variável esperada pelo seletor |
|---------|-------------------|-------------------------------|
| `%6`    | `neg` (#EA3943 vermelho) | `brd` (#E4EAF2 cinza) |
| `%7`    | `brd`             | `alt` (alternating row)       |
| `%8`    | `alt`             | `hov` (hover background)      |

Resultado: ~15 regras de borda (QMenu, QFrame#surface, QPushButton, inputs, gridline-color, QScrollBar handle, QStatusBar, QGroupBox, QTabWidget, etc.) recebiam `#EA3943` (vermelho) — bordas do app visualmente vermelhas.

### Solução

Migração completa do esquema posicional `%N` para tokens nomeados `@@TOKEN@@` + `qss.replace(...)`. Cada token tem semântica inequívoca:

```
@@TXT@@  @@BG@@  @@SURF@@  @@ACC@@  @@SUB@@
@@NEG@@  @@BRD@@  @@ALT@@  @@HOV@@
@@ACC_HOVER@@  @@ACC_PRESSED@@
```

- Bordas → `@@BRD@@` (15+ seletores)
- Erro → `@@NEG@@` (apenas `QLabel#error`)
- Alternating row → `@@ALT@@`
- Hover → `@@HOV@@`

### Correção menor — accent hover/pressed

Ajustados para tons coerentes com o accent base `#3B82F6`:
- `accHover`: `#5CC6FA` → `#60A5FA` (blue-400, mais claro)
- `accPressed`: `#1F7AAD` → `#2563EB` (blue-600, mais escuro)

### Acessor de teste

Adicionado `Theme::qssParaTeste()` (declarado no header, implementado no `.cpp`) — expõe o QSS gerado sem precisar de `aplicar()`. Permite testes de regressão isolados.

### Novos casos de teste em `test_theme_tokens.cpp`

| Caso | Verifica |
|------|---------|
| `scrollbarHandle_usaBordaNaoNegativa` | handle = `@@BRD@@`, não `@@NEG@@` |
| `labelError_usaNegativaCorreto` | `QLabel#error` = `@@NEG@@`, não `@@BRD@@` |
| `inputBorder_usaBordaNaoNegativa` | borda inputs = `@@BRD@@` |
| `gridline_usaBordaNaoNegativa` | gridline-color = `@@BRD@@` |
| `menuBarBorder_usaBordaNaoNegativa` | border-bottom = `@@BRD@@` |
| `borda_light` | `Theme::borda()` = `#E4EAF2` |
| `accHover_tomClaroCoerente` | QSS contém `#60A5FA` |
| `accPressed_tomEscuroCoerente` | QSS contém `#2563EB` |

### Resultado

```
cmake --build build -j$(nproc)      → 100% Built (sem erros)
ctest --test-dir build              → 28/28 Passed
cmake --build build-rel -j$(nproc) → 100% Built Release
```

Screenshot `/tmp/f1_fix_shot.png`: LoginDialog com borda `#E4EAF2` (cinza-azulado neutro), fundo branco, botão "Entrar" em azul `#3B82F6`. Nenhum elemento vermelho nos contornos — bug confirmado corrigido.

---

## Concerns / notas para Fase 2

1. Widgets que usavam tokens Breeze como valores hardcoded (ex.: `#3DAEE9` como cor inline em código C++) precisarão ser auditados — uma busca por `3DAEE9` e `EFF0F1` no código pode revelar referências não migradas.
2. A sidebar (navy `#1B2A41`) será usada pela Fase 2 (navegação/layout). Os tokens já estão prontos.
3. O `subtexto()` getter foi adicionado no header mas sem declaração `extern` no `namespace Color` pois é derivado dos tokens `SubtextLight/Dark` já existentes — segue o mesmo padrão de `acento()`, `positivo()` etc.
4. `atencao()` manteve suas cores originais (`#FFF3C4` / `#8A6D00`) — é um token semântico de estado (aviso), não de paleta principal, e não foi incluído no escopo da Fase 1.
