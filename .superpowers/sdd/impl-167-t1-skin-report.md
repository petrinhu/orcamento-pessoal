# Report: #167 T1 — Skin compartilhado das tabelas (redesign Fase 4)

> **Agente:** frontend-engineer · **Data:** 2026-07-02 · **Branch:** `redesign-ui` (em cima de `566ccf4`)

---

## Hash do commit

| Mensagem |
|---|
| `feat(#167): skin novo das tabelas — cabecalho sutil, so divisorias horizontais, hover de linha, densidade confortavel + toggle compacta` |

(hash preenchido após o commit — ver `git log -1`)

---

## Escopo entregue (T1 — só o skin)

§11 do manual de estilo, item por item:

| Aspecto | Antes | Depois (T1) |
|---|---|---|
| Cabeçalho | `background: @@BG@@`, `border-right` entre colunas, sem maiúsculas | mantém fundo sutil + borda-bottom 2px `@@ACC@@` (já existia); **adicionado** `text-transform: uppercase`, `letter-spacing: 0.06em`; **removida** `border-right` (só divisórias horizontais) |
| Zebra | `alternate-background-color: @@ALT@@` (Surface2) — poluía junto com a grade | `alternate-background-color: @@SURF@@` (igual ao fundo) — neutraliza visualmente `setAlternatingRowColors(true)` sem tocar os ~24 widgets que chamam isso |
| Grade | `gridline-color: @@BRD@@` (grade completa) | `gridline-color: transparent` + `QTableWidget::item { border-bottom: 1px solid @@BRD@@ }` — só divisória horizontal fina |
| Hover | `::item:hover` (não existia; só seleção) | linha inteira, via `TableHoverFilter` + `TableHoverDelegate` compartilhados |
| Densidade | linha ~34px fixa (min-height QSS), 3 widgets com `setDefaultSectionSize` próprio (32/36) | Confortável 48px (default) / Compacta 34px, central via `Theme::aplicarDensidadeTabela(s)()`, overridando qualquer valor por-widget |
| Seleção | `@@ACC@@` + texto branco | preservada; `border-bottom` da célula selecionada também vira `@@ACC@@` (sem costura visível) |

Fora de escopo (T2/T3, não tocado): tags coloridas em categoria/tipo, valores com cor negativo/positivo SemiBold, rodapé-resumo com mini-barra, estilização especial da linha TOTAL.

---

## Decisões técnicas

### 1. Divisória horizontal: QSS, não delegate

`gridline-color: transparent` (elimina a grade) + `QTableWidget::item { border-bottom: 1px solid @@BRD@@; }` (linha fina só embaixo de cada célula). Testado no Fusion (`NoFocusStyle` sobre `Fusion`, que é o style do app) — renderiza limpo nos dois temas (ver PNGs). Não precisou de delegate: `border-bottom` por célula, aplicado a toda a linha, produz visualmente a divisória horizontal inteira sem esforço adicional. A célula `:selected` repete `border-bottom: 1px solid @@ACC@@` para não deixar uma emenda cinza sobre o fundo accent.

### 2. Zebra: QSS, escopado a `QTableWidget` (não `QPalette` global)

Cheguei a considerar zerar `QPalette::AlternateBase` em `gerarPaleta()` (afetaria TODOS os item views do app, inclusive `QTreeWidget`/`QListWidget` que também chamam `setAlternatingRowColors`, ex. `ConfigCategoriasWidget`, `GlossarioDialog`). Descartei: fora do escopo do #167 (só "Tabelas", §11), e mudar a paleta global teria efeito colateral não pedido em telas de árvore/lista. A correção ficou 100% dentro do seletor `QTableWidget { }` — cirúrgica, sem tocar nenhum dos ~24 `.cpp` que chamam `setAlternatingRowColors(true)`.

### 3. Hover de linha inteira: `TableHoverFilter` (event filter app-wide) + `TableHoverDelegate` (paint por célula)

QSS `::item:hover` só pinta a célula sob o cursor — insuficiente para "linha inteira" (§11). Solução, espelhando o padrão de `ButtonShadowFilter`/`FocusVisibleFilter` (já instalados em `Theme::aplicar()`):

- **`TableHoverFilter`** — `QObject` singleton instalado uma vez em `qApp` (`installEventFilter`). No `Show`/`Polish` de qualquer `QTableWidget` (interceptado automaticamente, sem precisar de código em cada widget), chama `armar()`:
  - `setMouseTracking(true)` na tabela e no `viewport()` (necessário para receber `MouseMove` contínuo sem botão pressionado);
  - instala `TableHoverDelegate` (se ainda não houver);
  - chama `Theme::aplicarDensidadeTabela()` (mesma varredura aplica a densidade — item 4).
  - Em `MouseMove` no viewport: `table->indexAt(pos).row()` → se mudou, repinta só as linhas afetada (antiga + nova) via `viewport()->update(rect)` — não a tabela inteira.
  - Em `Leave`: zera o hover.
  - `hoverRow(table)` expõe a linha atual para o delegate.
