#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_GNC ::..")

PROJECT(plugin_import_gnc)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_gnc_SRCS
	skgimportplugingnc.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_gnc SOURCES ${skrooge_import_gnc_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_gnc KF6::Parts KF6::Archive skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

