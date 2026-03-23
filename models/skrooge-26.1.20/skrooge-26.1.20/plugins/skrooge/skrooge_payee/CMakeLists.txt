#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_PAYEE ::..")

PROJECT(plugin_payee)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_payee_SRCS
	skgpayeeplugin.cpp
	skgpayeepluginwidget.cpp)

ki18n_wrap_ui(skrooge_payee_SRCS skgpayeepluginwidget_base.ui skgpayeepluginwidget_pref.ui)
kconfig_add_kcfg_files(skrooge_payee_SRCS skgpayee_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skrooge_payee SOURCES ${skrooge_payee_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_payee KF6::Parts KF6::ItemViews skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_payee.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_payee )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgpayee_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )

