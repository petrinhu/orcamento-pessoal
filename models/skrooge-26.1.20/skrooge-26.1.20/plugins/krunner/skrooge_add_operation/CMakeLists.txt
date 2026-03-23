#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_ADD_OPERATION ::..")

PROJECT(plugin_add_operation)

FIND_PACKAGE(KF6 ${KF_MIN_VERSION} COMPONENTS
  Runner
)

IF(KF6Runner_FOUND)
  LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

  ADD_LIBRARY(skrooge_add_operation MODULE skgaddoperation.cpp)
  TARGET_LINK_LIBRARIES(skrooge_add_operation KF6::Runner KF6::WidgetsAddons KF6::I18n KF6::ConfigCore)

  ########### install files ###############
ENDIF()
