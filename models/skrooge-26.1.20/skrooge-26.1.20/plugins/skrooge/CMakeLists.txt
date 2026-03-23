#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
ADD_SUBDIRECTORY(skrooge_bank)
ADD_SUBDIRECTORY(skrooge_budget)
ADD_SUBDIRECTORY(skrooge_calculator)
ADD_SUBDIRECTORY(skrooge_categories)
ADD_SUBDIRECTORY(skrooge_importexport)

ADD_SUBDIRECTORY(skrooge_operation)
ADD_SUBDIRECTORY(skrooge_report)
ADD_SUBDIRECTORY(skrooge_scheduled)
ADD_SUBDIRECTORY(skrooge_search)
ADD_SUBDIRECTORY(skrooge_payee)
ADD_SUBDIRECTORY(skrooge_tracker)
ADD_SUBDIRECTORY(skrooge_unit)

INSTALL(DIRECTORY . DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/html FILES_MATCHING PATTERN "*.txt"
PATTERN ".svn" EXCLUDE
PATTERN "CMakeLists.txt" EXCLUDE
PATTERN "CMakeFiles" EXCLUDE
PATTERN "grantlee_filters" EXCLUDE
PATTERN "Testing" EXCLUDE)
INSTALL(DIRECTORY . DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/html FILES_MATCHING PATTERN "*.html"
PATTERN ".svn" EXCLUDE
PATTERN "CMakeLists.txt" EXCLUDE
PATTERN "CMakeFiles" EXCLUDE
PATTERN "grantlee_filters" EXCLUDE
PATTERN "Testing" EXCLUDE)
INSTALL(DIRECTORY . DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/html FILES_MATCHING PATTERN "*.qml"
PATTERN ".svn" EXCLUDE
PATTERN "CMakeLists.txt" EXCLUDE
PATTERN "CMakeFiles" EXCLUDE
PATTERN "grantlee_filters" EXCLUDE
PATTERN "Testing" EXCLUDE)
INSTALL(FILES skrooge_monthly.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
