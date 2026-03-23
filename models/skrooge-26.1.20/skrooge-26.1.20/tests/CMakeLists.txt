#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE TESTS ::..")

PROJECT(TESTS)

ADD_SUBDIRECTORY(skgbasemodelertest)
ADD_SUBDIRECTORY(skgbankmodelertest)
ADD_SUBDIRECTORY(skgbaseguitest)
ADD_SUBDIRECTORY(skgbankguitest)
ADD_SUBDIRECTORY(skroogetest)
#ADD_SUBDIRECTORY(skgmyapplitest) #<-- Experimental plugin, do not activate

#TESTS
CONFIGURE_FILE(scripts/init.sh.in ${CMAKE_CURRENT_SOURCE_DIR}/scripts/init.sh @ONLY)
ENABLE_TESTING()
INCLUDE(CTest)
