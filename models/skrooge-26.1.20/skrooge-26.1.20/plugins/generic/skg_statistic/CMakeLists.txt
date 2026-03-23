#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_STATISTIC ::..")

PROJECT(plugin_statistic)

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX("sys/utsname.h" HAVE_UNAME)
IF(HAVE_UNAME)
    MESSAGE( STATUS "uname found")
    add_definitions(-DHAVE_UNAME)
ELSE(HAVE_UNAME)
    MESSAGE( STATUS "uname NOT FOUND")
ENDIF(HAVE_UNAME)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_statistic_SRCS
	skgstatisticplugin.cpp)

KCOREADDONS_ADD_PLUGIN(skg_statistic SOURCES ${skg_statistic_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_statistic KF6::Parts Qt6::Core skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_statistic.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_statistic )
