# Fix #146 (Onda U-1, FIN-01) — perda de dado silenciosa em Meta/Dívida

**Branch:** `redesign-ui` · **Base:** `58c487f` · **Hash do fix:** ver `git log -1` após o commit anexado a este report.

## Resumo

`MetaDialog`/`DividaDialog` usam `QDateEdit` + `setSpecialValueText` para os
campos "sem prazo"/"sem vencimento" — o **estado padrão da tela**. Quando o
usuário não mexe no campo (fluxo mais comum, ex.: leigo criando a 1ª meta),
`meta()`/`divida()` devolvem `QString()` (NULL), não `""`. As colunas
`metas.data_prazo`/`dividas.data_venc` são `TEXT NOT NULL DEFAULT ''` —
`bindValue` de um `QString()` null insere `NULL` explícito, o que viola a
constraint (`DEFAULT` só vale quando a coluna é **omitida**, nunca quando
recebe `NULL`). `insert()` retornava `false`; `MetasWidget`/`DividasWidget`
descartavam o bool → o diálogo fechava "como se salvou" e o registro
**desaparecia**.

O #146 original (commit `9c90f77`) aplicou o helper `naoNulo()` em só
**2 de 16 repos** (`GastoAvulsoRepository`, `LancamentoEntradaRepository`) —
Meta e Dívida ficaram de fora, reabrindo o bug (achado FIN-01 da auditoria
2026-06-22, `AUDITORIA_2026-06-22.md` §3.1).

## Fix aplicado

### 1. `naoNulo()` extraído para header compartilhado

`src/repositories/RepositoryUtils.h` — função `inline QString naoNulo(const
QString&)` (null → `""`). `GastoAvulsoRepository.cpp` e
`LancamentoEntradaRepository.cpp` passaram a incluir o header em vez de
duplicar a implementação local (`#146` original tinha 2 cópias idênticas).

### 2. `naoNulo()` aplicado nos 16/16 repos (campos `TEXT NOT NULL`)

