cmake_minimum_required(VERSION 3.16)
project(OrcamentoPessoal VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ── Hardening de binário (DIST-09) ─────────────────────────────────────────────
# PIE em TODOS os build types (não só Release): o app mantém segredos/dados
# financeiros descriptografados em memória — PIE (ASLR do executável) reduz a
# superfície de ROP/exploit. Setado ANTES de find_package/FetchContent para
# que TODO target subsequente (inclusive QXlsx, vendorizado via FetchContent
# abaixo) herde -fPIE/-fPIC: um único objeto não-PIC linkado no binário final
# derruba o PIE do executável inteiro (é por isso que "No PIE" aparecia mesmo
# com o alvo principal compilado com -fPIE implícito do GCC).
#
# CMAKE_POSITION_INDEPENDENT_CODE sozinho só afeta a flag de COMPILAÇÃO
# (-fPIE). A flag de LINK (-pie) só é emitida se a policy CMP0083 estiver
# NEW, o que exige chamar check_pie_supported() explicitamente (módulo
# CheckPIESupported, CMake >= 3.14) — sem isso o binário compila com -fPIE
# mas linka sem -pie e checksec continua reportando "No PIE".
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
include(CheckPIESupported)
check_pie_supported()

# _FORTIFY_SOURCE=2 exige otimização (>=O1); mantido só em Release (Debug não
# otimiza, de propósito, para depuração). Aplicado via add_compile_options
# (diretiva de diretório, não target_compile_options) e ANTES do
# FetchContent_MakeAvailable(QXlsx) abaixo, para propagar a TODAS as TUs do
# binário — inclusive as do QXlsx. checksec conta "Fortified N/M" sobre o
# binário linkado inteiro; se só o alvo principal ganhasse a flag, o QXlsx
# ficaria de fora e o binário reportaria fortificação parcial/zero.
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2)
    add_link_options(-Wl,-z,relro -Wl,-z,now)
endif()

# ── Suporte a XLSX opcional (DIST-04) ──────────────────────────────────────────
# QXlsx usa API PRIVADA do Qt (QZipReader via <private/qzipreader_p.h>) e por
# isso exige Qt6::GuiPrivate — cujo cmake-config só existe em Qt >= 6.9
# (pacote qt6-qtbase-private-devel / qt6-base-private-dev). Em distros com Qt
# mais antigo (Debian stable ~6.8) ou que promovem o aviso de "private
# module" a erro (Arch), o build inteiro quebrava. PEUBUD_HAS_XLSX=OFF
# compila sem QXlsx/GuiPrivate: ExportService::toXlsx/toMultiXlsx e
# ImportService::fromXlsx já degradam graciosamente via `#ifdef
# QXLSX_LIBRARY` (retornam erro orientativo em vez de crash/link error).
option(PEUBUD_HAS_XLSX "Habilita suporte a XLSX via QXlsx (requer Qt6::GuiPrivate)" ON)

if(PEUBUD_HAS_XLSX)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui GuiPrivate Widgets Charts Network Sql LinguistTools PrintSupport)
else()
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Network Sql LinguistTools PrintSupport)
    message(STATUS "PEUBUD_HAS_XLSX=OFF: export/import XLSX desabilitado (fallback orientativo em runtime, sem GuiPrivate)")
