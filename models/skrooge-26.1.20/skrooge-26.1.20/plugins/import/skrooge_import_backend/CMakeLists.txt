#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_BACKEND ::..")

PROJECT(plugin_import_backend)

ADD_SUBDIRECTORY(backends)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_backend_SRCS
	skgimportpluginbackend.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_backend SOURCES ${skrooge_import_backend_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_backend KF6::Parts Qt6::Concurrent skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(PROGRAMS skrooge-sabb.py DESTINATION ${KDE_INSTALL_DATADIR}/skrooge)
INSTALL(PROGRAMS skrooge-woob.py DESTINATION ${KDE_INSTALL_DATADIR}/skrooge)
