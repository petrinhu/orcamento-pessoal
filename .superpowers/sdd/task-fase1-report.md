# Relatório — Fase 1 Redesign UI: Paleta Azul-Confiável (#164)

**Branch:** redesign-ui
**Data:** 2026-06-20
**Status:** DONE

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

## Concerns / notas para Fase 2

1. Widgets que usavam tokens Breeze como valores hardcoded (ex.: `#3DAEE9` como cor inline em código C++) precisarão ser auditados — uma busca por `3DAEE9` e `EFF0F1` no código pode revelar referências não migradas.
2. A sidebar (navy `#1B2A41`) será usada pela Fase 2 (navegação/layout). Os tokens já estão prontos.
3. O `subtexto()` getter foi adicionado no header mas sem declaração `extern` no `namespace Color` pois é derivado dos tokens `SubtextLight/Dark` já existentes — segue o mesmo padrão de `acento()`, `positivo()` etc.
4. `atencao()` manteve suas cores originais (`#FFF3C4` / `#8A6D00`) — é um token semântico de estado (aviso), não de paleta principal, e não foi incluído no escopo da Fase 1.