endif()

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
if(PEUBUD_HAS_XLSX)
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
endif()

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
    src/services/GlossarioService.cpp
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

    # Services — Activity pattern Investimentos (#89)
    src/services/investimento/InvestActivities.cpp
    src/services/investimento/InvestActivityFactory.cpp

    # External process wrapper (#86)
    src/services/external/ExternalProcessProvider.cpp

    # Core — secret store
    src/core/secret/SecretStoreService.cpp

    # Core — database
    src/core/database/DatabaseManager.cpp

    # Core — composition root do schema Mid/Backend (#200/ARQ-01)
    src/core/AppBootstrap.cpp

    # Core — platform (#192: handler de sinal SIGTERM/SIGINT via self-pipe)
    src/core/platform/UnixSignalHandler.cpp

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
    src/services/MarketProviderCatalog.cpp
    src/services/IMarketDataProvider.cpp
    src/services/BrapiProvider.cpp
    src/services/import/IMatchFinder.cpp
    src/services/import/BankPDFParsers.cpp
    src/services/ExportService.cpp

    # Export writers Strategy (#200/ARQ-05)
    src/services/export/ExportWriters.cpp
    src/services/export/ExportWriterRegistry.cpp

    # UI
    src/ui/UiFormatters.cpp
    src/ui/AderenciaOrcamento.cpp
    src/ui/TabHelp.cpp
    src/ui/NavRegistry.cpp
    src/ui/Theme.cpp
    src/ui/ButtonShadowFilter.cpp
    src/ui/FocusVisibleFilter.cpp
    src/ui/TableHoverFilter.cpp
    src/ui/TableHoverDelegate.cpp
    src/ui/GlossarioDecorator.cpp
    src/ui/GlossaryUnderlineFilter.cpp
    src/ui/LazyPageRouter.cpp
    src/ui/LoginDialog.cpp
    src/ui/MainWindow.cpp
    src/ui/widgets/ParcelasWidget.cpp
    src/ui/widgets/TabShell.cpp
    src/ui/widgets/ReceiptAttachmentWidget.cpp
    src/ui/widgets/AuditLogViewer.cpp
    src/ui/widgets/BackupConfigWidget.cpp
    src/ui/widgets/MarketConfigWidget.cpp
    src/ui/widgets/CurrencyPanelWidget.cpp
    src/ui/widgets/BolsaWidget.cpp
    src/ui/widgets/ProventoDialog.cpp
    src/ui/widgets/ProventosWidget.cpp
    src/ui/widgets/AjusteCorporativoDialog.cpp
    src/ui/widgets/AjustesCorporativosWidget.cpp
    src/ui/widgets/EmptyStateWidget.cpp
    src/ui/widgets/AddCompraDialog.cpp
    src/ui/widgets/ConfigWidget.cpp
    src/ui/widgets/PrefsWidget.cpp
    src/ui/widgets/AboutDialog.cpp
    src/ui/widgets/HelpDialog.cpp
    src/ui/widgets/GlossarioDialog.cpp
    src/ui/widgets/GlossaryLabel.cpp
    src/ui/widgets/GlossaryUnderline.cpp
    src/ui/widgets/OnboardingDialog.cpp
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
    src/services/ResumoService.cpp
    src/services/GastoService.cpp
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

    # Wizards guiados pergunta-a-pergunta (#108)
    src/ui/widgets/WizardGuiadoDialog.cpp

    # Sidebar de navegacao agrupada (#165 redesign Fase 2 Task 1)
    src/ui/widgets/SidebarNav.cpp
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(orcamento-pessoal ${SOURCES} ${RESOURCES})

# QXlsx exposes QXLSX_LIBRARY definition for conditional compilation.
# Só definido quando PEUBUD_HAS_XLSX=ON (DIST-04) — os call-sites
# (ExportService::toXlsx/toMultiXlsx, ImportService::fromXlsx) já checam
# `#ifdef QXLSX_LIBRARY` e degradam graciosamente sem essa macro.
if(PEUBUD_HAS_XLSX)
    target_compile_definitions(orcamento-pessoal PRIVATE QXLSX_LIBRARY)
endif()

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
)

if(PEUBUD_HAS_XLSX)
    target_link_libraries(orcamento-pessoal PRIVATE QXlsx::QXlsx)
endif()

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

# Hardening flags (release only) — aplicados globalmente via add_compile_options/
# add_link_options no topo do arquivo (DIST-09), para propagar também às TUs do
# QXlsx. Bloco antigo per-target removido daqui (duplicava a flag só no alvo
# principal, deixando o QXlsx de fora do FORTIFY).

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

