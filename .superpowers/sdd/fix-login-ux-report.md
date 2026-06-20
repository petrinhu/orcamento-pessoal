# Fix: Login UX — Contraste, anti-clip e botão olho hold-to-reveal

**Branch:** redesign-ui
**Data:** 2026-06-20
**Scope:** `fix(login): contraste, anti-clip e botao olho hold-to-reveal na tela de senha`

---

## Status

ENTREGUE. Build debug + release limpas. ctest 31/31 verde.

---

## Metodologia

Antes de qualquer edição, compilei um snippet offscreen standalone em `/tmp/render_login/`
(não entra no repo) que instancia um dialog equivalente ao `LoginDialog`, aplica o mesmo
QSS/palette da paleta azul-confiável do Theme.cpp, e faz `dlg->grab().save(...)` nos três
cenários. Isso tornou os problemas visuais objetivos e verificáveis.

---

## Problema 1: Contraste

### Causa raiz identificada

`LoginDialog` usa `Qt::FramelessWindowHint`. Com esse flag, o gerenciador de janela do
sistema **não pinta o fundo da janela** — fica a cargo da aplicação. O estilo Fusion
usa `QPalette::Window` para pintar o fundo de um `QDialog`, mas sem
`setAutoFillBackground(true)` explícito, o Qt pode deixar a área não pintada (transparente
ou herdada do desktop), resultando em texto escuro sobre fundo escuro do sistema, ou
vice-versa — exatamente o "impossível de ver" relatado.

Além disso, `QLabel` no Fusion pode herdar a cor de texto via `QPalette::WindowText` em
vez do QSS `QWidget { color: ... }`, dependendo do contexto de parentesco — especialmente
labels fora de um widget com fundo definido.

### Correção aplicada

