#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_PROPERTIES ::..")

PROJECT(plugin_properties)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_properties_SRCS
	skgpropertiesplugin.cpp
	skgpropertiesplugindockwidget.cpp)

ki18n_wrap_ui(skg_properties_SRCS skgpropertiesplugindockwidget_base.ui)

KCOREADDONS_ADD_PLUGIN(skg_properties SOURCES ${skg_properties_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_properties KF6::Parts KF6::ItemViews KF6::KIOCore KF6::KIOFileWidgets KF6::KIOWidgets skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_properties.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_properties )
