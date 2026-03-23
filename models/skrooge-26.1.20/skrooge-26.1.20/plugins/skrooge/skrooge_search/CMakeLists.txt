#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_SEARCH ::..")

PROJECT(plugin_search)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_search_SRCS
	skgsearchplugin.cpp
        skgsearchpluginwidget.cpp)

ki18n_wrap_ui(skrooge_search_SRCS skgsearchpluginwidget_base.ui skgsearchpluginwidget_pref.ui)
kconfig_add_kcfg_files(skrooge_search_SRCS skgsearch_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skrooge_search SOURCES ${skrooge_search_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_search KF6::Parts KF6::ItemViews skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_search.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_search )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgsearch_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
