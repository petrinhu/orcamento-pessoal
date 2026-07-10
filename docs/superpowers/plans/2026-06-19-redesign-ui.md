# Redesign UI (1.0) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: use superpowers:subagent-driven-development ou superpowers:executing-plans para executar tarefa a tarefa. Steps usam checkbox (`- [ ]`).

**Goal:** Transformar a UI do PeuBud (hoje "anos 90") no design moderno aprovado: amigavel + azuis confiaveis + **sidebar agrupada a esquerda** (substitui o QTabWidget no topo) + cards/tabelas/graficos modernos, sem quebrar a logica financeira.

**Architecture:** Centralizar tokens no `Theme`; trocar o shell do `MainWindow` (QTabWidget -> QHBoxLayout[sidebar + QStackedWidget]); construir componentes reutilizaveis; migrar aba a aba; navegacao por chave estavel.

**Tech Stack:** C++20, Qt6 Widgets/QSS, CMake, ctest (`QT_QPA_PLATFORM=offscreen`), pre-commit hook (cppcheck/ctest).

## Global Constraints (verbatim do projeto)

- Valores monetarios em **centavos qint64** (nunca float/double); juros `% x 10000`.
- Queries com `bindValue`; `PRAGMA foreign_keys = ON`.
- Commits em portugues, Conventional Commits, escopo `(#ID)`; rodape Co-Authored-By.
- Apenas ferramentas FOSS (dnf/apt/pacman).
- **Nao criar/editar codigo C++ sem o usuario aprovar o escopo.**
- Icone e nome "PeuBud" sao canonicos (nao mudam).
- ctest deve continuar 27/27; pre-commit hook e o gate.
- Spec de referencia: `docs/superpowers/specs/2026-06-19-redesign-ui-design.md`.

## Verificacao (realidade Qt)

- **Logica** (navegacao por chave, tokens, persistencia): teste ctest (TDD).
- **Visual** (aparencia, layout, cor): `cmake --build` + abrir o release + screenshot (spectacle) + olho do usuario. Cada fase termina com um checkpoint visual aprovado pelo usuario antes da proxima.
- Branch dedicado: `redesign-ui` (nao mexer na main direto durante o redesign).

---

## Fases (cada uma vira seu proprio plano bite-sized ao iniciar)

