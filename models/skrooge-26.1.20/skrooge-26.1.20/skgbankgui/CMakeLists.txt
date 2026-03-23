#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGBANKGUI ::..")

PROJECT(SKGBANKGUI)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skgbankgui_SRCS
    skgobjectmodel.cpp
    skgquerycreator.cpp
    skgpredicatcreator.cpp
    skgquerydelegate.cpp
    skgunitcombobox.cpp
)

SET(LIBS Qt6::Xml skgbankmodeler skgbasemodeler skgbasegui)
IF(SKG_WEBENGINE)
    SET(LIBS ${LIBS} Qt6::WebEngineWidgets)
ENDIF(SKG_WEBENGINE)

ki18n_wrap_ui(skgbankgui_SRCS skgquerycreator.ui)

ADD_LIBRARY(skgbankgui SHARED ${skgbankgui_SRCS})
TARGET_LINK_LIBRARIES(skgbankgui LINK_PUBLIC ${LIBS})
SET_TARGET_PROPERTIES(skgbankgui PROPERTIES VERSION ${SKG_VERSION} SOVERSION ${SOVERSION} )
GENERATE_EXPORT_HEADER(skgbankgui BASE_NAME skgbankgui)

########### install files ###############
INSTALL(TARGETS skgbankgui ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}  LIBRARY NAMELINK_SKIP  )
INSTALL(DIRECTORY icons_breeze/ DESTINATION ${KDE_INSTALL_ICONDIR}/breeze/actions/22 FILES_MATCHING PATTERN "*.svgz")
INSTALL(DIRECTORY icons_breeze-dark/ DESTINATION ${KDE_INSTALL_ICONDIR}/breeze-dark/actions/22 FILES_MATCHING PATTERN "*.svgz")

ECM_INSTALL_ICONS(ICONS
    icons_hicolor/16-actions-skrooge_credit_card.png
    icons_hicolor/16-actions-skrooge_less.png
    icons_hicolor/16-actions-skrooge_more.png
    icons_hicolor/16-actions-skrooge_much_less.png
    icons_hicolor/16-actions-skrooge_much_more.png
    icons_hicolor/16-actions-skrooge_type.png
    icons_hicolor/22-actions-skrooge_credit_card.png
    icons_hicolor/22-actions-skrooge_less.png
    icons_hicolor/22-actions-skrooge_more.png
    icons_hicolor/22-actions-skrooge_much_less.png
    icons_hicolor/22-actions-skrooge_much_more.png
    icons_hicolor/22-actions-skrooge_type.png
    icons_hicolor/32-actions-skrooge_credit_card.png
    icons_hicolor/32-actions-skrooge_less.png
    icons_hicolor/32-actions-skrooge_more.png
    icons_hicolor/32-actions-skrooge_much_less.png
    icons_hicolor/32-actions-skrooge_much_more.png
    icons_hicolor/32-actions-skrooge_type.png
    icons_hicolor/48-actions-skrooge_credit_card.png
    icons_hicolor/48-actions-skrooge_less.png
    icons_hicolor/48-actions-skrooge_more.png
    icons_hicolor/48-actions-skrooge_much_less.png
    icons_hicolor/48-actions-skrooge_much_more.png
    icons_hicolor/48-actions-skrooge_type.png
    icons_hicolor/64-actions-skrooge_credit_card.png
    icons_hicolor/64-actions-skrooge_less.png
    icons_hicolor/64-actions-skrooge_more.png
    icons_hicolor/64-actions-skrooge_much_less.png
    icons_hicolor/64-actions-skrooge_much_more.png
    icons_hicolor/64-actions-skrooge_type.png
    icons_hicolor/128-actions-skrooge_credit_card.png
    icons_hicolor/128-actions-skrooge_less.png
    icons_hicolor/128-actions-skrooge_more.png
    icons_hicolor/128-actions-skrooge_much_less.png
    icons_hicolor/128-actions-skrooge_much_more.png
    icons_hicolor/128-actions-skrooge_type.png
    icons_hicolor/256-actions-skrooge_credit_card.png
    icons_hicolor/256-actions-skrooge_less.png
    icons_hicolor/256-actions-skrooge_more.png
    icons_hicolor/256-actions-skrooge_much_less.png
    icons_hicolor/256-actions-skrooge_much_more.png
    icons_hicolor/256-actions-skrooge_type.png
    icons_hicolor/512-actions-skrooge_credit_card.png
    icons_hicolor/512-actions-skrooge_less.png
    icons_hicolor/512-actions-skrooge_more.png
    icons_hicolor/512-actions-skrooge_much_less.png
    icons_hicolor/512-actions-skrooge_much_more.png
    icons_hicolor/512-actions-skrooge_type.png
    icons_hicolor/sc-actions-skrooge_credit_card.svgz
    icons_hicolor/sc-actions-skrooge_less.svgz
    icons_hicolor/sc-actions-skrooge_more.svgz
    icons_hicolor/sc-actions-skrooge_much_less.svgz
    icons_hicolor/sc-actions-skrooge_much_more.svgz
    icons_hicolor/sc-actions-skrooge_type.svgz
    DESTINATION ${KDE_INSTALL_ICONDIR}
    THEME hicolor
)
