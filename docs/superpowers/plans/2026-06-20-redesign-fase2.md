# Redesign Fase 2: Shell + Sidebar agrupada (#165, #170)

> Executar via subagent-driven-development, tarefa a tarefa, na branch `redesign-ui`.

**Goal:** Trocar o `QTabWidget` no topo por uma **sidebar agrupada a esquerda** (navy, grupos Dia a dia/Planejar/Investir) + `QStackedWidget`, com navegacao por chave estavel; aposentar a `QMenuBar` (rodape da sidebar + cluster de utilidades + menu "... Mais").

**Decisao do usuario:** ordem da sidebar **FIXA pelo agrupamento** (sem arrastar/persistir/reset). Logo: REMOVER `ordemAbasDefault`/`aplicarOrdemAbasSalva`/`salvarOrdemAbas`/`resetarOrdemAbas`, o `connect(tabMoved...)`, e os nav-buttons ⏮◀▶⏭🔄.

**Tech Stack:** C++20, Qt6 Widgets/QSS, ctest (offscreen). Branch `redesign-ui`.

## Global Constraints
- Commits pt-br, Conventional Commits, escopo `(#165)`/`(#170)`, rodape Co-Authored-By + Claude-Session.
- ctest verde a cada passo; pre-commit hook e o gate. Apenas FOSS.
- Emoji em botoes/itens via `Theme::emojiIcon` (NUNCA emoji no texto de QPushButton/QToolButton). Tokens via `Theme` (Fase 1).
- NAO alterar a logica de dominio dos 13 widgets de conteudo (so a navegacao/shell).
- Verificacao visual: build release + screenshot + olho do usuario a cada checkpoint.

## Mapa atual (MainWindow.cpp) -- alvos
- `buildUi()` (~209-292): cria `m_tabs` (235), `addTab` dos 13 shells (274-286), `aplicarOrdemAbasSalva` (289), `initialTab` (291), `setCentralWidget(m_tabs)` (292).
- Persistencia de ordem: `ordemAbasDefault` (130), `aplicarOrdemAbasSalva` (150), `resetarOrdemAbas` (173), `salvarOrdemAbas` (197) + `connect(tabMoved)` (311) -> REMOVER.
- Nav buttons + reset em `buildThemeBar` (738-753) -> REMOVER (sidebar navega).
- Navegacao por INDICE a re-cablear: Dashboard `navegarParaAba -> setCurrentIndex` (420), BuscaGlobal idem (1242), Lembretes `setCurrentWidget(m_shellLembretes)` (550, ok), Cartoes `indexOf(m_shellCartoes)+setCurrentIndex` (1274), `currentChanged` lazy (464), Ctrl+1..9 (setupShortcuts).
- `buildMenuBar` (587) + toolbar (762-824) -> rodape sidebar + cluster + "... Mais".

---

## Task 1: Widget SidebarNav

**Files:** Create `src/ui/widgets/SidebarNav.{h,cpp}`; Test `tests/test_sidebar_nav.cpp` (+ CMake).

**Interfaces:**
- Produces:
  - `class SidebarNav : public QWidget` com `Q_OBJECT`.
  - `void addGrupo(const QString &titulo)` (rotulo de grupo).
  - `void addItem(const QString &chave, const QString &emoji, const QString &titulo)` (item navegavel; emoji via `Theme::emojiIcon`).
  - `void addFooterItem(const QString &chave, const QString &emoji, const QString &titulo)` (rodape: Preferencias/Ajuda/Tema).
  - `void setUsuario(const QString &nome)` (avatar com inicial + nome no rodape).
  - `void setAtivo(const QString &chave)` (destaca o item, "pill" azul).
  - `signals: void navegou(const QString &chave)`.
- Cada item e um `QToolButton`/`QPushButton` flat com objectName p/ QSS; clique emite `navegou(chave)` e marca ativo. Layout: QVBoxLayout (itens) + addStretch + rodape.