- **Fase 1 (detalhada abaixo):** Design system / tokens no Theme. -- #164
- **Fase 2:** Shell + Sidebar agrupada (refactor do MainWindow; navegacao por chave; resolve #157). -- #165, #170
- **Fase 3:** Componentes base (cards, tags, banner de ajuda, tooltips de glossario, empty states). -- #166
- **Fase 4:** Tabelas + graficos no novo estilo. -- #167, #168
- **Fase 5:** Redesign por aba (dia-a-dia + planejar). -- #169
- **Fase 6:** Tela de Configuracoes. -- #173
- **Pos-1.0 (placeholders, planos proprios depois):** detalhe do ativo / analise tecnica (#171), Assistente de IA (#172), Investimentos/Bolsa completos.

Ordem por dependencia: 1 -> 2 -> 3 -> (4 e 5 em paralelo logico) -> 6.

---

## FASE 1: Design system / tokens no Theme (#164)

Objetivo: a nova paleta e tokens vivem no `Theme`, sem ainda trocar a navegacao. Entrega: o app continua funcionando, com a paleta nova aplicada (cores/cards/tipografia). Baixo risco, base de tudo.

**Files:**
- Modify: `src/ui/Theme.h` (declarar getters novos)
- Modify: `src/ui/Theme.cpp` (paleta nova light/dark, QSS atualizado)
- Test: `tests/test_theme_tokens.cpp` (novo) + registrar no `CMakeLists.txt`

**Interfaces:**
- Produces: `Theme::bg2()`, `Theme::sidebar1()`, `Theme::sidebar2()`, `Theme::primarioHover()`, `Theme::primarioPressed()`, `Theme::dourado()` (QString hex), alem dos ja existentes (`acento/positivo/negativo/texto/subtexto/surface/borda/atencao`). Cores exatas do spec secao 3.

- [ ] **Step 1: Escrever teste dos tokens (falha)**

```cpp
// tests/test_theme_tokens.cpp
#include <QtTest>
#include "ui/Theme.h"
class TestThemeTokens : public QObject { Q_OBJECT
private slots:
    void tokensLight() {
        Theme::setMode(Theme::ThemeMode::Light);
        QCOMPARE(Theme::acento(), QStringLiteral("#3B82F6"));
        QCOMPARE(Theme::positivo(), QStringLiteral("#16C784"));
        QCOMPARE(Theme::negativo(), QStringLiteral("#EA3943"));
        QCOMPARE(Theme::dourado(), QStringLiteral("#E8A93B"));
        QVERIFY(!Theme::sidebar1().isEmpty());
    }
};
QTEST_MAIN(TestThemeTokens)
#include "test_theme_tokens.moc"
```

- [ ] **Step 2: Rodar o teste e ver falhar**

Run: `cmake --build build -j$(nproc) && QT_QPA_PLATFORM=offscreen ./build/test_theme_tokens`
Expected: FAIL (getters novos nao existem / cores antigas `#3DAEE9`).

- [ ] **Step 3: Atualizar a paleta no Theme**

Trocar os valores em `Theme.cpp` (namespace Color) para os do spec: `AccentLight=#3B82F6`, `PositiveLight=#16C784`, `NegativeLight=#EA3943`, adicionar `Dourado`, `Bg2`, `Sidebar1/2`, `PrimarioHover=#5CC6FA`, `PrimarioPressed=#1F7AAD`. Declarar/implementar os getters novos em `Theme.h/.cpp`. Atualizar o `gerarQss()` (fundo `#EEF3F9`, cards brancos raio 14, botoes raio 9) usando os tokens.

- [ ] **Step 4: Rodar o teste e ver passar**

Run: `cmake --build build -j$(nproc) && QT_QPA_PLATFORM=offscreen ./build/test_theme_tokens`
Expected: PASS. E `QT_QPA_PLATFORM=offscreen ctest --test-dir build` -> 28/28.

- [ ] **Step 5: Checkpoint visual + commit**

Abrir o release, screenshot, confirmar a paleta nova (fundo azul-claro, cards, acento). Com ok do usuario:
```bash
git add src/ui/Theme.h src/ui/Theme.cpp tests/test_theme_tokens.cpp CMakeLists.txt
git commit -m "feat(#164): paleta e tokens do redesign (azuis confiaveis) no Theme"
```

---

## FASE 2 (resumo de tarefas; vira plano bite-sized ao iniciar): Shell + Sidebar (#165, #170)

- **T1 SidebarNav (novo widget)** `src/ui/widgets/SidebarNav.{h,cpp}`: constroi Dashboard + grupos (Dia a dia/Planejar/Investir) a partir de um registro de chaves; emite `navegou(QString chave)`; item ativo destacado; colapsavel *(planejado aqui, **nao entregue** — corte de escopo decidido pelo lider em #165/DIST-12, 2026-07-10; ver `docs/manual_estilo.md` §8)*. Teste: cria N itens certos + emite a chave certa.
- **T2 Registro de abas** `src/ui/TabRegistry.{h,cpp}`: `chave -> {titulo, emoji, grupo}` (fonte unica; tambem alimenta o `TabHelp`).
- **T3 MainWindow shell:** trocar `QTabWidget` por `QHBoxLayout(SidebarNav + QStackedWidget)`. Mover as 13 abas pro stacked; conectar `navegou(chave) -> setCurrentWidget`. **Navegacao por chave** (helper `navegarParaChave`), migrando os pontos que usavam indice (Dashboard cards, BuscaGlobal) -> **resolve #157**. Manter persistencia da ordem por chave.
- **T4 Rodape da sidebar:** Preferencias, Ajuda, Tema, usuario.
- **T5 Menus (#170):** aposentar QMenuBar -> rodape + cluster topo-direito ("... Mais": Importar/Exportar/Reconciliar/Imprimir/Sobre; busca; calculadora).
- Verificacao: build + screenshot (navegar entre abas pela sidebar) + ctest 28/28.

## FASE 3 (resumo): Componentes base (#166)

- `Card`, `Tag`, `HelpBanner` (refina/substitui TabShell: descricao + "Como usar" expansivel), tooltips de glossario (sublinhado pontilhado + popover; via event filter, sem o bug do GlossarioDecorator), `EmptyState`. Aplicar em 1-2 abas piloto. Build + screenshot.

## FASE 4 (resumo): Tabelas + graficos (#167, #168)

- Helper de estilo de tabela (densidade, divisorias, hover, tags, rodape-resumo). Refatorar os charts (Dashboard/Tendencias) pro novo estilo (linha/area, donut, barras). Build + screenshot.

## FASE 5 (resumo): Redesign por aba (#169)

- Uma tarefa por aba do dia-a-dia/planejar, aplicando o padrao do spec secao 11 (Cartoes cartoes-visuais, Recorrencias switch, Lembretes status-cards, Planejamento barras, Metas anel, Dividas barra, Tendencias pills). Investimentos/Bolsa = placeholder. Cada aba: build + screenshot + ok do usuario.

## FASE 6 (resumo): Tela de Configuracoes (#173)

- Sub-navegacao + secoes (Aparencia/Moeda/Categorias/Fontes/Cartoes/Mercado/IA-placeholder/Backup/Seguranca/Sobre) no novo estilo. Build + screenshot.

---

## Self-review (plano vs spec)

- Cobertura: cada secao do spec mapeia numa fase (3->F1, 4/10->F2, 6->F3, 7/8->F4, 11->F5, 14->F6; 12/13 pos-1.0). OK.
- Sem placeholders na Fase 1 (detalhada). Fases 2-6 sao **resumos de tarefas por design** (cada uma vira plano bite-sized proprio ao iniciar, conforme escopo grande -> multiplos planos).
- Consistencia de tipos: getters do Theme nomeados uma vez (Fase 1) e reusados.
- Risco maior: Fase 2 (refactor MainWindow). Mitiga com branch dedicado + checkpoints visuais + ctest a cada passo.
