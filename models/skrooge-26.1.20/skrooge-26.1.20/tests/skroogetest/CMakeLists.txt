#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKROOGETEST ::..")

PROJECT(SKROOGETEST)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

#Add test
ENABLE_TESTING()
#ADD_TEST(NAME skg_sikuli_tst_budget COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skg_sikuli_tst_budget.sh)
#ADD_TEST(NAME skg_sikuli_tst_operation COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skg_sikuli_tst_operation.sh)
#ADD_TEST(NAME skg_sikuli_tst_print COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skg_sikuli_tst_print.sh)
#ADD_TEST(NAME skg_sikuli_tst_tracker COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skg_sikuli_tst_tracker.sh)
#ADD_TEST(NAME skg_sikuli_tst_search COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skg_sikuli_tst_search.sh)
#ADD_TEST(NAME skg_sikuli_tst_unit COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skg_sikuli_tst_unit.sh)

ADD_TEST(NAME skroogeconvert COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skroogeconvert.sh)

INCLUDE(CTest)
