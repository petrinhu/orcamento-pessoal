#!/usr/bin/env bash
# PeuBud — scripts/pre-release-audit.sh (#203a, Onda U, DIST-10)
#
# Automatiza os 8 itens da "Metodologia pré-distribuição" do CLAUDE.md:
#   1. clang-tidy   (análise estática)
#   2. cppcheck     (análise estática)
#   3. ASan+UBSan   (sanitizers)
#   4. valgrind     (memcheck)
#   5. checksec     (hardening do binário)
#   6. grype/trivy  (CVE de dependências)
#   7. ctest        (QA funcional)
#   8. GammaRay     (UX/UI — sempre manual, este script só lembra/abre)
#
# Antes deste script, cada item só rodava manualmente no terminal do dev
# (achado DIST-10 da AUDITORIA_2026-06-22.md §4) — nada ficava registrado,
# e a ausência de uma tool passava despercebida até alguém tentar usá-la.
#
# Filosofia: DEGRADA, NÃO ABORTA. Se uma ferramenta não está instalada, o
# script avisa e segue para a próxima — isto NÃO é o gate de CI (esse já
# existe em .forgejo/workflows/build.yml para cppcheck/clang-tidy/
# sanitizers/ctest, que SÃO bloqueantes lá). Aqui o objetivo é dar ao dev
# uma visão completa antes de cortar uma release, com log persistido em
# reports/ — mesmo rodando parcial (nem toda máquina de dev tem as 6 tools
# nativas instaladas).
#
# Uso:
#   ./scripts/pre-release-audit.sh [--build-dir DIR] [--skip-sanitizers] [--skip-valgrind]
#
# Saída: reports/pre-release-<timestamp>/*.log + resumo no stdout.
# Exit code: 0 se todos os itens executados passaram (SKIP não conta como
# falha); 1 se algum item executado falhou.

set -uo pipefail  # sem -e de propósito: uma etapa falhar não pode abortar as seguintes

# ── Args ─────────────────────────────────────────────────────────────────
BUILD_DIR="build-rel"
RUN_SANITIZERS=1
RUN_VALGRIND=1

while [ $# -gt 0 ]; do
  case "$1" in
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --skip-sanitizers) RUN_SANITIZERS=0; shift ;;
    --skip-valgrind) RUN_VALGRIND=0; shift ;;
    -h|--help)
      grep -E '^#( |$)' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *) echo "Argumento desconhecido: $1" >&2; exit 2 ;;
  esac
done

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || { cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd; })"
cd "$ROOT" || exit 1

TS="$(date +%Y%m%d-%H%M%S)"
REPORT_DIR="reports/pre-release-$TS"
mkdir -p "$REPORT_DIR"

APP_BIN="$BUILD_DIR/peubud"
# Cuidado OOM (CLAUDE.md): nunca -j maior que 2 em builds paralelos deste script.
JOBS=2

PASS=0
FAIL=0
SKIP=0
SUMMARY=()

section() {
  echo ""
  echo "════════════════════════════════════════════════════════════════"
  echo "▶ $1"
  echo "════════════════════════════════════════════════════════════════"
}

record_pass() { PASS=$((PASS+1)); SUMMARY+=("✓ PASS  — $1"); echo "✓ $1"; }
record_fail() { FAIL=$((FAIL+1)); SUMMARY+=("✗ FAIL  — $1 (log: $2)"); echo "✗ $1 — ver $2"; }
record_skip() { SKIP=$((SKIP+1)); SUMMARY+=("⚠ SKIP  — $1: $2"); echo "⚠ SKIP: $1 — $2"; }

need() { command -v "$1" >/dev/null 2>&1; }

