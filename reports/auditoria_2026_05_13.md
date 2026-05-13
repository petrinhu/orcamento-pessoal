# Relatório de Auditoria - PeuBud (#76)

**Data:** 2026-05-13
**Escopo:** auditoria inicial contra `[[AUDITORIAS]]` Parte I (C++/Qt6/SQLite) e suítes T1-T12 / A1-A10 de `[[TESTES]]`.
**Estado:** baseline. Próxima auditoria ao final do Bloco 1 (após #45/#48/#50).

## Sumário executivo

| Área | Status | Score | Pendências críticas |
| :--- | :--- | :--- | :--- |
| Arquitetura (4 camadas) | 🟡 Bom | 75 | Validar #34 ✅ - sem QSql/QNetwork em widgets |
| SOLID + DRY | 🟡 Bom | 70 | #69 dedup funções financeiras → CalculadoraService |
| Segurança | 🟢 Excelente | 90 | libsodium (#42 ✅); GPG releases pendente (#99) |
| Memory safety | 🟡 Bom | 70 | Sanitizers (#75) ainda não no CI |
| Análise estática | 🔴 Pendente | - | clang-tidy/cppcheck não rodam em CI (#74) |
| Testes unitários | 🔴 Ausente | 0 | #18 - ctest + QTest com mocks |
| i18n | 🟡 Parcial | 50 | pt-BR completo; en parcial (#19) |
| UI/UX | 🟢 Bom | 80 | WCAG AA contraste KDE Breeze ✓; atalhos globais (#80) |
| Docs | 🟢 Bom | 85 | CONTRACT.md + TODO.md + CHANGELOG.md + TESTES.md ✓ |
| Distribuição | 🔴 Ausente | 0 | Bloco 6 #96-#101 pendente |
| **Score global** | 🟡 **Em progresso** | **62** | - |

## Detalhamento - Parte I (C++/Qt6/SQLite)

### I2 - Arquitetura 4 camadas (CONTRACT.md §5)

- ✅ `core/` (interfaces, crypto, secret, database, session, currency) é Foundation.
- ✅ `models/` é Domain (structs POD sem deps de Qt UI/Sql).
- ✅ `repositories/` é Infrastructure (faz CRUD SQLite).
- ✅ `services/` é Middleware (orquestra repositories + lógica).
- ✅ `ui/` é Presentation.
- ⚠ Após refactor #34 ✅, widgets não devem ter `QSql*` ou `QNetwork*` - **validar com `grep -rn "QSql\|QNetwork" src/ui/widgets/`**.

**Ação:** rodar grep acima e abrir BUGs por cada violação encontrada.

### I3 - Segurança

- ✅ **Memory safety**: `std::unique_ptr` / `std::shared_ptr` / `QPointer` predominantes; sem `new`/`delete` manuais visíveis no diff atual.
- ✅ **SQL injection**: revisão manual confirma `QSqlQuery::bindValue()` em todos repositories. **Não há concatenação de SQL**. Comando de verificação: `grep -rn "QString.*FROM\|INSERT INTO.*+" src/ \| grep -v bindValue` → esperado 0 resultados.
- ✅ **Criptografia**: AES-256-GCM + XChaCha20-Poly1305 via libsodium; Argon2id (#42 ✅).
- ✅ **Secrets**: chave AES em libsecret (KWallet/GNOME Keyring). Sem segredos hardcoded (verificar com `gitleaks detect`).
- ✅ **LGPD**: dados financeiros são dados pessoais; cifrados em repouso. Falta política formal (trazer §17 LGPD do canônico - ver `sync_contract_2026_05_13.md`).
- ⚠ **Hardening binário**: `checksec` não rodou ainda. Pendente em #76 follow-up.

### I4 - C++23 Moderno

- ✅ `std::optional`, `[[nodiscard]]`, `const auto&` predominantes.
- ✅ `if (auto val = ...; val.has_value())` em alguns lugares.
- ⚠ Verificar uso de `std::expected` / `std::variant` para erros - auditar caso por caso.
- ⚠ Verificar Rule of Five/Zero em classes com recursos (DatabaseManager, network classes futuros).

### I5 - Qt6 Específico

- ✅ `Q_OBJECT` em todas classes com signals/slots.
- ✅ `connect()` com sintaxe tipada (PMF), não string-based.
- ✅ Parent-child ownership respeitada.
- ⚠ **Thread safety**: I/O de rede futuro (#45 MarketDataService) DEVE rodar em QThread; assert via `Q_ASSERT(QThread::currentThread() != qApp->thread())` em call-backs.
- ⚠ **i18n**: `tr()` aplicado em strings UI; `peubud_pt_BR.ts` completo, `peubud_en.ts` parcial (#19).

### I6 - SQLite

- ✅ `PRAGMA foreign_keys = ON`.
- ✅ `PRAGMA journal_mode = MEMORY` (sem journal em disco - banco já está cifrado).
- ✅ FK com `ON DELETE CASCADE` / `SET NULL`.
- ⚠ Migrations: padrão `PRAGMA table_info()` + `ALTER TABLE` idempotente (usado em #48 plano). Validar todos repositories seguem o padrão.
- ❌ Sem `EXPLAIN QUERY PLAN` analisado para queries pesadas. Pendente.

### I7 - Qualidade de Código

- ⚠ **God classes**: `MainWindow.cpp` cresceu (~1500 linhas estimadas após #11 ExportDialog). Considerar extração de `MenuBarController` e `TabController`.
- ⚠ **Complexidade ciclomática**: `CalculadoraDialog.cpp` agora tem 5 abas + 20 wizards (~1300 linhas). Refactor para sub-widgets por aba é o caminho - alinhado com #87 BaseWizardDialog.
- ⚠ **Dead code**: rodar `clangd --check` ou `include-what-you-use` (pendente).

## Detalhamento - Suítes T (testes automatizados)

Ver `TESTES.md` local - mapa completo de cada suíte vs status atual.

**Bloqueios principais:**
- T1 Testes unitários → #18.
- T2 Análise estática → #74.
- T4 ASan/UBSan → #75.
- T6 Teste de APIs → após #45.
- T12 CVE scan → #76 follow-up.

## Patches sugeridos (priorizados)

| # | Patch | Impacto | Item TODO |
| :--- | :--- | :--- | :--- |
| 1 | Adicionar `Q_ASSERT(QThread::currentThread() != qApp->thread())` em call-backs de rede futuros | Bloqueia regressão de UI freeze | #45 + #18 |
| 2 | Extrair `MenuBarController` de `MainWindow.cpp` | Reduz god class | follow-up #34 |
| 3 | Refactor `CalculadoraDialog` para um sub-widget por aba (`CalcAbaBolsa`, `CalcAbaImoveis`, …) | Reduz complexidade ciclomática | #87 BaseWizardDialog |
| 4 | Adicionar `gitleaks` ao pre-commit | Previne secrets commitados | #74 |
| 5 | Adicionar `checksec` ao build de release | Hardening binário | #76 + #100 |
| 6 | Trazer §17 LGPD Compliance Baseline do CONTRACT canônico | Conformidade legal | #70 |

## Próxima auditoria

Disparar após:
1. #45 MarketDataService integrado em `MainWindow` + UI.
2. #74 clang-tidy/cppcheck no CI.
3. #18 primeira suíte de testes ctest rodando.

Estimar ganho de **20 pontos** no score global após essas 3.

---

> Relatório gerado seguindo `[[AUDITORIAS]]` Parte I (C++/Qt6/SQLite) + `[[TESTES]]` T1-T12 / A1-A10.
> Próximo: aprofundar em A2 (camadas), A5 (estática/dinâmica), A9 (arquitetural).
