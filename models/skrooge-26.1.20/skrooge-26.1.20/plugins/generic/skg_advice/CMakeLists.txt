#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_ADVICE ::..")

PROJECT(plugin_advice)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_advice_SRCS 
	skgadviceplugin.cpp
	skgadviceboardwidget.cpp
	skgtipofdayboardwidget.cpp
)

ki18n_wrap_ui(skg_advice_SRCS skgtipofdayboardwidget.ui)

KCOREADDONS_ADD_PLUGIN(skg_advice SOURCES ${skg_advice_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_advice KF6::Parts KF6::ConfigWidgets skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_advice.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_advice )
