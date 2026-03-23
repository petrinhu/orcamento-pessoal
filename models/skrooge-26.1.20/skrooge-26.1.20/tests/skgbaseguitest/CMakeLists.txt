#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGBASEGUITEST ::..")

PROJECT(SKBBASEMODELERTEST)

ADD_DEFINITIONS(-DQT_GUI_LIB)
LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/tests/skgbasemodelertest )

#Add test
ENABLE_TESTING()
FILE(GLOB cpp_files "skgtest*.cpp")
LIST(SORT cpp_files)
FOREACH(file ${cpp_files})
    GET_FILENAME_COMPONENT(utname ${file} NAME_WE)
    if ((NOT SKG_DESIGNER) AND (utname STREQUAL "skgtestwidgetcollection"))
        continue()
    endif()
    ADD_EXECUTABLE(${utname} ${file})
    TARGET_LINK_LIBRARIES(${utname} Qt6::Gui Qt6::Core Qt6::Test skgbasegui skgbasemodeler)
    if (SKG_DESIGNER AND (utname STREQUAL "skgtestwidgetcollection"))
        TARGET_LINK_LIBRARIES(${utname} skgbaseguidesigner)
    endif()
    ADD_TEST(NAME ${utname} COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/${utname}.sh)
ENDFOREACH()
INCLUDE(CTest)
