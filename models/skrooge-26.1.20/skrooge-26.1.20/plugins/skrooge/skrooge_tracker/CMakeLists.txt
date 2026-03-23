#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_TRACKER ::..")

PROJECT(plugin_tracker)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_tracker_SRCS
	skgtrackerplugin.cpp
	skgtrackerpluginwidget.cpp)

ki18n_wrap_ui(skrooge_tracker_SRCS skgtrackerpluginwidget_base.ui skgtrackerpluginwidget_pref.ui)
kconfig_add_kcfg_files(skrooge_tracker_SRCS skgtracker_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skrooge_tracker SOURCES ${skrooge_tracker_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_tracker KF6::Parts KF6::ItemViews skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_tracker.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_tracker )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgtracker_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
