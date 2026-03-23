#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_CSV ::..")

PROJECT(plugin_import_csv)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_csv_SRCS
	skgimportplugincsv.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_csv SOURCES ${skrooge_import_csv_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_csv KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

