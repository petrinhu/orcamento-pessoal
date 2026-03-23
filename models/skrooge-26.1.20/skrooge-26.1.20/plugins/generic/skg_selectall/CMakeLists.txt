#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_SELECTALL ::..")

PROJECT(plugin_selectall)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_selectall_SRCS
	skgselectallplugin.cpp
)


KCOREADDONS_ADD_PLUGIN(skg_selectall SOURCES ${skg_selectall_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_selectall KF6::Parts skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_selectall.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_selectall )
