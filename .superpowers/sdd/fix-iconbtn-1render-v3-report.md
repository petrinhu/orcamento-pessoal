# Fix #165 (v3) — caixa do ◀️ no 1º render: diagnóstico no display REAL + correção determinística

**Status:** RESOLVIDO e verificado no compositor real (KDE/Wayland). ctest 40/40 PASS.
**Branch:** `redesign-ui`
**Commit:** ver hash no final (preenchido após commit).

## Por que 2 fixes anteriores falharam

| Tentativa | Abordagem | Resultado |
|---|---|---|
| `5858671` | `showEvent` repolish IMEDIATO dos #iconButton | falhou (veio com caixa) |
| `2ac6ccd` | `showEvent` repolish DEFERIDO (`singleShot(0)`) | falhou (veio com caixa) |

Ambos **passaram no teste offscreen** porque o offscreen **não reproduz o bug**: o
teste forçava `QT_QPA_PLATFORM=offscreen` e capturava com `QWidget::grab()` — dois
bypasses simultâneos (sem compositor atribuindo foco à janela + grab renderiza o
widget isolado, sem o estado de foco da top-level). Teste verde sem valor.

## O que a caixa REALMENTE era (diagnóstico no display real)

Montei um harness fiel à estrutura do `MainWindow`
(`QMainWindow → setCentralWidget → QStackedWidget → página com a navBar dos 4
#iconButton`), com `Theme::aplicar()` ativo (instala ButtonShadowFilter), rodando
no **Wayland real** (sem offscreen), e capturei com **screenshot externo do
compositor** (`spectacle -b -n -a`). O bug **reproduziu** (`/tmp/iconbtn_v3_antes.png`):
◀️ com **borda azul accent (#3B82F6) + fundo azul-claro**, ▶️🔒🖨️ limpos.

Eliminei os candidatos por evidência (logs de `size`/`hasFocus`/`FocusReason`):

- **NÃO é a borda cinza do estilo-base** (#E4EAF2) — o que o teste offscreen procurava. A borda é AZUL.
- **NÃO é tamanho/layout errado** — o botão é `32x32` em todos os instantes (1º render, pós-repolish, pós-resize). `min-width:32px` sempre aplicou.
- **NÃO é a sombra do ButtonShadowFilter** — o filtro pula `#iconButton` (linha 67-68).
- **É o estado `:focus`** do `#iconButton`:
  ```css
  QPushButton#iconButton:focus { background: @@HOV@@; border: 1px solid @@ACC@@; }
  ```

### Cadeia causal provada por log

| Momento | `hasFocus` | `FocusReason` |
|---|---|---|
| showEvent imediato | 0 | — |
| pós-`singleShot(0)` repolish (fix vigente) | 0 | — |
| **assentado, 1º render, SEM resize** | **1** | **3 = `Qt::ActiveWindowFocusReason`** |

O `btnPrev`, por ser o **primeiro widget focável** da janela, recebe **foco
automático** quando a janela fica ativa (`ActiveWindowFocusReason` — NÃO é
interação do usuário). A regra `:focus` então pinta a caixa.

**Por que só o ◀️:** só ele é o primeiro na tab order → só ele recebe o foco-inicial.
**Por que repolish (ambos os fixes) falhou:** repolish reaplica o QSS, mas o widget
**continua sendo o focusWidget** → `:focus` continua casando. Repolish nunca remove
foco. Além disso o foco chega **depois** do showEvent (no showEvent `hasFocus=0`),
então o `singleShot(0)` também não via foco — mexia no lugar errado.
**Por que "resize/troca de aba conserta":** trocar de aba tira o foco do botão; é o
foco que sumindo elimina a caixa, não o repolish.

## A correção (Opção A — filtro global `FocusVisibleFilter`)

Emula o `:focus-visible` da web: o anel de foco do `#iconButton` só aparece quando
o foco vem do **teclado** (Tab/Backtab/Shortcut), nunca do foco-automático que o Qt
dá ao 1º focável quando a janela ativa.

- **`src/ui/FocusVisibleFilter.{h,cpp}`** (novo): event filter global em `qApp`,
  espelhando o padrão do `ButtonShadowFilter` (`install()` idempotente). No `FocusIn`
  de um widget com `objectName=="iconButton"`, se o `reason()` não é de teclado,
  chama `clearFocus()`. (Seguro: `clearFocus()` no `FocusIn` vira `FocusOut`, sem loop.)
- **`Theme::aplicar()`**: chama `FocusVisibleFilter::install()` ao lado de
  `ButtonShadowFilter::install()`.
- **CMakeLists.txt**: registra o novo .cpp no target do app e em todos os recipes de
  teste que linkam `Theme.cpp` (Theme agora depende do filtro).
- **Cobre toda a classe do bug**, não só o Dashboard: qualquer `#iconButton` que seja
  o primeiro focável de qualquer aba/janela (toggle do TabShell, futuras telas).

### Código morto removido (limpeza das 2 tentativas falhas)

`DashboardWidget`:
- `showEvent()` override (repolish deferido) — removido
- membros `m_firstShow`, `m_navIconBtns` — removidos
- linha de registro `m_navIconBtns = {...}` em `buildUi()` — removida
- includes órfãos `<QTimer>`, `<QShowEvent>` (.cpp) e `<QList>` + fwd `class QPushButton;` (.h) — removidos

## Acessibilidade preservada (validado)

Simulei **Tab por teclado** no harness real: o foco chega com `reason=1 (Tab)` → o
filtro **mantém** o foco → o anel aparece corretamente (`btnPrevHasFocus=1`). A
navegação por teclado NÃO quebra — princípio não-negociável respeitado.

## Verificação

### Screenshot REAL do compositor (a verificação que vale)
- **Antes** (com caixa): `/tmp/iconbtn_v3_antes.png` — ◀️ com borda azul + fundo (:focus).
- **Depois** (com o `FocusVisibleFilter` REAL de produção, harness `harness_final.cpp`):
  `/tmp/iconbtn_v3_depois.png` — ◀️ **limpo**, sem caixa, idêntico ao ▶️🔒🖨️, no
  **1º render, sem resize**. Log: `[ASSENTADO-1render-SEM-resize] size=32x32 hasFocus=0`.

### Build + testes
- `cmake --build build -j16` (Debug, QT_DEBUG): **OK**
- `cmake --build build-rel -j16 --target orcamento-pessoal` (Release): **OK**
  (build-rel tem `BUILD_TESTING=OFF` por design — testes vivem no `build`)
- `ctest` (build, offscreen): **40/40 PASS** (gate verde, sem `--no-verify`)

### Teste atualizado (sem teste mentiroso offscreen)
`tests/test_iconbtn_render.cpp` reescrito: o cabeçalho documenta a lição (offscreen +
grab NÃO pegam este bug). Os grabs offscreen ficam só como **smoke** de render. O
teste que VALE é o **contrato do FocusVisibleFilter** (event-logic pura, roda
headless via `QFocusEvent` com reason específico):
- `focoAutomaticoLimpaAnelNoIconButton` — `ActiveWindow` → `clearFocus()` (sem anel)
- `focoDeTecladoMantemAnelNoIconButton` — Tab/Backtab/Shortcut → foco mantido (a11y)
- `focoAutomaticoNaoMexeEmWidgetComum` — widget não-iconButton intacto (cirúrgico)

Esses testes têm dente: sem o filtro, `setFocus(ActiveWindowFocusReason)` mantém o
foco → a 1ª assertiva falha. Não é fake-green.

## Arquivos tocados
- `src/ui/FocusVisibleFilter.h` (novo)
- `src/ui/FocusVisibleFilter.cpp` (novo)
- `src/ui/Theme.cpp` (install + include)
- `src/ui/widgets/DashboardWidget.cpp` (remove código morto)
- `src/ui/widgets/DashboardWidget.h` (remove código morto)
- `tests/test_iconbtn_render.cpp` (reescrito: contrato de foco + smoke)
- `tests/CMakeLists.txt` (linka FocusVisibleFilter.cpp nos recipes de Theme)
- `CMakeLists.txt` (registra o novo source no app)
