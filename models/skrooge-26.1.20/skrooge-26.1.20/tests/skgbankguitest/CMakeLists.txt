#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGBANKGUITEST ::..")

PROJECT(SKBBASEMODELERTEST)

ADD_DEFINITIONS(-DQT_GUI_LIB)
LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/tests/skgbasemodelertest )

#Make executables
ADD_EXECUTABLE(skgtestpredicatcreator skgtestpredicatcreator.cpp)
ADD_EXECUTABLE(skgtestmainpanel skgtestmainpanel.cpp)
ADD_EXECUTABLE(skgtestactions skgtestactions.cpp)
ADD_EXECUTABLE(skgtesttreeview skgtesttreeview.cpp)
ADD_EXECUTABLE(skgtestmodel skgtestmodel.cpp modeltest.cpp)


TARGET_LINK_LIBRARIES(skgtestpredicatcreator  Qt6::Gui Qt6::Core Qt6::Test skgbasemodeler skgbankmodeler skgbankgui skgbasegui)
TARGET_LINK_LIBRARIES(skgtestmainpanel  Qt6::Gui Qt6::Core Qt6::Test skgbasemodeler skgbankmodeler skgbankgui skgbasegui)
TARGET_LINK_LIBRARIES(skgtestactions  Qt6::Gui Qt6::Core Qt6::Test skgbasemodeler skgbankmodeler skgbankgui skgbasegui)
TARGET_LINK_LIBRARIES(skgtesttreeview Qt6::Core KF6::KIOWidgets Qt6::Test skgbankmodeler skgbasemodeler skgbankgui skgbasegui)
TARGET_LINK_LIBRARIES(skgtestmodel Qt6::Core KF6::KIOWidgets Qt6::Test skgbankmodeler skgbasemodeler skgbankgui skgbasegui)

#Add test
ENABLE_TESTING()
ADD_TEST(NAME skgtestpredicatcreator COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skgtestpredicatcreator.sh)
ADD_TEST(NAME skgtestmainpanel COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skgtestmainpanel.sh)
ADD_TEST(NAME skgtestactions COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skgtestactions.sh)
ADD_TEST(NAME skgtesttreeview COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skgtesttreeview.sh)
ADD_TEST(NAME skgtestmodel COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skgtestmodel.sh)

if(SKG_DESIGNER)
	ADD_EXECUTABLE(skgtestbankwidgets skgtestbankwidgets.cpp)
	TARGET_LINK_LIBRARIES(skgtestbankwidgets Qt6::Core KF6::KIOWidgets Qt6::Test skgbankmodeler skgbasemodeler skgbankguidesigner skgbasegui)
	ADD_TEST(NAME skgtestbankwidgets COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skgtestbankwidgets.sh)
endif()

#TODO: TO LONG ADD_TEST(NAME skgtestskrooge COMMAND ${CMAKE_SOURCE_DIR}/tests/scripts/skgtestskrooge.sh)

INCLUDE(CTest)
