# Task 5 — Fase 2 Redesign: Aposentar QMenuBar (#170)

**Branch:** `redesign-ui`
**Data:** 2026-06-20
**ctest:** 30/30 verde

---

## O que foi para onde

### Rodapé da sidebar
Adicionados imediatamente após o loop de itens normais da sidebar em `buildUi()`:
- `addFooterItem("act_prefs", "⚙️", "Preferências")`
- `addFooterItem("act_ajuda", "❓", "Ajuda")`
- `addFooterItem("act_tema",  "🌗", "Tema")`
- `setUsuario(UserSession::instance().name())` — mesma fonte da status bar

### Dispatcher `act_*` em `navegarParaChave`
Bloco de guarda adicionado no topo da função: se `chave.startsWith("act_")`, roteia sem tocar o stack ou o item ativo da sidebar:
- `act_prefs` → `onConfiguracoes()`
- `act_ajuda` → `onAjuda()`
- `act_tema` → cicla `Auto → Light → Dark → Auto` via `applyThemeMode(next)` (mesmo comportamento do botão antigo da toolbar; `next` inicializado para `Auto` para satisfazer `-Wmaybe-uninitialized`)

### Cluster de utilidades — `buildUtilBar()` (ex-`buildThemeBar`)
Toolbar fina no topo, não-móvel, ícone 20px:
- Botão `🔍 Buscar` → `onBuscaGlobal()`
- Botão `🧮 Calculadora` → `CalculadoraHub::instance().mostrarGlobal()`
- Botão `📅 Hoje` → `scrollParaMesAtual()` conforme página ativa
- Separador
- `QToolButton "⋯ Mais"` com `InstantPopup` → `QMenu` contendo:
  - `📂 Abrir…` (com Ctrl+O visível no menu), `💾 Salvar`, `💾 Salvar como…`
  - Separador
  - Submenu `📁 Importar de` (10 formatos — idêntico ao menu antigo)
  - Submenu `💾 Exportar` (11 formatos — idêntico ao menu antigo)
  - Separador
  - `🔄 Reconciliar extrato…`
  - Separador
  - `📝 Glossário financeiro…` (F2)
  - `ℹ️ Sobre PeuBud…`
  - Separador
  - `🔒 Bloquear (fechar)`
  - `Sair` (Ctrl+Q)

### Atalhos de teclado preservados via `buildMenuBar()` (esvaziada)
`menuBar()->setVisible(false)` oculta a barra nativa. Todas as QAction são recriadas e registradas via `this->addAction()` (contexto `MainWindow`) para que os atalhos continuem funcionando sem a barra visível:

| Atalho | Ação |
|--------|------|
| Ctrl+O | Abrir |
| Ctrl+S | Salvar |
| Ctrl+Shift+S | Salvar como |
| Ctrl+Q | Sair |
| Ctrl+Z | Desfazer (QUndoStack) |
| Ctrl+Y / Ctrl+Shift+Z | Refazer (QUndoStack) |
| Ctrl+C | Copiar |
| Ctrl+V | Colar |
| Ctrl+A | Selecionar tudo |
| Ctrl+F | Buscar transação |
| F1 | Ajuda |
| F2 | Glossário |

Os atalhos `Ctrl+1..9`, `Ctrl+,`, `Ctrl+B` já vinham de `setupShortcuts()` e não foram tocados.

### Removido / limpo
- Campo `m_btnTheme` removido do `.h` (QToolButton*)
- Método `updateThemeButton()` removido do `.h` e `.cpp`
- `buildThemeBar()` renomeado para `buildUtilBar()` no `.h` e `.cpp`
- `applyThemeMode()` simplificado: não chama mais `updateThemeButton()`
- Botão ⚙️ Configurações da toolbar antiga removido (foi pro rodapé)
- Botão 🔒 Bloquear desabilitado da toolbar antiga removido (está no menu Mais como ação real)

---

## Lista de ações conferidas

| Ação | Onde está | Atalho |
|------|-----------|--------|
| Abrir | Menu Mais + `this->addAction` | Ctrl+O |
| Salvar | Menu Mais + `this->addAction` | Ctrl+S |
| Salvar como | Menu Mais + `this->addAction` | Ctrl+Shift+S |
| Importar (10 fmts) | Submenu no Menu Mais | — |
| Exportar (11 fmts) | Submenu no Menu Mais | — |
| Reconciliar | Menu Mais | — |
| Preferências | Rodapé sidebar (`act_prefs`) + `this->addAction` via Ctrl+, (setupShortcuts) | Ctrl+, |
| Ajuda | Rodapé sidebar (`act_ajuda`) + `this->addAction` | F1 |
| Glossário | Menu Mais + `this->addAction` | F2 |
| Sobre | Menu Mais | — |
| Busca global | Toolbar cluster + `this->addAction` | Ctrl+F |
| Calculadora | Toolbar cluster | — |
| Hoje | Toolbar cluster | — |
| Tema (ciclar) | Rodapé sidebar (`act_tema`) | — |
| Desfazer | `this->addAction` (QUndoStack) | Ctrl+Z |
| Refazer | `this->addAction` (QUndoStack) | Ctrl+Y |
| Copiar | `this->addAction` | Ctrl+C |
| Colar | `this->addAction` | Ctrl+V |
| Selecionar tudo | `this->addAction` | Ctrl+A |
| Bloquear | Menu Mais | Ctrl+B (setupShortcuts) |
| Sair | Menu Mais + `this->addAction` | Ctrl+Q |

**Total: 21 ações confirmadas — nenhuma perdida.**

---

## Concerns / follow-up

1. **Atalhos duplicados no Menu Mais:** o `QMenu` mostra shortcuts visualmente (ex.: `Ctrl+O` ao lado de "Abrir"). Isso é correto — os atalhos funcionam via `this->addAction`, e o menu os exibe como informação. Sem conflito.
2. **Emoji `⋯`** no botão "Mais": Noto Color Emoji pode não ter esse glifo como colorido (é pontuação). Se renderizar em branco/genérico, trocar por `⚡` ou texto puro "..." — decisão visual fica com o usuário.
3. **`act_tema` no rodapé:** o emoji do botão é sempre `🌗` (fixo). Não atualiza para mostrar o modo atual. Se o usuário quiser feedback visual do modo ativo (☀️/🌙), precisaria de um sinal de `Theme` → `SidebarNav`. Não implementado agora (baixo custo de follow-up quando Task 1 de T3 redesenhar o rodapé).
4. **`buildMenuBar()` esvaziada**: mantida como método para não quebrar chamadas, mas declaração no header sinaliza o novo propósito. Pode ser removida completamente em refactor futuro.
