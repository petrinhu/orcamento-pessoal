#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKROOGE ::..")

PROJECT(SKROOGE)

FIND_PACKAGE(Qt6 COMPONENTS QuickControls2 REQUIRED)

FIND_PACKAGE(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
  DBusAddons               # Tier 1
)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_SRCS
   main.cpp
 )
INCLUDE(ECMAddAppIcon)
FILE(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons_hicolor/[0-9][0-9]-apps-skrooge.png")
ECM_ADD_APP_ICON(skrooge_SRCS ICONS
    "${CMAKE_CURRENT_SOURCE_DIR}/icons_hicolor/256-apps-skrooge.png"
    "${CMAKE_CURRENT_SOURCE_DIR}/icons_hicolor/128-apps-skrooge.png"
    ${ICONS_SRCS})

ADD_EXECUTABLE(skrooge ${skrooge_SRCS})

TARGET_LINK_LIBRARIES(skrooge KF6::DBusAddons skgbasemodeler skgbasegui skgbankmodeler skgbankgui Qt6::QuickControls2)

########### install files ###############
INSTALL(TARGETS skrooge ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )
INSTALL(PROGRAMS org.kde.skrooge.desktop  DESTINATION  ${KDE_INSTALL_APPDIR} )
INSTALL(FILES org.kde.skrooge.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
INSTALL(DIRECTORY theme DESTINATION ${KDE_INSTALL_DATADIR}/skrooge FILES_MATCHING PATTERN "*.css"
PATTERN ".svn" EXCLUDE
PATTERN "CMakeFiles" EXCLUDE
PATTERN "Testing" EXCLUDE)
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge.notifyrc  DESTINATION  ${KDE_INSTALL_KNOTIFYRCDIR} )

ECM_INSTALL_ICONS(ICONS
    icons_hicolor/16-apps-skrooge-black.png
    icons_hicolor/16-apps-skrooge.png
    icons_hicolor/16-mimetypes-application-x-skg.png
    icons_hicolor/16-mimetypes-application-x-skgc.png
    icons_hicolor/22-apps-skrooge-black.png
    icons_hicolor/22-apps-skrooge.png
    icons_hicolor/22-mimetypes-application-x-skg.png
    icons_hicolor/22-mimetypes-application-x-skgc.png
    icons_hicolor/32-apps-skrooge-black.png
    icons_hicolor/32-apps-skrooge.png
    icons_hicolor/32-mimetypes-application-x-skg.png
    icons_hicolor/32-mimetypes-application-x-skgc.png
    icons_hicolor/48-apps-skrooge-black.png
    icons_hicolor/48-apps-skrooge.png
    icons_hicolor/48-mimetypes-application-x-skg.png
    icons_hicolor/48-mimetypes-application-x-skgc.png 
    icons_hicolor/64-apps-skrooge-black.png
    icons_hicolor/64-apps-skrooge.png
    icons_hicolor/64-mimetypes-application-x-skg.png
    icons_hicolor/64-mimetypes-application-x-skgc.png
    icons_hicolor/128-apps-skrooge-black.png
    icons_hicolor/128-apps-skrooge.png
    icons_hicolor/128-mimetypes-application-x-skg.png
    icons_hicolor/128-mimetypes-application-x-skgc.png
    icons_hicolor/256-apps-skrooge-black.png
    icons_hicolor/256-apps-skrooge.png
    icons_hicolor/256-mimetypes-application-x-skg.png
    icons_hicolor/256-mimetypes-application-x-skgc.png
    icons_hicolor/512-apps-skrooge-black.png
    icons_hicolor/512-apps-skrooge.png
    icons_hicolor/512-mimetypes-application-x-skg.png
    icons_hicolor/512-mimetypes-application-x-skgc.png
    icons_hicolor/sc-apps-skrooge-black.svgz
    icons_hicolor/sc-apps-skrooge-initial.svgz
    icons_hicolor/sc-apps-skrooge.svgz
    icons_hicolor/sc-mimetypes-application-x-skg.svgz
    icons_hicolor/sc-mimetypes-application-x-skgc.svgz
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
if (NOT SHARED_MIME_INFO_MINIMUM_VERSION)
  set(SHARED_MIME_INFO_MINIMUM_VERSION "0.23")
endif (NOT SHARED_MIME_INFO_MINIMUM_VERSION)

FIND_PACKAGE(SharedMimeInfo REQUIRED)

install(FILES x-skg.xml DESTINATION ${KDE_INSTALL_MIMEDIR})
update_xdg_mimetypes(${KDE_INSTALL_MIMEDIR})

