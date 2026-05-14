cmake_minimum_required(VERSION 3.16)
project(OrcamentoPessoal VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui GuiPrivate Widgets Charts Network Sql LinguistTools PrintSupport)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSECRET REQUIRED libsecret-1)
pkg_check_modules(LIBSODIUM REQUIRED libsodium)

# Poppler-qt6 opcional (#93). Sem ele, ImportService::fromPdf retorna erro
# orientando o usuario a instalar poppler-qt6-devel.
pkg_check_modules(POPPLER_QT6 IMPORTED_TARGET poppler-qt6)
if(POPPLER_QT6_FOUND)
    message(STATUS "Poppler-qt6 encontrado: import PDF habilitado")
else()
    message(STATUS "Poppler-qt6 nao encontrado: import PDF retornara erro orientativo")
endif()

# ── QXlsx (MIT, pure Qt — XLSX read/write) ────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    QXlsx
    GIT_REPOSITORY https://github.com/QtExcel/QXlsx.git
    GIT_TAG        v1.4.9
    SOURCE_SUBDIR  QXlsx
)
FetchContent_MakeAvailable(QXlsx)

# Qt6::GuiPrivate propagates strict Qt defines (QT_NO_CAST_FROM_ASCII, etc.)
# that break QXlsx's own source code (which uses QString("literal") etc.)
# Undefine them for the QXlsx target only.
target_compile_options(QXlsx PRIVATE
    -UQT_NO_CAST_FROM_ASCII
    -UQT_NO_CAST_FROM_BYTEARRAY
    -UQT_NO_CAST_TO_ASCII
    -UQT_STRICT_ITERATORS
    -UQT_NO_URL_CAST_FROM_STRING
    -UQT_NO_KEYWORDS
    -UQT_NO_SIGNALS_SLOTS_KEYWORDS
)

