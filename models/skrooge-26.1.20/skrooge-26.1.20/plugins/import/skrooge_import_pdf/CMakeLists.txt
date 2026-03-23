#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_IMPORT_PDF ::..")

PROJECT(plugin_import_PDF)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_import_pdf_SRCS
	skgimportpluginpdf.cpp
)

KCOREADDONS_ADD_PLUGIN(skrooge_import_pdf SOURCES ${skrooge_import_pdf_SRCS} INSTALL_NAMESPACE "skrooge_import")
TARGET_LINK_LIBRARIES(skrooge_import_pdf KF6::Parts skgbasemodeler skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(DIRECTORY . DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/extractors FILES_MATCHING PATTERN "*.extractor" 
PATTERN ".svn" EXCLUDE
PATTERN "CMakeLists.txt" EXCLUDE
PATTERN "CMakeFiles" EXCLUDE
PATTERN "grantlee_filters" EXCLUDE
PATTERN "Testing" EXCLUDE)
