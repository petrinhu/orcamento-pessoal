#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGBASEGUIDESIGNER ::..")

PROJECT(SKGBASEGUIDESIGNER)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skgbaseguidesigner_SRCS 
    skgwidgetcollectiondesignerplugin.cpp
    skgtabwidgetdesignerplugin.cpp
    skgtablewidgetdesignerplugin.cpp
    skgtableviewdesignerplugin.cpp
    skgfilteredtableviewdesignerplugin.cpp
    skgtreeviewdesignerplugin.cpp
    skgcomboboxdesignerplugin.cpp
    skgcolorbuttondesignerplugin.cpp
    skgwidgetselectordesignerplugin.cpp
    skgwebviewdesignerplugin.cpp
    skgzoomselectordesignerplugin.cpp
    skgcalculatoreditdesignerplugin.cpp
    skggraphicsviewdesignerplugin.cpp
    skgshowdesignerplugin.cpp
    skgtablewithgraphdesignerplugin.cpp
    skgdateeditdesignerplugin.cpp
    skgprogressbardesignerplugin.cpp
    skgperiodeditdesignerplugin.cpp
    skgsimpleperiodeditdesignerplugin.cpp
)

SET(LIBS Qt6::Designer skgbasegui)

IF(SKG_WEBENGINE)
    SET(LIBS ${LIBS} Qt6::WebEngineWidgets)
ENDIF(SKG_WEBENGINE)

ADD_LIBRARY(skgbaseguidesigner SHARED ${skgbaseguidesigner_SRCS})
TARGET_LINK_LIBRARIES(skgbaseguidesigner LINK_PUBLIC ${LIBS})
GENERATE_EXPORT_HEADER(skgbaseguidesigner BASE_NAME skgbaseguidesigner)

########### install files ###############
IF(WIN32)
    INSTALL(TARGETS skgbaseguidesigner LIBRARY ARCHIVE DESTINATION ${KDE_INSTALL_PLUGINDIR}/designer )
ELSE(WIN32)
    INSTALL(TARGETS skgbaseguidesigner LIBRARY DESTINATION ${KDE_INSTALL_PLUGINDIR}/designer )
ENDIF(WIN32)
