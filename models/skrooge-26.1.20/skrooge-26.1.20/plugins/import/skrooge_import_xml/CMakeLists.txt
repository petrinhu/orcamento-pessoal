#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_XML ::..")

PROJECT(plugin_import_xml)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_xml_SRCS
	skgimportpluginxml.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_xml SOURCES ${skrooge_import_xml_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_xml KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ISO20022.xslt DESTINATION ${KDE_INSTALL_DATADIR}/skrooge)
