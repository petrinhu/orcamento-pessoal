#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_JSON ::..")

PROJECT(plugin_import_json)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_json_SRCS
	skgimportpluginjson.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_json SOURCES ${skrooge_import_json_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_json KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

