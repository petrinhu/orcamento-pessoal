# PeuBud — Planejamento

Arquivo canônico de pendências, auditoria técnica e conformidade com o contrato.

## Legenda

- **Grupo**: `FUNC` (funcionalidade) · `TESTE` (teste/qualidade) · `AUDIT` (auditoria técnica) · `CONTRATO` (conformidade com regras mandatórias) · `BUG` (achado concreto de auditoria de código) · `INFRA` (infraestrutura/build/CI) · `DOC` (documentação)
- **Prioridade**: `Crítica` · `Alta` · `Média` · `Baixa`
- **Pré-requisito**: `—` (nenhum) ou ID(s) de itens que precisam ser concluídos antes
- **Dificuldade**: `Alta` · `Média` · `Baixa`
- **Status**: ✅ Concluído · 🔄 Em andamento · 🟡 Parcial · ⏳ Pendente · 💡 Decisão tomada · 🎨 Pendente design · 🔍 Pendente verificação · ❌ Cancelado
- **Estado Auditado**: `—` (não auditado) · `✓` (auditado e aprovado) · `⚠` (auditado com ressalvas)

> **Convenção de ID:** `#NN` mantido como identificador estável — usado em commits (`feat(#44): ...`), branches e referências cruzadas. Próximo ID disponível: **#70**.
>
> **Hierarquia:** itens filhos só começam após pai ✅; irmãos com mesmo prefixo numérico (ex.: `1.2.1` e `1.2.2`) são paralelos.

---

## Bloco 0 — Concluídos