set(SOURCES
    src/main.cpp

    # Core — crypto
    src/core/crypto/AesCryptoService.cpp
    src/core/crypto/KeyDerivation.cpp
    src/core/crypto/SodiumCryptoService.cpp

    # Services — TVM
    src/services/TvmService.cpp

    # Services — FinanceEngine (#81)
    src/services/FinanceEngine.cpp
    src/services/FiscalService.cpp
    src/services/BackupService.cpp
    src/services/AjusteCorporativoService.cpp

    # Services — OAuth Device Flow RFC 8628 (#83)
    src/services/OAuthDeviceFlow.cpp

    # Services — FileObfuscator (#91)
    src/services/FileObfuscator.cpp

    # Services — Audit log triggers (#92)
    src/services/AuditLogService.cpp

    # Services — Fechamento mensal (#88)
    src/services/FechamentoMensalService.cpp

    # Services — Reconciliacao automatica subset-sum (#85)
    src/services/ReconciliacaoAutomatica.cpp

    # Services — Indicadores tecnicos (SMA/EMA/RSI/MACD/BBands) (#52)
    src/services/IndicadoresTecnicos.cpp

    # Services — Suporte/Resistencia (Pivot/Camarilla/Fib) (#53)
    src/services/SuporteResistencia.cpp

    # Services — DRIP Simulator (dividendos + comparativo CDI/IPCA/IBOV) (#63)
    src/services/DripSimulator.cpp

    # Core — secret store
    src/core/secret/SecretStoreService.cpp

    # Core — database
    src/core/database/DatabaseManager.cpp

    # Core — session
    src/core/session/UserSession.cpp

    # Core — calculator hub
    src/core/CalculadoraHub.cpp

    # Core — undo/redo commands
    src/core/commands/UndoCommands.cpp

    # Core — settings
    src/core/settings/AppSettings.cpp

    # Core — currency
    src/core/currency/FrankfurterProvider.cpp

    # Repositories
    src/repositories/PriceRepository.cpp
    src/repositories/CompraRepository.cpp
    src/repositories/ParcelaRepository.cpp
    src/repositories/FonteRendaRepository.cpp
    src/repositories/LancamentoEntradaRepository.cpp
    src/repositories/ProventoRepository.cpp

    # Services
    src/services/CalculadoraService.cpp
    src/services/ParcelaService.cpp
    src/services/CurrencyService.cpp
    src/services/ImportService.cpp
    src/services/MarketDataService.cpp
    src/services/import/IMatchFinder.cpp
    src/services/import/BankPDFParsers.cpp
    src/services/ExportService.cpp

    # UI
    src/ui/Theme.cpp
    src/ui/LoginDialog.cpp
    src/ui/MainWindow.cpp
    src/ui/IconGenerator.cpp
    src/ui/widgets/ParcelasWidget.cpp
    src/ui/widgets/ReceiptAttachmentWidget.cpp
    src/ui/widgets/AuditLogViewer.cpp
    src/ui/widgets/BackupConfigWidget.cpp
    src/ui/widgets/MarketConfigWidget.cpp
    src/ui/widgets/BolsaWidget.cpp
    src/ui/widgets/ProventoDialog.cpp
    src/ui/widgets/ProventosWidget.cpp
    src/ui/widgets/AjusteCorporativoDialog.cpp
    src/ui/widgets/AjustesCorporativosWidget.cpp
    src/ui/widgets/AddCompraDialog.cpp
    src/ui/widgets/ConfigWidget.cpp
    src/ui/widgets/PrefsWidget.cpp
    src/ui/widgets/AboutDialog.cpp
    src/ui/widgets/HelpDialog.cpp
    src/ui/widgets/EntradasWidget.cpp
    src/ui/widgets/LancamentoEntradaDialog.cpp
    src/ui/widgets/GastosWidget.cpp
    src/ui/widgets/GastoDialog.cpp
    src/repositories/GastoAvulsoRepository.cpp
    src/ui/widgets/ConfigFontesWidget.cpp
    src/ui/widgets/CompraDetailPanel.cpp
    src/ui/widgets/AmortizacaoDialog.cpp
    src/ui/widgets/CalculadoraEdit.cpp
    src/ui/widgets/CalculadoraDialog.cpp
    src/repositories/LancamentoInvestimentoRepository.cpp
    src/ui/widgets/InvestimentosWidget.cpp
    src/ui/widgets/InvestimentoDialog.cpp
    src/repositories/OrcamentoRepository.cpp
    src/ui/widgets/PlanejamentoWidget.cpp
    src/ui/widgets/ItemOrcamentoDialog.cpp
    src/repositories/CartaoRepository.cpp
    src/services/EntradaService.cpp
    src/services/CompraService.cpp
    src/ui/widgets/DashboardWidget.cpp
    src/ui/widgets/TendenciasWidget.cpp
    src/repositories/TransacaoRecorrenteRepository.cpp
    src/services/RecorrenciaService.cpp
    src/ui/widgets/RecorrenciaWidget.cpp
    src/ui/widgets/RecorrenciaDialog.cpp

    # Pagadores/Beneficiários (#29)
    src/repositories/PagadorRepository.cpp
    src/ui/widgets/PagadorDialog.cpp
    src/ui/widgets/ConfigPagadoresWidget.cpp

    # Categorias hierárquicas (#21)
    src/repositories/CategoriaRepository.cpp
    src/ui/widgets/CategoriaDialog.cpp
    src/ui/widgets/ConfigCategoriasWidget.cpp

    # Dívidas e empréstimos (#33)
    src/repositories/DividaRepository.cpp
    src/ui/widgets/DividaDialog.cpp
    src/ui/widgets/DividasWidget.cpp

    # Metas financeiras (#28)
    src/repositories/MetaRepository.cpp
    src/ui/widgets/MetaDialog.cpp
    src/ui/widgets/MetasWidget.cpp

    # Lembretes e alertas (#31)
    src/repositories/LembreteRepository.cpp
    src/ui/widgets/LembreteDialog.cpp
    src/ui/widgets/LembretesWidget.cpp

    # Exportação multi-seção (#11)
    src/ui/widgets/ExportDialog.cpp

    # Busca global (#32)
    src/ui/widgets/BuscaGlobalDialog.cpp

    # Reconciliação bancária (#27)
    src/ui/widgets/ReconciliacaoDialog.cpp

    # Base wizard reutilizavel (#87)
    src/ui/widgets/BaseWizardDialog.cpp
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(orcamento-pessoal ${SOURCES} ${RESOURCES})

# QXlsx exposes QXLSX_LIBRARY definition for conditional compilation
target_compile_definitions(orcamento-pessoal PRIVATE QXLSX_LIBRARY)

# Habilita import PDF se poppler-qt6-devel disponivel (#93)
if(POPPLER_QT6_FOUND)
    target_compile_definitions(orcamento-pessoal PRIVATE PEUBUD_HAS_POPPLER)
endif()

target_include_directories(orcamento-pessoal PRIVATE
    src
    ${LIBSECRET_INCLUDE_DIRS}
    ${LIBSODIUM_INCLUDE_DIRS}
)

target_link_libraries(orcamento-pessoal PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Network
    Qt6::Sql
    Qt6::PrintSupport
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSECRET_LIBRARIES}
    ${LIBSODIUM_LIBRARIES}
    QXlsx::QXlsx
)

