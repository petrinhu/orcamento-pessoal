#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_FILE ::..")

PROJECT(plugin_file)

IF(SKG_BUILD_TEST AND NOT WIN32)
    ADD_SUBDIRECTORY(tests)
ENDIF(SKG_BUILD_TEST AND NOT WIN32)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_file_SRCS skgfileplugin.cpp )

ki18n_wrap_ui(skg_file_SRCS skgfilepluginwidget_pref.ui )

kconfig_add_kcfg_files(skg_file_SRCS skgfile_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skg_file SOURCES ${skg_file_SRCS} INSTALL_NAMESPACE "skg_gui")

TARGET_LINK_LIBRARIES(skg_file KF6::Parts KF6::ItemViews KF6::Wallet skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgfile_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_file.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_file )
