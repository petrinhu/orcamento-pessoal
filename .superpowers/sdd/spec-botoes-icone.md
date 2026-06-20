# Spec — Botões SÓ-ÍCONE (variante sem caixa)

**Autor:** Art Director · **Para:** frontend-engineer · **Projeto:** PeuBud (Qt6, redesign "amigável + azuis confiáveis") · **Issue relacionada:** #165 (redesign Fase 1)

> Status: pronta para implementar. A **única** decisão que pede confirmação do
> líder (petrus) antes de canonizar é o **glifo da seta** — ele já vetou `‹ ›` e
> `◀️▶️`; a recomendação (◀ ▶ sólido sem VS16) está na seção 3 com o plano B.

---

## 0. Diagnóstico (por que está feio)

No print (dark, header escuro `@@BG@@ #0F172A`), os botões `‹ › 🔒 🖨️` aparecem
como **caixas com surface `#1B2430` + borda `#2A3340` flutuando** sobre o fundo.
Causa raiz **dupla**:

1. **Herança do QSS base.** Todo `QPushButton` pega
   `background:@@SURF@@; border:1px solid @@BRD@@; border-radius:9px` (Theme.cpp
   ~224-230). Num ícone de 32px isso vira chrome puro: o olho lê a **borda da
   caixa antes do ícone**. Quebra o teste dos 3 segundos — o chrome compete com o
   conteúdo do dashboard.
2. **`setFixedWidth(32)` + `padding:5px 14px`.** A largura travada briga com o
   padding horizontal da regra base; o ícone fica espremido e a caixa "sobra" ao
   redor. É a "área de delimitação" que o usuário descreveu.

**Princípio da correção:** botão só-ícone é **affordance silenciosa** — no estado
normal NÃO desenha contorno nem fundo (some no header); o realce (fundo suave +
tint azul no glifo) aparece **só no hover/foco**. É exatamente o
`.monthnav button` do mockup aprovado (`border:none; background:transparent;
:hover{ background:var(--surface-2); color:var(--primary) }`).

**Não** recriar o container-pílula `.monthnav` do mockup: aqui os botões vivem
soltos sobre o `@@BG@@` do header. Transparentes, eles expõem o próprio fundo —
que é o efeito desejado (invisíveis em repouso).

---

## 1. Estilo do botão-ícone (QSS canônico)

Novo `objectName` = **`iconButton`**. Regra a adicionar em `Theme.cpp::gerarQss()`
**depois** do bloco `QPushButton` base (~linha 238), para sobrepor a herança.
Tokens já existentes, nada de hex hard-coded:

```css
/* ── Botão só-ícone (nav de mês, fechar/imprimir, toggles) ──────────────
   Sem chrome em repouso: sem borda, sem fundo. Realce só no hover/press.
   Some sobre qualquer fundo (header @@BG@@, surface, sidebar). #165. */
QPushButton#iconButton {
    background: transparent;
    border: none;                /* zero contorno em repouso */
    border-radius: 8px;          /* cantos do realce de hover */
    padding: 0;                  /* nada de padding lateral — o glifo centraliza */
    min-width: 32px;
    min-height: 32px;
    color: @@SUB@@;              /* glifo discreto em repouso */
    font-size: 17px;            /* tamanho do chevron tipográfico (ver §3) */
}
QPushButton#iconButton:hover {
    background: @@HOV@@;         /* realce suave: aparece sobre @@BG@@ em light e dark */
    color: @@ACC@@;             /* glifo azul no hover — convida ao clique */
}
QPushButton#iconButton:pressed {
    background: @@ALT@@;         /* afunda um tom no clique (mais escuro que hover) */
    color: @@ACC@@;
}
QPushButton#iconButton:disabled {
    color: @@BRD@@;             /* glifo apagado, sem fundo */
    background: transparent;
}
QPushButton#iconButton:focus {
    /* teclado: realce idêntico ao hover + anel fino (a11y, sem caixa pesada) */
    background: @@HOV@@;
    border: 1px solid @@ACC@@;
}
```

### Por que estes tokens

- **`border:none`** em repouso = mata a "área ao redor" reclamada. Não usar
  "borda da cor do fundo" (o `@@BG@@` do header difere do fundo de um card; uma
  borda fixa do header vazaria se o botão for reusado sobre surface). Transparente
  é universal.
