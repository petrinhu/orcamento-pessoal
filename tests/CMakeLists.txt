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
    ${CMAKE_SOURCE_DIR}/src/services/GlossarioService.cpp)
