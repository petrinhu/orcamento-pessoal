# PeuBud - bootstrap de testes (#102)
# Cada teste vira um executable separado. Use peubud_add_test() para adicionar.

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
peubud_add_test(test_match_finder ${CMAKE_SOURCE_DIR}/src/services/import/IMatchFinder.cpp)
peubud_add_test(test_fiscal ${CMAKE_SOURCE_DIR}/src/services/FiscalService.cpp)
peubud_add_test(test_backup ${CMAKE_SOURCE_DIR}/src/services/BackupService.cpp)
peubud_add_test(test_pdf_parsers
    ${CMAKE_SOURCE_DIR}/src/services/import/BankPDFParsers.cpp)
peubud_add_test(test_ajustes)
peubud_add_test(test_sqlite_migrations)
peubud_add_test(test_parcela_service
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp)
peubud_add_test(test_tvm
    ${CMAKE_SOURCE_DIR}/src/services/TvmService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp)
peubud_add_test(test_currency_service
    ${CMAKE_SOURCE_DIR}/src/services/CurrencyService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/currency/FrankfurterProvider.cpp)
peubud_add_test(test_reconciliacao
    ${CMAKE_SOURCE_DIR}/src/services/ReconciliacaoAutomatica.cpp)
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
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp)
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
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FechamentoMensalService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp)
# DatabaseManager puxa crypto (OpenSSL + libsodium); linkar explicitamente.
target_link_libraries(test_resumo PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES})
target_include_directories(test_resumo PRIVATE ${LIBSODIUM_INCLUDE_DIRS})

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
    ${CMAKE_SOURCE_DIR}/src/services/AjusteCorporativoService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/AuditLogService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp)
target_link_libraries(test_fechamento PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES})
target_include_directories(test_fechamento PRIVATE ${LIBSODIUM_INCLUDE_DIRS})

# Teste de regressao dos triggers de auditoria (#142, fase RED).
# Exercita os triggers audit_* criados por createSchema(); prova o bug de
# colunas inexistentes (NEW.valor / NEW.data / ...). Cadeia minima: apenas
# DatabaseManager + AuditLogService + crypto.
peubud_add_test(test_audit_triggers
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
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
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp)
target_link_libraries(test_audit_triggers PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES})
target_include_directories(test_audit_triggers PRIVATE ${LIBSODIUM_INCLUDE_DIRS})

# Teste de Categoria (#118): classificacao fixo/variavel (TipoDespesa).
# Exercita a migration #118 (coluna `fixo`) + round-trip pelo repo, com o
# schema real (createSchema) e triggers de auditoria ativos.
peubud_add_test(test_categoria
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
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
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp)
target_link_libraries(test_categoria PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES})
target_include_directories(test_categoria PRIVATE ${LIBSODIUM_INCLUDE_DIRS})

# Teste do GastoService (#120, Onda 2): consolidacao fixo/variavel dos gastos.
# Cadeia: GastoService + repos (Categoria, GastoAvulso) + DatabaseManager +
# servicos puxados por createSchema/triggers + crypto.
peubud_add_test(test_gasto_service
    ${CMAKE_SOURCE_DIR}/src/services/GastoService.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/CategoriaRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/repositories/GastoAvulsoRepository.cpp
    ${CMAKE_SOURCE_DIR}/src/core/database/DatabaseManager.cpp
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
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp)
target_link_libraries(test_gasto_service PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES})
target_include_directories(test_gasto_service PRIVATE ${LIBSODIUM_INCLUDE_DIRS})
