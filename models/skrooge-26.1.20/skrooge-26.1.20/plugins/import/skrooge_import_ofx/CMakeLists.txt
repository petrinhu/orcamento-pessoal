#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_OFX ::..")

PROJECT(plugin_import_ofx)

MESSAGE( STATUS "OFX found. OFX support enabled")

INCLUDE(KDECompilerSettings NO_POLICY_SCOPE)
KDE_ENABLE_EXCEPTIONS()

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_ofx_SRCS
	skgimportpluginofx.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_ofx SOURCES ${skrooge_import_ofx_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_ofx KF6::Parts ${LIBOFX_LIBRARIES} skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############