- [ ] **Step 1: teste (falha)** `tests/test_sidebar_nav.cpp`: cria SidebarNav, `addItem("tab_x","🏠","X")`, conecta `navegou`, simula clique do item e QCOMPARE a chave emitida == "tab_x"; e `setAtivo("tab_x")` marca so esse ativo (checar via property/objectName). Registrar no CMake (peubud_add_test, offscreen).
- [ ] **Step 2: rodar e ver falhar.** `cmake --build build -j$(nproc) && QT_QPA_PLATFORM=offscreen ./build/tests/test_sidebar_nav`.
- [ ] **Step 3: implementar SidebarNav** (sem QSS fino ainda; so estrutura + sinais + ativo).
- [ ] **Step 4: rodar e ver passar; ctest verde.**
- [ ] **Step 5: commit** `feat(#165): widget SidebarNav (sidebar agrupada navegavel por chave)`.

## Task 2: Registro de navegacao (chave -> grupo/titulo/emoji)

**Files:** Create `src/ui/NavRegistry.{h,cpp}` (+ CMake). Test: dobra no test_sidebar_nav ou novo.

**Interfaces:**
- Produces: `struct NavItem { QString chave, emoji, titulo, grupo; };` e `const QList<NavItem>& navItens();` na ordem fixa:
  Dashboard (grupo "") ; "Dia a dia": tab_entradas 💵, tab_gastos 🧾, tab_cartoes 💳, tab_recorrencias 🔁, tab_lembretes 🔔 ; "Planejar": tab_planejamento 📋, tab_metas 🎯, tab_dividas 📉, tab_tendencias 📈 ; "Investir": tab_investimentos 📊, tab_proventos 💰, tab_bolsa 🏦. (chaves = as do TabHelp; Dashboard usa "tab_dashboard" 🏠.)
- [ ] Steps TDD: teste que `navItens()` tem 13 itens, chaves unicas, grupos certos -> implementar -> verde -> commit `feat(#165): NavRegistry (ordem fixa agrupada das abas)`.

## Task 3: Refactor buildUi -- QTabWidget -> Sidebar + QStackedWidget

**Files:** Modify `src/ui/MainWindow.h`, `src/ui/MainWindow.cpp`.

- Trocar membro `QTabWidget *m_tabs` por `QStackedWidget *m_stack` + `SidebarNav *m_sidebar` + `QMap<QString,QWidget*> m_paginas` (chave->shell).
- `buildUi`: criar `m_sidebar` (povoar via `navItens()` + emoji), `m_stack` (addWidget dos 13 shells na ordem do registro), `m_paginas[chave]=shell`. Layout central: `QWidget` com `QHBoxLayout(m_sidebar, m_stack)` em `setCentralWidget`.
- Conectar `m_sidebar->navegou(chave)` -> `navegarParaChave(chave)` (novo helper: `m_stack->setCurrentWidget(m_paginas[chave]); m_sidebar->setAtivo(chave)`).
- **REMOVER:** `ordemAbasDefault`, `aplicarOrdemAbasSalva`, `salvarOrdemAbas`, `resetarOrdemAbas`, o `connect(tabMoved)`, e (Task 5) os nav-buttons. Tirar as declaracoes do .h.
- `initialTab`: converter para chave (AppSettings guarda indice hoje; mapear indice->chave via `navItens()` ou migrar para chave). Aba inicial = `navegarParaChave(chave inicial)`.
- Verificacao: build + screenshot (sidebar a esquerda, troca de pagina ao clicar). ctest verde.
- [ ] Commit `feat(#165): MainWindow usa sidebar + QStackedWidget (remove QTabWidget e persistencia de ordem)`.

## Task 4: Re-cablear navegacao por chave (resolve #157)

**Files:** Modify `MainWindow.cpp`; `DashboardWidget.{h,cpp}`, `BuscaGlobalDialog.{h,cpp}` (sinais).

