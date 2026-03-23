#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_DELETE ::..")

PROJECT(plugin_delete)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skg_delete_SRCS skgdeleteplugin.cpp)

KCOREADDONS_ADD_PLUGIN(skg_delete SOURCES ${skg_delete_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skg_delete KF6::Parts skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skg_delete.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg_delete )
