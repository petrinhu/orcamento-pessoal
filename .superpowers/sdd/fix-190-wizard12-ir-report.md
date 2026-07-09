# Fix #190 — Wizard 12 IR errado (ARQ-02) + dedup FinanceEngine/FiscalService

## Resumo

O Wizard 12 ("Comparador RF — CDB vs LCI/LCA") do `CalculadoraDialog` reimplementava
a tabela de IR regressivo em **meses** em vez de chamar `FiscalService::irRendaFixaPorDias`
(que opera em **dias**). Como investimentos de calendário (6/12/24 meses) têm mais
dias corridos que `30 × meses`, o wizard cruzava a fronteira regressiva errada e
cobrava **+2,5 p.p. de IR a mais** — exatamente nos prazos redondos que o
placeholder ("ex: 24") sugere ao usuário.

## Conversão meses → dias usada

`FiscalService::irRendaFixaPorMeses(double meses)` (nova função, `FiscalService.h/.cpp`):

```cpp
const int dias = static_cast<int>(std::llround(meses * 365.25 / 12.0));
return irRendaFixaPorDias(dias);
```

Fator médio real de dias corridos por mês de calendário: `365,25 / 12 ≈ 30,4375`
(NÃO 30 fixo — 30 flat reproduziria o bug em 6 meses: `6*30=180` cai exatamente
na borda de 22,5%, igual ao valor errado antigo).

## Tabela old → new (Wizard 12)

| Meses | Dias reais (round) | IR antigo (bug) | IR novo (correto) | Δ |
|---|---|---|---|---|
| 6  | 183 | 22,5% | 20,0% | −2,5 p.p. |
| 12 | 365 | 20,0% | 17,5% | −2,5 p.p. |
| 24 | 731 | 17,5% | 15,0% | −2,5 p.p. |

Confirmado por golden-value script (`g++` standalone linkando `FiscalService.cpp`
real) antes de qualquer edição — ver seção Testes.

Também trocado o cálculo de rendimento bruto (`valor*(std::pow(1+i,n)-1)`) por
`FinanceEngine::fv(-valor, 0.0, i, n) - valor` (FV via convenção Excel/HP-12C,
`pv` negativo = saída do bolso; rendimento = FV − valor investido). Golden value
capturado ANTES do dedup (valor=10000, taxa CDB 13,5% a.a., 24 meses):
`old=2882.250000 new=2882.250000` — diff `0.00e+00`.

A fórmula de break-even (`(1+taxaCDB/100)^(1-ir) - 1) × 100`) foi mantida
inalterada — é uma transformação de taxa efetiva pós-IR, não uma reimplementação
de TVM/juros compostos duplicada em `FinanceEngine`.

## Dedup — wizards tocados

Aplicado (golden value idêntico ao original, confirmado por script standalone
ANTES da edição em produção):

| Wizard | Antes (inline) | Depois | Golden old→new |
|---|---|---|---|
| 9 — Saldo devedor | `pv*pot - pmt*(pot-1)/i` (`std::pow` inline) | `FinanceEngine::saldoDevedor(pv,i,n,k)` (nova função) | `43222.855772` → `43222.855772` (diff 0) / caso `i=0`: `7000.0` → `7000.0` (diff 0) |
| 10 — Meta financeira | `std::log(fv/pv)/std::log(1+i)` | `FinanceEngine::nper(-pv, 0.0, fv, i)` | `92.0864587716` → `92.0864587716` (diff 0) |
| 11 — Recálculo amortização (branch "reduzir N") | `-std::log(arg)/std::log(1+i)` + guard manual `arg<=0` | `FinanceEngine::nper(-novoSD, pmtOrig, 0.0, i)` com `try/catch(domain_error)` cobrindo o mesmo guard | `29.7076497179` → `29.7076497179` (diff `7.11e-15`, epsilon de ponto flutuante) |
| 19 — Reserva de emergência (`tempoAcumular`) | `std::ceil(std::log(1+gap*i/guarda)/std::log(1+i))` | `std::ceil(FinanceEngine::nper(0.0, -guarda, gap, i))` | `27` → `27` (diff 0) |

