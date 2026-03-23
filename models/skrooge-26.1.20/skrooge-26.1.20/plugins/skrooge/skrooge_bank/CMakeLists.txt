#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_BANK ::..")

PROJECT(plugin_bank)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_bank_SRCS
	skgbankplugin.cpp
	skgbankpluginwidget.cpp
	skgaccountboardwidget.cpp
)

ki18n_wrap_ui(skrooge_bank_SRCS skgbankpluginwidget_base.ui skgbankpluginwidget_board.ui)

KCOREADDONS_ADD_PLUGIN(skrooge_bank SOURCES ${skrooge_bank_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_bank KF6::Parts KF6::ItemViews skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_bank.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_bank )