| Repo | Campos protegidos | Status antes |
|---|---|---|
| `MetaRepository` | nome, categoria, data_inicio, data_prazo, status, obs | **sem proteção — bug confirmado** |
| `DividaRepository` | tipo, contraparte, descricao, data_inicio, data_venc, status, obs | **sem proteção — bug confirmado** |
| `GastoAvulsoRepository` | (já tinha, migrado pro header compartilhado) | protegido (#146 original) |
| `LancamentoEntradaRepository` | (já tinha, migrado pro header compartilhado) | protegido (#146 original) |
| `TransacaoRecorrenteRepository` | descricao, tipo, fonte, categoria, tipo_pgto, obs, frequencia, data_inicio | **sem proteção — 2º bug confirmado na varredura** |
| `LancamentoInvestimentoRepository` | nome, tipo_ativo, tipo_operacao, categoria, obs, ticker | **sem proteção — 3º bug confirmado na varredura** |
| `CompraRepository` | nome, moeda | defensivo (fluxo atual seguro) |
| `CartaoRepository` | apelido | defensivo |
| `CategoriaRepository` | nome, tipo | defensivo |
| `PagadorRepository` | nome, tipo, obs | defensivo |
| `OrcamentoRepository` | nome, tipo, categoria_ref | defensivo |
| `LembreteRepository` | descricao, data_venc, frequencia, status, obs | defensivo |
| `PriceRepository` | ticker, data, fonte, atualizado (precos_ativos); ticker, timestamp (cotacoes_tempo_real) | defensivo |
| `ProventoRepository` | ticker, tipo, data_evento, observacao | defensivo |
| `FonteRendaRepository` | nome | defensivo |
| `ParcelaRepository` | — (`data_fatura` é coluna **nullable**, já tratada corretamente com `QVariant()`; não mexido, conforme instrução de não forçar coluna nullable) | n/a |

"Defensivo" = varredura não achou fluxo real que produza `QString()` null
hoje (todo campo vem de `QLineEdit::text()`/`currentData().toString()`, que
nunca são null), mas o bind ganhou `naoNulo()` mesmo assim — custo zero
(`naoNulo(x) == x` quando `x` já é non-null) e elimina o risco de regressão
futura silenciosa, conforme pedido explicitamente no work-order (§3.1 FIN-01,
"aplicar nos campos TEXT NOT NULL dos 14 repos restantes").

### 3. Dois bugs-irmãos CONFIRMADOS na varredura dominó (não estavam no work-order, achados por rastreamento de dado)

- **`TransacaoRecorrenteRepository`** — `RecorrenciaDialog::resultado()` faz
  `t.categoria.clear(); t.tipoPgto.clear();` quando `tipo=="entrada"` (e
  `t.fonte.clear()` quando `tipo=="gasto"`). `QString::clear()` torna a
  string **NULL**, não apenas vazia — como um dos dois ramos SEMPRE limpa um
  desses campos `TEXT NOT NULL`, **toda transação recorrente, de qualquer
  tipo**, violava a constraint. Confirmado com teste RED
  (`insertRecorrenteEntradaComCamposClearedPersiste`,
  `insertRecorrenteGastoComFonteClearedPersiste`).
- **`LancamentoInvestimentoRepository`** — `InvestimentoDialog::lancamentos()`
  faz `catNome.clear();` quando o combo de categoria está em "sem categoria"
  (`catId<0`, seleção default/comum). `l.categoria` (`TEXT NOT NULL DEFAULT
  ''`) recebia `NULL` explícito. Confirmado com teste RED
  (`insertInvestimentoSemCategoriaPersiste`).

### 4. `MetasWidget`/`DividasWidget` param de descartar o retorno de `insert()`/`update()`

`onNova`/`onEditar`/`onExcluir`/`onAporte` (Metas) e `onNova`/`onEditar`/
`onExcluir`/`onQuitar`/`onCellDoubleClicked` (Dívidas) agora checam o bool:
em falha, `QMessageBox::warning` com mensagem específica e **não** empilham
no undo stack nem tratam a operação como sucesso (sem remoção/adição
otimista de linha).

### 5. Bug-irmão adicional achado em `MetasWidget` (arquitetura do undo stack)

Ao reestruturar o item 4, encontrei que `MetasWidget::onNova/onEditar/
onExcluir/onAporte` usavam o `LambdaCommand` (`core/commands/UndoCommands.h`)
**invertido** em relação ao contrato documentado no próprio header: o
padrão é "execute a operação real ANTES de `push()`, e o lambda `redo`
passado ao comando só serve para o *redo depois de um undo*" — porque
`QUndoStack::push()` chama `redo()` uma vez imediatamente, e
`LambdaCommand::redo()` **ignora de propósito essa 1ª chamada**
(`m_firstRedo`), assumindo que a operação real já rodou.

`MetasWidget` fazia o oposto: colocava o `MetaRepository().insert(m)` **só**
dentro do lambda passado como `redo` para `push()`. Resultado: na criação
inicial de uma meta (`m_undoStack` sempre setado em produção via
`MainWindow::setUndoStack`), o `insert()` **nunca rodava** — a 1ª chamada de
`redo()` era descartada pelo `m_firstRedo`. Verifiquei empiricamente com um
programa standalone que `QUndoStack::push()` chama `redo()` incondicionalmente
(`before push` / `redo called` / `after push`). `DividasWidget` já seguia o
padrão correto (operação antes do `push()`); só `MetasWidget` estava invertido.

Corrigido: `onNova`/`onEditar`/`onExcluir`/`onAporte` agora executam o
`insert()`/`update()`/`remove()` real primeiro, verificam o retorno, e só
then empilham o `LambdaCommand` (com `redo`/`undo` para os ciclos de
desfazer/refazer subsequentes) — espelhando `EntradasWidget`/`DividasWidget`.

## Testes (RED → GREEN provado)

- `tests/test_meta_repository.cpp` — 3 casos: `insertMetaSemPrazoPersiste`,
  `updateMetaVoltandoParaSemPrazoPersiste`,
  `insertComTodosCamposTextNulosPersiste`.
- `tests/test_divida_repository.cpp` — 3 casos espelhados para Dívida.
- `tests/test_repository_naonulo_domino.cpp` — 3 casos cobrindo os 2 bugs
  irmãos confirmados (recorrência entrada/gasto, investimento sem categoria).

**RED comprovado:** revertidos temporariamente (`git stash`) os 4 repos-alvo
(`MetaRepository`, `DividaRepository`, `TransacaoRecorrenteRepository`,
`LancamentoInvestimentoRepository`) para o estado pré-fix e rodados os 3
executáveis — **9/9 casos falharam** com
`"NOT NULL constraint failed: metas.data_prazo"` /
`"...dividas.data_venc"` / `"...transacoes_recorrentes.categoria"` /
`"...transacoes_recorrentes.fonte"` / `"...lancamentos_investimentos.obs"`
(qWarning idêntico ao repro `sqlite3` da auditoria). Restaurado o fix
(`git stash pop`) → **9/9 GREEN**.

## Build + ctest

- `cmake --build build -j$(nproc)` (Debug) — limpo, 0 warnings novos.
- `cmake --build build-rel -j$(nproc)` (Release) — limpo.
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build` — **52/52 PASS** (49
  testes preexistentes + 3 novos), incluindo os 3 suites deste fix.
- `cppcheck --enable=warning --library=qt --language=c++ --std=c++20` nos 18
  arquivos alterados — **0 achados**.

## Arquivos alterados

**Novo:** `src/repositories/RepositoryUtils.h`

**Repos (naoNulo aplicado/migrado):** `MetaRepository.cpp`,
`DividaRepository.cpp`, `GastoAvulsoRepository.cpp`,
`LancamentoEntradaRepository.cpp`, `TransacaoRecorrenteRepository.cpp`,
`LancamentoInvestimentoRepository.cpp`, `CompraRepository.cpp`,
`CartaoRepository.cpp`, `CategoriaRepository.cpp`, `PagadorRepository.cpp`,
`OrcamentoRepository.cpp`, `LembreteRepository.cpp`, `PriceRepository.cpp`,
`ProventoRepository.cpp`, `FonteRendaRepository.cpp`

**Widgets (checagem de retorno + fix de ordering do undo stack):**
`src/ui/widgets/MetasWidget.cpp`, `src/ui/widgets/DividasWidget.cpp`

**Testes (novos):** `tests/test_meta_repository.cpp`,
`tests/test_divida_repository.cpp`,
`tests/test_repository_naonulo_domino.cpp`, `tests/CMakeLists.txt`

**Pendência:** `TODO.md` (#146 → 🔍 Pendente verificação)

## Residual fora de escopo (não tocado, sinalizado para follow-up)

- `RecorrenciaWidget`/`InvestimentosWidget` ainda descartam o bool de
  `insert()`/`update()` em alguns caminhos (o dado agora persiste
  corretamente após o fix de `naoNulo()`, mas a checagem defensiva de outras
  falhas — disco cheio, etc. — não foi adicionada, por não estar no escopo
  explícito do work-order, que limitou o item 3 a `MetasWidget`/
  `DividasWidget`).