- **`@@HOV@@` como fundo de hover** é o mesmo token que `.monthnav button:hover`
  usa em espírito (surface-2). Contraste verificado:
  - Light: hover `#DCE8FA` sobre header `#EEF3F9` → visível (azulado claro).
  - Dark: hover `#222D3D` sobre header `#0F172A` → visível (sobe ~2 tons). E sobre
    surface `#1B2430` (se reusado num card) também distingue.
- **`color` `@@SUB@@`→`@@ACC@@`**: glifo cinza-discreto em repouso, azul-confiável
  no hover. Casa com a paleta do redesign. Vale para o chevron tipográfico (a cor
  do QSS pinta o glifo). **Para os ícones emoji 🔒🖨️ a cor do QSS não afeta o
  pixmap colorido** — eles permanecem coloridos; o que muda é o **fundo** do botão
  (transparente → @@HOV@@), que é o necessário pra remover a caixa. OK.
- **`border-radius:8px`** só molda o retângulo de hover (cantos suaves, casa com o
  raio 7px do mockup e o 9px dos botões de texto).

### Tamanho

`min-width/min-height: 32px` substitui o `setFixedWidth(32)` no C++ — **remover os
`setFixedWidth(32)`** dos botões-ícone (o QSS passa a controlar). Se algum precisar
travar altura, usar `setFixedSize(32,32)` (quadrado), nunca só largura.

---

## 2. Light vs Dark — cores exatas por estado

| Estado | Propriedade | Light | Dark |
|---|---|---|---|
| **Normal** | background | transparent | transparent |
| | border | none | none |
| | glifo (color) | `#67768D` @@SUB@@ | `#8A97A8` @@SUB@@ |
| **Hover** | background | `#DCE8FA` @@HOV@@ | `#222D3D` @@HOV@@ |
| | glifo (color) | `#3B82F6` @@ACC@@ | `#3B82F6` @@ACC@@ |
| **Pressed** | background | `#F7FAFD` @@ALT@@ | `#171F2D` @@ALT@@ |
| | glifo (color) | `#3B82F6` @@ACC@@ | `#3B82F6` @@ACC@@ |
| **Focus** (teclado) | background | `#DCE8FA` @@HOV@@ | `#222D3D` @@HOV@@ |
| | border | `1px #3B82F6` @@ACC@@ | `1px #3B82F6` @@ACC@@ |
| **Disabled** | background | transparent | transparent |
| | glifo (color) | `#E4EAF2` @@BRD@@ | `#2A3340` @@BRD@@ |

Emoji (🔒🖨️🔼🔽➡️) ignoram `color`; herdam só o **background** acima.

---

## 3. A seta prev/next — DECISÃO (pede martelo do líder)

Critérios: legível a 32px · monocromática no tom do tema (não-laranja) ·
encorpada (não-fina) · casa com 🔒🖨️ · em light **e** dark.

| Candidato | Glifo | Veredito |
|---|---|---|
| Guillemet (atual) | `‹ ›` U+2039/203A | **VETADO** pelo líder — stroke fino, parece `< >`. |
| Seta fina | `← →` U+2190/2192 | Traço único; em Inter vira filete fino a 32px — risco de repetir a queixa do `‹›`. Semântica "voltar/avançar" > "mês". Reprovado. |
| Media-emoji | `◀️ ▶️` | **VETADO** — o VS16 (`️`) força o Noto a render a **caixa laranja** de media-button. |
| **Triângulo sólido SEM VS16** | **`◀ ▶`** U+25C0/25B6 | **RECOMENDADO.** Sem o seletor de emoji, Noto/Inter renderiza como **glifo de texto monocromático preenchido** (não a caixa laranja). Encorpado, lê nítido a 32px, **herda a cor do QSS** (@@SUB@@→@@ACC@@). É o "chevron encorpado, bem dimensionado" pedido, sem ser fino nem laranja. |

### Recomendação: `◀` / `▶` (U+25C0 / U+25B6) **sem** `️`

No C++ (DashboardWidget.cpp ~158-159), trocar os guillemets e **garantir que NÃO
há VS16**:

```cpp
// U+25C0 / U+25B6 — triângulo sólido. SEM ️: render monocromático
// (texto), pinta com a cor do QSS (@@SUB@@→@@ACC@@). NÃO usar ◀️▶️ (caixa
// laranja de media-button). #165.
auto *btnPrev = new QPushButton(QString(QChar(0x25C0)), this);  // ◀
auto *btnNext = new QPushButton(QString(QChar(0x25B6)), this);  // ▶
btnPrev->setObjectName(QStringLiteral("iconButton"));
btnNext->setObjectName(QStringLiteral("iconButton"));
```