# ── 1/8 clang-tidy ───────────────────────────────────────────────────────
section "1/8 clang-tidy — análise estática (bugprone/*, easily-swappable-parameters etc.)"
if need clang-tidy; then
  if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    LOG="$REPORT_DIR/clang-tidy.log"
    # Informativo por padrão (mesma decisão do CI, DIST-07): backlog de
    # ~2-3k avisos nunca triado. Erros de verdade (não warnings) falham o item.
    mapfile -t CPP_FILES < <(find src -name '*.cpp')
    clang-tidy -p "$BUILD_DIR" "${CPP_FILES[@]}" > "$LOG" 2>&1
    # grep -c sai 1 (sem "falhar") quando a contagem é 0 mas AINDA imprime
    # "0" no stdout — usar `|| echo 0` aqui duplicaria a saída ("0\n0").
    WARNS=$(grep -c "warning:" "$LOG" 2>/dev/null); WARNS="${WARNS:-0}"
    ERRS=$(grep -c "error:" "$LOG" 2>/dev/null); ERRS="${ERRS:-0}"
    echo "  $WARNS aviso(s), $ERRS erro(s) — $LOG"
    if [ "${ERRS:-0}" -gt 0 ]; then
      record_fail "clang-tidy ($ERRS erro(s) real(is))" "$LOG"
    else
      record_pass "clang-tidy ($WARNS aviso(s) informativo — backlog DIST-07)"
    fi
  else
    record_skip "clang-tidy" "compile_commands.json ausente em $BUILD_DIR — rode: cmake -B $BUILD_DIR -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Debug"
  fi
else
  record_skip "clang-tidy" "não instalado — dnf install clang-tools-extra / apt install clang-tidy / pacman -S clang-tools-extra"
fi

# ── 2/8 cppcheck ─────────────────────────────────────────────────────────
section "2/8 cppcheck — análise estática (mesma config do pre-commit hook / CI)"
if need cppcheck; then
  LOG="$REPORT_DIR/cppcheck.log"
  cppcheck --enable=warning \
           --library=qt \
           --language=c++ --std=c++20 \
           --inline-suppr \
           --suppressions-list=.cppcheck-suppressions \
           --quiet \
           src > /dev/null 2> "$LOG"
  N="$(wc -l < "$LOG" | tr -d ' ')"
  if [ "$N" -gt 0 ]; then
    record_fail "cppcheck ($N linha(s) de achado — categoria warning)" "$LOG"
  else
    record_pass "cppcheck limpo (categoria warning)"
  fi
else
  record_skip "cppcheck" "não instalado — dnf install cppcheck / apt install cppcheck / pacman -S cppcheck"
fi

# ── 3/8 Sanitizers (ASan+UBSan) ──────────────────────────────────────────
section "3/8 ASan + UBSan — build debug instrumentado + ctest"
if [ "$RUN_SANITIZERS" -eq 1 ]; then
  if need cmake && need make; then
    SAN_DIR="build-sanitize"
    LOG="$REPORT_DIR/sanitizers.log"
    {
      cmake -B "$SAN_DIR" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_TESTING=ON \
        -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
        -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
      && cmake --build "$SAN_DIR" -j"$JOBS" \
      && QT_QPA_PLATFORM=offscreen \
         ASAN_OPTIONS="detect_leaks=1:halt_on_error=1:abort_on_error=1" \
         UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1" \
         LSAN_OPTIONS="suppressions=$ROOT/.lsan-suppress.txt" \
         ctest --test-dir "$SAN_DIR" --output-on-failure --no-tests=error
    } > "$LOG" 2>&1
    RC=$?
    if [ "$RC" -eq 0 ]; then
      record_pass "ASan/UBSan (ctest verde sob instrumentação)"
    else
      record_fail "ASan/UBSan (build ou ctest falhou, rc=$RC)" "$LOG"
    fi
  else
    record_skip "ASan/UBSan" "cmake/make ausentes"
  fi
else
  record_skip "ASan/UBSan" "--skip-sanitizers passado"
fi