- `DashboardWidget::navegarParaAba(int)` -> `navegarParaChave(QString)` (emitir chave: cards mapeiam p/ "tab_entradas"/"tab_gastos"/"tab_cartoes"/"tab_investimentos"/"tab_proventos"). Idem `BuscaGlobalDialog`. Conectar a `MainWindow::navegarParaChave`.
- `currentChanged` lazy-load (464): trocar por `connect(m_stack, &QStackedWidget::currentChanged, ...)` usando o widget atual (mapear de volta p/ chave se preciso).
- Cartoes da calculadora (1274): `m_stack->setCurrentWidget(m_shellCartoes)`.
- Lembretes (550): `m_stack->setCurrentWidget(m_shellLembretes)` (ja por widget).
- Ctrl+1..9 (setupShortcuts): mapear p/ os primeiros itens de `navItens()` por chave.
- Verificacao: clicar cards do Dashboard / busca global / Ctrl+N levam a aba certa (testar build + screenshot). ctest verde.
- [ ] Commit `fix(#157,#165): navegacao por chave estavel (cards/busca/atalhos)`.

## Task 5: Rodape da sidebar + utilidades + aposentar QMenuBar (#170)

**Files:** Modify `MainWindow.cpp` (buildMenuBar, buildThemeBar, toolbar).

- Rodape da sidebar (via `addFooterItem` + `setUsuario`): ⚙️ Preferencias (onConfiguracoes), ❓ Ajuda (onAjuda), 🌗 Tema (cicla applyThemeMode), usuario (nome de UserSession).
- Cluster topo-direito (numa toolbar fina OU no header do stack): 🔍 Busca global (onBuscaGlobal), 🧮 Calculadora. E um "... Mais" (QMenu) com: 📂 Abrir, 💾 Salvar, 💾 Salvar como, 📁 Importar (submenu), 💾 Exportar (submenu), 🔄 Reconciliar, 🖨️ Imprimir, ℹ️ Sobre.
- REMOVER a `QMenuBar` (buildMenuBar) e os nav-buttons da buildThemeBar (mantendo o botao de tema, que vai pro rodape). Manter todas as ACOES (onAbrir/onSalvar/etc.) conectadas aos novos pontos.
- Verificacao: tudo acessivel (Preferencias, Ajuda, Tema, Importar/Exportar, Reconciliar, Busca, Calc) via rodape/cluster/menu. Build + screenshot. ctest verde.
- [ ] Commit `feat(#170): aposenta QMenuBar -> rodape da sidebar + cluster + menu Mais`.

## Task 6: QSS da sidebar (navy, grupos, pill ativo)

**Files:** Modify `src/ui/Theme.cpp` (gerarQss): regras p/ `SidebarNav` e seus itens.

- Sidebar: fundo degrade `@@SIDEBAR1@@`->`@@SIDEBAR2@@` (tokens da Fase 1: `Theme::sidebar1/2`), texto `@@SIDEBARTXT@@`, largura ~236. Rotulo de grupo: maiusculo pequeno, cinza-azulado. Item: raio 9, hover leve clareada; **ativo = pill `@@ACC@@`** com sombra. Rodape: separador + usuario.
- Adicionar tokens nomeados `@@SIDEBAR1@@/@@SIDEBAR2@@/@@SIDEBARTXT@@` no replace (consistente com a Fase 2 fix).
- Verificacao: build release + screenshot -> sidebar navy com grupos e pill azul no ativo, idem mockup `docs/design/mockups/peubud_redesign_dashboard.html`. Olho do usuario.
- [ ] Commit `feat(#165): estilo QSS da sidebar (navy, grupos, pill ativo)`.

## Final
Apos as 6 tasks: review whole-branch + checkpoint visual com o usuario antes de seguir p/ Fase 3.

## Self-review
- Cobre #165 (sidebar+stack+nav por chave, T1-T4,T6) e #170 (menus, T5). Resolve #157 (T4).
- Risco: T3/T4 (refactor MainWindow). Mitiga: commits por task + screenshot + ctest a cada passo; nao mexe na logica dos widgets.
- Sem placeholders nos contratos; emoji sempre via emojiIcon; tokens nomeados no QSS.
