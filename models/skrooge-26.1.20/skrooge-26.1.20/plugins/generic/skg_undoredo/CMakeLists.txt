#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_UNDOREDO ::..")

PROJECT(plugin_undoredo)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_undoredo_SRCS skgundoredoplugin.cpp skgundoredoplugindockwidget.cpp)

ki18n_wrap_ui(skg_undoredo_SRCS skgundoredoplugindockwidget_base.ui skgundoredopluginwidget_pref.ui)

kconfig_add_kcfg_files(skg_undoredo_SRCS skgundoredo_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skg_undoredo SOURCES ${skg_undoredo_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_undoredo KF6::Parts skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgundoredo_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_undoredo.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_undoredo )