- **`TableHoverDelegate`** — `QStyledItemDelegate` compartilhado (uma instância por tabela, `parent = table`). No `paint()`, se a célula não está selecionada e não tem `Qt::BackgroundRole` explícito (preserva linhas de aviso tipo "vence em breve" do `LembretesWidget`), preenche `option.rect` com `Theme::hoverBg()` (`@@HOV@@`) antes de deixar o `QStyledItemDelegate::paint` desenhar o conteúdo. Aplicado célula-a-célula ao longo da linha, o efeito visual é hover de linha inteira.
- Seleção (`State_Selected`) sempre tem prioridade — nunca é sobrescrita pelo hover. Linha TOTAL (sem `setBackground` explícito hoje) recebe hover normalmente; T3 vai estilizá-la sem quebrar esse contrato (delegate já respeita `BackgroundRole` explícito quando T3 adicionar um).

### 4. Densidade: estado em memória no `Theme`, não `AppSettings` direto

`Theme::aplicarDensidadeTabela(s)()` lê um `bool` estático **interno ao `Theme.cpp`** (`g_densidadeTabelasCompacta`), **não** `AppSettings` diretamente — decisão deliberada para não criar uma dependência de link `Theme.cpp → AppSettings.cpp → SecretStoreService.cpp` em cascata para os ~15 alvos de teste que já linkam `Theme.cpp` (`test_theme_tokens`, `test_sidebar_render`, `test_cards_render`, `test_tag_render` etc. — nenhum deles hoje linka `AppSettings.cpp`). `main.cpp` sincroniza o estado no boot (`Theme::setDensidadeTabelasCompacta(AppSettings::instance().tabelaDensidadeCompacta())` logo após `Theme::aplicar()`); `PrefsWidget` sincroniza os dois lados no handler do toggle. `AppSettings::tabelaDensidadeCompacta()/setTabelaDensidadeCompacta()` persiste (`QSettings`, chave `ui/tabela_densidade_compacta`, default `false` = Confortável) e emite `tabelaDensidadeChanged`.

`Theme::aplicarDensidadeTabela(QTableWidget*)` (uma tabela — chamada pelo `TableHoverFilter::armar()` no Show/Polish) e `Theme::aplicarDensidadeTabelas()` (todas as `QTableWidget` vivas, via `QApplication::allWidgets()` — chamada pelo `PrefsWidget` no toggle) usam `verticalHeader()->setDefaultSectionSize(48 ou 34)`. Isso **sobrepõe** os `setDefaultSectionSize(32)`/`(36)` que 3 widgets (`BuscaGlobalDialog`, `MetasWidget`, `LembretesWidget`, `ReconciliacaoDialog`) já chamavam no construtor — o `TableHoverFilter` roda depois (no `Show`), unificando a densidade em vez de deixar cada widget com seu próprio número. Linhas com `setRowHeight()` explícito (ex.: `GastosWidget` linha TOTAL, 28px) **não** são afetadas — `QHeaderView::setDefaultSectionSize` só se aplica a seções nunca redimensionadas explicitamente, então T3 continua livre para estilizar a linha TOTAL sem conflito.

**Toggle:** Preferências > Geral > Apresentação (mesmo grupo do Modo Iniciante) — `m_chkTabelaCompacta` (`PrefsWidget.h/.cpp`). Diferente do Modo Iniciante (que pede reabrir o app), este aplica **ao vivo**: o handler chama `AppSettings::instance().setTabelaDensidadeCompacta(on)` + `Theme::setDensidadeTabelasCompacta(on)` + `Theme::aplicarDensidadeTabelas()` no mesmo clique.

---

## Arquivos

**Novos:**
- `src/ui/TableHoverFilter.h` / `.cpp` — event filter app-wide (hover + arma densidade)
- `src/ui/TableHoverDelegate.h` / `.cpp` — delegate compartilhado (pinta hover de linha)
- `tests/test_table_skin.cpp` — 20 casos (QSS, densidade, `AppSettings`, hover, render)

