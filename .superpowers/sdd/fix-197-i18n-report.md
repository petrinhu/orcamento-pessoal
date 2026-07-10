# Report — #197 (Onda U-4): prontidão i18n [I18N-01..05]

Fonte: `AUDITORIA_2026-06-22.md` §3.8. Branch `redesign-ui`, em cima de `46356377`.

## Hash do commit

Ver footer do commit `fix(#197): ...` (registrado após este report).

## I18N-01 — moeda locale-naive → locale-aware (fix inteiro, sem `double`)

- **Função central:** `uifmt::formatCentavosBRL(qint64)` (`src/ui/UiFormatters.cpp`) reescrita:
  agrupamento de milhar via `QLocale::toString(qint64)` (pt-BR "1.234" / en-US "1,234"),
  símbolo via `ParcelaService::simboloMoeda(AppSettings::defaultCurrency())`, separador
  decimal e posição do sinal/símbolo por `switch(AppSettings::language())`. **Sem `double`
  em nenhum ponto** — só `qint64`. Nome mantido (compat com os 22 call-sites existentes,
  que agora herdam o fix automaticamente sem precisar tocar cada um).
- **5 duplicatas locais** que reimplementavam o MESMO padrão `"R$ %1%2,%3"` hardcoded
  (perdidas na consolidação do #122) agora delegam para a função central:
  `DashboardWidget`, `LembretesWidget`, `DividasWidget`, `MetasWidget`,
  `ReconciliacaoDialog`. Crítico porque Dashboard é a 1ª tela do usuário.
- **`MainWindow.cpp`**: template de notificação de lembrete tinha `"R$"`+vírgula
  **dentro da própria string traduzível** (pior que hardcoded solto — tradutor não
  consegue corrigir por locale); refatorado para computar o valor fora do `tr()`.
- **3 títulos de eixo Y de gráfico** ("R$" fixo) em `DashboardWidget`/`TendenciasWidget`/
  `InvestimentosWidget` → novo helper `uifmt::simboloMoedaAtiva()`.
- **2 placeholders estáticos** ("R$ 0,00") em `DashboardWidget`/`LancamentoEntradaDialog`
  → `uifmt::formatCentavosBRL(0)`.
- **Fora de escopo (decisão explícita, flag para follow-up):** `ParcelaService::formatarValor`
  é um formatador **separado e independente**, usado ~20× no módulo Cartões/Calculadora/
  Amortização (flagship original do app) — também locale-naive (sem agrupamento de milhar
  nenhum, decimal sempre vírgula) mas **não citado pelo grep do auditor** (que só contou
  `uifmt::formatCentavosBRL`). Fixá-lo é uma superfície de risco maior (módulo mais usado)
  e não estava no work-order — recomendo item novo (`I18N-07`) em vez de expandir
  silenciosamente o escopo de #197.

## I18N-02 — plurais EN vazios

- `lupdate` (target `OrcamentoPessoal_lupdate`, `lupdate-qt6`) rodado — achou **618 strings
  novas** (não só as minhas: o app não rodava lupdate desde 11/06, então isso também
  resgata ~1 mês de features que nunca entraram no pipeline de tradução — efeito colateral
  bom, alinhado com o achado do auditor).
- **3 mensagens `numerus="yes"` com `numerusform` vazio** preenchidas (2 do escopo original
  + 1 bônus achado a 6 linhas de distância, mesmo arquivo/bug):
  - `%n lançamento(s)` → `%n entry` / `%n entries`
  - `%n parcela(s)` → `%n installment` / `%n installments`
  - `%n evento(s) em %1` → `%n event in %1` / `%n events in %1` (Proventos do Dashboard —
    não citado pelo auditor, mas mesma classe de bug, mesmo widget)
- `lrelease` (target `OrcamentoPessoal_lrelease`) recompilado: 1065 traduções finalizadas,
  0 `numerusform` vazio no arquivo inteiro.
- **Gate de pre-commit implementado com escopo ESTREITO** (ver seção própria abaixo).

## I18N-03 — diálogos fora de `tr()`

- `main.cpp`: `QMessageBox::warning` ("Senha incorreta" + corpo) → `QCoreApplication::translate("main", ...)`
  (função livre, sem `Q_OBJECT` — `tr()` não resolve sem contexto explícito).
- `ParcelasWidget.cpp`: dialog "Atualizar parcelas" **inteiro** estava fora de `tr()`
  (título + label + 4 radio buttons) — todos convertidos.
- `HelpDialog.cpp`: título `"Ajuda — PeuBud"` → `tr("Ajuda — %1").arg(QStringLiteral("PeuBud"))`
  ("PeuBud" é marca, fica fora do `tr()` por princípio — não traduzir nomes próprios).

## I18N-04 — glossário hardcoded → `tr()`

- `GlossarioService.h`: `Q_DECLARE_TR_FUNCTIONS(GlossarioService)`.
- `GlossarioService.cpp`: script Python (brace-depth aware, preservando as 92 CHAVES de
  lookup case-insensitive intocadas) converteu **276 `QStringLiteral()`** → `GlossarioService::tr()`
  nos 3 campos (`definicao`/`microcopy`/`coloquial`) de cada uma das 92 entradas. Confirmado
  no `.ts`: contexto `GlossarioService` com 276 `location` apontando pro `.cpp` (gate do
  auditor "`.ts` contém `GlossarioService.cpp`" satisfeito).
- **Seguro por design:** a tabela é `static const QHash` local, computada 1× na 1ª chamada
  — troca de idioma em runtime não re-executa `tr()` (já exige restart, comportamento
  documentado no header).
- **Não traduzido agora (intencional):** as 276 strings entram no pipeline como
  `unfinished` — **não foram traduzidas via MT** (anti-pattern explícito: tradução
  automática sem revisão humana em UI cliente-facing). É um passe de tradução
  profissional separado (glossário financeiro de 92 termos, ~1-3 frases cada). Sem
  regressão: fallback do Qt exibe o texto-fonte (pt-BR) quando não há tradução, igual
  ao comportamento de antes (não ficam em branco).

## I18N-05 — datas `dd/MM/yyyy` hardcoded

- 4 funções novas em `uifmt`: `dataDisplayFormat()` (dd/MM/yyyy × MM/dd/yyyy — ordem
  dia/mês É ambígua entre locales), `dataDisplayFormatCurta()` (sem ano),
  `dataDisplayFormatAnoCurto()` (ano de 2 dígitos), `mesAnoDisplayFormat()` (MM/yyyy —
  não ambíguo, centralizado por consistência).
- Aplicado em **29 pontos / 22 arquivos** batendo exatamente com a contagem do auditor
  (26 pontos/20 arquivos com `"dd/MM/yyyy"` literal + `MainWindow.cpp` `"dd/MM"` (2 pontos)
  + `AddCompraDialog.cpp` `"MM/yyyy"` (1 ponto) = 29/22). Estendido também (mesma classe de
  bug, achada na varredura) a `"dd/MM/yy"` (3 pontos: `EntradasWidget`/`GastosWidget`/
  `InvestimentosWidget`) e `"MM/yyyy"` em `TendenciasWidget` (2 pontos) — 34 pontos/23
  arquivos no total real.
- **Intocado por design:** `ImportService`/`ExportService`/`BankPDFParsers`/`ParcelaService`
  (formato de arquivo/interop e campo de persistência — corretamente fixos, confirmado
  pelo auditor) e o parser multi-formato de `ReconciliacaoDialog::importarExtratoCsv`
  (tenta `yyyy-MM-dd`/`dd/MM/yyyy`/`MM/dd/yyyy`/`dd-MM-yyyy` de propósito — parsing
  flexível, não display).

## Gate de pre-commit (`scripts/pre-commit`) — escopo ESTREITO, decisão justificada

O work-order pedia "Gate opcional: pre-commit falha se `.ts` tiver `unfinished`/numerusform
vazio". Implementei **só a metade `numerusform` vazio**, não o "`unfinished` genérico":

- O app tem **569 strings `unfinished`** agora (backlog real de tradução — a maioria
  pré-existente, resgatada pelo lupdate atrasado desde 11/06, mais as 276 novas do
  glossário). Um gate bloqueando qualquer `unfinished` travaria **todo commit futuro** até
  um passe de tradução profissional completo — não é uma trava razoável para hoje.
- `numerusform` vazio é qualitativamente diferente: produz **string em BRANCO na UI**
  (foi exatamente o bug do #197/I18N-02 original — contadores do Dashboard sumindo em
  EN). `unfinished` comum cai no fallback pt-BR (nunca fica em branco). O gate estreito
  cobre o bug catastrófico sem bloquear o fluxo de tradução progressivo normal.
- Instalado em `.git/hooks/pre-commit` (sincronizado com `scripts/pre-commit`).

**Decisão do líder recomendada:** se quiser o gate `unfinished` completo, é necessário
primeiro fechar um item de tradução profissional do glossário (candidato a item novo,
ex. `I18N-04b`/`I18N-08`) — não recomendo ligar o gate genérico antes disso.

## ⚠️ Incidente durante o TDD — pollução acidental de `~/.config/petrinhu/PeuBud.conf`

Ao escrever `test_locale_moeda.cpp`, a 1ª versão usava `AppSettings::instance()` (mesmo
singleton do app real, `QSettings("petrinhu","PeuBud")`) com save/restore de valor —
padrão já usado em `test_table_skin.cpp`. Descobri tarde demais que
`setDefaultCurrency()` grava uma flag interna `currency_manual` **sem setter público
para desfazer**, e troquei para isolamento via `QSettings::setPath()` + `QTemporaryDir`.

**Só que a 1ª tentativa de isolamento tinha um bug**: `QSettings::setPath(QSettings::IniFormat, ...)`
não redireciona um `QSettings(org, app)` construído sem formato explícito — esse
construtor resolve para `QSettings::NativeFormat`, um enum **diferente** de `IniFormat`
mesmo no Linux (onde os dois são ini-compatíveis por baixo, mas `setPath` distingue os
identificadores). Rodei os testes 1× nesse estado e **gravei de verdade** em
`~/.config/petrinhu/PeuBud.conf`: `language=en-US`, `currency=EUR`, `currency_manual=true`.

**Corrigido:** `QSettings::setPath()` chamado para os DOIS formatos (`IniFormat` +
`NativeFormat`) em `initTestCase()`, confirmado por `stat` (mtime do arquivo real não
muda mais rodando os testes isoladamente).

**Resíduo que NÃO consegui limpar** (sandbox bloqueou escrita fora do repo em arquivo de
config pré-existente do usuário, corretamente): `~/.config/petrinhu/PeuBud.conf` ainda
tem as 3 linhas `language=en-US` / `currency=EUR` / `currency_manual=true` da minha
1ª tentativa. **Se esse não era o idioma/moeda real que você usa no app**, apague essas
3 linhas manualmente (ou rode o app e re-selecione o idioma/moeda em Preferências —
qualquer um dos dois caminhos corrige). O resto do arquivo (tabs, geometria da
calculadora, tabhelp expandido, onboarding) não foi tocado.

## Testes

- `test_locale_moeda.cpp` (9 casos): EN+USD → `"$1,234.56"` (espec do work-order),
  pt-BR+BRL → `"R$ 1.234,56"`, sinal negativo idiomático por locale
  (`-$0.50` en / `R$ -0,50` pt-BR), zero, agrupamento de milhar duplo (`1.234.567,89` /
  `1,234,567.89`), moeda não fica hardcoded em BRL (EUR também funciona).
- `test_locale_datas.cpp` (8 casos): `dataDisplayFormat()`/`Curta()`/`AnoCurto()` nos 2
  idiomas + `mesAnoDisplayFormat()` invariante + prova de integração via
  `QDate::toString()` (25/12/2025 → "25/12/2025" pt-BR / "12/25/2025" en-US).
- Ambos isolados de `~/.config` real via `QSettings::setPath()` + `QTemporaryDir`.

## Build & CI local

- `cmake --build build -j4`: **limpo**, 0 erros, 0 warnings novos.
- `cmake --build build-rel -j4`: **limpo**.
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build`: **66/66 verde** (64 pré-existentes
  + `test_locale_moeda` + `test_locale_datas`, novos).

## Arquivos tocados

Núcleo: `src/ui/UiFormatters.{h,cpp}`, `src/services/GlossarioService.{h,cpp}`,
`src/main.cpp`, `src/ui/MainWindow.cpp`, `src/ui/widgets/ParcelasWidget.cpp`,
`src/ui/widgets/HelpDialog.cpp` + 22 widgets de data + 5 widgets de moeda duplicada +
`tests/CMakeLists.txt` (2 novos targets + 1 fix de link em `test_dashboard_glossario`/
`test_ui_formatters`) + `resources/translations/peubud_en.ts` (lupdate/lrelease) +
`scripts/pre-commit` (gate estreito) + `TODO.md` (#197 → 🔍 Pendente verificação).

Novos: `tests/test_locale_moeda.cpp`, `tests/test_locale_datas.cpp`.
