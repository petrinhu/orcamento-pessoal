# Testes & Auditoria - PeuBud

Este documento aponta para o manual canônico `[[TESTES]]` no vault segundo cérebro
(`/home/petrus/IDrive/Documentos/projetos_claudebrain/TESTES.md`) e adapta as suítes
T1-T15 e A1-A13 ao stack do projeto.

## Pré-requisitos

```bash
# Fedora
sudo dnf install -y \
  cppcheck clang-tools-extra valgrind \
  qt6-qttools-devel \
  gitleaks trivy grype

# Debian/Ubuntu
sudo apt install -y \
  cppcheck clang-tidy valgrind \
  qt6-tools-dev \
  gitleaks
# trivy/grype via instalador oficial (não há pacote estável)

# Arch
sudo pacman -S --needed \
  cppcheck clang valgrind \
  qt6-tools \
  gitleaks trivy grype
```

## Suítes aplicáveis ao PeuBud (C++20/Qt6/SQLite)

| Suíte | Status PeuBud | Comando | Item TODO |
| :--- | :--- | :--- | :--- |
| **T1 - Testes Unitários** | ❌ Pendente | `ctest --test-dir build` (placeholder) | #18 |
| **T2 - Análise Estática** | ❌ Pendente | `cppcheck --enable=all --suppress=missingIncludeSystem src/` + `clang-tidy -p build src/**/*.cpp` | #74 |
| **T3 - Fuzzing de Inputs** | ⏳ Não aplicável (sem inputs externos não-controlados) | - | - |
| **T4 - ASan / UBSan** | ❌ Pendente | `cmake -B build-asan -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g -O1"` | #75 |
| **T5 - Dependency Scan** | ❌ Pendente | `trivy fs --scanners vuln .` ou `grype dir:.` | #76 |
| **T6 - Teste de APIs** | ⏳ Aplicável após #45 (MarketDataService) | testes contra `cotacao.b3.com.br` / `frankfurter.app` / `brapi.dev` (mocks) | #45 + #18 |
| **T7 - Scanning de Binário** | ❌ Pendente | `checksec --file=build/orcamento-pessoal` | #76 |
| **T8 - Verificação de Secrets** | ✅ Aprovado (revisão manual) | `gitleaks detect --source . --no-banner` | - |
| **T9 - Teste de Rede** | ⏳ Aplicável após #45 | timeouts (connect 10s, read 30s), retry backoff exponencial | #45 |
| **T10 - SQL Injection** | ✅ Aprovado (todas queries usam `bindValue`, sem concatenação) | `grep -rn "QString.*FROM\|INSERT INTO.*+" src/ \| grep -v bindValue` | - |
| **T11 - Fuzzing de Protocolos** | ⏳ N/A | - | - |
| **T12 - Busca de CVEs** | ❌ Pendente | `trivy fs --severity HIGH,CRITICAL .` | #76 |
| **A1 - Descoberta e Modelagem** | ✅ Aprovado | Diagrama de camadas em CLAUDE.md e CONTRACT.md §5 | - |
| **A2 - Auditoria de Camadas** | ❌ Pendente | `grep -rn "QSql\|QNetwork" src/ui/widgets/` → deve retornar 0 | #76 |
| **A3 - UI/UX e Acessibilidade** | ⏳ Parcial | contraste KDE Breeze ≥ 4.5:1 ✓; navegação por teclado ⏳ (#80) | #80 + #76 |
| **A4 - QA Geral C++** | ❌ Pendente | revisão manual + checklist em `[[AUDITORIAS]]` Parte I | #76 |
| **A5 - Análise Estática/Dinâmica** | ❌ Pendente | combinação T2 + T4 | #74 + #75 |
| **A6 - Cobertura de Testes** | ❌ Pendente (sem testes) | `gcov` + `lcov --capture` após #18 | #18 |
| **A7 - Dependências e Acoplamento** | ❌ Pendente | `include-what-you-use src/**/*.cpp` | #76 |
| **A8 - Consistência .h vs .cpp** | ❌ Pendente | revisão manual + cppcheck | #76 |
| **A9 - Análise Arquitetural Geral** | ❌ Pendente | revisão contra CONTRACT.md §5 (camadas) e §3 (SOLID) | #76 |
| **A10 - Relatório Final** | ❌ Pendente | gera `reports/auditoria_<data>.md` consolidando T1-T12 + A1-A9 | #76 |

## Critérios de aprovação para release

Antes de qualquer release (`v1.0` em diante), passar:

```
[ ] T1 Testes unitários - 0 falhas, cobertura ≥ 70% em Services
[ ] T2 Análise estática - 0 warnings críticos
[ ] T4 ASan/UBSan - 0 ERROR SUMMARY
[ ] T8 Secrets scan - 0 detectados
[ ] T10 SQL injection - 0 queries concatenadas
[ ] T12 CVE scan - 0 CRITICAL não-patched
[ ] A2 Camadas - 0 violações (widgets sem QSql/QNetwork)
[ ] A10 Relatório de auditoria gerado e revisado
```

Ver também:
- `[[DEPLOY_CHECKLIST]]` - 7 fases de deploy irreversível.
- `[[AUDITORIAS]]` - checklists C++/Python/Web por área.
- `CONTRACT.md` (local) - padrões de código, RFC 2119, regras C++/Qt obrigatórias.
