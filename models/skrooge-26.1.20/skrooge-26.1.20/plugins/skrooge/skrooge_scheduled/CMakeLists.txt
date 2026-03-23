#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_SCHEDULED ::..")

PROJECT(plugin_scheduled)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_scheduled_SRCS
	skgscheduledplugin.cpp
	skgscheduledpluginwidget.cpp
        skgscheduledboardwidget.cpp
)

ki18n_wrap_ui(skrooge_scheduled_SRCS skgscheduledpluginwidget_base.ui skgscheduledpluginwidget_pref.ui)
kconfig_add_kcfg_files(skrooge_scheduled_SRCS skgscheduled_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skrooge_scheduled SOURCES ${skrooge_scheduled_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_scheduled KF6::Parts KF6::ItemViews skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_scheduled.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_scheduled )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgscheduled_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )

