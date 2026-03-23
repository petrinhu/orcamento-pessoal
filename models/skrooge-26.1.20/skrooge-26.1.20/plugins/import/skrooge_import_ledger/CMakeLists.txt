#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_LEDGER ::..")

PROJECT(plugin_import_ledger)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_ledger_SRCS
	skgimportpluginledger.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_ledger SOURCES ${skrooge_import_ledger_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_ledger KF6::Parts Qt6::Core skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

