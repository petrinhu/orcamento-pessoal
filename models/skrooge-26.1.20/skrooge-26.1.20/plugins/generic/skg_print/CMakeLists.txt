#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_PRINT ::..")

PROJECT(plugin_print)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_print_SRCS
	skgprintplugin.cpp)


ki18n_wrap_ui(skg_print_SRCS skgprintpluginwidget_pref.ui)
kconfig_add_kcfg_files(skg_print_SRCS skgprint_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skg_print SOURCES ${skg_print_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_print KF6::Parts Qt6::PrintSupport skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_print.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_print )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgprint_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