# ── 4/8 valgrind (memcheck) ──────────────────────────────────────────────
section "4/8 valgrind — memcheck sobre os binários de teste"
if [ "$RUN_VALGRIND" -eq 1 ]; then
  if need valgrind; then
    if [ -d "$BUILD_DIR/tests" ]; then
      LOG="$REPORT_DIR/valgrind.log"
      : > "$LOG"
      VG_FAIL=0
      VG_COUNT=0
      # Amostra: valgrind é lento demais para rodar a suíte inteira em CI
      # local a cada release; roda sobre os binários de teste presentes no
      # build (cada um já cobre uma fatia do domínio). Ajustar a lista
      # abaixo se algum binário crítico precisar de checagem dedicada.
      for bin in "$BUILD_DIR"/tests/test_*; do
        [ -x "$bin" ] || continue
        VG_COUNT=$((VG_COUNT+1))
        echo "── $(basename "$bin") ──" >> "$LOG"
        QT_QPA_PLATFORM=offscreen valgrind \
          --leak-check=summary --error-exitcode=99 --quiet \
          "$bin" >> "$LOG" 2>&1
        RC=$?
        if [ "$RC" -eq 99 ]; then
          VG_FAIL=$((VG_FAIL+1))
          echo "  ✗ erro de memória detectado (rc=$RC)" >> "$LOG"
        fi
      done
      if [ "$VG_COUNT" -eq 0 ]; then
        record_skip "valgrind" "nenhum binário de teste em $BUILD_DIR/tests — builde com -DBUILD_TESTING=ON antes"
      elif [ "$VG_FAIL" -gt 0 ]; then
        record_fail "valgrind ($VG_FAIL/$VG_COUNT binário(s) com erro de memória)" "$LOG"
      else
        record_pass "valgrind ($VG_COUNT binário(s), sem erro de memória)"
      fi
    else
      record_skip "valgrind" "$BUILD_DIR/tests ausente — builde com -DBUILD_TESTING=ON antes"
    fi
  else
    record_skip "valgrind" "não instalado — dnf install valgrind / apt install valgrind / pacman -S valgrind"
  fi
else
  record_skip "valgrind" "--skip-valgrind passado"
fi

# ── 5/8 checksec ─────────────────────────────────────────────────────────
section "5/8 checksec — hardening do binário (PIE/RELRO/Canary/FORTIFY/NX)"
if need checksec; then
  if [ -x "$APP_BIN" ]; then
    LOG="$REPORT_DIR/checksec.log"
    checksec --file="$APP_BIN" > "$LOG" 2>&1
    cat "$LOG"
    # checksec não tem exit-code padronizado de pass/fail; achado real
    # (DIST-09) é PIE/FORTIFY — grep no output, informativo aqui (o gate
    # de verdade é o CI, que já cobre PIE via #202).
    if grep -qi "No PIE" "$LOG"; then
      record_fail "checksec (binário sem PIE — ver DIST-09/#202)" "$LOG"
    else
      record_pass "checksec (PIE ok — ver $LOG p/ RELRO/Canary/FORTIFY completos)"
    fi
  else
    record_skip "checksec" "$APP_BIN ausente — rode: cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=Release && cmake --build $BUILD_DIR -j$JOBS"
  fi
else
  record_skip "checksec" "não instalado — dnf install checksec / apt install checksec / pacman -S checksec"
fi

# ── 6/8 grype OU trivy (CVE de dependências) ─────────────────────────────
section "6/8 grype/trivy — CVE de dependências (SBOM-less scan do repositório)"
# models/ (apps de terceiros só de referência, nunca versionados — DIST-01)
# e os diretórios de build ficam de fora: são ruído (CVE de projetos
# terceiros/artefatos gerados, não do PeuBud) e inflam o achado sem sinal
# acionável.
GRYPE_EXCLUDE=(--exclude './models/**' --exclude './build*/**' --exclude './.git/**')
if need grype; then
  LOG="$REPORT_DIR/grype.log"
  grype dir:. "${GRYPE_EXCLUDE[@]}" --quiet -o table > "$LOG" 2>&1
  RC=$?
  CRIT="$(grep -ic 'Critical' "$LOG" 2>/dev/null)"; CRIT="${CRIT:-0}"
  if [ "$RC" -ne 0 ]; then
    record_fail "grype (rc=$RC — DB pode estar desatualizada, rode: grype db update)" "$LOG"
  elif [ "$CRIT" -gt 0 ]; then
    record_fail "grype ($CRIT achado(s) Critical)" "$LOG"
  else
    record_pass "grype (sem CVE Critical — ver $LOG p/ severidades menores)"
  fi
elif need trivy; then
  LOG="$REPORT_DIR/trivy.log"
  trivy fs --scanners vuln --severity CRITICAL,HIGH --quiet \
       --skip-dirs models --skip-dirs 'build*' . > "$LOG" 2>&1
  RC=$?
  if [ "$RC" -ne 0 ]; then
    record_fail "trivy (rc=$RC)" "$LOG"
  else
    record_pass "trivy (fs scan concluído — ver $LOG)"
  fi
else
  record_skip "grype/trivy" "nenhum instalado — grype: https://github.com/anchore/grype#installation (script oficial, sem pacote dnf/apt/pacman nativo) / apt install trivy (Debian tem pacote; Fedora/Arch via binário oficial)"
fi

# ── 7/8 ctest (QA funcional) ─────────────────────────────────────────────
section "7/8 ctest — QA funcional (build normal, não instrumentado)"
if need ctest; then
  if [ -d "$BUILD_DIR" ]; then
    LOG="$REPORT_DIR/ctest.log"
    QT_QPA_PLATFORM=offscreen ctest --test-dir "$BUILD_DIR" --output-on-failure --no-tests=error > "$LOG" 2>&1
    RC=$?
    TOTAL_LINE="$(grep -E '^[0-9]+% tests passed' "$LOG" | tail -1)"
    if [ "$RC" -eq 0 ]; then
      record_pass "ctest verde ${TOTAL_LINE:+— $TOTAL_LINE}"
    else
      record_fail "ctest (rc=$RC) ${TOTAL_LINE:+— $TOTAL_LINE}" "$LOG"
    fi
  else
    record_skip "ctest" "$BUILD_DIR ausente — rode: cmake -B $BUILD_DIR -DBUILD_TESTING=ON && cmake --build $BUILD_DIR -j$JOBS"
  fi
else
  record_skip "ctest" "não instalado — vem com o pacote cmake (dnf/apt/pacman install cmake)"
fi

# ── 8/8 GammaRay (UX/UI — sempre manual) ─────────────────────────────────
section "8/8 GammaRay — revisão manual de UX/UI"
if need gammaray; then
  echo "  GammaRay instalado. Revisão de UX/UI é manual por natureza (inspeção"
  echo "  visual de layout/foco/acessibilidade) — este script NÃO automatiza,"
  echo "  só lembra. Para inspecionar o app rodando:"
  echo ""
  echo "    gammaray $APP_BIN"
  echo ""
  record_skip "GammaRay" "revisão manual — rode 'gammaray $APP_BIN' e inspecione (não automatizável, item 5 do checklist CLAUDE.md)"
else
  record_skip "GammaRay" "não instalado — dnf install gammaray-qt6 / apt install gammaray / pacman -S gammaray"
fi

# ── Resumo ────────────────────────────────────────────────────────────────
section "Resumo — reports/pre-release-$TS/"
for line in "${SUMMARY[@]}"; do echo "  $line"; done
echo ""
echo "PASS=$PASS  FAIL=$FAIL  SKIP=$SKIP"
echo "Logs em: $REPORT_DIR/"

if [ "$FAIL" -gt 0 ]; then
  exit 1
fi
exit 0
