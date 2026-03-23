#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_BUDGET ::..")

PROJECT(plugin_budget)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_budget_SRCS
	skgbudgetdelegate.cpp
	skgbudgetplugin.cpp
	skgbudgetpluginwidget.cpp)

ki18n_wrap_ui(skrooge_budget_SRCS skgbudgetpluginwidget_base.ui)

KCOREADDONS_ADD_PLUGIN(skrooge_budget SOURCES ${skrooge_budget_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_budget KF6::ColorScheme KF6::Parts KF6::ItemViews skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_budget.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_budget )
