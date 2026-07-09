# Auditoria Técnica Completa — PeuBud (2026-06-22)

> **Dossiê fix-ready.** Cada achado é uma *work-order* executável por um implementer lendo SÓ este arquivo.
> **Localização por arquivo + âncora de símbolo/snippet greppável — NUNCA por número de linha** (linha apodrece a cada fix).
> Conduzida por Caetano/CTO (plano) + 10 especialistas Sonnet (read-only) + 8 verificadores independentes (2ª passada) + internal-auditor (distribuição). Nenhum código foi alterado; a auditoria é read-only. Consolidação: orquestrador.

## 1. Sumário executivo

**Veredito de prontidão para distribuição (DEB/RPM/AUR/tar.gz): NO-GO.** 5 bloqueadores confirmados.

| Severidade | Qtd |
|---|---|
| 🔴 CRÍTICO | 13 (todos os de dado/precisão/perf **verificados em 2ª passada independente**) |
| 🟠 IMPORTANTE | 27 |
| ⚪ COSMÉTICO | 17 |

**Top riscos (bloqueiam release):**
1. **FIN-01** — Meta/Dívida "sem prazo" (estado padrão da tela) → registro **desaparece silenciosamente**. Repro `sqlite3`. Reabre #146 (fix aplicado em só 2 de 16 repos).
2. **QA-02 + #156** — `CurrencyService`: (a) perde 1 centavo em ~32% dos empates de meio-centavo (double); (b) 9 das 30 moedas do combo não existem no provider → erro de ~288× silencioso. Ambos confirmados (oráculo C++ + curl).
3. **ARQ-02** — `CalculadoraDialog` reimplementa tabela de IR no frontend em meses → **+2,5 p.p. de IR errado** nos valores redondos que o placeholder sugere (6/12/24 meses).
4. **UX-01** — card "Parcelas" do Dashboard perde a contagem real (sobrescrita por rótulo fixo); todo usuário novo, 1ª tela. Repro fim-a-fim.
5. **DIST-03** — banco `data/` em caminho relativo ao CWD + `.desktop` sem `Path=` → quebra 100% dos lançamentos via menu em pacote instalado.
6. **DIST-02** — `AboutDialog` afirma que AGPL-3.0 proíbe uso comercial (falso) e descreve cripto ativa como AES-256-CBC (é o formato legado) — shipado em todo pacote.
7. **DIST-01** — `CPACK_SOURCE_IGNORE_FILES` incompleto → `package_source` vaza 670MB de apps de terceiros (`models/`).

**Prioridade de execução:** ordem interna respeita **Precisão financeira > Robustez > Segurança > UX** + dependências.

## 2. Método de verificação (anti-falso-positivo)

Os 8 CRÍTICOs de dado/precisão/perf passaram por **2ª passada por agente ≠ autor**, que re-derivou do zero e **executou o repro** (sqlite3, oráculo C++ `-O2`, microbench `hyperfine`, `curl`, contagem de query, repro linkando o código de produção). **Resultado: 8/8 CONFIRMADO**, vários reforçados (ARQ-02 achou a divergência real de fronteira; PERF-01 mediu 1200× não 69×; QA-01 rebaixado a *latente* por estar fora do build). Nenhum descartado.

---

## 3. Achados por dimensão (schema work-order)

Cada achado: **Sev · Arquivo · Âncora (símbolo) · Evidência · Impacto · Plano de fix · Teste · Esforço · [Verdito]**.

### 3.1 Precisão & Backend (FIN) — prioridade nº1

**FIN-01 · 🔴 CRÍTICO · [CONFIRMADO — repro sqlite3] · reabre #146**
Arquivo: `MetaRepository.cpp`, `DividaRepository.cpp`, `MetaDialog.cpp`, `MetasWidget.cpp`, `DividaDialog.cpp`, `DividasWidget.cpp`, `DatabaseManager.cpp` (DDL `metas`/`dividas`).
Âncora: `MetaDialog::meta()` (`m.dataPrazo = (date()==minimumDate()) ? QString() : ...`); `MetaRepository::insert()` (`bindValue(":data_prazo", m.dataPrazo)` sem `naoNulo`); coluna `data_prazo TEXT NOT NULL DEFAULT ''`; chamadas `MetaRepository().insert(m)` em `MetasWidget` com retorno descartado.
Evidência: "sem prazo" é o **default** (QDateEdit init em `minimumDate()` + `setSpecialValueText`). `bindValue` envia `NULL` → SQLite rejeita (`NOT NULL constraint failed`) — o `DEFAULT ''` NÃO se aplica a NULL explícito (só a coluna omitida). `insert()` retorna `false`; o widget descarta o bool → diálogo fecha "como se salvou". Repro: `INSERT INTO metas(...) VALUES(...,NULL,...)` → 0 linhas.
Impacto: **perda determinística e silenciosa** da meta/dívida no fluxo mais comum (leigo cria sem mexer na data). O #146 (marcado ✅/auditado) aplicou `naoNulo()` em só **2 de 16 repos** — Meta/Dívida entre os 14 descobertos.
Plano de fix: (1) extrair `naoNulo()` para `RepositoryUtils.h` e aplicar nos campos `TEXT NOT NULL` dos 14 repos restantes; (2) `MetasWidget`/`DividasWidget` param de descartar o bool de `insert()`/`update()` — em falha: `QMessageBox` de erro + não fechar; (3) auditar todo `QDateEdit` com `setSpecialValueText` (padrão do campo opcional que vira `QString()` null) — considerar coluna nullable no schema, como já é feito em `transacoes_recorrentes.data_fim`.
Teste: `tests/test_meta_repository.cpp::insertMetaSemPrazoPersiste` (insert sem prazo → `insert()==true`, `id>0`, `findById` traz o valor). Idem `test_divida_repository.cpp`. Teste parametrizado cobrindo os 16 repos.
Esforço: M.

