#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_DEBUG ::..")

PROJECT(plugin_debug)

IF(SKG_BUILD_TEST AND NOT WIN32)
    ADD_SUBDIRECTORY(tests)
ENDIF(SKG_BUILD_TEST AND NOT WIN32)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_debug_SRCS skgdebugplugin.cpp skgdebugpluginwidget.cpp)

ki18n_wrap_ui(skg_debug_SRCS skgdebugpluginwidget_base.ui)

KCOREADDONS_ADD_PLUGIN(skg_debug SOURCES ${skg_debug_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_debug KF6::Parts skgbasemodeler skgbasegui Qt6::Qml)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_debug.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_debug )
