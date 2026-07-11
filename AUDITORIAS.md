# AUDITORIAS.md — Manual de auditoria do PeuBud

Checklist canônico de auditorias aplicáveis ao stack **C++20 / Qt6 / SQLite / Linux desktop**.
Pareia com [`TESTES.md`](TESTES.md) (o que testar) e [`CONTRACT.md`](CONTRACT.md) (RFC 2119, o que é obrigatório).
Dossiê de auditoria mais recente: [`AUDITORIA_2026-06-22.md`](AUDITORIA_2026-06-22.md) (57 achados, remediados na Onda U).

**Prioridade do projeto:** Precisão financeira > Robustez > Segurança > UX.
**Ferramentas:** APENAS FOSS, disponíveis via dnf/apt/pacman. Nada pago (PVS/Coverity/SonarCloud pago proibidos).
**Regra dura:** dinheiro só em `qint64` (centavos); nenhum `double`/`float` no caminho de cálculo — vírgula só na formatação final.

Cada auditoria roda **antes de qualquer release** (tag de versão). Achado → registra no dossiê (`AUDITORIA_<data>.md`) por severidade **CRÍTICO / IMPORTANTE / COSMÉTICO**, com âncora por símbolo (não por linha, pra não apodrecer), e vira feat de remediação na tabela (`TODO.md`).

---

## A0 · Precisão financeira (a mais crítica)

**Por que é crítica:** é a razão de existir do app; um centavo errado numa base de decisão financeira é inaceitável (prioridade #1 do projeto).

- **Grep de `double`/`float` no caminho de cálculo** — nenhum valor monetário pode transitar em ponto flutuante; só taxa/percentual/contagem (isolados, arredondados 1× na saída via `llround`).
  ```bash
  grep -rnE '\b(double|float)\b' src/services src/models src/repositories | grep -viE 'taxa|aliquota|juros|selic|percent|meses|fração|rate'
  ```
- **Oráculo de precisão congelado** — sweep `long double`/`__int128` vs o caminho de produção, cobrindo o range realista (PV, taxa, nº de parcelas), com tolerância documentada (≤1 centavo). Ver `test_parcela_service_oracle` (#206/FIN-03).
- **Fórmulas financeiras únicas** — PMT/PV/FV/nper/IR sem reimplementação divergente (dedup em `FinanceEngine`/`FiscalService`).

## A1 · Análise estática

**Por que:** pega bugs prováveis (uso não-inicializado, retorno ignorado, leak lógico) sem executar.

- **cppcheck** (gate — categoria `warning`):
  ```bash
  cppcheck --enable=warning --library=qt --language=c++ --std=c++20 \
           --inline-suppr --suppressions-list=.cppcheck-suppressions \
           --error-exitcode=2 --quiet src
  ```
  Backlog amplo (style/performance/portability) = informativo, onda futura de triagem.
- **clang-tidy** (informativo hoje; promover achados reais ao gate):
  ```bash
  clang-tidy -p build $(find src -name '*.cpp')
  ```
- Instalar: `dnf install cppcheck clang-tools-extra` · `apt install cppcheck clang-tidy` · `pacman -S cppcheck clang`.

## A2 · Sanitizers + memória

**Por que:** pega UB e leaks que a análise estática e os testes normais não veem (ex.: o downcast UB do `TableHoverFilter` durante `~QTableWidget`, achado só pelo UBSan).

- **ASan + UBSan** (build dedicado + ctest):
  ```bash
  cmake -B build-sanitize -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
  cmake --build build-sanitize -j$(nproc)
  QT_QPA_PLATFORM=offscreen \
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 \
    UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
    LSAN_OPTIONS="suppressions=$PWD/.lsan-suppress.txt" \
    ctest --test-dir build-sanitize --output-on-failure
  ```
  `.lsan-suppress.txt` cobre leaks conhecidos do Qt (path SEMPRE absoluto — o CWD do teste é `build-sanitize/tests/`).
- **valgrind** (memcheck, complementar em binários sem instrumentação):
  ```bash
  QT_QPA_PLATFORM=offscreen valgrind --leak-check=full --error-exitcode=1 ./build/<binario>
  ```
- Instalar: `dnf install libasan libubsan valgrind` · `apt install ...` · `pacman -S ...`.

## A3 · Segurança / CVE / hardening

**Por que:** app distribuído em binário, com cripto (libsodium) e segredos (libsecret) — superfície real.

- **checksec** — PIE, RELRO, stack canary, FORTIFY no binário final:
  ```bash
  checksec --file=build/<binario>
  ```
  (PIE via `check_pie_supported`/CMP0083 — sem o link-flag `-pie` fica "No PIE".)
- **grype** OU **trivy** — CVE nas dependências (Qt, OpenSSL, libsodium, QXlsx vendorizado):
  ```bash
  grype dir:. ; # ou: trivy fs .
  ```
- **gitleaks** — segredos no histórico/working tree (roda no pre-commit também).
- **Revisão manual AppSec** — senha/chave zeroizada (`sodium_memzero`), sem segredo em QSettings claro, URL com encoding (ticker), sem SQL por concatenação (`bindValue` sempre).

## A4 · QA funcional

**Por que:** garante que a lógica faz o que promete; rede de anti-regressão.

- **ctest** (suíte completa, headless):
  ```bash
  QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure --no-tests=error
  ```
- Testes que exigem ambiente ausente em CI headless (display real / Secret Service) usam `QSKIP` guardado (offscreen / sem D-Bus) — rodam no display do dev, pulam no CI.
- Cobertura significativa (não vaidade); TDD em toda feature (T1 ride com o código).

## A5 · UX/UI

**Por que:** app para **leigos** — clareza e acessibilidade são requisito, não enfeite.

- **Acessibilidade (WCAG 2.2 AA):** `accessibleName` em botões só-ícone, foco de teclado visível, contraste de texto ≥4.5:1 (tokens de texto de status distintos dos de fill), glossário alcançável por teclado/leitor.
- **GammaRay** — inspeção de árvore de widgets / QSS / sinais em runtime.
- **Revisão manual no display real** (KDE Plasma) — render offscreen ≠ compositor real; validar sombras/paint/navegação no app de verdade.
- Manual de estilo canônico: [`docs/manual_estilo.md`](docs/manual_estilo.md).

---

## Fluxo pré-release (ordem)

`A0 precisão → A1 estática → A4 ctest → A2 sanitizers → A3 segurança → A5 UX` → gerar dossiê `AUDITORIA_<data>.md` → remediar por severidade (feats na `TODO.md`) → re-auditar → tag.

Automação: `scripts/pre-release-audit.sh` (DIST-10) cobre A1-A5, degradando com aviso se a tool faltar, log em `reports/`.

Ver também: [[CONTRACT]] · [[TESTES]] · [[DEPLOY_CHECKLIST]] · `AUDITORIA_2026-06-22.md`.
