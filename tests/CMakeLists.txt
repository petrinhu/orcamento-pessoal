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

# #178: garantia de pasta antes de save/autosave — logica pura de QDir::mkpath
# (ramo GUI/QMessageBox nao testavel headless; cobrimos o contrato de criacao).
peubud_add_test(test_dir_guard)

# #132: CalculadoraService — funcoes puras (sem dependencias externas).
peubud_add_test(test_calculadora
    ${CMAKE_SOURCE_DIR}/src/services/CalculadoraService.cpp)

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
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp)
target_link_libraries(test_recorrencia PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSODIUM_LIBRARIES})
target_include_directories(test_recorrencia PRIVATE ${LIBSODIUM_INCLUDE_DIRS})

# #133: ImportService parsers via API publica fromCsv/fromOfx. Cadeia:
# ImportService + parsers PDF + match finder + ParcelaService/FinanceEngine.
peubud_add_test(test_import
    ${CMAKE_SOURCE_DIR}/src/services/ImportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/BankPDFParsers.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/IMatchFinder.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)

# #133: ExportService round-trip OFX + format machine-readable. Puxa
# ParcelaService/FinanceEngine e a cadeia de import (para reimportar).
peubud_add_test(test_export
    ${CMAKE_SOURCE_DIR}/src/services/ExportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ImportService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/BankPDFParsers.cpp
    ${CMAKE_SOURCE_DIR}/src/services/import/IMatchFinder.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
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
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
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
    ${CMAKE_SOURCE_DIR}/src/ui/GlossarioDecorator.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings/AppSettings.cpp
    # #129: AppSettings::brapiToken usa SecretStoreService (libsecret).
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp
    ${CMAKE_SOURCE_DIR}/src/services/ParcelaService.cpp
    # #123: ParcelaService usa FinanceEngine::pmt.
    ${CMAKE_SOURCE_DIR}/src/services/FinanceEngine.cpp)
# SecretStoreService puxa libsecret; linkar explicitamente.
target_link_libraries(test_glossario PRIVATE ${LIBSECRET_LIBRARIES})
target_include_directories(test_glossario PRIVATE ${LIBSECRET_INCLUDE_DIRS})
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

# Teste de tokens de paleta do Theme (#164 redesign fase 1).
# Verifica valores da nova paleta azul-confiavel e getters adicionados.
# Usa QT_QPA_PLATFORM=offscreen (setMode chama setPalette que precisa de GUI).
peubud_add_test(test_theme_tokens
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp)
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

# Teste do SidebarNav (#165 redesign fase 2 task 1).
# Widget puro (QWidget + QPushButton): sem DB, sem cripto, sem network.
# Precisa de display offscreen para construcao de QWidget + QPushButton.
peubud_add_test(test_sidebar_nav
    ${CMAKE_SOURCE_DIR}/src/ui/widgets/SidebarNav.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp)
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

# Render offscreen fix #176: sombra dos botoes nao clipa na borda dos containers.
# Verifica que Theme::aplicar() + ButtonShadowFilter nao quebram render offscreen
# e gera /tmp/shadow_fix.png para inspecao visual.
peubud_add_test(test_shadow_render
    ${CMAKE_SOURCE_DIR}/src/ui/Theme.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/ButtonShadowFilter.cpp)
set_tests_properties(test_shadow_render PROPERTIES
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
    ${CMAKE_SOURCE_DIR}/src/core/session/UserSession.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/SodiumCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/AesCryptoService.cpp
    ${CMAKE_SOURCE_DIR}/src/core/crypto/KeyDerivation.cpp
    ${CMAKE_SOURCE_DIR}/src/core/secret/SecretStoreService.cpp)
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
