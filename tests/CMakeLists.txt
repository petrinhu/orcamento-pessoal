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
