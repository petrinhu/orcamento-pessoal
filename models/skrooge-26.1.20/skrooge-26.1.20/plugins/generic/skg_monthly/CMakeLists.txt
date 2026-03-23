#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_MONTHLY ::..")

PROJECT(plugin_monthly)

ADD_SUBDIRECTORY(grantlee_filters)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_monthly_SRCS
	skgmonthlyplugin.cpp
	skgmonthlypluginwidget.cpp)

ki18n_wrap_ui(skg_monthly_SRCS skgmonthlypluginwidget_base.ui)

KCOREADDONS_ADD_PLUGIN(skg_monthly SOURCES ${skg_monthly_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_monthly KF6::Parts KF6::ItemViews KF6::NewStuffWidgets KF6::Archive Qt6::PrintSupport skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_monthly.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_monthly )
