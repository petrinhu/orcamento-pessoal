#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGBANKGUIDESIGNER ::..")

PROJECT(SKGBANKGUI)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skgbankguidesigner_SRCS ${skgbankgui_SRCS}
    skgquerycreatordesignerplugin.cpp
    skgbkwidgetcollectiondesignerplugin.cpp
    skgpredicatcreatordesignerplugin.cpp
    skgunitcomboboxdesignerplugin.cpp
)

SET(LIBS Qt6::Designer skgbankgui)

IF(SKG_WEBENGINE)
    SET(LIBS ${LIBS} Qt6::WebEngineWidgets)
ENDIF(SKG_WEBENGINE)

ADD_LIBRARY(skgbankguidesigner SHARED ${skgbankguidesigner_SRCS})
TARGET_LINK_LIBRARIES(skgbankguidesigner LINK_PUBLIC ${LIBS})
GENERATE_EXPORT_HEADER(skgbankguidesigner BASE_NAME skgbankguidesigner)

########### install files ###############
IF(WIN32)
    INSTALL(TARGETS skgbankguidesigner LIBRARY ARCHIVE DESTINATION ${KDE_INSTALL_PLUGINDIR}/designer )
ELSE(WIN32)
    INSTALL(TARGETS skgbankguidesigner LIBRARY DESTINATION ${KDE_INSTALL_PLUGINDIR}/designer )
ENDIF(WIN32)
