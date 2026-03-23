#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_CALCULATOR ::..")

PROJECT(plugin_calculator)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_calculator_SRCS
	skgcalculatorplugin.cpp
	skgcalculatorpluginwidget.cpp)

ki18n_wrap_ui(skrooge_calculator_SRCS skgcalculatorpluginwidget_base.ui)

KCOREADDONS_ADD_PLUGIN(skrooge_calculator SOURCES ${skrooge_calculator_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_calculator KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_calculator.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_calculator )