| ID | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| #1 | FUNC | Tema: aba ativa mesma cor do conteúdo | Alta | — | Baixa | ✅ Concluído | — |
| #2 | FUNC | Entradas: clique simples abre dialog | Alta | — | Baixa | ✅ Concluído | — |
| #3 | FUNC | Cartões: renomear aba "Parcelas" → "Cartões" | Alta | — | Baixa | ✅ Concluído | — |
| #4 | FUNC | Cartões: renomear "Nova compra" → "Novo item" | Alta | — | Baixa | ✅ Concluído | — |
| #5 | FUNC | Entradas: labels sub-colunas (linha 1) | Alta | — | Média | ✅ Concluído | — |
| #6 | FUNC | Dashboard: resumo mensal/anual | Média | — | Alta | ✅ Concluído | — |
| #7 | FUNC | Colunas: auto-ajuste expand-only | Média | — | Média | ✅ Concluído | — |
| #8 | FUNC | Entradas: botão "Organizar por" | Média | — | Média | ✅ Concluído | — |
| #9 | FUNC | Navegação: botão "Hoje" + atalhos teclado | Média | — | Média | ✅ Concluído | — |
| #10 | FUNC | UX: totais na status bar | Média | — | Baixa | ✅ Concluído | — |
| #11 | FUNC | Export: escolha de seções (CSV/XLSX/JSON/MD/HTML/PDF/TXT/OFX/QIF) — `ExportDialog` com checkboxes | Baixa | — | Média | ✅ Concluído | — |
| #12 | FUNC | UX: estado vazio com msg "Clique para adicionar" | Baixa | — | Baixa | ✅ Concluído | — |
| #13 | FUNC | Configurações: reorganizar UI/UX | Média | — | Média | ✅ Concluído | — |
| #14 | FUNC | Aba Gastos: lançamentos avulsos PIX/débito/dinheiro/fixos | Alta | — | Alta | ✅ Concluído | — |
| #15 | FUNC | Conexões entre abas: vincular valores/cálculos entre Cartões, Entradas, Gastos | Média | — | Alta | ✅ Concluído | — |
| #16 | FUNC | Barra de menu: ícones ⏮◀▶⏭ + Abrir/Salvar/Calc/Config/Bloquear | Média | — | Baixa | ✅ Concluído | — |
| #17 | FUNC | Calculadora v1: aba Comum + CalculadoraEdit + CalculadoraHub + botão 🧮 | Média | — | Média | ✅ Concluído | — |
| #20 | FUNC | Aba investimentos | Alta | — | Alta | ✅ Concluído | — |
| #21 | FUNC | Categorias hierárquicas (gastos, entradas, investimentos) | Média | — | Média | ✅ Concluído | — |
| #22 | FUNC | Aba planejamento e orçamento | Alta | — | Alta | ✅ Concluído | — |
| #23 | FUNC | Aba tendências de gastos | Média | — | Média | ✅ Concluído | — |
| #24 | FUNC | Gráficos (Qt Charts — barras, pizza, linha) | Média | — | Média | ✅ Concluído | — |
| #26 | FUNC | Transações recorrentes agendadas | Alta | — | Média | ✅ Concluído | — |
| #27 | FUNC | Reconciliação bancária (extrato vs. app) | Média | — | Alta | ✅ Concluído | — |
| #28 | FUNC | Metas financeiras (savings goals) | Média | — | Média | ✅ Concluído | — |
| #29 | FUNC | Gestão de pagadores/recebedores | Baixa | — | Baixa | ✅ Concluído | — |
| #30 | FUNC | Undo/Redo (QUndoStack único em MainWindow, LambdaCommand genérico) | Média | — | Alta | ✅ Concluído | — |
| #31 | FUNC | Lembretes/alertas de contas a pagar | Média | — | Média | ✅ Concluído | — |
| #32 | FUNC | Busca/filtro global de transações | Média | — | Baixa | ✅ Concluído | — |
| #33 | FUNC | Controle de dívidas e empréstimos | Média | — | Alta | ✅ Concluído | — |
| #34 | AUDIT | Refactor: extrair acesso a Repository/SQL dos widgets para Services | Alta | — | Alta | ✅ Concluído | — |
| #35 | FUNC | Cartões: painel inferior com campos editáveis + tabela de amortização Price | Média | — | Alta | ✅ Concluído | — |
| #36 | FUNC | Calculadora v2: aba Financeira HP-style (TVM, amortização, BEGIN/END, "Enviar para Cartões") | Média | — | Alta | ✅ Concluído | — |
| #37 | FUNC | Calculadora v3: aba Wizards (CET, juros, parcelas, FV, renda passiva, ROI, comprometimento, saldo devedor, meta, amortização parcial) | Baixa | — | Alta | ✅ Concluído | — |
| #38 | FUNC | Calculadora: entrada por teclado físico | Média | — | Baixa | ✅ Concluído | — |
| #39 | FUNC | Calculadora: fechar junto com janela principal | Média | — | Baixa | ✅ Concluído | — |
| #40 | FUNC | Calculadora: botão "Sempre visível" | Média | — | Baixa | ✅ Concluído | — |
| #41 | FUNC | Configurações: aba inicial ao abrir o app | Baixa | — | Baixa | ✅ Concluído | — |
| #42 | CONTRATO | Migrar criptografia para modelo híbrido AES-256-GCM + XChaCha20-Poly1305 via libsodium; Argon2id; formato versionado; backward-compat CBC | Alta | — | Alta | ✅ Concluído | ✓ |
| #44 | FUNC | Infraestrutura: `PrecoAtivo` + `PriceRepository` + tabelas `precos_ativos` e `cotacoes_tempo_real` | Alta | — | Baixa | ✅ Concluído | — |
| #49 | FUNC | Calculadora v3: adições — Comparador RF + FIRE number | Baixa | #37 | Baixa | ✅ Concluído | — |
| #60 | FUNC | Calculadora v3+: FIRE variants (Lean/Fat/Coast/Barista/Geographic Arbitrage) | Baixa | #49 | Média | ✅ Concluído | — |

---

## Bloco 1 — Infraestrutura de Mercado & Dados em Tempo Real