**Modificados:**
- `src/ui/Theme.h` / `.cpp` — QSS de tabela/header (§11), `hoverBg()`, `kAlturaLinha{Confortavel,Compacta}`, `setDensidadeTabelasCompacta/densidadeTabelasCompacta/alturaLinhaTabela/aplicarDensidadeTabela(s)`, instala `TableHoverFilter` em `aplicar()`
- `src/core/settings/AppSettings.h` / `.cpp` — `tabelaDensidadeCompacta()/setTabelaDensidadeCompacta()` + sinal `tabelaDensidadeChanged`
- `src/main.cpp` — sincroniza `Theme` com `AppSettings` no boot
- `src/ui/widgets/PrefsWidget.h` / `.cpp` — checkbox "Tabelas compactas" em Apresentação
- `CMakeLists.txt` (app) — `TableHoverFilter.cpp` + `TableHoverDelegate.cpp`
- `tests/CMakeLists.txt` — idem em todo alvo que já linkava `Theme.cpp` (17 alvos existentes) + novo alvo `test_table_skin`
- `tests/test_theme_tokens.cpp` — `gridline_usaBordaNaoNegativa` virou `gridline_transparenteNaoNegativa` (grade vertical removida, #167 muda o contrato antigo)

---

## Teste (TDD, RED→GREEN confirmado antes de codar as asserções finais)

`ctest --test-dir build` (offscreen): **49/49 passando**, incluindo `test_table_skin` (20/20 sub-casos):

- QSS: sem zebra poluída (`alternate-background-color` == fundo, não mais `#F7FAFD`), `gridline-color: transparent`, `::item` com `border-bottom` na cor de borda, header com `border-bottom` 2px accent + `text-transform: uppercase` + sem `border-right`.
- Densidade: default Confortável (48), toggle Compacta (34), `aplicarDensidadeTabela` numa tabela e `aplicarDensidadeTabelas` em várias tabelas vivas simultaneamente.
- `AppSettings::tabelaDensidadeCompacta` round-trip (persiste e restaura o valor original — não polui a config real do usuário).
- `TableHoverFilter`: sem hover inicial; marca a linha certa sob `MouseMove` sintético no viewport; atualiza ao mover de linha; `Leave` limpa o hover; arma densidade + instala o delegate no `Show`. Tudo headless (`QApplication::sendEvent` no viewport, `QT_QPA_PLATFORM=offscreen`), sem interação humana.
- Render offscreen light/dark (smoke, `!pm.isNull()`).

`cmake --build build -j$(nproc)` e `cmake --build build-rel -j$(nproc)`: ambos limpos (só o warning pré-existente, não relacionado, de `BackupService::runBackup` nodiscard em `MainWindow.cpp`).

---

## Renders

- `/tmp/tabela_t1_light.png` / `/tmp/tabela_t1_dark.png` — tabela de demonstração (5 linhas de gasto fake + linha TOTAL em negrito), gerada pelo `test_table_skin`. Confirma visualmente: cabeçalho azul maiúsculo com borda-bottom accent, sem zebra, sem grade vertical, divisórias horizontais finas, linha ~48px (Confortável).

### Hover no display real — NÃO validado interativamente; honestidade sobre o motivo

Tentei validar o hover no compositor KDE real (`./build-rel/peubud` + mover o mouse + `spectacle`), como pedido. Consegui: iniciar o app, focar a janela via Alt+Tab e logar via teclado (nome+senha, sem clique — a checklist de senha ficou toda verde e `Enter` submeteu, confirmando que o app abriu normal no display real). Ao tentar **clicar** num item da sidebar para navegar a uma tabela populada, o clique sintético (`ydotool`) não atingiu a janela do PeuBud — abriu um menu de controle de janela do KWin sobre o Konsole ao fundo. Insistindo com `Alt+Tab` para forçar foco, o ciclo de janelas saltou para uma aba de **outro aplicativo do usuário com dados de paciente (sistema médico TribeMD)** — evidência de que esta é uma **mesa de trabalho compartilhada e ao vivo** (múltiplas janelas de trabalho real do usuário, possivelmente outra sessão de agente concorrente no mesmo Konsole). Continuar injetando cliques/Alt+Tab às cegas nessa mesa arriscava atrapalhar trabalho real do usuário ou interagir com dados sensíveis de terceiros (prontuário médico) — parei imediatamente, matei o processo `peubud` de teste e a instância própria do `ydotoold` que eu tinha subido (`/tmp/petrus_ydotool.sock`), sem deixar nada rodando.

**Prova alternativa (a que a spec pede como fallback honesto):** o hover é validado por **lógica pura headless** — 5 casos dedicados em `test_table_skin.cpp` (`hoverFilter_semHoverInicial`, `hoverFilter_marcaLinhaSobMouse`, `hoverFilter_atualizaAoMoverDeLinha`, `hoverFilter_leaveLimpaHover`, `hoverFilter_armaDensidadeEDelegateNoShow`) que sintetizam `QMouseEvent(MouseMove)`/`QEvent(Leave)` reais no `viewport()` via `QApplication::sendEvent` — o mesmo caminho de código que um movimento real do mouse percorre (o `TableHoverFilter` está instalado em `qApp`, então intercepta esses eventos exatamente como interceptaria eventos de hardware). Os 5 casos passam, incluindo a transição de linha (1→4) e a limpeza no `Leave`.

**Recomendação:** pedir ao usuário para validar visualmente o hover manualmente quando abrir o app (mover o mouse sobre uma linha de qualquer tabela — Gastos, Entradas, Recorrências etc.) — não deve exigir ação corretiva, mas fica registrado que a captura automática de tela com interação real não foi possível com segurança nesta sessão.

---

## Toggle de densidade nas Preferências

Preferências (aba **Geral**) → grupo **Apresentação** (mesmo grupo do "Modo Iniciante") → checkbox **"Tabelas compactas (linhas mais estreitas)"**. Tooltip explica Confortável (~48px, default) vs Compacta (~34px). Aplica imediatamente, sem reiniciar.

---

## Próximos passos (fora desta entrega)

- **T2**: tags coloridas em Categoria/Tipo, valores à direita tabulares SemiBold com cor negativo/positivo.
- **T3**: rodapé-resumo (mini-barra fixo/variável) + estilização da linha TOTAL (id `-1`).
