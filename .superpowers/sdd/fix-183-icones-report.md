# Report: fix #183 — Ícone da calculadora deixa de ser Breeze mono

**Data:** 2026-06-20
**Agent:** frontend-engineer
**Branch:** redesign-ui
**Item:** #183 — TUDO COLORIDO, cinza só quando desabilitado (política v1.1)

---

## Status

Concluído. Build limpo (debug + release), ctest 40/40 verde.

---

## O que mudou

### `src/ui/widgets/CalculadoraEdit.cpp`

Remoção do bloco `QIcon::fromTheme` + fallback condicional. Substituído por chamada direta
a `Theme::emojiIcon`:

**Antes (linhas 25-32):**
```cpp
const QIcon icon = QIcon::fromTheme(QStringLiteral("accessories-calculator"));
if (!icon.isNull()) {
    m_btn->setIcon(icon);
    m_btn->setIconSize(QSize(22, 22));
} else {
    m_btn->setIcon(Theme::emojiIcon(QStringLiteral("🧮"), 22));
    m_btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
}
```

**Depois:**
```cpp
// #183: TUDO COLORIDO — emojiIcon direto, sem fromTheme (que renderizaria Breeze mono)
m_btn->setIcon(Theme::emojiIcon(QStringLiteral("🧮"), 22));
m_btn->setIconSize(QSize(22, 22));
m_btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
```

O `#include <QIcon>` também foi removido (ficou órfão — `Theme.h` já inclui `<QIcon>`
e é suficiente para o tipo de retorno de `emojiIcon`).

**Razão:** `QIcon::fromTheme("accessories-calculator")` encontrava o ícone Breeze no
KDE Plasma e renderizava um 🧮 monocromático/cinza, **vencendo** o fallback colorido.
Isso violava a política "TUDO COLORIDO" (#183 / manual de estilo seção 13 v1.1).
Com `emojiIcon` direto, o 🧮 Noto Color Emoji colorido aparece em qualquer distro,
independente do tema de ícones do sistema.

### `docs/manual_estilo.md`

Atualizado pelo `visual-design-director` antes deste commit (seção 13 → v1.1). Incluído
no commit conforme instrução do líder. Consagra a política "TUDO COLORIDO" e detalha:
- Regra de ouro nº 0: todo ícone habilitado é colorido.
- Por que proibir `fromTheme`/`standardIcon` para ícones do app.
- O único cinza legítimo: estado `disabled` (automático do Qt, não decisão de design).

---

## Varredura preventiva

Comando executado:
```
grep -rnE 'QIcon::fromTheme|QStyle::standardIcon|QStyle::SP_' src/ui/
```

**Resultado: zero ocorrências restantes** em `src/ui/` após o fix. O único
`QIcon::fromTheme` do app estava em `CalculadoraEdit.cpp` e foi removido.

Confirmação: nenhum ícone de UI própria usa mais `fromTheme` ou `standardIcon`.
Ícones de sistema de diálogo (`QMessageBox::Warning/Critical/…`) não foram tocados
(estão fora de `src/ui/` e não fazem parte do design próprio do app).

---

## Verificação de build e testes

| Passo | Resultado |
|---|---|
| `cmake --build build -j$(nproc)` (debug) | Limpo — 0 erros, 0 warnings |
| `cmake --build build-rel -j$(nproc)` (release) | Limpo — 0 erros, 0 warnings |
| `QT_QPA_PLATFORM=offscreen ctest --test-dir build` | **40/40 passed** (1.59s) |

---

## O manual entrou no commit?

Sim. O `docs/manual_estilo.md` (editado pelo `visual-design-director`, seção 13 v1.1,
não commitado) foi incluído no mesmo commit do fix, conforme instrução do líder.
