# tests/ - Bootstrap mínimo de TDD (#102)

Esta pasta hospeda a suíte de testes do PeuBud (ctest + QTest).

> Decisão (2026-05-13): TDD obrigatório em toda feature; suíte completa de regressão (#18) e auditoria full (#76) ficam para o pré-release. Ver memória `feedback-tdd-sempre-suites-no-fim`.

## Habilitar

Build padrão NÃO compila testes. Para habilitar:

```bash
cmake -B build -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

## Adicionar um teste novo

1. Criar `tests/test_<nome>.cpp` no padrão do `test_canary.cpp` (classe `QObject`, slots `private`, `QTEST_GUILESS_MAIN`).
2. Adicionar uma linha em `tests/CMakeLists.txt`:
   ```cmake
   peubud_add_test(test_<nome>)
   ```
3. Reconfigurar (`cmake -B build -DBUILD_TESTING=ON`) e rodar.

## Fluxo TDD (Red → Green → Refactor)

Para qualquer feature ou bugfix com cálculo financeiro, parser, conversão ou regra de negócio:

1. **Red**: escreva o teste com caso conhecido (entrada + saída esperada). Rode `ctest` e veja falhar.
2. **Green**: implemente o mínimo para o teste passar. Sem floreio.
3. **Refactor**: melhore o código mantendo os testes verdes. Repita para próximos casos.

## Variantes de `QTEST_*_MAIN`

| Macro | Quando usar |
| :--- | :--- |
| `QTEST_GUILESS_MAIN` | Testes sem QWidget, sem event loop de GUI. **Padrão.** |
| `QTEST_MAIN` | Testes que precisam de `QApplication` (UI). |
| `QTEST_APPLESS_MAIN` | Testes puros sem nenhum `QCoreApplication`. Raro. |

## Cobertura (futuro)

Cobertura mínima formal vem em #18 (pré-release). Por enquanto, foco em **TDD pragmático**: cada algoritmo financeiro novo vem com pelo menos 1 teste de caso conhecido antes da implementação.

## Convenções

- Nome do arquivo: `test_<modulo>.cpp` (snake_case).
- Nome da classe: `Test<Modulo>` (CamelCase, espelha o módulo testado).
- `QCOMPARE(actual, expected)` sempre nessa ordem.
- Dinheiro em testes: `qint64` em centavos (nunca `double`/`float`); juros como `% × 10000`. Ver `CONTRACT.md` §12.1.
- Helpers compartilhados entre testes: `tests/helpers/<algo>.h` (criar quando aparecer 2º uso, nunca 1º - YAGNI).

## Próximos testes naturais (prioridade)

Quando implementar:
- `#81` FinanceEngine → `test_finance_engine.cpp` (PMT/PV/FV/N + Newton-Raphson contra valores conhecidos de HP-12C).
- `#82` proportionnel vs actuariel → `test_taxa_periodica.cpp`.
- `#83` OAuth Device Flow → `test_oauth_device_flow.cpp` (mock de `QNetworkAccessManager`).
- `#69` dedup CalculadoraService → `test_calculadora_service.cpp` (garantir paridade com lambdas antigas).

## Referências

- `CONTRACT.md` §6.4 (error handling) + §12.1 (C++/Qt).
- `[[TESTES]]` canônico - T1 Testes Unitários.
- `TESTES.md` local - mapa de suítes T1-T15 / A1-A13.
