#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: SKGSQLCIPHER ::..")

PROJECT(skgsqlcipher)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skgsqlcipher_sources skgsqlcipherdriverplugin.cpp q6sql_sqlite.cpp)
ADD_LIBRARY(libskgsqlcipher MODULE ${skgsqlcipher_sources})
TARGET_INCLUDE_DIRECTORIES(libskgsqlcipher PRIVATE ${Qt6Sql_PRIVATE_INCLUDE_DIRS} ${SQLCIPHER_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(libskgsqlcipher Qt6::Sql Qt6::SqlPrivate ${SQLCIPHER_LIBRARIES})

GENERATE_EXPORT_HEADER(libskgsqlcipher BASE_NAME skgsqlcipher)

########### install files ###############
INSTALL(TARGETS libskgsqlcipher DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/sqldrivers)    

