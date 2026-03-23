#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKROOGECONVERT ::..")

PROJECT(SKROOGECONVERT)

INCLUDE(ECMMarkNonGuiExecutable)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skroogeconvert_SRCS
   main.cpp
 )
ADD_EXECUTABLE(skroogeconvert ${skroogeconvert_SRCS})
ECM_MARK_NONGUI_EXECUTABLE(skroogeconvert)

TARGET_LINK_LIBRARIES(skroogeconvert Qt6::Widgets skgbasemodeler skgbankmodeler)

########### install files ###############
INSTALL(TARGETS skroogeconvert ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
