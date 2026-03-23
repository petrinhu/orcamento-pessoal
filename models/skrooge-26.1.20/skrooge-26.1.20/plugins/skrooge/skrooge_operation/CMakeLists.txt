#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_OPERATION ::..")

PROJECT(plugin_operation)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_operation_SRCS
	skgsplittabledelegate.cpp
	skgoperationplugin.cpp
	skgoperationpluginwidget.cpp 
	skgoperationboardwidgetqml.cpp
)

ki18n_wrap_ui(skrooge_operation_SRCS skgoperationpluginwidget_base.ui skgoperationpluginwidget_pref.ui)
kconfig_add_kcfg_files(skrooge_operation_SRCS skgoperation_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skrooge_operation SOURCES ${skrooge_operation_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_operation KF6::Parts KF6::ItemViews skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_operation.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_operation )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgoperation_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