| ID | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| #59 | FUNC | B3CotahistProvider: download e cache histórico completo da B3 (COTAHIST anual/diário + cotação RT cotacao.b3.com.br + composição índices + Hub Dados Públicos) | Alta | #44 | Alta | ⏳ Pendente | — |
| #45 | FUNC | MarketDataService: fetch assíncrono, providers selecionáveis (B3 Direto/Yahoo/B3 Oficial/Brapi) + FintzProvider (dividendos/splits/fundamentos/PTAX) | Alta | #44 | Alta | 🔄 Em andamento | — |
| #48 | FUNC | InvestimentosWidget: migração schema (`ticker`/`quantidade`/`preco_unitario`) + preços de mercado em tempo real + DY% via Fintz | Alta | #45 | Alta | ⏳ Pendente | — |
| #66 | FUNC | Dividendos e Proventos tracker — tabela `proventos_recebidos`, sub-painel por ativo, card Dashboard | Média | #48 | Média | ⏳ Pendente | — |
| #46 | FUNC | Configurações: seção "Mercado" — provider padrão, credenciais Brapi, histórico COTAHIST, polling, moedas slots 1/2 | Alta | #45 | Baixa | ⏳ Pendente | — |
| #47 | FUNC | Painel câmbio LED (`CurrencyPanelWidget`, DSEG7) — 50 fiat + 30 cripto; BRL via PTAX Fintz, demais via Frankfurter/CoinGecko | Alta | #46 | Alta | ⏳ Pendente | — |
| #58 | FUNC | BrapiProvider (free 15k req · Startup R$59,99 · Pro R$99,99 · auth Bearer · `/api/quote` `/api/crypto` `/api/currency` `/api/inflation` `/api/interest-rate`) | Alta | #46 | Média | ⏳ Pendente | — |
| #63 | FUNC | Simulador de Rentabilidade e Dividendos — DRIP, ajuste por splits, comparativo CDI/IPCA/IBOV | Baixa | #58, #59 | Alta | ⏳ Pendente | — |
| #50 | FUNC | Aba Bolsa: infraestrutura + busca + seletor de provider em runtime | Alta | #46 | Média | ⏳ Pendente | — |
| #51 | FUNC | Aba Bolsa: gráfico de candles fluido sem piscar (QCustomPlot v2.1, layer separado `liveCandle` + `rpQueuedReplot`) | Alta | #50 | Alta | ⏳ Pendente | — |
| #52 | FUNC | Aba Bolsa: indicadores técnicos (TA-Lib — MA/EMA/RSI/MACD/Stoch/BBands/OBV/ATR) | Alta | #51 | Alta | ⏳ Pendente | — |
| #53 | FUNC | Aba Bolsa: suporte/resistência (automático Pivot/Fib/Camarilla/Swing + traçado manual persistido) | Média | #51 | Alta | ⏳ Pendente | — |
| #64 | FUNC | Aba Bolsa: assistente IA integrado (16 providers configuráveis, rotação de chaves, fallback chain, cooldown exponencial, streaming com coalescing, system prompt cache-stable) | Média | #50 | Alta | ⏳ Pendente | — |
| #65 | FUNC | Ajuste automático por splits e bonificações — tabela `ajustes_corporativos`, fator cumulativo retroativo em `precos_ativos` | Alta | #44 | Média | ⏳ Pendente | — |

---

## Bloco 2 — Calculadoras

| ID | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| #43 | FUNC | Calculadora v4: Bolsa de Valores (19 wizards — Operacional 7 + Fundamentalista 7 + FIIs 5; dados via Fintz `/v1/financials` quando ticker preenchido) | Baixa | #37 | Alta | 🔄 Em andamento | — |
| #54 | FUNC | Calculadora v5: Imóveis (SAC vs Price, FGTS, CET imobiliário com MIP/DFI, amortização extra, comprometimento CEF, aluguel vs financiamento) | Baixa | #37 | Alta | 🔄 Em andamento | — |
| #55 | FUNC | Calculadora v6: Renda Fixa (%CDI bruto/líquido, IPCA+, prefixado, CDB vs LCI/LCA, Tesouro Selic vs CDB, IOF 30d, **Wizard Tesouro Direto J1–J4** com série histórica `tesouro_yields`) | Baixa | #37 | Alta | 🔄 Em andamento | — |
| #56 | FUNC | Calculadora v7: IR & Tributos (IRPF mensal, PGBL benefício fiscal, PGBL vs VGBL, carnê-leão, simplificada vs completa) | Baixa | #37 | Alta | 🔄 Em andamento | — |
| #57 | FUNC | Calculadora v8: Inflação & Câmbio (IPCA/IGP-M/TR/INPC, poder de compra, Fisher real, conversão com IOF, meta corrigida) | Baixa | #37 | Média | 🔄 Em andamento | — |
| #61 | FUNC | Calculadora Reserva de Emergência (despesas × meses, gap, comparativo Poupança/CDB/Tesouro Selic líquidos) | Baixa | #37 | Baixa | ⏳ Pendente | — |
| #62 | FUNC | Calculadora CLT vs PJ (Simples Nacional anexos I–V + Lucro Presumido, break-even faturamento) | Baixa | #37 | Alta | ⏳ Pendente | — |

---

## Bloco 3 — Melhorias Gerais