- **Tamanho:** `font-size:17px` (já no QSS do `#iconButton`). O triângulo a 17px
  ocupa bem o quadrado de 32px sem encostar nas bordas. Remover o
  `f.setPointSize(15)` manual — o QSS manda.
- **Cor:** @@SUB@@ repouso / @@ACC@@ hover (do QSS). Combina tonalmente com 🔒🖨️
  (que são coloridos mas discretos) — todos viram "controles cinza-azulados que
  acendem azul no hover".
- **Verificação obrigatória na implementação:** rodar o app em dark E light e
  confirmar visualmente que `◀ ▶` saem **monocromáticos** (não laranja). Em raras
  combinações de fontconfig o Noto Color Emoji pode capturar U+25C0 mesmo sem
  VS16. Se isso ocorrer → **plano B** abaixo.

### Plano B (só se o triângulo render laranja na máquina alvo)

`QStyle::SP_ArrowLeft` / `SP_ArrowRight` via
`style()->standardIcon(QStyle::SP_ArrowBack/SP_ArrowForward)` como `setIcon`,
forçando monocromático no tom do tema. Custo: o desenho varia por estilo Qt e
perde-se o controle fino de cor (não acende @@ACC@@ no hover tão limpo). Por isso
é plano B, não a recomendação.

---

## 4. Consistência do texto de ajuda (emoji inline)

O `TabHelp.cpp` referencia as setas como emoji laranja inline em vários pontos:

| Arquivo:linha | Texto atual | Ação |
|---|---|---|
| `TabHelp.cpp:22` | `Use as setas ◀️ ▶️ no topo pra trocar o mês.` | trocar |
| `TabHelp.cpp:98` | `as setas <b>◀️ ▶️</b>.` | trocar |
| `TabHelp.cpp:183` | `Setas ◀️ ▶️ escolhem o 📅 mês.` | trocar |

**Regra:** texto corrido **não** deve embutir o glifo de seta (nem o triângulo
sólido — fora de um botão e sem controle de tamanho ele fica órfão e
inconsistente). **Descrever em palavras**, ancorando na posição:

- `TabHelp.cpp:22` → `"Use as setas no topo da tela pra trocar o mês."`
- `TabHelp.cpp:98` → `"as setas no topo."` (remover as tags `<b>◀️ ▶️</b>`)
- `TabHelp.cpp:183` → `"As setas no topo escolhem o 📅 mês."`

Manter `Ctrl+← / Ctrl+→` onde já aparece (atalho de teclado é legítimo citar o
caractere de seta — é tecla, não decoração). Não tocar nas linhas 55/77/239.

---

## 5. Inventário — TODOS os botões-ícone que adotam `#iconButton`

Varredura feita. Adotam o variante (remover `setFixedWidth(32)` quando houver,
setar `objectName("iconButton")`):

| # | Arquivo | Botão | Hoje | Ação |
|---|---|---|---|---|
| 1 | `DashboardWidget.cpp:158` | `btnPrev` (mês anterior) | `QPushButton "‹"`, fixedWidth 32 | `#iconButton` + glifo ◀ (§3); remover fixedWidth e setPointSize |
| 2 | `DashboardWidget.cpp:159` | `btnNext` (próximo mês) | `QPushButton "›"`, fixedWidth 32 | `#iconButton` + glifo ▶ (§3); idem |
| 3 | `DashboardWidget.cpp:185` | `btnFechar` (🔒 fechar mês) | `QPushButton` icon 🔒, fixedWidth 32 | `#iconButton`; remover fixedWidth |
| 4 | `DashboardWidget.cpp:179` | `btnPrint` (🖨️ imprimir) | `QPushButton` icon 🖨️, fixedWidth 32 | `#iconButton`; remover fixedWidth |
| 5 | `TabShell.cpp:77-112` | toggle colapso (🔼/🔽) | `QToolButton` icon emoji | ver nota A |
| 6 | `DashboardWidget.cpp:118` | seta `➡️` dos cards | `QLabel "➡️"` (não é botão) | ver nota B |
| 7 | `LoginDialog.cpp:133` | `m_btnOlho` (mostrar senha) | `setFixedSize(32,32)` | `#iconButton` (mantém fixedSize 32×32, é quadrado) |
| 8 | `ReceiptAttachmentWidget.cpp:49` | `m_btnRemove` | `setFixedWidth(32)` | `#iconButton` se for icon-only; confirmar que não tem texto |
| 9 | `PrefsWidget.cpp:181` | `btnBrowseSaves` (📁 browse) | `setFixedWidth(32)` | `#iconButton` se icon-only |
| 10 | `PrefsWidget.cpp:195` | `btnBrowseAutosaves` (📁) | `setFixedWidth(32)` | `#iconButton` se icon-only |

