#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE IMAGES ::..")

PROJECT(images)

ADD_SUBDIRECTORY( logos )

#GENERATE SPLASH SCREEN
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/splash.svg.in ${PROJECT_BINARY_DIR}/splash.svg @ONLY)

INSTALL(FILES splash.png DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/images)
