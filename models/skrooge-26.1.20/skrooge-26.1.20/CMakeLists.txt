#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE ROOT ::..")
CMAKE_MINIMUM_REQUIRED(VERSION 3.16.0)
PROJECT(skrooge)

SET(KF_IGNORE_PLATFORM_CHECK TRUE)

ADD_COMPILE_DEFINITIONS(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

OPTION(SKG_BUILD_TEST "Build the test" ON)
# OPTION(SKG_DESIGNER   "Build designer library" ON)
OPTION(SKG_WEBENGINE  "Build Skrooge with WebEngine" ON)
OPTION(SKG_DBUS  "Build with DBUS" ON)
IF(WIN32)
    SET(SKG_WEBENGINE OFF)
    SET(SKG_DESIGNER OFF)
ENDIF(WIN32)

SET(KF_MIN_VERSION "6.3.0")
SET(SKG_DESIGNER OFF)
ADD_DEFINITIONS(-DBUILD_WITH_QT6=ON)

IF(POLICY CMP0063)
  CMAKE_POLICY(SET CMP0063 NEW)
ENDIF(POLICY CMP0063)

FIND_PACKAGE(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${skrooge_SOURCE_DIR}/cmake/modules)
INCLUDE(ECMInstallIcons)
INCLUDE(KDEInstallDirs)
INCLUDE(KDECompilerSettings)
INCLUDE(KDECMakeSettings)
INCLUDE(FeatureSummary)
INCLUDE(CheckCXXCompilerFlag)
INCLUDE(GenerateExportHeader)
INCLUDE(ECMSetupVersion)
INCLUDE(ECMGenerateHeaders)
INCLUDE(CMakePackageConfigHelpers)
INCLUDE(KDEFrameworkCompilerSettings)

# Qt 6 build
ADD_COMPILE_DEFINITIONS(
#    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_DEPRECATED_WARNINGS
    -DQT_STRICT_ITERATORS
)

FIND_PACKAGE(Qt6 REQUIRED COMPONENTS Widgets Sql Test PrintSupport Svg Xml Concurrent Qml QuickWidgets Core5Compat)

IF(Qt6Core_VERSION VERSION_GREATER_EQUAL 6.10)
    FIND_PACKAGE(Qt6 REQUIRED COMPONENTS SqlPrivate)
ENDIF()

IF(SKG_WEBENGINE)
    FIND_PACKAGE(Qt6 REQUIRED COMPONENTS WebEngineWidgets)
    MESSAGE( STATUS "     Mode WebEngine")
    ADD_DEFINITIONS(-DSKG_WEBENGINE=${SKG_WEBENGINE})
ENDIF(SKG_WEBENGINE)

IF(SKG_DBUS)
    FIND_PACKAGE(Qt6 REQUIRED COMPONENTS DBus)
    MESSAGE( STATUS "     DBUS enabled")
    ADD_DEFINITIONS(-DSKG_DBUS=${SKG_DBUS})
ELSE(SKG_DBUS)
    MESSAGE( STATUS "     DBUS disabled")
ENDIF(SKG_DBUS)

IF(SKG_DESIGNER)
  FIND_PACKAGE(Qt6 REQUIRED COMPONENTS Designer )
  FIND_PACKAGE(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    DesignerPlugin        # Tier 3
  )
ENDIF(SKG_DESIGNER)

FIND_PACKAGE(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
  Archive               # Tier 1
  Config                # Tier 1
  CoreAddons            # Tier 1
  I18n                  # Tier 1
  ItemViews             # Tier 1
  WidgetsAddons         # Tier 1
  GuiAddons             # Tier 1
  Completion            # Tier 2
  ConfigWidgets         # Tier 3
  IconThemes            # Tier 3
  KIO                   # Tier 3
  NewStuffCore          # Tier 3
  NewStuff              # Tier 3
  Parts                 # Tier 3
  Wallet                # Tier 3
  XmlGui                # Tier 3
  NotifyConfig          # Tier 3
  Notifications
  ColorScheme
  TextTemplate  
)

FIND_PACKAGE(KF6DocTools)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(SQLCIPHER REQUIRED sqlcipher)
MESSAGE( STATUS "     Mode SQLCIPHER enabled")
MESSAGE( STATUS "     SQLCIPHER INCLUDE   :  " ${SQLCIPHER_INCLUDE_DIRS})
SET(SQLITE_LIBRARIES ${SQLCIPHER_LIBRARIES})
SET(SQLITE_INCLUDE_DIR ${SQLCIPHER_INCLUDE_DIRS})
link_directories(${SQLCIPHER_LIBRARY_DIRS})

SET_PACKAGE_PROPERTIES(Sqlcipher PROPERTIES DESCRIPTION "Support for encryption"
                       URL "https://www.zetetic.net/sqlcipher/"
                       TYPE REQUIRED)


SET(SKG_VERSION "26.1.20")
SET(SKG_BETA "")
SET(SOVERSION 2)

IF (CMAKE_BUILD_TYPE)
ELSE (CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE profile)
#"release": optimized for speed, no debug symbols or debug messages
#"relwithdebinfo": optimized for speed, debug symbols for backtraces
#"debug": optimized, but debuggable
#"debugfull": no optimizations, full debug support
#"profile": adds coverage flags to debugfull
#"none": the build flags are manually set using the CMAKE_CXX_FLAGS option.
ENDIF (CMAKE_BUILD_TYPE)

FIND_PROGRAM(SKG_BASH bash)
MARK_AS_ADVANCED(SKG_BASH)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
MESSAGE( STATUS "     PROJECT_SOURCE_DIR      : " ${PROJECT_SOURCE_DIR} )
MESSAGE( STATUS "     PROJECT_BINARY_DIR      : " ${PROJECT_BINARY_DIR} )
MESSAGE( STATUS "     CMAKE_BUILD_TYPE        : " ${CMAKE_BUILD_TYPE} )
MESSAGE( STATUS "     CMAKE_MODULE_PATH       : " ${CMAKE_MODULE_PATH} )
MESSAGE( STATUS "     CMAKE_INSTALL_PREFIX    : " ${CMAKE_INSTALL_PREFIX} )
MESSAGE( STATUS "     SKG_BUILD_TEST          : " ${SKG_BUILD_TEST} )
MESSAGE( STATUS "     SKG_DESIGNER            : " ${SKG_DESIGNER} )
MESSAGE( STATUS "     SKG_WEBENGINE           : " ${SKG_WEBENGINE} )
MESSAGE( STATUS "     SKG_DBUS                : " ${SKG_DBUS} )
MESSAGE( STATUS "     KDE_INSTALL_QTPLUGINDIR : " ${KDE_INSTALL_QTPLUGINDIR})
MESSAGE( STATUS "     KDE_INSTALL_TARGETS_DEFAULT_ARGS : " ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

#IF(CMAKE_BUILD_TYPE MATCHES "profile")
#  MESSAGE( STATUS "     TRACES              : All traces activated")
  #add_definitions(-DSKGFULLTRACES)
#ELSE(CMAKE_BUILD_TYPE MATCHES "profile")
#  IF(CMAKE_BUILD_TYPE MATCHES "debug" OR SKG_BETA MATCHES "BETA")
    MESSAGE( STATUS "     TRACES              : Some traces activated")
#  ELSE(CMAKE_BUILD_TYPE MATCHES "debug" OR SKG_BETA MATCHES "BETA")
#    MESSAGE( STATUS "     TRACES              : All traces disabled")
#    add_definitions(-DSKGNOTRACES)
#  ENDIF(CMAKE_BUILD_TYPE MATCHES "debug" OR SKG_BETA MATCHES "BETA")
#ENDIF(CMAKE_BUILD_TYPE MATCHES "profile")

# FIX: Qt was built with -reduce-relocations
IF(Qt5_POSITION_INDEPENDENT_CODE)
  SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
ENDIF(Qt5_POSITION_INDEPENDENT_CODE)

ADD_DEFINITIONS(-DSKGVERSION=${SKG_VERSION}${SKG_BETA})
REMOVE_DEFINITIONS(-DQT_NO_CAST_FROM_ASCII)

#Correction bug 207249 vvvv
IF(CMAKE_COMPILER_IS_GNUCXX)
    CHECK_CXX_COMPILER_FLAG("-Wlogical-op" WLOGICALOP)
    IF(WLOGICALOP)
        ADD_COMPILE_OPTIONS(-Wlogical-op)
    ENDIF(WLOGICALOP)

    ADD_COMPILE_OPTIONS(-std=c++17 -Wno-pedantic -Wall -Wextra -Wno-variadic-macros -Wparentheses -Wmissing-braces -Wcast-align -Wcast-qual -Wformat=2 -Winit-self -Wmissing-include-dirs -Woverloaded-virtual -Wsign-promo -Wstrict-null-sentinel -Wundef -Wno-unused)
    #  ADD_COMPILE_OPTIONS(-Wstrict-overflow=5  -Wshadow  -Wctor-dtor-privacy  -Wshadow -Wold-style-cast -Wswitch-default -Wredundant-decls)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)
#Correction bug 207249 ^^^^

#Default installation path
SET(CMAKE_INSTALL_PREFIX ${BUILD_DIR})

# Win32 specific configuration
IF(WIN32)
    ADD_DEFINITIONS(-DSKG_PLATFORM_WIN32)
    IF(CMAKE_COMPILER_IS_GNUCXX)
	SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--enable-runtime-pseudo-reloc -Wl,--export-all-symbols" CACHE STRING "" FORCE)
    ENDIF(CMAKE_COMPILER_IS_GNUCXX)

    IF (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        ADD_DEFINITIONS( -DQT_DEBUG )
    ENDIF (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
ELSE(WIN32)
#To solve Bug 209912:"ldd -u <library>""
#does not work on MACOS, must be done manually		SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--as-needed" CACHE STRING "" FORCE)
ENDIF(WIN32)
MESSAGE(STATUS "     CMAKE_SHARED_LINKER_FLAGS : ${CMAKE_SHARED_LINKER_FLAGS}")

#ADD_SUBDIRECTORY
ADD_SUBDIRECTORY(skgsqlcipher)
ADD_SUBDIRECTORY(skgbasemodeler)
ADD_SUBDIRECTORY(skgbankmodeler)
ADD_SUBDIRECTORY(skgbasegui)
ADD_SUBDIRECTORY(skgbankgui)

IF(SKG_DESIGNER)
    ADD_SUBDIRECTORY(skgbaseguidesigner)
    ADD_SUBDIRECTORY(skgbankguidesigner)
ENDIF(SKG_DESIGNER)

ADD_SUBDIRECTORY(plugins)
ADD_SUBDIRECTORY(images)

IF(SKG_BUILD_TEST AND NOT WIN32)
    ADD_SUBDIRECTORY(tests)
ENDIF(SKG_BUILD_TEST AND NOT WIN32)

IF(KF6DocTools_FOUND)
  ADD_SUBDIRECTORY(doc)
ENDIF()

#Main application
ADD_SUBDIRECTORY(skrooge)
ADD_SUBDIRECTORY(skroogeconvert)

# ADD_SUBDIRECTORY(skrooge-companion)

#DOXYGEN
ADD_CUSTOM_TARGET(apidox "doxygen" "${PROJECT_SOURCE_DIR}/Doxyfile")

#GENERATE SPLASH SCREEN
CONFIGURE_FILE(scripts/generateSplash.sh.in scripts/generateSplash.sh @ONLY)
ADD_CUSTOM_TARGET(splash "scripts/generateSplash.sh")

#STATIC CODE CHECKS
CONFIGURE_FILE(scripts/codecheck.sh.in scripts/codecheck.sh @ONLY)
ADD_CUSTOM_TARGET(codecheck "scripts/codecheck.sh")

#FULL
CONFIGURE_FILE(scripts/full.sh scripts/full.sh @ONLY)

#PO files
KI18N_INSTALL(po)

FIND_PACKAGE(KF6DocTools CONFIG)
IF(KF6DocTools_FOUND)
    KDOCTOOLS_INSTALL(po)
ENDIF()

FEATURE_SUMMARY(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
