#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_AFB120 ::..")

PROJECT(plugin_import_AFB120)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_afb120_SRCS
	skgimportpluginafb120.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_afb120 SOURCES ${skrooge_import_afb120_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_afb120 KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