# Desktop integration (#98)
install(FILES resources/desktop/org.peubud.PeuBud.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES resources/desktop/org.peubud.PeuBud.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

# Icone hicolor multi-size (#152). PNGs 16..512 gerados a partir do fonte
# 1024x1024 (resources/icon/icon.png) e instalados em cada bucket hicolor.
# Sem SVG ainda: nao ha fonte vetorial real — raster -> SVG nao agrega
# (pendencia de design vetorial).
foreach(_icon_size 16 22 32 48 64 128 256 512)
    install(FILES
        resources/icon/hicolor/${_icon_size}x${_icon_size}/apps/org.peubud.PeuBud.png
        DESTINATION
        ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${_icon_size}x${_icon_size}/apps)
endforeach()

install(FILES resources/desktop/peubud.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

# Licenças de fontes de terceiros embutidas no binário (DIST-05). A OFL 1.1
# (Inter, Noto Color Emoji) exige que o texto da licença acompanhe toda
# redistribuição — instalar em /usr/share/licenses/peubud/ (convenção RPM;
# funciona também para DEB/TGZ, que não exigem local fixo). QXlsx (MIT) e
# libsodium (ISC) são citados no AboutDialog (seção "Terceiros"), mas não têm
# arquivo de licença embarcado no repo — texto completo disponível upstream.
install(FILES
    resources/fonts/LICENSE-Inter.txt
    resources/fonts/LICENSE-NotoColorEmoji.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/peubud)

# Renomeia binário instalado para "peubud" (Exec= no .desktop)
set_target_properties(orcamento-pessoal PROPERTIES OUTPUT_NAME peubud)

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
    "/build-rel/"
    "/build-verif/"
    "/\\\\.git/"
    "/\\\\.idea/"
    "/\\\\.superpowers/"
    # #198 (achado durante o gate de ctest — adjacente a #96/DIST-01):
    # .claude/worktrees (agent worktrees do Claude Code) e .cache/clangd
    # (index do clangd, ja em .gitignore mas o CPack tem lista PROPRIA,
    # independente de .gitignore) inflavam o tarball fonte para >5MB assim
    # que essas pastas de ferramenta existiam no working tree local.
    "/\\\\.claude/"
    "/\\\\.cache/"
    "/data/"
    "\\\\.user$"
    "\\\\.qm$"
    # #96 (Onda U, DIST-01): exclui SO o models/ da RAIZ do projeto (apps de
    # referencia de terceiros, 670MB) - ancorado em CMAKE_SOURCE_DIR para nao
    # colidir com src/models/ (domain structs POD legitimos, tem que ir no
    # tarball fonte).
    "^${CMAKE_SOURCE_DIR}/models/"
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
# poppler-qt6 e opcional (PEUBUD_HAS_POPPLER): habilita import de PDF.
# Declarado como dependencia fraca (Suggests) — nao bloqueia instalacao.
set(CPACK_DEBIAN_PACKAGE_SUGGESTS    "libpoppler-qt6-3")

# ── RPM ─────────────────────────────────────────────────────────────────────
set(CPACK_RPM_PACKAGE_NAME           "peubud")
set(CPACK_RPM_PACKAGE_GROUP          "Applications/Office")
set(CPACK_RPM_PACKAGE_LICENSE        "AGPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_URL            "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_REQUIRES
    "qt6-qtbase, qt6-qtcharts, openssl-libs, libsecret, libsodium")
# poppler-qt6 e opcional (PEUBUD_HAS_POPPLER): habilita import de PDF.
# Weak dependency RPM — recomendada, mas nao obrigatoria.
set(CPACK_RPM_PACKAGE_SUGGESTS       "poppler-qt6")
set(CPACK_RPM_PACKAGE_AUTOREQPROV    ON)
set(CPACK_RPM_PACKAGE_RELOCATABLE    OFF)

# ── TGZ ─────────────────────────────────────────────────────────────────────
set(CPACK_ARCHIVE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

include(CPack)