if(POPPLER_QT6_FOUND)
    target_link_libraries(orcamento-pessoal PRIVATE PkgConfig::POPPLER_QT6)
endif()

# ── Translations (Qt Linguist) ────────────────────────────────────────────────
set(TS_FILES resources/translations/peubud_en.ts)
qt_add_lupdate(orcamento-pessoal TS_FILES ${TS_FILES} SOURCES ${SOURCES})
qt_add_lrelease(orcamento-pessoal
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE APP_QM_FILES
)
qt_add_resources(orcamento-pessoal "i18n"
    PREFIX  "/translations"
    BASE    "${CMAKE_CURRENT_BINARY_DIR}"
    FILES   ${APP_QM_FILES}
)

# Hardening flags (release only)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(orcamento-pessoal PRIVATE
        -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2
    )
    target_link_options(orcamento-pessoal PRIVATE
        -Wl,-z,relro -Wl,-z,now
    )
endif()

# Warnings
target_compile_options(orcamento-pessoal PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
)

# ── Testes (#102) ─────────────────────────────────────────────────────────────
# Bootstrap mínimo para TDD. Default OFF: build normal não é afetado.
# Habilitar com:  cmake -B build -DBUILD_TESTING=ON
# Rodar:          ctest --test-dir build --output-on-failure
option(BUILD_TESTING "Compilar testes (ctest + QTest)" OFF)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()

# ═════════════════════════════════════════════════════════════════════════════
# CPack: gera DEB + RPM + TGZ + source tarball (#96)
# Uso: cd build && cpack -G DEB     (Debian/Ubuntu)
#      cd build && cpack -G RPM     (Fedora/RHEL)
#      cd build && cpack -G TGZ     (genérico)
#      cd build && cpack --config CPackSourceConfig.cmake  (source tarball)
# ═════════════════════════════════════════════════════════════════════════════

include(GNUInstallDirs)

# Instalação dos artefatos no destino
install(TARGETS orcamento-pessoal
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Metadados CPack
set(CPACK_PACKAGE_NAME            "peubud")
set(CPACK_PACKAGE_VENDOR          "Petrus Silva Costa")
set(CPACK_PACKAGE_CONTACT         "petrinhu@yahoo.com.br")
set(CPACK_PACKAGE_VERSION         "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Aplicativo de orçamento pessoal em C++20/Qt6 com banco cifrado")
set(CPACK_PACKAGE_HOMEPAGE_URL
    "https://codeberg.org/petrinhu/orcamento_pessoal")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PeuBud")
set(CPACK_RESOURCE_FILE_LICENSE   "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README    "${CMAKE_SOURCE_DIR}/README.md")

set(CPACK_GENERATOR "TGZ;DEB;RPM")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    "/build/"
    "/build-test/"
    "/build-sanitize/"
    "/\\\\.git/"
    "/\\\\.idea/"
    "/data/"
    "\\\\.user$"
    "\\\\.qm$"
)

# ── DEB ─────────────────────────────────────────────────────────────────────
set(CPACK_DEBIAN_PACKAGE_NAME        "peubud")
set(CPACK_DEBIAN_PACKAGE_SECTION     "misc")
set(CPACK_DEBIAN_PACKAGE_PRIORITY    "optional")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER  "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE    "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS   ON)
# Fallback explicito caso shlibdeps nao seja executado
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6, libqt6gui6, libqt6widgets6, libqt6charts6, libqt6sql6, libqt6network6, libqt6printsupport6, libssl3, libsecret-1-0, libsodium23")

# ── RPM ─────────────────────────────────────────────────────────────────────
set(CPACK_RPM_PACKAGE_NAME           "peubud")
set(CPACK_RPM_PACKAGE_GROUP          "Applications/Office")
set(CPACK_RPM_PACKAGE_LICENSE        "PolyForm Noncommercial 1.0.0")
set(CPACK_RPM_PACKAGE_URL            "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_REQUIRES
    "qt6-qtbase, qt6-qtcharts, openssl-libs, libsecret, libsodium")
set(CPACK_RPM_PACKAGE_AUTOREQPROV    ON)
set(CPACK_RPM_PACKAGE_RELOCATABLE    OFF)

# ── TGZ ─────────────────────────────────────────────────────────────────────
set(CPACK_ARCHIVE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)
