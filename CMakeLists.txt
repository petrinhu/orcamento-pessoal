cmake_minimum_required(VERSION 3.16)
project(OrcamentoPessoal VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts Network Sql)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSECRET REQUIRED libsecret-1)

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

    # Core — currency
    src/core/currency/FrankfurterProvider.cpp

    # Repositories
    src/repositories/CompraRepository.cpp
    src/repositories/ParcelaRepository.cpp

    # Services
    src/services/ParcelaService.cpp
    src/services/CurrencyService.cpp

    # UI
    src/ui/Theme.cpp
    src/ui/LoginDialog.cpp
    src/ui/MainWindow.cpp
    src/ui/widgets/ParcelasWidget.cpp
    src/ui/widgets/AddCompraDialog.cpp
    src/ui/widgets/ConfigWidget.cpp
)

set(RESOURCES
    resources/resources.qrc
)

add_executable(orcamento-pessoal ${SOURCES} ${RESOURCES})

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