| ID | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| #25 | FUNC | Consultor financeiro IA — modo "copiar prompt" + modo API direta (mesmos 16 providers de #64, default sugerido Venice AI private); memória persistente `ia_memoria_consultor` | Baixa | — | Alta | ⏳ Pendente | — |
| #18 | TESTE | Testes automatizados CLI (ctest + QTest) — `ParcelaService` (CET/Newton-Raphson), `CurrencyService`, `ImportService`, `ExportService`; mocks para `DatabaseManager`. **Pré-release: suíte completa fica para o fim do projeto** (ver [[feedback-tdd-sempre-suites-no-fim]]). TDD durante features usa #102 bootstrap | Baixa | #102 | Alta | ⏳ Pendente | — |
| #19 | FUNC | Tradução i18n completa pt-BR / en — completar `peubud_en.ts` via Qt Linguist; locale-aware datas/moeda | Baixa | — | Média | ⏳ Pendente | — |
| #68 | FUNC | Alertas de preço e eventos via Telegram Bot — `QNetworkAccessManager` puro, retry 3× com `retry_after`, streaming IA via `editMessageText` | Baixa | #45 | Média | ⏳ Pendente | — |
| #69 | AUDIT | Eliminar duplicação de funções financeiras — extrair fórmulas inline de `CalculadoraDialog` lambdas + `ParcelaService`/`TvmService` para `CalculadoraService` (stateless, métodos estáticos, fonte única) | Baixa | — | Média | ⏳ Pendente | — |

---

## Bloco 4 — Conformidade, Infra & Auditoria (novas)

> Itens derivados de auditoria de contexto (sessão 2026-05-13). Próximo ID disponível após este bloco: **#81**.

| ID | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| #70 | CONTRATO | Sincronizar `CONTRACT.md` local (fork, 1048 linhas) com `[[CONTRACT]]` canônico do vault (945 linhas) — gerar diff, classificar adições C++/Qt como upstream-candidates ou local-only, documentar política de sync | Média | — | Média | 🟡 Parcial | — |
| #71 | DOC | Criar `TESTES.md` local apontando para suítes T1–T15 / A1–A13 do `[[TESTES]]` canônico — `CONTRACT.md §11` referencia arquivo que não existe no repo (link quebrado) | Alta | — | Baixa | ✅ Concluído | — |
| #72 | INFRA | Codeberg Actions workflow `.forgejo/workflows/build.yml` — CMake build matrix Fedora/Debian/Arch, instala dependências via dnf/apt/pacman, executa `cmake --build` (badge no README já aponta para essa URL — workflow ainda não existe) | Alta | — | Média | 🟡 Parcial | — |
| #73 | TESTE | Testes de integração SQLite — cobrir padrão `PRAGMA table_info` + `ALTER TABLE` idempotente (migração #48), garantir backward-compat de schemas antigos. **Pré-release: fica para o fim do projeto** | Média | #18 | Média | ⏳ Pendente | — |
| #74 | AUDIT | Pre-commit hook com `clang-tidy` + `cppcheck` + `gitleaks` + **`ctest`** (sempre, ver [[feedback-ctest-pre-commit]]) — instala via dnf/apt/pacman; configuração `.clang-tidy` checada no repo | Média | #102 | Média | ✅ Concluído | ✓ |
| #75 | AUDIT | CI: build dedicado com ASan/UBSan + execução de toda suíte `ctest` — falha pipeline se `ERROR SUMMARY` > 0. **Pré-release: fica para o fim do projeto** | Média | #72, #18 | Média | ⏳ Pendente | — |
| #76 | AUDIT | Auditoria completa contra `[[AUDITORIAS]]` checklist C++ — gerar `reports/auditoria_<data>.md`, abrir BUGs como itens novos (#NN AUDIT) com `Estado Auditado: ⚠`. **Pré-release: auditoria completa fica para o fim** (baseline 2026-05-13 ✓ em `reports/auditoria_2026_05_13.md`) | Alta | — | Alta | 🟡 Parcial | — |
| #77 | INFRA | ~~Empacotamento Flatpak (`org.peubud.PeuBud`) + AppImage para distribuição cross-distro sem depender de pacotes Qt6 do sistema~~ — cancelado em 2026-05-13 | Baixa | #72 | Alta | ❌ Cancelado | — |
| #78 | DOC | `CHANGELOG.md` formato Keep a Changelog (1.1.0); seções por versão, agrupadas por `Added/Changed/Fixed/Security/Removed`; release tag git | Média | — | Baixa | ✅ Concluído | — |
| #79 | FUNC | Backup automático cifrado do banco — schedule (diário/semanal), retenção configurável (N últimas cópias), destino local ou WebDAV; cifrado com mesma chave libsodium do banco principal | Média | — | Média | ⏳ Pendente | — |
| #80 | FUNC | Atalhos teclado globais — `Ctrl+F` busca global, `Ctrl+,` Configurações, `Ctrl+1..9` mudar aba, `Ctrl+B` bloquear sessão (Ctrl+N novo lançamento contextual fica para follow-up); documentar em Help dialog | Baixa | — | Baixa | ✅ Concluído | — |

---

## Bloco 5 — Aprendizados dos Modelos (auditoria `./models/`)