### Nota A — toggle de colapso do TabShell (item 5)

É `QToolButton` (não QPushButton), hoje sem objectName explícito → cai no estilo
default do QToolButton (provavelmente já sem caixa, mas inconsistente). **Padronizar:**
criar regra-gêmea `QToolButton#iconButton` com **as mesmas** declarações do §1
(copiar o bloco trocando o seletor). Aplicar `setObjectName("iconButton")` no
toggle. Assim o 🔼/🔽 ganha o mesmo hover suave dos demais. Os ícones 🔼🔽
continuam coloridos (emoji); muda só o fundo. **Não** trocar 🔼/🔽 por glifo — ali
o emoji comunica "recolher/expandir" e está num bloco de texto, ok manter.

### Nota B — seta `➡️` dos cards (item 6)

Hoje é **QLabel**, não botão (o card inteiro é clicável via eventFilter). Dois
problemas: (1) `➡️` com VS16 é emoji colorido — fora de padrão com o resto; (2)
sendo label, não tem hover.

**Recomendação:** trocar o `QLabel "➡️"` por um glifo monocromático que herde
`@@SUB@@` (cor de subtitle, que o label já usa via `objectName("subtitle")`):
usar `▸` (U+25B8, triângulo pequeno sólido) **sem** VS16, ou `→` (U+2192). Como é
só dica visual de "clicável → pula pra aba", `▸` casa com o `◀ ▶` da nav (mesma
família de triângulo sólido). Manter como QLabel (não vira botão). Pintar com
@@SUB@@ no repouso; **opcional**: no `:hover` do card, subir pra @@ACC@@ (precisa
de QSS no `QFrame#surface:hover QLabel` ou property — fora do escopo mínimo;
aceitável deixar @@SUB@@ fixo nesta fase).

> Itens 8/9/10: o frontend deve confirmar que são **icon-only** (sem texto) antes
> de aplicar — se algum tiver rótulo textual curto, NÃO é `#iconButton` (mantém a
> caixa do botão de texto). Pelo nome (`btnBrowse…`, `btnRemove`) são ícone-só,
> mas confirmar no código.

---

## 6. Fronteira: o que NÃO muda

- **Botões de texto** (`#primary` azul cheio; `QPushButton` base com label) —
  **mantêm a caixa**. O variante `#iconButton` é exclusivo de botão sem rótulo.
  A caixa comunica "clique aqui" quando há texto; é affordance correta lá.
- **`QToolButton#themeBtn`** (ciclo de tema) já é transparente/sem-borda — **não
  precisa** virar `#iconButton`, mas pode adotar pra unificar (opcional, sem
  regressão).
- Tokens, raios de card (14px), sombras (`ButtonShadowFilter`) — intactos.

---

## 7. Checklist de aceite (art review)

- [ ] No header escuro (dark), `◀ ▶ 🔒 🖨️` **sem nenhuma caixa/borda** em repouso.
- [ ] Hover: fundo suave aparece + glifo `◀ ▶` fica **azul** (@@ACC@@).
- [ ] `◀ ▶` renderizam **monocromáticos** (NÃO laranja) em light **e** dark.
- [ ] Mesmo teste em **light**: botões somem no header claro, acendem no hover.
- [ ] Teste dos 3 segundos: olho vai pro **valor dos cards / mês**, não pro chrome
      dos botões.
- [ ] Foco por teclado (Tab) mostra anel azul fino — a11y preservada.
- [ ] Nenhum texto de ajuda contém `◀️ ▶️` inline (§4).
- [ ] `setFixedWidth(32)` removido dos botões-ícone (QSS controla o tamanho).

---

## Refs canônicas

- **Mockup alvo:** `docs/design/mockups/peubud_redesign_dashboard.html`,
  seletor `.monthnav button` — origem do princípio "sem borda, fundo só no hover".
  *Por que importa:* já validado pelo líder; é o contrato visual desta fase.
- **Paleta #164/#165** (Theme.cpp): tokens @@SUB@@/@@ACC@@/@@HOV@@/@@ALT@@ — o
  variante reusa o que já existe, zero cor nova.
