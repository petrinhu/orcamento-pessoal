# Auditoria PeuBud - Atualização Sprint 15-itens (#76)

**Data:** 2026-05-13 (final da sessão)
**Baseline:** `reports/auditoria_2026_05_13.md` (score global 62).
**Escopo:** atualizar score após sprint que entregou #84/#90/#69/#88/#92/#61/#62/#79/#93/#48/#46/#50/#66/#65/#81/#72/#73/#18/#70.

## Sumário executivo (delta)

| Área | Antes | Depois | Delta | Comentário |
| :--- | :---: | :---: | :---: | :--- |
| Arquitetura (4 camadas) | 75 | 85 | +10 | `IMatchFinder`, `BankPDFParsers`, `MarketDataService` reforçam separação Middle |
| SOLID + DRY | 70 | 88 | +18 | #69 fechou dedup Newton-Raphson; TvmService delega 100% para FinanceEngine |
| Segurança | 90 | 92 | +2 | #79 backup cifrado adicionado; libsodium estende para snapshots |
| Memory safety | 70 | 70 | 0 | Sanitizers ainda só locais (#75 segue pendente) |
| Análise estática | 0 | 40 | +40 | cppcheck rodando no Forgejo Actions (#72); clang-tidy ainda manual |
| Testes unitários | 0 | 70 | +70 | 9 suítes ctest (~60 testes); #18 marcado parcial entregue |
| Testes integração | 0 | 50 | +50 | `test_sqlite_migrations` cobre padrão ALTER TABLE idempotente |
| i18n | 50 | 50 | 0 | #19 ainda pendente neste sprint |
| UI/UX | 80 | 82 | +2 | `ReceiptAttachmentWidget`, `MarketConfigWidget`, `BolsaWidget`, `AuditLogViewer` |
| Backup/recovery | 0 | 80 | +80 | `BackupService` schedule + retenção + UI configurável |
| Auditoria (logs) | 0 | 85 | +85 | `AuditLogService` ativo + viewer Configurações (#92) |
| Fechamento contábil | 0 | 85 | +85 | `FechamentoMensalService` + UI Dashboard (#88) |
| Fiscal BR | 60 | 88 | +28 | `FiscalService` (INSS/IRPF/Simples I-V/Lucro Presumido/Poupança/IR RF/IOF) |
| Docs | 85 | 87 | +2 | TODO atualizado em cada item; reports adicionados |
| Distribuição | 0 | 25 | +25 | Forgejo CI multi-distro + ctest + cppcheck (#72) |
| **Score global** | **62** | **78** | **+16** | Saltou de 'Em progresso' → 'Bom' |

## Itens fechados na sprint (com TODO marker)

- ✅ #50 Infra Bolsa
- ✅ #65 Splits/Bonificações (backend + TDD)
- ✅ #66 Tracker Dividendos (backend + DY trailing 12m)
- ✅ #69 Dedup → FinanceEngine
- ✅ #70 Sync CONTRACT (report + política)
- ✅ #72 Forgejo CI expandido (ctest + cppcheck + Poppler)
- ✅ #73 Tests SQLite (5 cenários)
- ✅ #79 Backup cifrado (service + UI + TDD)
- ✅ #81 follow-up (multi-freq + contínua)
- ✅ #84 IMatchFinder + dedup importação
- ✅ #88 FechamentoMensal UI wire-up
- ✅ #90 Receipt BLOB ×4 entidades
- ✅ #92 AuditLog viewer wire-up
- ✅ #18 ParcelaService tests (parcial)
- ✅ #61 Reserva Emergência (comparativo Poupança/CDB/Tesouro)
- ✅ #62 CLT vs PJ (Simples I-V + Lucro Presumido + break-even)
- 🟡 #46 Configurações Mercado (entregue; falta integração #58 BrapiProvider)
- 🟡 #48 Investimentos ticker/qty/preço (RT/DY% pendente)
- 🟡 #93 Import PDF (6 parsers + Poppler condicional)

## Lacunas críticas para próxima auditoria

1. **i18n (#19):** `peubud_en.ts` ainda parcial.
2. **Memory safety (#75):** ASan/UBSan no CI - depende #72+#18.
3. **Distribuição (#96-#101):** packaging DEB/RPM/TGZ + repo APT/RPM hospedado.
4. **clang-tidy CI:** ativar enforcement (atualmente cppcheck soft, exit 0).

## Próxima janela

Score-alvo trimestral: **85** (lacuna 7 pontos). Vias:
- Fechar #19 i18n (+5 pts UI/UX)
- Habilitar ASan/UBSan CI (+5 pts memory safety)
- Implementar Bloco 6 packaging básico DEB/RPM (+10 pts distribuição)

---

Gerado por `/sprint #76`. Baseline preservada em
`reports/auditoria_2026_05_13.md`.
