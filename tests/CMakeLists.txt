# PeuBud - bootstrap de testes (#102)
# Cada teste vira um executable separado. Use peubud_add_test() para adicionar.

# #195 (Onda U, PERF-03): sqlite3_trace_v2 para contar queries reais na
# conexao do QSQLITE (nao mock) -- usado por test_resumo para travar o teto
# de queries de ResumoService::serie(). PkgConfig ja resolvido no CMakeLists raiz.
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Helper: adiciona executable de teste + registra no ctest.
# Uso: peubud_add_test(test_nome [arquivo_extra1.cpp arquivo_extra2.cpp ...])
function(peubud_add_test test_name)
    set(test_sources "${test_name}.cpp" ${ARGN})
    add_executable(${test_name} ${test_sources})
    target_link_libraries(${test_name} PRIVATE
        Qt6::Core
        Qt6::Test
        Qt6::Sql
        Qt6::Network
        Qt6::Widgets
    )
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    target_compile_options(${test_name} PRIVATE
        -Wall -Wextra -Wpedantic
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# ── Testes registrados ───────────────────────────────────────────────────────
peubud_add_test(test_canary)
peubud_add_test(test_finance_engine ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)

# #178: garantia de pasta antes de save/autosave — logica pura de QDir::mkpath
# (ramo GUI/QMessageBox nao testavel headless; cobrimos o contrato de criacao).
peubud_add_test(test_dir_guard)

# #181: pastas de save fixas ~/peubud/save/<slug>/database[.auto].pbd
# Testa: formato do caminho fixo, mkpath da pasta, logica de migracao
# (cofre legado data/<slug>.pbd → novo destino + .bak, idempotencia,
# novo usuario sem cofre, atomicidade do .new).
# Depende apenas de Qt::Core + Qt::Test — sem DB, cripto ou rede.
peubud_add_test(test_pastas_fixas)

# #192 (Onda U, DIST-03/SEC-02): .db de trabalho em local FIXO
# (QStandardPaths::AppDataLocation), nao mais "data/" relativo ao CWD do
# processo (quebrava 100% do lancamento via .desktop, CWD imprevisivel).
# AppSettings::brapiToken() usa SecretStoreService (libsecret); ParcelaService
# arrasta FinanceEngine.
peubud_add_test(test_working_db_path
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_working_db_path PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_working_db_path PRIVATE ${LIBSECRET_INCLUDE_DIRS})

# #192 (Onda U, SEC-02): handler de SIGTERM/SIGINT via self-pipe -- prova que
# o handler POSIX (so write(), async-signal-safe) acorda o event loop do Qt
# via QSocketNotifier e emite terminationRequested(); a limpeza real (fechar/
# cifrar o banco) roda la, nunca dentro do handler de sinal.
peubud_add_test(test_unix_signal_handler
    ${CMAKE_SOURCE_DIR}/src/core/platform/UnixSignalHandler.cpp)

# #132: CalculadoraService — funcoes puras. FIN-04 (auditoria 2026-06-22
# §3.1) acrescentou calcularReservaEmergencia/calcularCltVsPj, que delegam
# taxa/aliquota a FiscalService/FinanceEngine -- arrasta os dois.
peubud_add_test(test_calculadora
    ${CMAKE_SOURCE_DIR}/src/services/CalculadoraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FiscalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)

# #132: RecorrenciaService::proximaApos — funcao PURA de data. O .cpp tambem
# contem gerarPendentes(), que arrasta repos/servicos/DatabaseManager/crypto;
# linkar a cadeia (espelha test_resumo) para resolver os simbolos mesmo
# testando apenas a estatica proximaApos.
peubud_add_test(test_recorrencia
    ${CMAKE_SOURCE_DIR}/src/services/RecorrenciaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/TransacaoRecorrenteRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    # FechamentoMensalService::simulate puxa a cadeia do ResumoService.
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    # (#181) DatabaseManager agora depende de AppSettings (savePathFixo).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_recorrencia PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_recorrencia PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# #133: ImportService parsers via API publica fromCsv/fromOfx. Cadeia:
# ImportService + parsers PDF + match finder + ParcelaService/FinanceEngine.
# (#204) ParcelaService::formatarValor agora locale-aware via AppSettings.
peubud_add_test(test_import
    ${CMAKE_SOURCE_DIR}/src/services/ImportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/BankPDFParsers.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/IMatchFinder.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_import PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_import PRIVATE ${LIBSECRET_INCLUDE_DIRS})

# #133: ExportService round-trip OFX + format machine-readable. Puxa
# ParcelaService/FinanceEngine e a cadeia de import (para reimportar).
# (#204) ParcelaService::formatarValor agora locale-aware via AppSettings.
peubud_add_test(test_export
    ${CMAKE_SOURCE_DIR}/src/services/ExportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ImportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/BankPDFParsers.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/IMatchFinder.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_export PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_export PRIVATE ${LIBSECRET_INCLUDE_DIRS})

# (#200/ARQ-05) Golden-file: cada IExportWriter deve produzir saída
# byte-idêntica (ou, p/ xlsx com timestamp embutido, equivalente) ao método
# estático de ExportService que ele encapsula — prova que o Strategy não
# mudou comportamento. Nao precisa da cadeia de import (sem reimportacao).
peubud_add_test(test_export_writers
    ${CMAKE_SOURCE_DIR}/src/services/ExportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/export/ExportWriters.cpp
    ${CMAKE_SOURCE_DIR}/src/services/export/ExportWriterRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_export_writers PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_export_writers PRIVATE ${LIBSECRET_INCLUDE_DIRS})
# toMultiPdf() usa QFont/QTextDocument (QGuiApplication) — QTEST_MAIN, offscreen.
set_tests_properties(test_export_writers PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
peubud_add_test(test_match_finder ${CMAKE_SOURCE_DIR}/src/services/import/IMatchFinder.cpp)
peubud_add_test(test_fiscal ${CMAKE_SOURCE_DIR}/src/services/FiscalService.cpp)
peubud_add_test(test_backup ${CMAKE_SOURCE_DIR}/src/services/BackupService.cpp)
peubud_add_test(test_pdf_parsers
    ${CMAKE_SOURCE_DIR}/src/services/import/BankPDFParsers.cpp)
peubud_add_test(test_ajustes)
peubud_add_test(test_sqlite_migrations)
peubud_add_test(test_parcela_service
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    # #123: ParcelaService usa FinanceEngine::pmt (formula Price centralizada).
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    # (#204) formatarValor agora locale-aware via AppSettings::language()
    # (gêmeo de uifmt::formatCentavosBRL/#197) -- arrasta AppSettings.cpp
    # (+ SecretStoreService/libsecret via brapiToken()).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_parcela_service PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_parcela_service PRIVATE ${LIBSECRET_INCLUDE_DIRS})

# FIN-03 (auditoria 2026-06-22 §3.1): prova congelada de precisao -- sweep
# PV ate R$100M / taxa 0,05-20% a.m. / ate 480 parcelas contra oraculo
# long double (maior precisao que o double de producao).
peubud_add_test(test_parcela_service_oracle
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_parcela_service_oracle PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_parcela_service_oracle PRIVATE ${LIBSECRET_INCLUDE_DIRS})
peubud_add_test(test_tvm
    ${CMAKE_SOURCE_DIR}/src/services/TvmService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    # (#204) idem test_parcela_service.
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_tvm PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_tvm PRIVATE ${LIBSECRET_INCLUDE_DIRS})
peubud_add_test(test_currency_service
    ${CMAKE_SOURCE_DIR}/src/services/CurrencyService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/currency/FrankfurterProvider.cpp)

# FIN-05 (auditoria 2026-06-22 §3.1): fetchSync() sem setTransferTimeout()
# travava a GUI indefinidamente se o servidor aceitasse a conexao e nunca
# respondesse. TIMEOUT de ctest = watchdog de seguranca: se alguem remover
# o fix, o teste falha por timeout do ctest (30s) em vez de travar a
# maquina de CI para sempre.
peubud_add_test(test_frankfurter_provider
    ${CMAKE_SOURCE_DIR}/src/core/currency/FrankfurterProvider.cpp)
set_tests_properties(test_frankfurter_provider PROPERTIES TIMEOUT 30)

peubud_add_test(test_reconciliacao
    ${CMAKE_SOURCE_DIR}/src/services/ReconciliacaoAutomatica.cpp)

# BuscaService (#139, Onda 7 — arquitetura): SQL extraido de BuscaGlobalDialog.
# Cadeia de dependencia espelha test_resumo (DatabaseManager puxa crypto +
# AppBootstrap puxa os schema providers de Proventos/AjusteCorporativo/
# FechamentoMensal/AuditLog).
peubud_add_test(test_busca_service
    ${CMAKE_SOURCE_DIR}/src/services/BuscaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/DividaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_busca_service PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES}
    ${SQLITE3_LIBRARIES})
target_include_directories(test_busca_service PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS})
peubud_add_test(test_indicadores
    ${CMAKE_SOURCE_DIR}/src/services/IndicadoresTecnicos.cpp)
peubud_add_test(test_suporte_resistencia
    ${CMAKE_SOURCE_DIR}/src/services/SuporteResistencia.cpp)
peubud_add_test(test_drip
    ${CMAKE_SOURCE_DIR}/src/services/DripSimulator.cpp)
peubud_add_test(test_invest_activities
    ${CMAKE_SOURCE_DIR}/src/services/investimento/InvestActivities.cpp
    ${CMAKE_SOURCE_DIR}/src/services/investimento/InvestActivityFactory.cpp)
peubud_add_test(test_external_process
    ${CMAKE_SOURCE_DIR}/src/services/external/ExternalProcessProvider.cpp)
peubud_add_test(test_glossario
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/GlossarioDecorator.cpp
    # #166 §9.6: GlossarioDecorator::aplicarTooltips instala o sublinhado via filter.
    ${CMAKE_SOURCE_DIR}/src/ui/GlossaryUnderlineFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryUnderline.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    # #129: AppSettings::brapiToken usa SecretStoreService (libsecret).
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    # #123: ParcelaService usa FinanceEngine::pmt.
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    # #166 Peca 3: qssContemRegraQToolTip usa Theme::qssParaTeste().
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
# SecretStoreService puxa libsecret; linkar explicitamente.
target_link_libraries(test_glossario PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_glossario PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_glossario PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# #194 (Onda U, PERF-01): budget de performance de
# GlossarioService::primeiroTermoEncontrado -- 1000 chamadas sem match
# devem rodar em <5ms (alternation-regex static+optimize() em vez de
# recompilar 92 regexes por chamada). So depende de GlossarioService.cpp
# (sem UI/DB/cripto).
peubud_add_test(test_glossario_perf
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp)

peubud_add_test(test_resumo
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    # (#181) DatabaseManager agora depende de AppSettings (savePathFixo).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
# DatabaseManager puxa crypto (OpenSSL + libsodium + libsecret); linkar explicitamente.
target_link_libraries(test_resumo PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES}
    ${SQLITE3_LIBRARIES})
target_include_directories(test_resumo PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS}
    ${SQLITE3_INCLUDE_DIRS})

# Teste do FechamentoMensalService (#144). Espelha a cadeia de test_resumo:
# simulate() delega ao ResumoService, logo precisa da mesma arvore de servicos,
# repositorios, DatabaseManager e crypto.
peubud_add_test(test_fechamento
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    # (#181) DatabaseManager agora depende de AppSettings (savePathFixo).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_fechamento PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_fechamento PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# Teste de regressao dos triggers de auditoria (#142, fase RED).
# Exercita os triggers audit_* criados por createSchema(); prova o bug de
# colunas inexistentes (NEW.valor / NEW.data / ...). Cadeia minima: apenas
# DatabaseManager + AuditLogService + crypto.
peubud_add_test(test_audit_triggers
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    # #144: FechamentoMensalService::simulate delega ao ResumoService; arrastar
    # sua cadeia para resolver o link mesmo neste teste que so exercita triggers.
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    # (#181) DatabaseManager agora depende de AppSettings (savePathFixo).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_audit_triggers PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_audit_triggers PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# Smoke test de schema (#200/ARQ-01): banco novo -> as 16 tabelas inline do
# Foundation + as 4 tabelas Mid/Backend criadas via
# DatabaseManager::registerSchemaProvider (AppBootstrap) existem todas.
# Cadeia identica a test_audit_triggers (mesma dependencia de createSchema).
peubud_add_test(test_schema_smoke
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_schema_smoke PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_schema_smoke PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# Teste de Categoria (#118): classificacao fixo/variavel (TipoDespesa).
# Exercita a migration #118 (coluna `fixo`) + round-trip pelo repo, com o
# schema real (createSchema) e triggers de auditoria ativos.
peubud_add_test(test_categoria
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CategoriaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    # #144: cadeia do ResumoService puxada por FechamentoMensalService::simulate.
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    # (#181) DatabaseManager agora depende de AppSettings (savePathFixo).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_categoria PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_categoria PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# Teste de tokens de paleta do Theme (#164 redesign fase 1).
# Verifica valores da nova paleta azul-confiavel e getters adicionados.
# Usa QT_QPA_PLATFORM=offscreen (setMode chama setPalette que precisa de GUI).
peubud_add_test(test_theme_tokens
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
set_tests_properties(test_theme_tokens PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Teste do GastoService (#120, Onda 2): consolidacao fixo/variavel dos gastos.
# Cadeia: GastoService + repos (Categoria, GastoAvulso) + DatabaseManager +
# servicos puxados por createSchema/triggers + crypto.
peubud_add_test(test_gasto_service
    ${CMAKE_SOURCE_DIR}/src/services/GastoService.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CategoriaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    # #144: cadeia do ResumoService puxada por FechamentoMensalService::simulate
    # (GastoAvulsoRepository ja consta acima — nao duplicar).
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    # (#181) DatabaseManager agora depende de AppSettings (savePathFixo).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_gasto_service PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_gasto_service PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# Teste do SidebarNav (#165 redesign fase 2 task 1).
# Widget puro (QWidget + QPushButton): sem DB, sem cripto, sem network.
# Precisa de display offscreen para construcao de QWidget + QPushButton.
peubud_add_test(test_sidebar_nav
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/SidebarNav.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
set_tests_properties(test_sidebar_nav PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Teste do NavRegistry (#165 redesign fase 2 task 2).
# Fonte unica da ordem e agrupamento das abas. Sem dependencias externas (puro Qt::Core).
peubud_add_test(test_nav_registry
    ${CMAKE_SOURCE_DIR}/src/ui/NavRegistry.cpp)

# Render offscreen do SidebarNav (#165 contraste fix, task 6).
# Verifica H1 (QSS + palette) e H2 (palette sozinha como fallback).
# Salva PNGs em /tmp para inspecao manual.
peubud_add_test(test_sidebar_render
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/SidebarNav.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/NavRegistry.cpp)
set_tests_properties(test_sidebar_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Reproducao offscreen dos bugs #165 + rodape navy/cambio moderno (#47, #165).
# Salva /tmp/repro_full.png (1100x690), /tmp/repro_short.png (1100x470),
# e /tmp/rodape_navy.png (alias de repro_full para verificacao do rodape).
# CurrencyPanelWidget + AppSettings + SecretStoreService sao necessarios
# para o painel de cambio real (usa AppSettings::currencySlot1/2).
peubud_add_test(test_sidebar_repro
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/SidebarNav.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/CurrencyPanelWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/NavRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    # AppSettings::currencySymbol() chama ParcelaService::simboloMoeda().
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_sidebar_repro PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_sidebar_repro PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_sidebar_repro PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Regressão #175: siglas de mês em pt-BR (QLocale pt-BR vs locale C = inglês).
# Puro Qt::Core + Qt::Test — sem UI, DB, crypto ou rede.
peubud_add_test(test_locale_meses)

# #197 (Onda U-4, I18N-01): UiFormatters::formatCentavosBRL concatenava
# "R$"+vírgula fixos, ignorando AppSettings::language()/defaultCurrency()
# (~24 call-sites). Fix em aritmética inteira (grouping via QLocale, símbolo
# via ParcelaService::simboloMoeda, separador/posição por switch(language())).
# QSettings::setPath() isola de ~/.config real (ver comentário no .cpp).
peubud_add_test(test_locale_moeda
    ${CMAKE_SOURCE_DIR}/src/ui/UiFormatters.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_locale_moeda PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_locale_moeda PRIVATE ${LIBSECRET_INCLUDE_DIRS})

# #197 (Onda U-4, I18N-05): "dd/MM/yyyy" hardcoded em 29 pontos/22 arquivos de
# UI -- ordem dia/mês é ambígua entre locales (pt-BR dd/MM x en-US MM/dd).
# Fix: uifmt::dataDisplayFormat()+variantes por AppSettings::language().
peubud_add_test(test_locale_datas
    ${CMAKE_SOURCE_DIR}/src/ui/UiFormatters.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_locale_datas PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_locale_datas PRIVATE ${LIBSECRET_INCLUDE_DIRS})

# Render offscreen fix #176: sombra dos botoes nao clipa na borda dos containers.
# Verifica que Theme::aplicar() + ButtonShadowFilter nao quebram render offscreen
# e gera /tmp/shadow_fix.png para inspecao visual.
peubud_add_test(test_shadow_render
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
set_tests_properties(test_shadow_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Render offscreen fix #165: setas de navegacao de mes devem ser monocromaticas.
# Verifica que glifos ‹/› (U+2039/203A) nao geram assinatura laranja do
# Noto Color Emoji (regressao dos antigos ◀️/▶️ que causavam "caixa laranja").
# Salva /tmp/setas_fix.png para inspecao visual.
peubud_add_test(test_setas_mono_render
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
set_tests_properties(test_setas_mono_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# #165 iconButton: contrato do FocusVisibleFilter (anel de foco só por teclado)
# + render offscreen da navBar (◀ mês ▶ 🔒 🖨️) p/ aprovação visual.
# NOTA (lição #165): o bug da "caixa no 1º render" é o estado :focus do botão
# (foco automático ActiveWindow), que o offscreen + QWidget::grab() NÃO reproduzem.
# Por isso o teste que VALE aqui é o de LÓGICA DE FOCO (FocusVisibleFilter),
# testável sem compositor; os grabs offscreen ficam só como smoke de render.
peubud_add_test(test_iconbtn_render
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
set_tests_properties(test_iconbtn_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Render offscreen dos callouts de ajuda (#166 tipografia dos blocos de ajuda).
# Verifica tokens de ajuda no QSS, callouts com objectNames corretos, pílulas
# numeradas e render em light+dark. Salva /tmp/tipo_impl_{light,dark}.png.
peubud_add_test(test_ajuda_render
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/TabShell.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TabHelp.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    # AppSettings::brapiToken usa SecretStoreService (libsecret).
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_ajuda_render PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_ajuda_render PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_ajuda_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Regressão #174: Enter no login dispara Entrar, nao Cancelar.
# Verifica m_btnEntrar->isDefault()==true, m_btnCancelar->autoDefault()==false
# e returnPressed no campo nome aceita o dialog quando form valido.
# Compilado em QT_DEBUG (sem crypto/libsecret em runtime).
# Cadeia: LoginDialog + Theme + UserSession + crypto (headers/codigo necessarios
# para compilar o .cpp do dialog, mesmo que o codigo de crypto nao execute).
peubud_add_test(test_login_enter
    ${CMAKE_SOURCE_DIR}/src/ui/LoginDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/core/session/UserSession.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    # (ADR-001 §6/D6, Fatia 4) UserSession::slug() agora delega a
    # AppSettings::slugify() -- arrasta AppSettings.cpp (+ ParcelaService/
    # FinanceEngine, dependência transitiva de AppSettings::currencySymbol).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
# QT_DEBUG: LoginDialog pula o bloco de cripto/libsecret em runtime.
target_compile_definitions(test_login_enter PRIVATE QT_DEBUG)
target_link_libraries(test_login_enter PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_login_enter PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_login_enter PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Render offscreen das tags coloridas (#166 Fase 3, Peca 1).
# Verifica: criarTag -> QLabel com objectName "tag" + propriedade "variante" corretas,
# QSS contem os seletores das variantes, sem tokens residuais, render light+dark.
# Salva /tmp/tags_light.png e /tmp/tags_dark.png para aprovacao visual.
peubud_add_test(test_tag_render
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
set_tests_properties(test_tag_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Consistência de cards (#166 Peça 4): QFrame#dashCard unificado com #surface.
# Verifica: QSS contém seletores #surface e #dashCard com border-radius 14px,
# sem tokens residuais, render offscreen light+dark de QFrame#dashCard nao nulo.
# Salva /tmp/cards_light.png e /tmp/cards_dark.png para aprovacao visual.
peubud_add_test(test_cards_render
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
set_tests_properties(test_cards_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# #196 (Onda U, UX-01): card "Parcelas" do Dashboard perde a contagem real
# (sobrescrita por rotulo fixo do glossario coloquial). Instancia o
# DashboardWidget de verdade (sem abrir DB — atualizarDados() so precisa que
# os repositorios nao crashem com QSqlDatabase invalida, o que ja e o
# contrato deles) e prova que GlossarioDecorator::aplicarLabelsColoquiais NAO
# altera o sub-label "N parcela(s)". Cadeia de dependencia espelha
# test_fechamento (DashboardWidget arrasta ResumoService -> FechamentoMensalService
# -> DatabaseManager -> crypto) + GastoService/CategoriaRepository (card
# "Fixos x Variaveis") + UserSession (onFecharMes) + Theme/filters (emojiIcon,
# QSS) + Glossario* (mecanismo sob teste).
peubud_add_test(test_dashboard_glossario
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/DashboardWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/UiFormatters.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/GlossarioDecorator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/GlossaryUnderlineFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryUnderline.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GastoService.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CategoriaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/session/UserSession.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_dashboard_glossario PRIVATE
    Qt6::Charts
    Qt6::PrintSupport
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_dashboard_glossario PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_dashboard_glossario PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# #198 (Onda U-4, A11Y-01/A11Y-03): botoes icon-only (◀️▶️🖨️🔒 Dashboard, 👁
# LoginDialog, 🧮 CalculadoraEdit em 10 dialogos, 🗑️ ReceiptAttachmentWidget)
# precisam de accessibleName (WCAG 4.1.2) + StrongFocus/equivalente de
# teclado onde antes era Qt::NoFocus (WCAG 2.1.1). Instancia producao real —
# cadeia de dependencia = uniao de test_dashboard_glossario (DashboardWidget)
# + test_login_enter (LoginDialog, forcando QT_NO_DEBUG p/ exercitar o ramo
# release onde o botao 👁 existe) + CalculadoraEdit->CalculadoraHub->
# CalculadoraDialog (GlossaryLabel/GlossarioDecorator/TvmService/FiscalService/
# WizardGuiadoDialog) + ReceiptAttachmentWidget (so Theme).
peubud_add_test(test_a11y_botoes_icone
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/UiFormatters.cpp
    # LoginDialog
    ${CMAKE_SOURCE_DIR}/src/ui/LoginDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/core/session/UserSession.cpp
    # DashboardWidget
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/DashboardWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GastoService.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CategoriaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    # ReceiptAttachmentWidget (so Theme, ja coberto acima)
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/ReceiptAttachmentWidget.cpp
    # CalculadoraEdit -> CalculadoraHub -> CalculadoraDialog
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/CalculadoraEdit.cpp
    ${CMAKE_SOURCE_DIR}/src/core/CalculadoraHub.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CalculadoraService.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/CalculadoraDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryLabel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/GlossarioDecorator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/GlossaryUnderlineFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryUnderline.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/TvmService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FiscalService.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/WizardGuiadoDialog.cpp
    # Crypto (LoginDialog inclui os headers mesmo em QT_NO_DEBUG)
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
# QT_NO_DEBUG (NAO QT_DEBUG): forca o ramo release do LoginDialog (#ifndef
# QT_DEBUG) para que o botao 👁 (m_btnOlho) exista e possa ser testado. Sem
# isso, Qt::qglobal.h auto-define QT_DEBUG em builds sem -DNDEBUG (a arvore
# build/ deste projeto), excluindo o botao da compilacao.
target_compile_definitions(test_a11y_botoes_icone PRIVATE QT_NO_DEBUG)
target_link_libraries(test_a11y_botoes_icone PRIVATE
    Qt6::Charts
    Qt6::PrintSupport
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_a11y_botoes_icone PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_a11y_botoes_icone PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ARQ-03 (auditoria 2026-06-22 §3.3): caracterização da aba Wizards da
# CalculadoraDialog (buildTabWizards era god-function ~1.388 linhas, virou
# registrador fino + 20 buildWizardX()). Instância real offscreen: prova
# que os 20 painéis continuam na mesma ordem combo<->stack e que o fluxo
# digitar->Calcular->ler resultado produz o MESMO texto de antes do refactor.
peubud_add_test(test_calculadora_dialog_wizards
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/CalculadoraDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CalculadoraService.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryLabel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/GlossarioDecorator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/GlossaryUnderlineFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryUnderline.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/TvmService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FiscalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/UiFormatters.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
target_link_libraries(test_calculadora_dialog_wizards PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_calculadora_dialog_wizards PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_calculadora_dialog_wizards PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Empty states guiados (#166 Peça 2): EmptyStateWidget com emoji+título+frase+CTA.
# Verifica: labels corretos, copy exata das 12 telas, CTA presente/ausente conforme
# tabela, QSS contém seletores #emptyStateTitle/#emptyStateFrase, sem tokens
# residuais, render offscreen light+dark das telas representativas.
# Salva /tmp/empty_*_{light,dark}.png para aprovacao visual.
peubud_add_test(test_empty_state_render
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/EmptyStateWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
set_tests_properties(test_empty_state_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Render offscreen do LoginDialog pos-#186: icone @128px + tagline.
# Verifica: QLabel#loginIcon existe, pixmap nao-nulo 128x128 (:/icon/icon.png),
# titulo de texto removido, tagline presente; salva light+dark em /tmp/.
# Mesma cadeia de dependencias do test_login_enter (LoginDialog arrasta crypto
# mesmo em QT_DEBUG, pois o .cpp inclui os headers de cripto no bloco #ifndef).
# O resources.qrc e adicionado explicitamente para que :/icon/icon.png esteja
# disponivel no contexto do teste (sem .qrc o pixmap seria nulo).
peubud_add_test(test_login_header_render
    ${CMAKE_SOURCE_DIR}/src/ui/LoginDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/core/session/UserSession.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    # (ADR-001 §6/D6, Fatia 4) UserSession::slug() agora delega a
    # AppSettings::slugify() -- arrasta AppSettings.cpp (+ ParcelaService/
    # FinanceEngine, dependência transitiva de AppSettings::currencySymbol).
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/resources/resources.qrc)
# QT_DEBUG: pula o bloco de cripto/libsecret em runtime (LoginDialog).
target_compile_definitions(test_login_header_render PRIVATE QT_DEBUG)
target_link_libraries(test_login_header_render PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_login_header_render PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_login_header_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Regressão #188: conteúdo das abas deve rolar em janela pequena (não clipar).
# Verifica: QScrollArea existe no TabShell, conteúdo (setContent) está dentro
# do scroll, widgetResizable==true, render em 720x500 não nulo, callouts fora.
# TabShell puxa TabHelp + Theme + AppSettings + SecretStoreService + ParcelaService.
peubud_add_test(test_tab_shell_scroll
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/TabShell.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TabHelp.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_tab_shell_scroll PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_tab_shell_scroll PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_tab_shell_scroll PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# GlossaryLabel (#166 Peca 3 §9.6): cue estatico de glossario com sublinhado
# pontilhado no paintEvent. Verifica contrato (a-f): nao-destrutivo, tooltip do
# servico, construcao sem crash, cursor, tooltip vazio p/ desconhecido, ":" final.
# Cadeia minima: GlossaryLabel + GlossarioService + AppSettings + SecretStoreService
# + ParcelaService/FinanceEngine (AppSettings arrasta SecretStoreService).
peubud_add_test(test_glossary_label
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryLabel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryUnderline.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
target_link_libraries(test_glossary_label PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_glossary_label PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_glossary_label PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Render offscreen do GlossaryLabel (#166 Peca 3 §9.6).
# Verifica: sublinhado pontilhado pintado (diferenca de pixel vs QLabel comum),
# render nao-nulo em light+dark.
# Salva /tmp/glossary_cue_light.png e /tmp/glossary_cue_dark.png para aprovacao visual.
peubud_add_test(test_glossary_label_render
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryLabel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryUnderline.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
target_link_libraries(test_glossary_label_render PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_glossary_label_render PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_glossary_label_render PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# Cobertura AMPLA do cue (#166 §9.6): GlossarioDecorator::aplicarTooltips agora
# tambem instala o sublinhado pontilhado (via GlossaryUnderlineFilter) nos labels
# -chave curtos da mesma varredura. Verifica: label-chave recebe a propriedade
# "glossarioUnderlined"; prosa longa NAO recebe (so tooltip); idempotencia;
# nao-destrutivo (text() intacto); render diff de pixel + PNGs de cobertura.
peubud_add_test(test_glossary_coverage
    ${CMAKE_SOURCE_DIR}/src/ui/GlossarioDecorator.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/GlossaryUnderlineFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/GlossaryUnderline.cpp
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp)
target_link_libraries(test_glossary_coverage PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_glossary_coverage PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_glossary_coverage PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# FIN-01/#146 (Onda U-1): regressao de perda de dado silenciosa em Meta "sem
# prazo" -- MetaRepository::insert()/update() com dataPrazo/obs QString() null
# (estado padrao do MetaDialog). Cadeia identica a test_categoria: precisa do
# DatabaseManager real (createSchema + triggers de auditoria) e da cascata que
# ele arrasta (AjusteCorporativoService/AuditLogService/FechamentoMensalService
# -> ResumoService -> repos + crypto).
peubud_add_test(test_meta_repository
    ${CMAKE_SOURCE_DIR}/src/repositories/MetaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_meta_repository PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_meta_repository PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# FIN-01/#146 (Onda U-1): mesma regressao, para Divida "sem vencimento"
# (DividaDialog). Mesma cadeia de dependencias de test_meta_repository.
peubud_add_test(test_divida_repository
    ${CMAKE_SOURCE_DIR}/src/repositories/DividaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_divida_repository PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_divida_repository PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# FIN-01/#146 (Onda U-1): varredura dominio -- 2 casos CONFIRMADOS adicionais
# de QString null em coluna TEXT NOT NULL (TransacaoRecorrenteRepository via
# RecorrenciaDialog::resultado().clear(), LancamentoInvestimentoRepository via
# InvestimentoDialog::lancamentos().clear()). Mesma cadeia de dependencias.
peubud_add_test(test_repository_naonulo_domino
    ${CMAKE_SOURCE_DIR}/src/repositories/TransacaoRecorrenteRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_repository_naonulo_domino PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_repository_naonulo_domino PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# Durabilidade do banco (#191, FIN-02, Onda U-1): journal_mode=DELETE
# (crash-safe, sem residuo tipo -wal) + PRAGMA integrity_check como gate
# dentro de encryptAndSave() antes de cifrar por cima do ultimo .pbd bom.
# Cadeia identica a test_audit_triggers/test_categoria: precisa do
# DatabaseManager real (createSchema + triggers) e da cascata que ele
# arrasta, mais AppSettings::savePathFixo() para localizar o cofre .pbd.
#
# QT_NO_DEBUG (nao QT_DEBUG): o proprio header do Qt (qtconfigmacros.h)
# auto-define QT_DEBUG quando NEM QT_DEBUG NEM QT_NO_DEBUG sao passados via
# -D -- e exatamente o caso do target de app em CMAKE_BUILD_TYPE=Debug deste
# projeto (ver CMakeLists.txt raiz: so build-rel passa -DQT_NO_DEBUG).
# encryptAndSave()/checkFileIntegrity() (o cerne do #191) so rodam no ramo
# `#ifndef QT_DEBUG` de save()/close() -- sem esta definicao o teste
# compilaria no ramo "debug" (sem persistir .pbd) e NUNCA exercitaria o gate
# de integrity_check, mesmo dentro do `build/` (Debug) padrao do ctest.
peubud_add_test(test_db_durabilidade
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    # #144: FechamentoMensalService::simulate delega ao ResumoService; arrastar
    # sua cadeia para resolver o link mesmo neste teste que so exercita
    # save()/close()/checkFileIntegrity().
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_compile_definitions(test_db_durabilidade PRIVATE QT_NO_DEBUG)
target_link_libraries(test_db_durabilidade PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_db_durabilidade PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# Skin compartilhado das tabelas (#167 T1, §11 manual de estilo).
# Verifica: QSS sem zebra poluída (alternate-background-color = fundo, não
# @@ALT@@) e sem grade vertical (gridline transparente) em QTableWidget;
# header com borda-bottom 2px accent + maiúsculas (text-transform); densidade
# Confortável (48px, default) vs Compacta (34px) via
# Theme::aplicarDensidadeTabela/aplicarDensidadeTabelas; TableHoverFilter
# rastreia a linha certa sob o mouse (MouseMove/Leave sintéticos no viewport,
# headless via QT_QPA_PLATFORM=offscreen — sem interação humana).
# Salva /tmp/tabela_t1_{light,dark}.png para aprovação visual.
peubud_add_test(test_table_skin
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_table_skin PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_table_skin PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_table_skin PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# #193 (Onda U, DIST-02): AboutDialog nao pode afirmar que a AGPL-3.0 proibe
# uso comercial (falso) nem descrever a cripto ATIVA como AES-256-CBC (e o
# formato legado, so leitura). Render offscreen + gate regex estatico em
# AboutDialog.cpp e peubud_en.ts (fonte + traducao, sincronizado com #154).
peubud_add_test(test_about_dialog_render
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/AboutDialog.cpp)
target_compile_definitions(test_about_dialog_render PRIVATE
    PEUBUD_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

# #96 (Onda U, DIST-01): CPACK_SOURCE_IGNORE_FILES nao excluia models/
# (670MB terceiros), build-rel/, build-verif/, .superpowers/ -> cpack
# package_source vazava tudo. Gate real: roda cpack contra a
# CPackSourceConfig.cmake gerada por ESTE build (mesma CMakeLists.txt de
# producao) e inspeciona o tarball resultante.
peubud_add_test(test_package_source_gate
    ${CMAKE_SOURCE_DIR}/src/services/external/ExternalProcessProvider.cpp)
target_compile_definitions(test_package_source_gate PRIVATE
    PEUBUD_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    PEUBUD_BINARY_DIR="${CMAKE_BINARY_DIR}")

# #199 (Onda U, QA-03): uifmt::parseCentavos("1.234") truncava p/ R$ 1,23
# (ramo "so ponto" caia em toDouble literal). Fix: heuristica de milhar.
# #197 (Onda U, I18N-01): formatCentavosBRL/dataDisplayFormat* passaram a
# consultar AppSettings::language()/defaultCurrency() (+ ParcelaService::
# simboloMoeda) -- arrasta AppSettings.cpp (+ libsecret via brapiToken()) e
# ParcelaService.cpp (+ FinanceEngine.cpp) para o link.
peubud_add_test(test_ui_formatters
    ${CMAKE_SOURCE_DIR}/src/ui/UiFormatters.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_ui_formatters PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_ui_formatters PRIVATE ${LIBSECRET_INCLUDE_DIRS})

# #199 (Onda U, QA-01): parser posicional B3 COTAHIST lia PREMED em vez de
# PREULT em `fechamento` (dormente -- CotahistProvider.cpp/.h ficam FORA do
# CMakeLists do app, Bolsa congelada, e alem do bug de campo tinham erros de
# compilacao Qt6 nao relacionados -- QTextCodec/setCodec removidos, atributo
# QNetworkRequest::FollowRedirectsAttribute removido, bug de captura de
# lambda -- nunca portados, fora de escopo de QA-01). O parser foi extraido
# p/ services/CotahistParser.h/.cpp (funcao livre, sem QObject/rede/DB,
# mesmo padrao de BankPDFParsers) -- compila limpo e isolado.
peubud_add_test(test_cotahist_parser
    ${CMAKE_SOURCE_DIR}/src/services/CotahistParser.cpp)

# #199 (Onda U, QA-04): ParcelaService::dataParaParcela/gerarDatas sem teste
# (virada de ano, offset negativo de parcelaInicial, N grande).
# (ja adicionado como slots extras em test_parcela_service, ver abaixo)

# #199 (Onda U, QA-06): round-trip XLSX (export->import->centavos iguais)
# nunca foi provado -- so OFX tinha (test_export::roundTripOfxPreservaCentavos).
# Alvo separado porque precisa linkar QXlsx::QXlsx + QXLSX_LIBRARY (os alvos
# test_export/test_import existentes NAO linkam QXlsx -> caem no stub #else
# de toXlsx/fromXlsx e nunca exercitam o parser real).
peubud_add_test(test_xlsx_roundtrip
    ${CMAKE_SOURCE_DIR}/src/services/ExportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ImportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/BankPDFParsers.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/IMatchFinder.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    # (#204) idem test_export.
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_xlsx_roundtrip PRIVATE QXlsx::QXlsx ${LIBSECRET_LIBRARIES})
target_include_directories(test_xlsx_roundtrip PRIVATE ${LIBSECRET_INCLUDE_DIRS})
target_compile_definitions(test_xlsx_roundtrip PRIVATE QXLSX_LIBRARY)

# #199 (Onda U, QA-07): 4 dos 6 repos de dinheiro apontados pela auditoria
# sem prova INSERT->SELECT (Divida/Meta ja cobertos por
# test_divida_repository/test_meta_repository, ainda que como regressao
# FIN-01, nao CRUD generico). Cobre Orcamento/Price/Lembrete/Pagador.
peubud_add_test(test_repos_sem_cobertura
    ${CMAKE_SOURCE_DIR}/src/repositories/OrcamentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/PriceRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LembreteRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/PagadorRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/AppBootstrap.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ResumoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/EntradaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/CompraService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoInvestimentoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/LancamentoEntradaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/FonteRendaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CompraRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ParcelaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CartaoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/ProventoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
target_link_libraries(test_repos_sem_cobertura PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_repos_sem_cobertura PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# #199 (Onda U, QA-05): providers de mercado (BrapiProvider) sem suite --
# gate de processo (nao fechar #45/#58 sem teste de contrato).
# fetchQuote/fetchOHLCV tinha a conversao JSON->struct inline nas lambdas de
# QNetworkReply::finished (sem rede real, intestavel). Extraida em metodo
# static puro (BrapiProvider::parseQuoteJson/parseOhlcvJson) -- refactor
# comportamento-preservado, testado aqui com fixture JSON (resposta real
# brapi.dev). OAuthDeviceFlow removido em #137 (dead module, 0 consumidor).
peubud_add_test(test_market_providers_contract
    ${CMAKE_SOURCE_DIR}/src/services/BrapiProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/services/IMarketDataProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_market_providers_contract PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_market_providers_contract PRIVATE ${LIBSECRET_INCLUDE_DIRS})

# UX-02 (auditoria 2026-06-22, Onda U-5): combo de provider da Bolsa com
# jargao cru + status falso (default "frankfurter" nunca registrado no
# MarketDataService, cai pra "brapi" em silencio mas a UI confirmava sucesso)
# + busca sem timeout (ficava "Buscando..." pra sempre). Fix: rotulos
# amigaveis via MarketProviderCatalog, status honesto via
# MarketDataService::activeProviderName() + BolsaWidget::mensagemStatusProvider
# (logica pura extraida da UI), timeout de busca com mensagem de erro.
peubud_add_test(test_ux02_provider_bolsa
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/BolsaWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/EmptyStateWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/services/MarketDataService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/MarketProviderCatalog.cpp
    ${CMAKE_SOURCE_DIR}/src/services/IMarketDataProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_ux02_provider_bolsa PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_ux02_provider_bolsa PRIVATE ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_ux02_provider_bolsa PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# #201 (Onda U-4): 3 fixes bounded — SEC-01 (senha em claro não limpa no
# sucesso do LoginDialog), FE-01 (setStyleSheet inline mata QSS global do
# callout do TabShell), FE-03 (leak em TabShell::setContent sem deleteLater).
# QT_NO_DEBUG (não QT_DEBUG) força o ramo release do LoginDialog — mesmo
# motivo de test_a11y_botoes_icone: precisa do campo de senha + botão olho
# existirem para exercitar o fluxo de login completo (com libsecret real).
peubud_add_test(test_tabshell_seguranca
    ${CMAKE_SOURCE_DIR}/src/ui/LoginDialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/TabShell.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TabHelp.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/FocusVisibleFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverFilter.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/TableHoverDelegate.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/session/UserSession.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_compile_definitions(test_tabshell_seguranca PRIVATE QT_NO_DEBUG)
target_link_libraries(test_tabshell_seguranca PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_tabshell_seguranca PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})
set_tests_properties(test_tabshell_seguranca PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# #203b (Onda U, A11Y-07): sufixo textual redundante à cor na coluna "%" do
# Planejamento — lógica pura, sem QApplication/DB.
peubud_add_test(test_aderencia_orcamento
    ${CMAKE_SOURCE_DIR}/src/ui/AderenciaOrcamento.cpp)

# #208 (Onda U-5, PERF-02): budget de startup das abas da MainWindow.
# LazyPageRouter extraído p/ ser testável sem arrastar a árvore inteira de
# widgets financeiros do app (MainWindow.cpp puxaria ~150 fontes p/ linkar —
# custo de build proibitivo na máquina de dev, cap -j2 por RAM). Prova o
# mecanismo de que MainWindow::navegarParaChave depende: fábrica só roda na
# 1a navegação da chave; abas não visitadas nunca são instanciadas.
# Só Qt::Core/Widgets — sem DB/cripto/rede.
peubud_add_test(test_lazy_page_router
    ${CMAKE_SOURCE_DIR}/src/ui/LazyPageRouter.cpp)
set_tests_properties(test_lazy_page_router PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# ADR-001 #212 (gestão de usuários, Fatia 1 — Foundation): primitivas NOVAS
# de SodiumCryptoService (wrapKey/unwrapKey via crypto_secretbox, deriveKey
# de 4 args com ops/mem explícitos, Ed25519 sign/verify). Só depende de
# libsodium — sem DB/UI/rede.
peubud_add_test(test_crypto_envelope
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp)
target_link_libraries(test_crypto_envelope PRIVATE ${LIBSODIUM_LIBRARIES})
target_include_directories(test_crypto_envelope PRIVATE ${LIBSODIUM_INCLUDE_DIRS})

# ADR-001 #212 (Fatia 1): UserAccountRepository — I/O burro de users.json +
# verificação de assinatura Ed25519 (detecção de adulteração, CWE-269).
# Path injetável via QTemporaryDir — não toca no users.json real da máquina.
peubud_add_test(test_user_account_repository
    ${CMAKE_SOURCE_DIR}/src/repositories/UserAccountRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp)
target_link_libraries(test_user_account_repository PRIVATE ${LIBSODIUM_LIBRARIES})
target_include_directories(test_user_account_repository PRIVATE ${LIBSODIUM_INCLUDE_DIRS})

# ADR-001 #212 (Fatia 2 + Fatia 4 — regra de negócio): UserAccountService.
# Envelope completo (createAdmin/createUser/completeFirstLogin/verifyLogin/
# removeUser/resetPasswordViaRecovery) + âncora anti-pubkey-swap via
# SecretStoreService real (libsecret) + integração `.pbd` v2 (Fatia 4:
# createAdmin/completeFirstLogin agora abrem/fecham DatabaseManager pra
# bootstrapar o cofre — arrasta a cadeia inteira de crypto/AppSettings/
# UserSession, mesmo padrão de test_resumo/test_busca_service). A maioria
# dos testes chama createAdmin() -> SecretStoreService::store(); em CI
# headless sem D-Bus/Secret Service isso pode crashar o processo (mesmo
# achado documentado em test_tabshell_seguranca) -- por isso o guard fica em
# init() (roda antes de CADA teste, não só de um). T0 (Fatia 4): isola
# HOME/XDG_DATA_HOME em QTemporaryDir (mesmo padrão de
# test_working_db_path.cpp) -- sem isso, createAdmin/completeFirstLogin
# escreveriam `.pbd`/users.json no ambiente REAL de quem roda o ctest.
peubud_add_test(test_user_account_service
    ${CMAKE_SOURCE_DIR}/src/services/UserAccountService.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/UserAccountRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/session/UserSession.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_user_account_service PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_user_account_service PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})

# ADR-001 #212 (Fatia 3 — Foundation): VaultFile — framing PURO do .pbd v2
# auto-contido (§5.2). Componente isolado, sem estado, sem I/O — NÃO toca
# DatabaseManager/UserAccountService/UI. Só depende de SodiumCryptoService
# (delega isNewFormat() para detect() v1, e os testes usam wrapKey/
# randomBytes p/ montar headers de amostra realistas) — sem DB/UI/rede.
peubud_add_test(test_vault_file
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp)
target_link_libraries(test_vault_file PRIVATE ${LIBSODIUM_LIBRARIES})
target_include_directories(test_vault_file PRIVATE ${LIBSODIUM_INCLUDE_DIRS})

# ADR-001 #212 (Fatia 4 — Foundation): DatabaseManager v2 — setVaultHeader/
# vaultHeader/vaultIsV2 + open() com autodetect (V2/V1/legado AES-CBC) +
# encryptAndSave() envelopando em `.pbd` v2 + close() resetando o header
# (singleton reusado entre logins). Provas T3(a-d): bootstrap de cofre novo
# grava magic PBD\x02 com header recuperável; reabertura com o mesmo K
# preserva dado E header; `.pbd` v1 e legado AES-CBC continuam abrindo SEM
# REGRESSÃO (bloqueante — app em produção com dado real). Isola HOME/
# XDG_DATA_HOME em QTemporaryDir por teste (mesmo padrão de
# test_working_db_path.cpp) + close() defensivo no singleton entre testes.
peubud_add_test(test_database_manager_vault_v2
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/VaultFile.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
target_link_libraries(test_database_manager_vault_v2 PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES}
    ${LIBSECRET_LIBRARIES})
target_include_directories(test_database_manager_vault_v2 PRIVATE
    ${LIBSODIUM_INCLUDE_DIRS}
    ${LIBSECRET_INCLUDE_DIRS})