> Itens derivados de auditoria de `models/` (kmymoney, skrooge, gnucash, grisbi, bot-tesouro-direto, openclaw, Denaro, web-budget) — 2026-05-13. Algoritmos extraídos como domínio público (fórmulas matemáticas), padrões arquiteturais reimplementados limpos em C++20/Qt6. Próximo ID disponível após este bloco: **#96**.

| ID | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| #81 | FUNC | `FinanceEngine` — motor financeiro stateless centralizado (PMT/PV/FV/N/i via Newton-Raphson; 5 frequências capitalização × 5 pagamento; discreta/contínua; início/fim período). Fonte única para Cartões/Calculadoras/Investimentos. Algoritmo de `gnucash/libgnucash/app-utils/calculation/fin.c:1304-1366` (`f(i)=A*(PV+C)+PV+FV` + derivada, guess inicial em 3 ramos). Documentar fórmulas inline com referências eq.1–18. Suporta backend de #19/#54/#55/#56/#69. **Entregue:** 5 TVM básicos (PMT/PV/FV/NPER/RATE), convenção Excel/HP-12C, tratamento i=0, TDD com 8 testes contra valores Excel verificáveis. Frequências múltiplas e discreta/contínua ficam em follow-up | Alta | — | Alta | 🟡 Parcial | ✓ |
| #82 | FUNC | `ParcelaService`/`FinanceEngine`: suporte explícito a **proportionnel** (linear `r/n`) vs **actuariel** (composto `exp(log(1+r)/n)-1`) — cobre modelo BR de consignado (parcela fixa + taxa % saldo) vs SAC/Price atuarial. Algoritmo de `grisbi/src/bet_data_finance.c:73-105`. Crítico para CET correto em #54. **Entregue:** `FinanceEngine::taxaEquivalente` + `taxaProporcional`, 4 testes TDD | Alta | #81 | Média | ✅ Concluído | ✓ |
| #83 | FUNC | `OAuthDeviceFlow` em C++ — implementação RFC 8628 (Device Authorization Grant). POST `/login/device/code` → exibe `user_code` + URL → polling `/login/oauth/access_token` com trato de `authorization_pending`, `slow_down` (+2s), `expired_token`, `access_denied`. Client ID público GitHub Copilot `Iv1.b507a08c87ecfe98`. Token cifrado via libsecret. Base de #64 (GitHub Copilot + OpenAI Codex ChatGPT). Template em `openclaw/extensions/github-copilot/login.ts`. **Entregue:** `OAuthDeviceFlow` (QObject) + struct `OAuthDeviceFlowConfig`; signals `userCodeReceived`, `accessTokenReceived`, `flowError`. Wire-up em UI fica para #64 | Alta | — | Média | ✅ Concluído | — |
| #84 | AUDIT | Refactor `ImportService`: introduzir `IMatchFinder` interface + `MatchResult` enum `{NotFound, Imprecise, Precise, Duplicate}` + janela de dias configurável (`m_matchWindow=4`). Duas implementações: `ExistingTransactionMatchFinder` + `ScheduledTransactionMatchFinder`. Heurísticas: bankId, amount±%, payee, account. Padrão de `kmymoney/converter/transactionmatchfinder.h:22-149` | Média | — | Média | ⏳ Pendente | — |
| #85 | FUNC | Reconciliação automática por knapsack/subset-sum — dado saldo-alvo, achar combinação de lançamentos que bate. DP com bounds `rem_pos`/`rem_neg` + timeout 2–5s + fallback manual. Melhora #27. Algoritmo de `gnucash/libgnucash/app-utils/gnc-autoclear.cpp` | Média | #27 | Alta | ⏳ Pendente | — |
| #86 | INFRA | Provider externo via `QProcess` — wrapping de scripts Python helpers para cotações/câmbio (isola HTTP + parsing de libs externas; atualiza fora do binário). Padrão de `skrooge/skgbankmodeler/skgunitobject.cpp:2292+` (Python) e `gnucash/libgnucash/app-utils/gnc-quotes.cpp` (Perl `Finance::Quote`). Alternativa a `QNetworkAccessManager` puro para providers complexos | Média | #45 | Média | ⏳ Pendente | — |
| #87 | FUNC | `BaseWizardDialog` reutilizável — `QDialog` puro + sidebar de steps + área de página + botões; páginas implementam `nextPage()`/`previousPage()` virtuais (navegação não-linear, mais flexível que `QWizard`). Base de todos os wizards do Bloco 2 (#43, #54, #55, #56, #57, #61, #62). Padrão de `kmymoney/wizards/kmymoneywizard.h`. **Entregue:** classe base com `addStep()`, `setCurrentStep()`, virtuais `validarPasso()`/`aoConcluir()`, signals `concluido`/`cancelado`/`mudouPasso` | Média | — | Média | ✅ Concluído | — |
| #88 | FUNC | `FechamentoMensalService` — snapshot imutável do mês fechado (entradas, gastos, investimentos, patrimônio, saldo); operações `close(periodo)`, `reopen(periodo)`, `simulate(periodo)`. Tabela `fechamentos_mensais` (período, dados serializados JSON, criado_em, fechado_por). Garante histórico consistente após edições retroativas. Padrão de `web-budget/.../ClosingService.java:43-110`. **Entregue:** struct `FechamentoMensal` (POD) + struct `FechamentoResumo` + service com `ensureSchema/simulate/close/reopen/findByPeriod`. Wire-up UI (botão "Fechar mês" no Dashboard) fica para follow-up | Média | — | Média | 🟡 Parcial | ✓ |
| #89 | FUNC | Activity pattern em `OperacaoInvestimentoDialog` — sub-classe por tipo de operação (10: `BuyShares, SellShares, Dividend, ReinvestDividend, Yield, AddShares, RemoveShares, SplitShares, InterestIncome, Bonificacao`); cada uma com `showWidgets()` (campos dinâmicos), `consistencyCheck()`, `type()`. Substitui `if/switch` por dispatch polimórfico. Padrão de `kmymoney/views/investactivities.cpp:360-510` + enum `kmymoney/mymoney/mymoneyenums.h:357-368`. Refactor de #48 | Média | #48 | Média | ⏳ Pendente | — |
| #90 | FUNC | Anexar comprovante (Receipt) por lançamento — campo `BLOB` em `lancamentos_entradas`/`lancamentos_gastos`/`compras` para foto/scan do comprovante. UI: botão "📎 Anexar" + thumbnail + viewer expandido. Útil para IRPF (declaração ano-base). Padrão de `Denaro/.../Transaction.cs:75` | Baixa | — | Média | ⏳ Pendente | — |
| #91 | INFRA | `FileObfuscator` — gerar export anonimizado do banco (substitui nomes de pessoas/empresas/contas por dummies; preserva estrutura/valores) para envio em bug reports. Padrão de `grisbi/src/file_obfuscate.c`. **Entregue:** classe estática com `obfuscate(db)` + `saveToFile()`, mapeamento de 12 tabelas, hash MD5 truncado para descrições livres + dummies sequenciais para nomes. UI viewer (botão Configurações → Diagnóstico) fica para follow-up | Baixa | — | Baixa | ✅ Concluído | — |
| #92 | AUDIT | Trilha de auditoria — tabela `audit_log (id, tabela, op INSERT/UPDATE/DELETE, registro_id, dados_antigos JSON, dados_novos JSON, timestamp, usuario)` populada via SQLite triggers `AFTER INSERT/UPDATE/DELETE` em todas as tabelas financeiras. Viewer em Configurações → Auditoria com filtros por data/tabela/op. Padrão de `web-budget` Hibernate Envers. **Entregue:** `AuditLogService::ensureSchema()` cria tabela + 21 triggers (7 tabelas × 3 ops) via SQLite `json_object()`; `query()` retorna `AuditLogEntry` filtrado. Wire-up em DatabaseManager + UI viewer ficam para follow-up | Baixa | — | Média | 🟡 Parcial | ✓ |
| #93 | FUNC | Import PDF (extrato bancário BR) — `Poppler-qt6` para extrair texto; parsers heurísticos por banco (Itaú, Bradesco, BB, Caixa, Nubank, Inter); cada parser implementa `IBankPDFParser`; fallback a parser genérico tabular. Gap entre 9 formatos atuais e 18 do skrooge | Baixa | #84 | Alta | ⏳ Pendente | — |
| #94 | FUNC | Print/Preview do Dashboard — `QPrinter` + `QPrintPreviewDialog` para imprimir relatório mensal/anual com layout otimizado A4 (header com mês, KPIs, tabela de categorias, gráfico). Reaproveitar layout existente do Dashboard. Padrão de `kmymoney/views/khomeview.cpp:140-160` | Baixa | — | Média | ✅ Concluído | — |
| #95 | FUNC | Debounce 300ms nas calculadoras — `QTimer m_timer` singleShot conectado a `onCalculoDelayed`; campos pesados (TVM/Price/SAC/CET) param de recalcular a cada keystroke, só após 300ms de inatividade. Padrão de `skrooge/.../skgcalculatorpluginwidget.cpp:37-129`. Reduz CPU em wizards complexos do Bloco 2 | Baixa | — | Baixa | ✅ Concluído | — |

