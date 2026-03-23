#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_BOOKMARK ::..")

PROJECT(plugin_bookmark)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_bookmark_SRCS skgbookmarkplugin.cpp skgbookmarkplugindockwidget.cpp)

ki18n_wrap_ui(skg_bookmark_SRCS skgbookmarkplugindockwidget_base.ui skgbookmarkpluginwidget_pref.ui)

kconfig_add_kcfg_files(skg_bookmark_SRCS skgbookmark_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skg_bookmark SOURCES ${skg_bookmark_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_bookmark KF6::Parts KF6::ItemViews KF6::IconThemes KF6::IconWidgets skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgbookmark_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_bookmark.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_bookmark )