`FinanceEngine::saldoDevedor` (nova função, `FinanceEngine.h/.cpp`): saldo Price
após `k` parcelas de `n` totais, derivando `pmt` internamente via `pmt()`.
Lança `std::domain_error` se `k < 0` ou `k > n` (guard de invariante, não valor
silenciosamente errado).

## Deixado para #200 (risco > benefício aqui)

Wizards **13 (FIRE Number)** e **16 (Coast FIRE)** usam `std::pow(1+i, anos)`
com `anos` vindo de `parseF` (double, potencialmente fracionário — ex.: "17,5"
anos). `FinanceEngine::fv`/`pv`/`pmt`/`nper` exigem `n` como `int`, então o
dedup exigiria `static_cast<int>(anos)`, **truncando silenciosamente** entradas
fracionárias que hoje funcionam corretamente com `std::pow` (aceita expoente
double). Não há ganho de correção (esses 2 wizards já dão resultado certo,
DRY puro) que justifique esse risco sem alterar a assinatura de `FinanceEngine`
para aceitar `n` fracionário — fora do escopo do #190 (que é sobre o bug do
Wizard 12 + dedups seguros). Registrar como candidato a #200 se/quando
`FinanceEngine` ganhar suporte a `n` double.

Wizard 20 (CLT vs PJ) usa `std::pow` em vários pontos (linhas ~2343-2464) mas
não foi listado no ARQ-02 como candidato de TVM/log duplicado — está fora do
escopo (é FIN-04, double em dinheiro, achado diferente).

## Testes (RED→GREEN)

- `tests/test_fiscal.cpp`:
  - `irRendaFixaPorMesesFronteira6_12_24` — fixa os 3 valores corrigidos
    (20,0% / 17,5% / 15,0%) e **verifica explicitamente que os valores antigos
    (22,5% / 20,0% / 17,5%) NÃO são mais retornados** (guarda de regressão do bug).
  - `irRendaFixaPorMesesConsistenteComDias` — para 9 prazos (1 a 60 meses),
    confere que `irRendaFixaPorMeses(m) == irRendaFixaPorDias(round(m*365,25/12))`
    exatamente — trava a conversão como fonte única da verdade.
- `tests/test_finance_engine.cpp`:
  - `saldoDevedorAposKParcelas`, `saldoDevedorTaxaZero`, `saldoDevedorNoInicioIgualPv`,
    `saldoDevedorForaDoIntervaloLanca` (domain_error para `k<0`/`k>n`).
  - `nperEquivaleAMetaFinanceiraWizard10`, `nperEquivaleARecalculoAmortizacaoWizard11`,
    `nperEquivaleATempoAcumuloWizard19` — golden values exatos capturados das
    fórmulas `log()` inline ANTES do dedup, provando equivalência algébrica.

## Build e testes

- `cmake --build build -j$(nproc)` — limpo, sem warning novo (`-Wall -Wextra -Wpedantic`).
- `cmake --build build-rel -j$(nproc)` — limpo.
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build` — **52/52 passed** (100%), 2.41s.
  `test_fiscal`: 47/47 PASS (2 slots novos: fronteira 6/12/24 + consistência
  com dias). `test_finance_engine`: 26/26 PASS (7 slots novos: 4 de
  `saldoDevedor` + 3 golden-value de `nper` para os wizards 10/11/19). 0 FAIL
  em ambos.

## Commit

Hash: `df19a19`

`fix(#190): Wizard 12 IR regressivo usa FiscalService (dias, nao meses) — corrige +2,5pp + dedup FinanceEngine (ARQ-02)`

## Arquivos alterados

- `src/services/FinanceEngine.h` / `.cpp` — nova `saldoDevedor()`.
- `src/services/FiscalService.h` / `.cpp` — nova `irRendaFixaPorMeses()`.
- `src/ui/widgets/CalculadoraDialog.cpp` — Wizards 9, 10, 11, 12, 19.
- `tests/test_finance_engine.cpp`, `tests/test_fiscal.cpp` — testes novos.
