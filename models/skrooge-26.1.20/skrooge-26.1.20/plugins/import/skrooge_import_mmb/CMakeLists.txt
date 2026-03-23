#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_MMB ::..")

PROJECT(plugin_import_mmb)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_mmb_SRCS
	skgimportpluginmmb.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_mmb SOURCES ${skrooge_import_mmb_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_mmb KF6::Parts Qt6::Sql skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

