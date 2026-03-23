#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGMYAPPLITEST ::..")

PROJECT(skgmyapplitest)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skgmyapplitest_SRCS
   main.cpp
 )
kde4_ADD_EXECUTABLE(skgmyapplitest ${skgmyapplitest_SRCS})

TARGET_LINK_LIBRARIES(skgmyapplitest KF6::Parts skgbasemodeler skgbasegui )

########### install files ###############
INSTALL(TARGETS skgmyapplitest ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/org.kde.skgmyapplitest.notifyrc  DESTINATION  ${KDE_INSTALL_KNOTIFY5RCDIR} )
INSTALL(PROGRAMS org.kde.skgmyapplitest.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
ECM_INSTALL_ICONS( ${KDE_INSTALL_ICONDIR}   )