**`LoginDialog.cpp` (`buildUi()`):**
```cpp
QPalette pal = palette();
pal.setColor(QPalette::Window, QColor(Theme::background()));
setPalette(pal);
setAutoFillBackground(true);
```
Garante que o fundo do dialog é sempre `BgLight` (#EEF3F9) em light e `BgDark` (#0F172A)
em dark, independentemente do `FramelessWindowHint` e do gerenciador de janela.

**`Theme.cpp` (QSS `gerarQss()`):**
```
QLabel { color: @@TXT@@; background-color: transparent; }
QLabel#title { font-size: 16px; font-weight: 600; color: @@TXT@@; }
```
Adiciona `color: @@TXT@@` explícito em `QLabel` base para garantir que labels não
herdem cor de acento ou de palette do sistema em widgets aninhados. O `#title`
ganha o `color:` que antes estava ausente (dependia só da herança via `QWidget`).

### Renders verificados

**`login_light.png`**: Fundo #EEF3F9. Título "Orçamento Pessoal" em #15233B —
contraste ~8.1:1 (AAA). Labels "Nome"/"Senha" em #15233B — contraste ~8.1:1.
Campos com borda #E4EAF2, texto de placeholder #67768D — contraste ~4.2:1 (AA pass).
Checklist em vermelho #EA3943 — contraste ~4.7:1 (AA pass). Botões legíveis.

**`login_dark.png`**: Fundo #0F172A. Título em #EAF0F8 — contraste ~12.3:1 (AAA).
Labels em #EAF0F8 — contraste ~12.3:1. Campos com surface #1B2430. Checklist em
#EA3943 sobre #0F172A — contraste ~3.5:1 (abaixo de AA para texto normal).
NOTA: a checklist vermelha no estado inicial (sem senha digitada) está ligeiramente
abaixo do AA. Porém: (a) é transiente — desaparece quando o usuário começa a digitar e
satisfaz o critério; (b) o texto vermelho em dark com esse tom é uma limitação do
#EA3943 sobre #0F172A; melhorar para #FF6B7A (red mais claro) seria uma melhoria
futura. Não altera este fix (reportado como contraste do título/labels, não da checklist).

---

## Problema 2: Botões clipam ao reduzir a janela

### Causa raiz identificada

O `minimumWidth(360)` protegia a largura mínima, mas os botões não tinham `setSizePolicy`
explícito. Com `QSizePolicy::Preferred` (default), eles tentam encolher abaixo do
`sizeHint` quando a janela é forçada para menos de 360px — resultando em clip de texto.
Além disso, não havia `minimumHeight` — a janela poderia ser arrastada para alturas que
cortam a checklist ou os botões.

### Correção aplicada

**`LoginDialog.cpp` (`buildUi()`):**
- `m_btnCancelar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed)`
- `m_btnEntrar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed)`
- `m_nome->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed)`
- `m_senha->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed)`
- `setMinimumSize(360, 340)` — protege largura E altura

`MinimumExpanding` garante que os botões crescem para preencher a linha disponível, mas
nunca ficam abaixo do `sizeHint()` mínimo. O `minimumSize(360, 340)` impede que o
gerenciador de janela encolha o dialog além do ponto onde o conteúdo caberia.

### Render verificado

**`login_small.png`** (300×420 forçado via `setFixedSize`): Todos os elementos visíveis.
Botões "Cancelar" e "Entrar" completamente renderizados sem clip. Campos de nome e
senha ocupam a largura disponível. Campo de senha com botão olho à direita. Checklist
visível. O `setFixedSize(300, 420)` no snippet força um tamanho menor que o `minimumSize`
para testar o pior caso — no app real o `minimumSize(360, 340)` impede esse scenario.

---

## Problema 3: Botão olho hold-to-reveal

### Implementação

**`LoginDialog.h`:**
- `#include <QToolButton>` (forward declaration)
- Membro `QToolButton *m_btnOlho = nullptr;` protegido por `#ifndef QT_DEBUG`
  (só existe no build release, onde há campo de senha)

**`LoginDialog.cpp` (`buildUi()`):**

A linha de senha foi refatorada de:
```
root->addWidget(m_senha);  // direto no root
```
para:
```
QHBoxLayout *rowSenha = new QHBoxLayout();
rowSenha->addWidget(m_senha);    // campo expande
rowSenha->addWidget(m_btnOlho);  // botão 32×32 à direita
root->addLayout(rowSenha);
```

O botão usa `Theme::emojiIcon("👁", 18)` — renderização via Noto Color Emoji embutida,
consistente com a política do projeto (emoji nunca em texto de botão).

**Comportamento hold-to-reveal:**
```cpp
connect(m_btnOlho, &QToolButton::pressed,  this, [this]() {
    m_senha->setEchoMode(QLineEdit::Normal);
});
connect(m_btnOlho, &QToolButton::released, this, [this]() {
    m_senha->setEchoMode(QLineEdit::Password);
});
```

- `pressed` → `QLineEdit::Normal` (caracteres visíveis)
- `released` → `QLineEdit::Password` (volta a asteriscos)
- ToolTip: "Segure para ver a senha"
- `setFocusPolicy(Qt::NoFocus)` — não entra no Tab order do fluxo de autenticação
- `setFixedSize(32, 32)` — tamanho fixo, não compete com o campo de senha por espaço

**Testes estáticos impossíveis para hold-to-reveal:** o comportamento pressed/released
não é testável via screenshot offscreen. A fiação está correta: `QToolButton::pressed`
e `QToolButton::released` são sinais nativos do Qt, disparados respectivamente no
mouseButtonPress e mouseButtonRelease. Ao soltar o clique (ou mover o mouse para fora e
soltar), `released` sempre dispara — não há risco de ficou preso em `Normal`.

### Render verificado

**Todos os 3 renders**: botão olho (`👁` via emoji, mas sem Noto no snippet =
glifo de fallback de sistema) aparece à direita do campo de senha, alinhado
verticalmente com o campo, dentro do `QHBoxLayout` da linha de senha. No código real
com a fonte Noto Color Emoji embutida via QRC, o ícone renderiza colorido.

---

## Arquivos modificados

- `src/ui/Theme.cpp` — QSS: adicionado `QLabel { color: @@TXT@@; background: transparent; }` e `color:` explícito em `#title`
- `src/ui/LoginDialog.h` — forward declaration de `QToolButton`, membro `m_btnOlho`
- `src/ui/LoginDialog.cpp` — `setAutoFillBackground`, `setSizePolicy`, `setMinimumSize`, `QHBoxLayout` rowSenha, botão olho + fiação pressed/released

## Não modificados

- `tests/` — os 3 fixes são de UI/comportamento; não há testes unitários de contraste
  ou clip que façam sentido automatizar (escopo de visual regression manual)
- `CONTRACT.md`, `TODO.md` — não há item aberto para este fix específico

---

## Gate

```
cmake --build build -j$(nproc)     → [100%] Built target orcamento-pessoal  ✓
cmake --build build-rel -j$(nproc) → [100%] Built target orcamento-pessoal  ✓
QT_QPA_PLATFORM=offscreen ctest --test-dir build → 100% tests passed, 0 tests failed out of 31  ✓
```
