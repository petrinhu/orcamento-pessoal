#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE IMAGES/LOGOS ::..")

PROJECT(logos)

INSTALL(DIRECTORY . DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/images/logo/ FILES_MATCHING PATTERN "*.png"
PATTERN ".svn" EXCLUDE
PATTERN "CMakeFiles" EXCLUDE
PATTERN "Testing" EXCLUDE)
INSTALL(DIRECTORY . DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/images/logo/ FILES_MATCHING PATTERN "list_bank.txt"
PATTERN ".svn" EXCLUDE
PATTERN "CMakeFiles" EXCLUDE
PATTERN "Testing" EXCLUDE)
