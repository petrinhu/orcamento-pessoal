#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_KMY ::..")

PROJECT(plugin_import_kmy)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_kmy_SRCS
	skgimportpluginkmy.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_kmy SOURCES ${skrooge_import_kmy_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_kmy KF6::Parts KF6::Archive skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

