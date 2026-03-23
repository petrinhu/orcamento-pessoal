#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_QIF ::..")

PROJECT(plugin_import_qif)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_qif_SRCS
	skgimportpluginqif.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_qif SOURCES ${skrooge_import_qif_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_qif KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

