#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGBASEGUI ::..")

PROJECT(SKGBASEGUI)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skgbasegui_SRCS
    skgmainpanel.cpp
    skgmenuitem.cpp
    skgobjectmodelbase.cpp
    skgwidget.cpp
    skgperiodedit.cpp
    skgtabwidget.cpp
    skgtablewidget.cpp
    skgtabpage.cpp
    skginterfaceplugin.cpp
    skgtableview.cpp
    skgfilteredtableview.cpp
    skgtreeview.cpp
    skgcombobox.cpp
    skgcolorbutton.cpp
    skgzoomselector.cpp
    skgcalculatoredit.cpp
    skggraphicsscene.cpp
    skggraphicsview.cpp
    skghtmlboardwidget.cpp
    skgboardwidget.cpp
    skgtablewithgraph.cpp
    skgdateedit.cpp
    skgprogressbar.cpp
    kdateedit.cpp
    kdatepickerpopup.cpp
    kdatevalidator.cpp
    skgsortfilterproxymodel.cpp
    skgflowlayout.cpp
    skgwebview.cpp
    skgshow.cpp
    skgwidgetselector.cpp
    skgsimpleperiodedit.cpp
)

SET(LIBS KF6::Parts KF6::KIOFileWidgets KF6::NotifyConfig KF6::Notifications Qt6::QuickWidgets Qt6::PrintSupport Qt6::Svg Qt6::Core5Compat skgbasemodeler)
FIND_PACKAGE(KF6 ${KF_MIN_VERSION} COMPONENTS StatusNotifierItem)
if (StatusNotifierItem_FOUND)
    message(STATUS "StatusNotifierItem FOUND")
    SET(LIBS ${LIBS} KF6::StatusNotifierItem)
    set(HAS_STATUSNOTIFIER TRUE)
endif()

IF(SKG_WEBENGINE)
    SET(LIBS ${LIBS} Qt6::WebEngineWidgets)
ENDIF(SKG_WEBENGINE)

find_package(PlasmaActivities)
if (PlasmaActivities_FOUND)
    message(STATUS "PlasmaActivities FOUND")
    SET(LIBS ${LIBS} Plasma::Activities)
    set(HAS_ACTIVITIES TRUE)
endif()

ki18n_wrap_ui(skgbasegui_SRCS skgmainpanel_base.ui skgmainpanel_pref.ui skggraphicsview.ui skgtablewithgraph.ui skgcolorbutton.ui skgzoomselector.ui skgperiodedit.ui skgfilteredtableview.ui skgwidgetselector.ui )
kconfig_add_kcfg_files(skgbasegui_SRCS skgbasegui_settings.kcfgc )

ADD_LIBRARY(skgbasegui SHARED ${skgbasegui_SRCS})
TARGET_LINK_LIBRARIES(skgbasegui LINK_PUBLIC ${LIBS})
SET_TARGET_PROPERTIES(skgbasegui PROPERTIES VERSION ${SKG_VERSION} SOVERSION ${SOVERSION} )
GENERATE_EXPORT_HEADER(skgbasegui BASE_NAME skgbasegui)

########### install files ###############
INSTALL(TARGETS skgbasegui ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}  LIBRARY NAMELINK_SKIP  )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgmainpanel.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skg )
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skgbasegui_settings.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )

INSTALL(DIRECTORY icons_breeze/ DESTINATION ${KDE_INSTALL_ICONDIR}/breeze/actions/22 FILES_MATCHING PATTERN "*.svgz")
INSTALL(DIRECTORY icons_breeze-dark/ DESTINATION ${KDE_INSTALL_ICONDIR}/breeze-dark/actions/22 FILES_MATCHING PATTERN "*.svgz")

ECM_INSTALL_ICONS(ICONS
    icons_hicolor/16-actions-skg-chart-bubble.png
    icons_hicolor/22-actions-skg-chart-bubble.png
    icons_hicolor/32-actions-skg-chart-bubble.png
    icons_hicolor/48-actions-skg-chart-bubble.png
    icons_hicolor/64-actions-skg-chart-bubble.png
    icons_hicolor/128-actions-skg-chart-bubble.png
    icons_hicolor/256-actions-skg-chart-bubble.png
    icons_hicolor/512-actions-skg-chart-bubble.png
    icons_hicolor/sc-actions-skg-chart-bubble.svgz
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
