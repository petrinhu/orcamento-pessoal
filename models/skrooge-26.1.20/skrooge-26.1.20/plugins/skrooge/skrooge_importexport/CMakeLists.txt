#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORTEXPORT ::..")

PROJECT(plugin_importexport)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_importexport_SRCS
skgimportexportplugin.cpp)

ki18n_wrap_ui(skrooge_importexport_SRCS skgimportexportpluginwidget_pref.ui)
kconfig_add_kcfg_files(skrooge_importexport_SRCS skgimportexport_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skrooge_importexport SOURCES ${skrooge_importexport_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_importexport KF6::Parts KF6::KIOCore KF6::KIOFileWidgets KF6::KIOWidgets skgbasemodeler skgbasegui skgbankmodeler)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_importexport.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_importexport )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgimportexport_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
