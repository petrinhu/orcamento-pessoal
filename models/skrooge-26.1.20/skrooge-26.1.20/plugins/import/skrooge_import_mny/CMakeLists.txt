#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_MNY ::..")

PROJECT(plugin_import_mny)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_mny_SRCS
	skgimportpluginmny.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_mny SOURCES ${skrooge_import_mny_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_mny KF6::Parts Qt6::Core skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

