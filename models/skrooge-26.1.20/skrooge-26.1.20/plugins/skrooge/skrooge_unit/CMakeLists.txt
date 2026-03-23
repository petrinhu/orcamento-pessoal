#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_UNIT ::..")

PROJECT(plugin_unit)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_unit_SRCS skgunitplugin.cpp skgunitpluginwidget.cpp skgunitboardwidget.cpp)

ki18n_wrap_ui(skrooge_unit_SRCS skgunitpluginwidget_base.ui skgunitpluginwidget_pref.ui)

kconfig_add_kcfg_files(skrooge_unit_SRCS skgunit_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skrooge_unit SOURCES ${skrooge_unit_SRCS} INSTALL_NAMESPACE "skg_gui")
FIND_PACKAGE(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS NewStuff)
TARGET_LINK_LIBRARIES(skrooge_unit KF6::Parts KF6::NewStuffWidgets KF6::Archive skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgunit_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_unit.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_unit )
INSTALL(FILES skrooge_unit.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