---

## Bloco 6 — Distribuição & Empacotamento

> Empacotamento multi-distro para release oficial. Snap explicitamente excluído (decisão do usuário 2026-05-13). Flatpak/AppImage também excluídos (ver #77 ❌). Próximo ID disponível após este bloco: **#101**.

| ID | Grupo | Descrição Técnica | Prioridade | Pré-requisito | Dificuldade | Status | Estado Auditado |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| #96 | INFRA | **CPack: DEB + RPM + TGZ** — CMake-native, fonte única. `set(CPACK_GENERATOR "DEB;RPM;TGZ")`. **DEB**: `CPACK_DEBIAN_PACKAGE_DEPENDS=libqt6core6, libqt6gui6, libqt6widgets6, libqt6charts6, libqt6sql6, libqt6network6, libssl3, libsecret-1-0, libsodium23`; `CPACK_DEBIAN_PACKAGE_SECTION=misc`; `CPACK_DEBIAN_PACKAGE_MAINTAINER`; `CPACK_DEBIAN_PACKAGE_SHLIBDEPS=ON` (resolve deps automaticamente). **RPM**: `CPACK_RPM_PACKAGE_REQUIRES=qt6-qtbase, qt6-qtcharts, openssl-libs, libsecret, libsodium`; `CPACK_RPM_PACKAGE_GROUP=Applications/Office`; `CPACK_RPM_PACKAGE_LICENSE`. **TGZ**: source release via `cmake --build build --target package_source` (CPack source gen) — alternativa: `git archive --format=tar.gz --prefix=peubud-$VERSION/ HEAD`. Auditoria automática: **lintian** (DEB) + **rpmlint** (RPM) — falha CI se warnings críticos | Alta | #72 | Média | ⏳ Pendente | — |
| #97 | INFRA | **AUR PKGBUILD** — `peubud/PKGBUILD` (stable) + `peubud-git/PKGBUILD` (rolling). Campos: `pkgname, pkgver, pkgrel, pkgdesc, arch=('x86_64'), url, license=('GPL3'), depends=('qt6-base' 'qt6-charts' 'openssl' 'libsecret' 'libsodium')`, `makedepends=('cmake' 'gcc')`, `source=("$pkgname-$pkgver.tar.gz::$url/archive/v$pkgver.tar.gz")`, `sha256sums`. Funções `build()` (cmake -B build) + `package()` (DESTDIR install). Auditoria: **namcap** clean. Submissão: `git clone ssh://aur@aur.archlinux.org/peubud.git`, commit, push. Script `scripts/release-aur.sh` automatiza bump `pkgver` + `updpkgsums` + push | Alta | #96 | Média | ⏳ Pendente | — |
| #98 | INFRA | **Integração desktop completa** — pré-requisito para KDE Discover / GNOME Software listar o app. **(1)** `org.peubud.PeuBud.desktop` (`Name`, `Comment` pt-br + en, `Categories=Office;Finance;Qt;`, `Keywords=`, `Exec`, `Icon`, `StartupWMClass`). **(2)** `org.peubud.PeuBud.metainfo.xml` AppStream (release notes por versão, screenshots URL, content_rating, descrição multi-idioma, OARS); validar com `appstreamcli validate`. **(3)** Ícones hicolor PNG `16/22/32/48/64/128/256/512` + scalable SVG; instalar em `/usr/share/icons/hicolor/SIZE/apps/`. **(4)** Man page `peubud.1` em groff; gerar via `help2man` ou manual. **(5)** Tradução `.desktop` via `msgfmt --desktop` consumindo `peubud_*.po` (alinhar com #19). CMake `install(FILES ...)` para cada artefato | Alta | #96 | Média | ⏳ Pendente | — |
| #99 | INFRA | **Assinatura GPG dos releases** — gerar chave RSA-4096 dedicada do projeto (não usar pessoal); `gpg --detach-sign --armor file.deb` → `file.deb.asc`; idem `.rpm`/`.tar.gz`. **(1)** Publicar chave pública em `keyserver.ubuntu.com` + arquivo `KEYS` no repo + fingerprint no README. **(2)** Verificação documentada (`gpg --verify peubud-X.Y.Z.tar.gz.asc peubud-X.Y.Z.tar.gz`). **(3)** Atomic signing no Codeberg Releases (assets `.asc` ao lado de cada binário). **(4)** Repositório RPM/DEB hospedado (#100) usa a mesma chave para metadata signing (`createrepo_c --update` + `gpg` para `repomd.xml.asc`; `reprepro` para repo APT) | Média | #96, #97 | Alta | ⏳ Pendente | — |
| #100 | INFRA | **Codeberg Releases automatizado via Forgejo Actions** — workflow `.forgejo/workflows/release.yml` disparado em `push: tags: ['v*']`. **Matrix**: Fedora 41 / Debian 13 / Arch latest. **Steps**: checkout → install deps via dnf/apt/pacman → `cmake -B build -DCMAKE_BUILD_TYPE=Release` → `cmake --build build -j` → `cd build && cpack -G DEB` (no runner Debian) / `cpack -G RPM` (Fedora) / `cpack -G TGZ` (qualquer); upload artefato. **Submit step**: usa `forgejo-release` action (equivalente do GitHub `softprops/action-gh-release`) para criar release com tag, body do `CHANGELOG.md` (#78), assets `.deb` + `.rpm` + `.tar.gz` + `.asc`. **Side-job AUR**: trigger `release-aur.sh` em runner com chave SSH AUR. **Side-job COPR**: alternativa hosting RPM (Fedora COPR free) — `copr-cli build peubud peubud-X.Y.Z.spec`. Side-job PPA Launchpad (opcional) | Média | #96, #97, #98, #99, #78, #72 | Alta | ⏳ Pendente | — |
| #102 | TESTE | **Bootstrap mínimo de TDD** — `tests/CMakeLists.txt` + `tests/test_canary.cpp` com `QTEST_GUILESS_MAIN` (ou `QTEST_MAIN` quando precisar de event loop); integrar via `enable_testing()` + `add_subdirectory(tests)` no `CMakeLists.txt` raiz; documentar em `tests/README.md` o fluxo Red→Green→Refactor + comando `ctest --test-dir build --output-on-failure`. Sem mocks, sem cobertura — apenas esqueleto para permitir TDD em features novas (ver [[feedback-tdd-sempre-suites-no-fim]]). Suíte completa de regressão fica em #18 (pré-release) | Alta | — | Baixa | ✅ Concluído | ✓ |
| #101 | INFRA | **Repo APT + RPM auto-hostado via Codeberg Pages** — independência de COPR/PPA. **APT (DEB)**: usar **`reprepro`** ou **`aptly`** para gerar estrutura `dists/<codename>/{main}/binary-amd64/{Packages,Packages.gz,Release}` assinada com chave GPG (#99); `conf/distributions` declara codenames (`bookworm`, `trixie`, `noble`, `jammy`); workflow incremental `reprepro -b repo includedeb <codename> *.deb` em cada release. **RPM**: usar **`createrepo_c`** para gerar `repodata/{repomd.xml,primary.xml.gz,filelists.xml.gz,other.xml.gz}` + `gpg --detach-sign --armor repodata/repomd.xml` (#99); `peubud.repo` (`baseurl, enabled=1, gpgcheck=1, gpgkey=...`). **Publicação**: branch `gh-pages` (Codeberg Pages serve em `petrinhu.codeberg.page/orcamento_pessoal/` ou domínio custom); workflow `.forgejo/workflows/pages.yml` faz `git clone gh-pages → reprepro/createrepo_c → commit → push`. **Setup do usuário final**: comandos `curl -s pages-url/KEY.gpg \| sudo tee /etc/apt/keyrings/peubud.gpg` + `echo "deb [signed-by=...] pages-url/apt bookworm main" \| sudo tee /etc/apt/sources.list.d/peubud.list` (DEB); `sudo dnf config-manager --add-repo pages-url/rpm/peubud.repo` (RPM); documentados no README. **Auditoria**: `apt-key list` deprecado — usar `signed-by=` keyring por repo (boa prática Debian 12+) | Baixa | #99, #100 | Alta | ⏳ Pendente | — |

---

## Referências

- Padrões e regras: `CONTRACT.md` (fork local) · `[[CONTRACT]]` (canônico do vault)
- Testes/auditoria: `[[TESTES]]` · `[[AUDITORIAS]]`
- Deploy: `[[DEPLOY_CHECKLIST]]`
- Sprint/INVEST/PDCA: `[[AGILE]]`
- Convenções de código: `.claude/CLAUDE.md` (centavos qint64, juros ×10000, bindValue, PRAGMA FK)
- Segurança histórica (migração #42): conteúdo consolidado neste arquivo — `SECURITY.md` removido após conclusão
