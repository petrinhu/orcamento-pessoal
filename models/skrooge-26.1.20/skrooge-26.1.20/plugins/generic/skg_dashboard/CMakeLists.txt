#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_DASHBOARD ::..")

PROJECT(plugin_dashboard)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_dashboard_SRCS
	skgdashboardplugin.cpp
	skgdashboardpluginwidget.cpp
        skgdashboardwidget.cpp
        skgdashboardboardwidget.cpp)

KCOREADDONS_ADD_PLUGIN(skg_dashboard SOURCES ${skg_dashboard_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_dashboard Qt6::Gui KF6::Parts skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_dashboard.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_dashboard )