**FIN-02 · 🟠 IMPORTANTE · durabilidade**
Arquivo: `DatabaseManager.cpp`.
Âncora: `PRAGMA journal_mode = MEMORY` (setup) + `encryptAndSave()`.
Evidência: `journal_mode=MEMORY` (journal em RAM) — crash no meio de transação pode corromper o `.db`. `save()`/`close()` faz `readAll()+cifra+grava` **sem `PRAGMA integrity_check` antes** → pode cifrar dado corrompido por cima do último `.pbd` bom. PRAGMAs com `exec()` sem checar retorno.
Impacto: perda/corrupção de dado financeiro num caminho de saída limpa sobre um `.db` sutilmente corrompido (o fallback de #127c só cobre crash total, não este caminho).
Plano de fix: trocar `journal_mode=MEMORY`→`WAL` (+`synchronous=NORMAL`, combo seguro recomendado pelo SQLite); rodar `integrity_check` antes de `encryptAndSave` e **abortar o save** se falhar (preserva o `.pbd` anterior); checar retorno dos PRAGMAs.
Teste: integration que abre um `.db` truncado e verifica que `save()` recusa persistir.
Esforço: M.

**FIN-03 · 🟠 IMPORTANTE · precisão (prova ausente)**
Arquivo: `FinanceEngine.{h,cpp}`, `ParcelaService.cpp`.
Âncora: `FinanceEngine::pmt`/`pv`/`fv`/`nper`/`rate` (retornam `double` que É o valor monetário, por causa de `(1+i)^n` transcendental); `ParcelaService` isola o `double` e arredonda 1× na saída (padrão correto).
Evidência: matemática transcendental inevitável — o valor monetário é isolado e arredondado uma vez (regra do líder respeitada), **mas falta a prova congelada**. Sweep de 200k amostras (auditoria) achou **0 divergência** no range realista; essa garantia não existe na suíte.
Plano de fix: portar o sweep (oráculo `Decimal` vs `double`, PV até R$100M, taxa 0,05-20% a.m., até 480 parcelas) para `tests/`, documentando o range provado.
Teste: `test_parcela_service_oracle.cpp`.
Esforço: P.

**FIN-04 · 🟠 IMPORTANTE · dinheiro em double (multiplicação simples)**
Arquivo: `CalculadoraDialog.cpp` (Wizards "CLT vs PJ", "Reserva de Emergência").
Âncora: `parseF(edtCLT)` → `double bruto`; `double impPJ = pjFat * aliqPJ`.
Evidência: matemática monetária (multiplicação simples, NÃO transcendental) em `double` — 100% inteirizável. Não persiste, mas é base de decisão financeira real exibida ao usuário. Viola a régua "dinheiro nunca em double no cálculo".
Plano de fix: reescrever os wizards em `qint64` centavos (parse via `uifmt::parseCentavos`), delegando só a taxa a `FiscalService`; `parseF`/`fmtF` só para exibição. (Depende da extração do ARQ-03.)
Teste: `test_calculadora_service` (após extrair a lógica da UI).
Esforço: M.

**FIN-05 · 🟠 IMPORTANTE · rede síncrona trava GUI**
Arquivo: `FrankfurterProvider.cpp`.
Âncora: `fetchSync()` — `QEventLoop loop; loop.exec()` sem `setTransferTimeout`.
Evidência: chamada de rede síncrona bloqueia a GUI thread; sem timeout → congela indefinidamente se o servidor não responder. Chamada durante refresh de `ParcelasWidget`.
Plano de fix: `req.setTransferTimeout(5000)` + mover para thread/assíncrono; usar base `api.frankfurter.dev` direto (evitar depender do redirect 301).
Esforço: P.

**FIN-06 · ⚪ COSMÉTICO · 41 `q.exec()` sem checar retorno em `createSchema()`**
Migrations idempotentes (auto-curam interrupção), mas falha genuína (disco cheio) passa em silêncio → checar cada `exec()` + `qWarning(lastError)` + abortar `open()` se `CREATE TABLE` básica falhar. Esforço P.

### 3.2 Câmbio (FIN/QA) — cluster CurrencyService

**QA-02 · 🔴 CRÍTICO · [CONFIRMADO — oráculo C++] · precisão**
Arquivo: `CurrencyService.cpp`, `tests/test_currency_service.cpp`.
Âncora: `CurrencyService::converter` (`std::round(static_cast<double>(centavos) * taxa(de,para))`); teste `idaVoltaPreservaComArredondamento` (tolerância `±1`).
Evidência: `double` perde 1 centavo em empates de meio-centavo. Oráculo: `38075 × 2,78` → `105848` (deveria ser `105849` half-up). Varredura: **~32% dos empates exatos** (12687/40000) perdem 1 centavo com taxa 2,78. O teste com tolerância `±1` **mascara** o erro.
Impacto: perda silenciosa de centavo em toda conversão de moeda — viola a prioridade nº1.
Plano de fix: eliminar `double` do caminho monetário — capturar a taxa em micro-unidades inteiras (`qint64`, ×1e6, arredondada 1× na borda de parse do JSON) e fazer `centavos * microTaxaPara / microTaxaDe` com widening (`__int128`); arredondamento half-up explícito único no fim.
Teste: `converterEmpateExatoDeCentavoArrendondaHalfUp` (`QCOMPARE(converter(38075,BRL,XXX@2.78), 105849)` — RED hoje).
Esforço: P (teste) + M (fix).

**#156 · 🔴 CRÍTICO · [CONFIRMADO — curl] · valor errado silencioso (já rastreado, GATE 1.0)**
Arquivo: `CurrencyService.cpp` (`taxa`), `PrefsWidget.cpp` (`buildCurrencyList`), `FrankfurterProvider.cpp`.
Âncora: `taxa()` (`m_rates.value(cur, 1.0)`); `buildCurrencyList` (30 moedas).
Evidência: `curl .../v1/currencies` → 9 das 30 moedas do combo (ARS, CLP, COP, PEN, UYU, BOB, PYG, GTQ, RUB) **não existem** no provider → `value(cur, 1.0)` fabrica taxa 1.0 → `converter` retorna inalterado (ex.: BRL→ARS de R$1.500 mostra "150000", ~288× menos). A guarda `rDe<1e-12` é estruturalmente incapaz de pegar moeda ausente.
Plano de fix: `taxa()` lança exceção quando a moeda (origem OU destino) não está no mapa do provider (não confundir "ausente" com "fallback 1.0"); reescrever os testes que pinam o fallback como esperado para esperar a exceção; opcionalmente filtrar o combo às moedas suportadas pelo provider ativo.
Teste: `moedaAusenteLancaExcecao` (inverter os dois testes atuais).
Esforço: M.

### 3.3 Arquitetura (ARQ)

**ARQ-02 · 🔴 CRÍTICO · [CONFIRMADO — divergência +2,5pp] · lógica financeira no frontend**
Arquivo: `CalculadoraDialog.cpp` (Wizard 12 "Comparador RF"), `FiscalService.cpp`, `FinanceEngine.h`.
Âncora: `CalculadoraDialog::buildTabWizards` bloco `ir=0.225; if(meses>24)ir=0.15; else if(meses>12)...`; `FiscalService::irRendaFixaPorDias`.
Evidência: o Wizard 12 reimplementa a tabela de IR regressivo em **meses** (double) em vez de chamar `FiscalService::irRendaFixaPorDias` (em **dias**). O wizard usa `meses<=N`; prazos redondos de calendário ultrapassam o limite em dias reais → 6 meses = 181 dias → canônico 20% vs wizard 22,5% = **+2,5 p.p. de IR errado** (idem 12 e 24 meses), exatamente nos valores que o placeholder "ex: 24" sugere. Distorce o vencedor CDB×LCI e o break-even exibido.
Impacto: número financeiro errado ao usuário (prioridade nº1). Dominó: 7 de 20 wizards reimplementam TVM/composto inline (`std::pow`/`std::log`), mas só o Wizard 12 dá resultado errado; os outros 6 são DRY.
Plano de fix: Wizard 12 chama `FiscalService::irRendaFixaPorDias(dias reais entre datas)` (ou muda o input para dias); substituir `std::pow`/`std::log` inline por `FinanceEngine::fv`/`nper`/`saldoDevedor` (adicionar `saldoDevedor` ao `FinanceEngine`). Capturar o output atual como golden value ANTES de trocar (regressão numérica).
Teste: `test_fiscal.cpp` fronteira 6/12/24 meses vs dias; `test_finance_engine.cpp` para os helpers.
Esforço: M.

**ARQ-01 · 🟠 IMPORTANTE · inversão de camada**
`DatabaseManager` (Foundation) inclui 4 classes concretas Mid/Backend (`FechamentoMensalService`, `AuditLogService`, `ProventoRepository`, `AjusteCorporativoService`) para `ensureSchema()`, enquanto 16 tabelas usam SQL inline — dois padrões inconsistentes. Âncora: bloco de `#include` no topo de `DatabaseManager.cpp` + `createSchema()`. Fix: injeção via `registerSchemaProvider(std::function)` (padrão já usado em `BackupService`, #79); mover os 4 includes+chamadas para um ponto de composição (`main.cpp`/novo `AppBootstrap`). Teste smoke. Esforço P.

**ARQ-03 · 🟠 IMPORTANTE · god-function `buildTabWizards` (~1.388 linhas / arquivo ~2.500)** — viola CONTRACT §6.2. Fix: extrair cada wizard para classe `IWizardPanel`/`WizardX` própria (padrão `WizardGuiadoDialog` #108), fatiável (1-2 por PR, começando pelos 3 do ARQ-02). Esforço G (fatiável).

**ARQ-05 · 🟠 IMPORTANTE · OCP no export** — formato novo exige tocar 3 camadas (dispatch por string em `MainWindow::onExportar` + `ExportService.h`/`.cpp`). Fix: `IExportWriter` (Strategy, espelha `IBankPDFParser` já usado no import); 10 writers finos delegando aos métodos estáticos existentes. Golden-file por formato. Esforço M.

**ARQ-04 · ⚪ COSMÉTICO · `ISecretStore` abstração morta** (nenhum consumidor polimórfico). Fix: (a) remover a interface, OU (b) usá-la p/ `FakeSecretStore` em teste (se houver dor de libsecret/D-Bus em CI). Esforço P.

### 3.4 Testes & QA

**QA-01 · 🔴 CRÍTICO LATENTE · [CONFIRMADO — layout B3] · parser posicional errado (dormente)**
Arquivo: `CotahistProvider.cpp` (fora do build — Bolsa congelada).
Âncora: `parseCotahistLine` — `ultimoStr = line.mid(95, 13)` populando `p.fechamento`.
Evidência: layout B3 COTAHIST — posições 96-108 = **PREMED** (preço médio), 109-121 = **PREULT** (fechamento). `mid(95,13)` pega PREMED. O comentário do código já documenta o campo errado. Sem teste; `grep -c CotahistProvider CMakeLists.txt` = 0.
Impacto: `fechamento` recebe preço médio — corromperia gráficos/indicadores quando a Bolsa for reativada. **Urgência baixa** (dormente); corrigir + testar ANTES de reintroduzir no CMake.
Plano de fix: `line.mid(108, 13)`; extrair parser puro (padrão `BankPDFParsers`) + teste com fixture de linha real.
Teste: `test_cotahist_parser.cpp` (`p.fechamento == PREULT`, não PREMED).
Esforço: M.

**QA-03 · 🟠 IMPORTANTE · `parseCentavos("1.234")` → R$ 1,23 (truncamento 1000×)** — gêmeo em `UiFormatters::parseCentavos` + `ParcelaService::parsearValor`; ramo "só ponto sem vírgula" cai direto no `toDouble`. Sem `QValidator` em ~10 diálogos. Fix: decisão de produto (validator/máscara `0,00` vs heurística de milhar) + teste de caracterização. Teste: `test_ui_formatters.cpp` (novo). Esforço P.

**QA-04 · 🟠 IMPORTANTE · `ParcelaService::dataParaParcela`/`gerarDatas` sem teste** (virada de ano, offset negativo, N=420). Teste: `test_parcela_service.cpp` (3 casos derivados à mão). Esforço P.

**QA-05 · 🟠 IMPORTANTE · providers de mercado sem suíte** (`MarketDataService`/`BrapiProvider`/`OAuthDeviceFlow`, compilados no app). Gate de processo: não fechar #45/#58 sem teste de contrato (JSON fixture + máquina de estados OAuth). Esforço M.

**QA-06 · 🟠 IMPORTANTE · round-trip só provado p/ OFX** — XLSX/CSV sem `export→import→igual`. Teste: `roundTripXlsxPreservaCentavos`. Esforço P.

**QA-07 · 🟠 IMPORTANTE · 6 repos sem teste** (Divida/Meta/Orcamento/Price/Lembrete/Pagador) — CRUD de dinheiro sem prova INSERT→SELECT. Teste: `test_repos_sem_cobertura.cpp`. Esforço M.

### 3.5 Performance

**PERF-01 · 🔴 CRÍTICO · [CONFIRMADO — microbench 1200×] · glossário O(widgets × termos)**
Arquivo: `GlossarioService.cpp`, `GlossarioDecorator.cpp`.
Âncora: `GlossarioService::primeiroTermoEncontrado` (`tabela().keys()` + `std::sort` + `QRegularExpression rx(...)` compilada por termo, por chamada); chamado por widget sobre `findChildren` da MainWindow inteira em `buildUi`.
Evidência: **92 termos** (não 77) × ~centenas de widgets. Medido: ~365ms (55% do `buildUi`). Microbench isolado: **~1200-1700× de speedup** com uma alternation-regex `static`.
Plano de fix: uma única `QRegularExpression` com alternation (`\b(T1|T2|…)\b`), `static` + `optimize()`, lista ordenada por tamanho decrescente calculada 1×; retorno via `captured(1)`.
Teste: `test_glossario_perf.cpp` (1000 chamadas sem match < 5ms). (Sinergia com PERF-02: rodar por-aba reduz o N.)
Esforço: P.

**PERF-03 · 🔴 CRÍTICO · [CONFIRMADO — fórmula M×(4+C)] · N+1 sem teto**
Arquivo: `ResumoService.cpp`, `CompraService.cpp`, `EntradaService.cpp`, `DashboardWidget.cpp`.
Âncora: `resumoDoMes()` (laço `for compra: parcelasDeCompra(c.id)` = N+1; `EntradaService::lancamentos()` = `findAll` sem filtro; `svc.compras()` = `findAll` sem filtro de data); `serie()` (chama `resumoDoMes` por mês, sem cache); `DashboardWidget::atualizarDados` (síncrono no construtor, `serie` M=6).
Evidência: `resumoDoMes` = `4 + C` queries (C = total de compras da vida). `serie` = `M × (4+C)`. Dashboard M=6 no startup → 200 compras = **1224 queries** no boot. Cresce sem teto (C não filtrado por data). `ParcelaRepository::findAll` e `LancamentoEntradaRepository::findByMesAno` existem e são ignorados.
Plano de fix: `serie()` busca compras/parcelas (`ParcelaRepository::findAll`)/lançamentos **1× fora do laço de meses**, agrupa em memória por `(ano,mes)` (`QHash`); `resumoDoMes` vira lookup O(1). Trocar `lancamentos()` por `findByMesAno` no uso mês-a-mês.
Teste: contador de queries — `serie(M=6, C=50)` ≤ 10 queries (hoje 324) + regressão numérica vs `test_resumo`.
Esforço: M (risco financeiro — reteste da suíte T1-T15).

**PERF-02 · 🟠 IMPORTANTE · 13 abas construídas eager no startup** (~285ms + a query storm do PERF-03). Fix: lazy-init por aba via `QMap<chave, factory>`; construir só a aba visível em `navegarParaChave`; glossário passa a rodar por-aba (reduz o N do PERF-01). Mover `setUndoStack`/connects cross-widget para o momento da criação lazy. Teste: budget de startup. Esforço M.

### 3.6 Segurança (núcleo SÓLIDO — 0 CRÍTICO)

> Confirmado: **100% das queries com `bindValue`**; TLS fail-secure (nenhum `ignoreSslErrors`); Argon2id MODERATE ok; nonce AES-GCM 96-bit random com margem de colisão desprezível (análise documentada); `brapiToken` já em libsecret; deps sem CVE crítico. Os gaps são de ciclo de vida.

**SEC-01 · 🟠 IMPORTANTE · senha bruta viva a sessão inteira** — `LoginDialog::onEntrar()` não faz `m_senha->clear()` no sucesso; `m_senha` (senha em claro) vive até o fim do processo (o `sodium_memzero` só cobre a chave derivada). Fix: `m_senha->clear()` após login. Teste: `m_senha->text().isEmpty()` pós-`onEntrar`. Esforço P.

**SEC-02 · 🟠 IMPORTANTE · `data/` CWD-relativo + sem handler de sinal** — ver **DIST-03** (mesma raiz, ângulo de distribuição é o bloqueador). Sinais: instalar handler `SIGTERM`/`SIGINT` (self-pipe, nunca chamar `close()` direto no handler) p/ liberar o `.db`. Esforço M.

**SEC-03 · ⚪ COSMÉTICO · ticker sem encoding na URL** (`BrapiProvider`/`B3DirectQuoteProvider` concatenam `ticker.toUpper()` no path; `m_edtTicker` sem validator). Fix: `QRegularExpressionValidator("[A-Z0-9]{1,12}")` + `QUrl::toPercentEncoding`. Esforço P.

**SEC-04 · ⚪ COSMÉTICO · download COTAHIST sem cap de tamanho/redirect** (`CotahistProvider`, dormente). Fix: `setMaximumRedirectsAllowed(5)` + teto de bytes. Esforço P.

### 3.7 Frontend / Qt (redesign SÓLIDO — 0 CRÍTICO)

**FE-01 · 🟠 IMPORTANTE · callouts de ajuda não seguem o toggle de tema** — `TabShell::montarCallout` usa `setStyleSheet()` inline (avaliado 1× no construtor), que mata o QSS global do `Theme` p/ esses seletores; ao trocar o tema, os 12 callouts ficam presos na cor do boot (risco de contraste/a11y). Fix: remover os 3 `setStyleSheet` inline, deixar o QSS global (`QFrame#helpDescBox` etc.) assumir. Teste: construir em Light, `setMode(Dark)+aplicar`, cor do callout muda. Esforço P.

**FE-03 · 🟠 IMPORTANTE · leak latente em `TabShell::setContent`** — `m_content->setParent(nullptr)` sem `deleteLater()` → leak se `setContent` for reusado (contrato público prevê). Fix: adicionar `m_content->deleteLater()`. Teste: `QPointer` nulo após 2º `setContent`+`processEvents`. Esforço P.

**FE-02 · ⚪ COSMÉTICO · `Theme::bg2()` ghost code** (declarado/definido, zero chamadores; o valor real está hardcoded em 2 lugares). Fix: remover, ou usar `bg2()` nos 2 pontos (DRY). Esforço trivial.

### 3.8 i18n / l10n (infra existe — cobertura furada)

**I18N-01 · 🟠 IMPORTANTE · moeda formatada só em pt-BR** — `UiFormatters::formatCentavosBRL` concatena "R$" + vírgula fixos (24 call-sites), ignorando `AppSettings::language()`; `grep toCurrencyString` = 0. Em modo EN/USD mostra "R$ 1.234,56". Fix (**opção inteira, alinhada com a régua do float**): grouping inteiro via `QLocale::toString(qint64)` + símbolo via `ParcelaService::simboloMoeda` + separador por `switch(language())` — sem `double`. Teste: `test_locale_moeda.cpp` (EN+USD → "$1,234.56"). Esforço M.

**I18N-02 · 🟠 IMPORTANTE · contadores do Dashboard somem em EN** — `peubud_en.ts` tem `%n lançamento(s)`/`%n parcela(s)` com `<numerusform>` **vazio** (lupdate não roda desde 11/06); em EN o `.qm` retorna string vazia → contadores desaparecem. Fix: `lupdate`+traduzir os 2 plurais+`lrelease`. Gate: pre-commit falha se `.ts` tiver `unfinished`/numerusform vazio. Esforço P.

**I18N-04 · 🟠 IMPORTANTE · glossário inteiro fora do pipeline** — `GlossarioService` (~277 `QStringLiteral`, 92 termos) hardcoded; usuário EN vê definições em pt. Fix: `Q_DECLARE_TR_FUNCTIONS` + `tr()` nos 3 campos de cada `Entrada` (seguro: troca de idioma já exige restart) — via script regex + revisão. Gate: `.ts` contém `GlossarioService.cpp`. Esforço G.

**I18N-03 · 🟠 IMPORTANTE · diálogos 100% fora de `tr()`** — `main.cpp` (`QMessageBox "Senha incorreta"`), `ParcelasWidget` (dialog "Atualizar parcelas"), `HelpDialog` (título). Fix: `tr()`/`QCoreApplication::translate` (padrão já usado em ~50 diálogos). Esforço P.

**I18N-05 · 🟠 IMPORTANTE · formato de data `dd/MM/yyyy` hardcoded** em 29 pontos / 22 arquivos de UI (fora ImportService/ExportService, corretamente fixos). Fix: `uifmt::dataDisplayFormat()` = `QLocale.dateFormat` por `language()`. Esforço M.

**I18N-06 · ⚪ COSMÉTICO · placeholder "ex:" pt-BR fora de `tr()`** (AjusteCorporativoDialog/ProventoDialog). Esforço P.

### 3.9 UX/UI (público LEIGO)

**UX-01 · 🔴 CRÍTICO · [CONFIRMADO — repro fim-a-fim] · Dashboard esconde dado real** — ver detalhe: `DashboardWidget::atualizarDados` seta `m_subParcelas` com `%n parcela(s)`; `MainWindow` ctor chama `GlossarioDecorator::aplicarLabelsColoquiais` que casa `\bPARCELA\b` em "PARCELA(S)" e faz `setText("Parcela mensal")`, destruindo a contagem. `modoIniciante` é default → todo usuário novo, 1ª tela. Repro: `'0 parcela(s)' → 'Parcela mensal'`. **Vindica o manual §9.6** ("bug de sobrescrever"). Fix: `setProperty("glossarioIgnore", true)` nos ~8 labels dinâmicos de `makeCard` (o mecanismo já respeita a property); estrutural: labels de conteúdo de `makeCard` marcam `glossarioIgnore` por padrão. Teste: `aplicarLabelsColoquiais` não altera `m_subParcelas->text()`. Esforço P.

**UX-02 · 🟠 IMPORTANTE · combo de provider da Bolsa: jargão cru + status falso** — mostra `none/frankfurter/brapi/b3direct/cotahist` (nomes internos) e **mente**: default "frankfurter" nunca registrado no `MarketDataService` → cai pra brapi em silêncio, mas a UI confirma sucesso falso; sem estado de erro na busca (fica "Buscando…" pra sempre). Fix: reusar `kProviderLabels` do `MarketConfigWidget`; `onProviderChanged` checa o retorno de `setActiveProvider` e mostra erro real; timeout + microcopy de erro. Esforço M.

**UX-03 · 🟠 IMPORTANTE · sidebar "colapsável" não existe** — ver **DIST-12** (#165 falso-done). Decisão do usuário: implementar vs corrigir doc.

**UX-04 · ⚪ COSMÉTICO · rótulo "Contraparte:" (jargão) sem glossário** em DividaDialog. Fix: renomear p/ "Credor / devedor:" ou cadastrar no glossário. Esforço P.

### 3.10 Acessibilidade (WCAG 2.2 AA)

**A11Y-01 · 🟠 IMPORTANTE · botões só-ícone sem nome acessível (sistêmico)** — `grep accessibleName src` = **1 hit** em todo o `/src`. ◀️▶️ (Dashboard), 👁 (login), 🧮 (calculadora, 10 diálogos) mudos p/ Orca. WCAG 4.1.2. Fix: `setAccessibleName(tr(...))` + `setToolTip` em todo `iconButton` (replicar `TabShell` que já faz). Teste: itera botões `icon && text vazio` e falha se `accessibleName` vazio. Esforço M.

**A11Y-02 · 🟠 IMPORTANTE · foco de teclado invisível na sidebar** — `#navItem:focus` só zera `outline`/`border`, sem indicador. WCAG 2.4.7. Fix: `#navItem:focus { background: rgba(255,255,255,0.14); border: 1px solid @@ACC@@ }` (validar contraste sobre navy). Esforço P.

**A11Y-03 · 🟠 IMPORTANTE · features só-mouse** — 👁 hold-to-reveal e 🧮 calculadora com `Qt::NoFocus` → inacessíveis por teclado. WCAG 2.1.1. Fix: `StrongFocus` + equivalente de teclado (Space toggle). Esforço M.

**A11Y-04 · 🟠 IMPORTANTE · `#3B82F6` como texto de botão hover reprova AA** (3.68:1 light / 4.26:1 dark) — mesma falha que o manual §15 já corrigiu p/ blocos de ajuda (`#1D4ED8`), não propagada. Fix: token `@@ACC_TEXT@@` (light `#1D4ED8`, dark `#7FB0FF`) no `:hover`. Esforço P.

**A11Y-05 · 🟠 IMPORTANTE · ⚠️ TOCA REGRA DE OURO — cores de status reprovam contraste como texto** — `positivo()` #16C784 (2.20:1 light), `negativo()` #EA3943 (4.07 light / 3.85 dark), barra `atencao()` pastel (1.11:1). Fix requer tokens de "texto de status" distintos dos de tag → **decisão do líder** (mexe na regra de ouro "negativo sempre #EA3943"). Esforço M.

**A11Y-06 · 🟠 IMPORTANTE · ⚠️ glossário inacessível por teclado/leitor** — `GlossaryLabel` só tem tooltip por mouse (`WhatsThisCursor` não ativa nada). WCAG 2.1.1/1.4.13. Fix: `TabFocus` + `setWhatsThis`/`accessibleDescription` (ou popup em `FocusIn`) → **decisão do líder** (pode poluir a tab order; §9.6 é regra de ouro). Esforço M.

**A11Y-07 · ⚪ COSMÉTICO · cor como único veículo na coluna "%" do Planejamento** (verde/azul/vermelho sem sinal/ícone) — daltônicos perdem a info. WCAG 1.4.1. Fix: sufixo `✓`/`⚠` redundante. Esforço P.

---

## 4. Prontidão de distribuição (DIST) — **NO-GO**

**DIST-03 · 🔴 CRÍTICO · bloqueador · `data/` CWD-relativo** (=SEC-02) — `m_dbPath = "data/.%1.db"` relativo; `.desktop` sem `Path=` → lançamento via menu grava em local imprevisível (`$HOME`/`/`), quebra 100% na 1ª execução do pacote. Fix: `QStandardPaths::AppDataLocation`. Teste: empacotar DEB, instalar em container limpo, `gtk-launch`. Esforço M.

**DIST-01 · 🔴 CRÍTICO · tarball vaza 670MB** — `CPACK_SOURCE_IGNORE_FILES` não exclui `models/` (670M, apps de terceiros), `build-rel/`, `build-verif/`, `.superpowers/`. `package_source` vazaria tudo. Fix: adicionar esses ao ignore. Reabre #96 (Estado Auditado→⚠). Gate: extrair tarball, falhar se >5MB ou contiver `models/`. Esforço P.

**DIST-02 · 🔴 CRÍTICO · AboutDialog legalmente/tecnicamente incorreto** — `lblLic` afirma "AGPL-3.0 ... uso comercial/venda proibidos" (FALSO — AGPL permite comercial, exige só disponibilizar fonte); `lblDesc` diz "AES-256-CBC" (é o formato LEGADO; ativo é AEAD+Argon2id). Shipado em todo pacote. Fix: reescrever com termos reais do AGPL + cripto ativa. Cross-ref compliance-legal. Gate: regex barra "AES-256-CBC" sem "legado" e "proibid"+"comercial" no contexto AGPL. Esforço P.

**DIST-04 · 🟠 IMPORTANTE · Qt6 `GuiPrivate` obrigatório quebra Debian/Arch** (QXlsx usa API privada, exige Qt≥6.9) — CI já removeu o matrix Debian/Arch por isso; #148 está prioridade Baixa, desalinhado com o roadmap de distribuição. Fix: elevar #148; tornar XLSX opcional via `option(PEUBUD_HAS_XLSX)` (padrão do Poppler já usado) ou trocar de lib. Gate: CI matrix Fedora+Debian+Arch verde antes de v1.0. Esforço G.

**DIST-06 · 🟠 IMPORTANTE · 59 commits de UI nunca rodaram CI** — CI só dispara em push/PR contra `main`; a branch `redesign-ui` (59 commits) só passou pelo hook local (sem sanitizers). Fix: abrir PR `redesign-ui→main` p/ disparar CI antes do merge; branch protection no Codeberg. Esforço M.

**DIST-05 · 🟠 IMPORTANTE · licença da fonte Inter (OFL) ausente** — `resources/fonts/` tem licença do Noto mas não da Inter; nenhum aviso de terceiros no About/metainfo (Inter OFL, QXlsx MIT, libsodium ISC). OFL exige o texto acompanhar a redistribuição. Fix: `LICENSE-Inter.txt` + seção "Terceiros". Gate: `scancode-toolkit` em `resources/`. Esforço P.

**DIST-07 · 🟠 IMPORTANTE · clang-tidy desligado por premissa falsa** — o gate está off porque "crasha no GCC16", mas rodou 0-crash hoje (LLVM 22) em 12 arquivos; backlog estimado ~2-3k avisos, incl. `bugprone-easily-swappable-parameters` em `FinanceEngine::taxaEquivalente`/`pmtMultiFreq` (parâmetros financeiros trocáveis — classe sensível). Fix: revalidar; reativar como job CI informativo; triar `bugprone-*` em `Finance*.cpp`/`TvmService.cpp` primeiro. Esforço G.

**DIST-08 · 🟠 IMPORTANTE · cppcheck CI decorativo** (`--error-exitcode=0`) — 1053 achados no repo completo, incl. 155 `ignoredReturnErrorCode` (possíveis `exec()` de SQL ignorados, classe do #146). Fix: triar os 155; promover bugs reais a gate real. Esforço M.

**DIST-09 · 🟠 IMPORTANTE · No PIE + FORTIFY inefetivo** — `checksec build-rel/peubud`: Full RELRO/Canary/NX ok, mas **No PIE** e `Fortified 0/3`. Num app com segredos/dados financeiros descriptografados em memória. Fix: `-fPIE`/`-pie` (ou `CMAKE_POSITION_INDEPENDENT_CODE ON`); investigar propagação do FORTIFY (QXlsx estático?). Gate: `checksec` em CI. Esforço P.

**DIST-10 · ⚪ COSMÉTICO · pipeline pré-release não automatizado** — valgrind/GammaRay/checksec só no terminal do dev; DB do grype desatualizado; trivy não instalado. Fix: `scripts/pre-release-audit.sh` cobrindo os 8 itens, log em `reports/`. Esforço M.

### Meta-achados (tabela de pendências desalinhada do código)

**DIST-11 (=FIN-01) · 🔴 · #146 "concluído falso"** — marcado ✅/auditado, mas `naoNulo()` só em 2 de 16 repos → perda de dado latente em 14. **Reabrir #146** (Status→🔄, Auditado→⚠). Detalhe do fix em FIN-01.

**DIST-12 (=UX-03) · 🔴 · #165 "concluído falso"** — manual §8 + #165 (✅) prometem "sidebar colapsável"; `grep colaps SidebarNav` = 0. **Decisão do líder:** implementar o colapso, OU (se foi corte de escopo) corrigir o manual + #165. Esforço M (implementar) / P (corrigir doc).

---

## 5. Mapa achados → feats (proposta de Onda U — urgente)

> A gravar no `TODO.md` **só após aprovação do líder.** IDs novos a partir de **#189**. CRÍTICO→Prioridade Crítica; IMPORTANTE→Alta/Média; COSMÉTICO→Baixa (onda pós-1.0). Ordem interna: **Precisão > Robustez > Segurança > UX**. Reaberturas (#146, #96, #148, #165) não ganham ID novo.

**Onda U-1 — Precisão financeira (bloqueadores, primeiro):**
- **#146** (reabrir) ← FIN-01/DIST-11 — perda de dado Meta/Dívida (14 repos).
- **#NNN** ← QA-02 — CurrencyService double perde centavo.
- **#156** (já existe, GATE 1.0) ← #156 — moeda desconhecida 288×.
- **#NNN** ← ARQ-02 — Wizard 12 IR errado +2,5pp.
- **#NNN** ← FIN-02 — journal_mode/integrity_check (robustez de dado).

**Onda U-2 — Bloqueadores de distribuição:**
- **#NNN** ← DIST-03/SEC-02 — `data/` CWD-relativo (`QStandardPaths`).
- **#NNN** ← DIST-02 — AboutDialog AGPL/cripto.
- **#96** (reabrir) ← DIST-01 — tarball vaza models/.
- **#165** (reabrir) ← DIST-12/UX-03 — sidebar colapsável (decisão).

**Onda U-3 — Perf + correção visível ao usuário:**
- **#NNN** ← PERF-01 — glossário alternation-regex.
- **#NNN** ← PERF-03 — ResumoService N+1.
- **#NNN** ← UX-01 — Dashboard contagem sobrescrita.

**Onda U-4 — IMPORTANTES agrupados por tema** (i18n; a11y [2 tocam regra de ouro — A11Y-05/06 esperam decisão]; testes QA-03..07; arquitetura ARQ-01/03/05; segurança SEC-01; frontend FE-01/03; distribuição DIST-04..09).

**Pós-1.0 (Baixa):** COSMÉTICOs (FIN-06, ARQ-04, SEC-03/04, FE-02, I18N-06, UX-04, A11Y-07, DIST-10).

---

## Apêndice A — Vereditos de verificação (2ª passada, autor ≠ verificador, repro executado)

| Achado | Verdito | Prova executada |
|---|---|---|
| FIN-01 | ✅ CONFIRMADO | `sqlite3`: `NOT NULL constraint failed`, 0 linhas; `git show` do #146 |
| QA-02 | ✅ CONFIRMADO | oráculo C++ `-O2`: 105848 vs 105849; 32% dos empates divergem |
| #156 | ✅ CONFIRMADO | `curl`: 9 moedas ausentes; trace do fallback 1.0 |
| ARQ-02 | ✅ CONFIRMADO | divergência de fronteira +2,5pp (6/12/24 meses); dominó 7/20 wizards |
| UX-01 | ✅ CONFIRMADO | repro linkando `GlossarioService.cpp` real: `'0 parcela(s)'→'Parcela mensal'` |
| PERF-01 | ✅ CONFIRMADO | microbench `hyperfine`: ~1200-1700× speedup; 92 termos |
| PERF-03 | ✅ CONFIRMADO | fórmula `M×(4+C)` re-derivada; 1224 queries com C=200 |
| QA-01 | ✅ CONFIRMADO (latente) | layout B3 conferido; fora do build (CMake) |

## Apêndice B — Método e limitações

- **Frota:** 10 especialistas (arquitetura, segurança, precisão/backend, QA, performance, frontend/Qt, i18n, UX, a11y, distribuição) + 8 verificadores independentes.
- **Read-only:** nenhum arquivo de código foi alterado; os únicos writes desta auditoria são este dossiê e (após aprovação) `TODO.md`.
- **Não auditado (fora de escopo v1):** RTL (só pt-BR/en-US LTR); teste de carga (app desktop cliente, sem servidor); `AjusteCorporativoService`/`DripSimulator`/`IndicadoresTecnicos` (código não integrado — aplicar a mesma disciplina quando ligados).
- **Régua de severidade:** os "críticos" de i18n/a11y dos relatórios originais foram **normalizados para IMPORTANTE** (qualidade de distribuição/inclusão; EN é opt-in; sem perda de dado). CRÍTICO reservado a perda/corrupção de dado, valor monetário errado, vuln explorável, crash em fluxo principal, ou bloqueador de distribuição — todos com prova.
