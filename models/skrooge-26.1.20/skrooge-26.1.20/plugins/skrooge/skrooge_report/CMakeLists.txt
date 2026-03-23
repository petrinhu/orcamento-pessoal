#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_REPORT ::..")

PROJECT(plugin_report)

IF(SKG_BUILD_TEST AND NOT WIN32)
    ADD_SUBDIRECTORY(tests)
ENDIF(SKG_BUILD_TEST AND NOT WIN32)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_report_SRCS
	skgreportplugin.cpp
	skgreportpluginwidget.cpp
	skgreportboardwidget.cpp)

ki18n_wrap_ui(skrooge_report_SRCS skgreportpluginwidget_base.ui skgreportpluginwidget_pref.ui)
kconfig_add_kcfg_files(skrooge_report_SRCS skgreport_settings.kcfgc )

KCOREADDONS_ADD_PLUGIN(skrooge_report SOURCES ${skrooge_report_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_report KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_report.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_report )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgreport_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
