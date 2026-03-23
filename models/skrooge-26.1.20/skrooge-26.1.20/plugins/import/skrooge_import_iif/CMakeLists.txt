#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_IIF ::..")

PROJECT(plugin_import_iif)

INCLUDE_DIRECTORIES( ${PROJECT_BINARY_DIR} ${KDE4_INCLUDES} ${QT_INCLUDES}
${CMAKE_SOURCE_DIR}/skgbankmodeler ${CMAKE_SOURCE_DIR}/skgbasemodeler ${CMAKE_SOURCE_DIR}/skgbankgui ${CMAKE_SOURCE_DIR}/skgbasegui
${CMAKE_BINARY_DIR}/skgbasegui
)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_iif_SRCS
	skgimportpluginiif.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_iif SOURCES ${skrooge_import_iif_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_iif KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

