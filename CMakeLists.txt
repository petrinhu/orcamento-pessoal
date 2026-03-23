cmake_minimum_required(VERSION 3.16)
project(OrcamentoPessoal VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui GuiPrivate Widgets Charts Network Sql LinguistTools)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSECRET REQUIRED libsecret-1)

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

    # Core — secret store
    src/core/secret/SecretStoreService.cpp

    # Core — database
    src/core/database/DatabaseManager.cpp

    # Core — session
    src/core/session/UserSession.cpp

    # Core — calculator hub
    src/core/CalculadoraHub.cpp

    # Core — settings
    src/core/settings/AppSettings.cpp

    # Core — currency
    src/core/currency/FrankfurterProvider.cpp

    # Repositories
    src/repositories/CompraRepository.cpp
    src/repositories/ParcelaRepository.cpp
    src/repositories/FonteRendaRepository.cpp
    src/repositories/LancamentoEntradaRepository.cpp

    # Services
    src/services/CalculadoraService.cpp
    src/services/ParcelaService.cpp
    src/services/CurrencyService.cpp
    src/services/ImportService.cpp
    src/services/ExportService.cpp

    # UI
    src/ui/Theme.cpp
    src/ui/LoginDialog.cpp
    src/ui/MainWindow.cpp
    src/ui/IconGenerator.cpp
    src/ui/widgets/ParcelasWidget.cpp
    src/ui/widgets/AddCompraDialog.cpp
    src/ui/widgets/ConfigWidget.cpp
    src/ui/widgets/PrefsWidget.cpp
    src/ui/widgets/AboutDialog.cpp
    src/ui/widgets/HelpDialog.cpp
    src/ui/widgets/EntradasWidget.cpp
    src/ui/widgets/LancamentoEntradaDialog.cpp
    src/ui/widgets/ConfigFontesWidget.cpp
    src/ui/widgets/CompraDetailPanel.cpp
    src/ui/widgets/AmortizacaoDialog.cpp
    src/ui/widgets/CalculadoraEdit.cpp
    src/ui/widgets/CalculadoraDialog.cpp
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(orcamento-pessoal ${SOURCES} ${RESOURCES})

# QXlsx exposes QXLSX_LIBRARY definition for conditional compilation
target_compile_definitions(orcamento-pessoal PRIVATE QXLSX_LIBRARY)

target_include_directories(orcamento-pessoal PRIVATE
    src
    ${LIBSECRET_INCLUDE_DIRS}
)

target_link_libraries(orcamento-pessoal PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Charts
    Qt6::Network
    Qt6::Sql
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBSECRET_LIBRARIES}
    QXlsx::QXlsx
)

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
