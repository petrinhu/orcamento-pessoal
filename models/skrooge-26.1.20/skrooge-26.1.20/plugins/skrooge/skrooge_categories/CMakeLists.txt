#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE PLUGIN_CATEGORIES ::..")

PROJECT(plugin_categories)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skrooge_categories_SRCS 
	skgcategoriesplugin.cpp 
	skgcategoriespluginwidget.cpp)

ki18n_wrap_ui(skrooge_categories_SRCS skgcategoriespluginwidget_base.ui)

KCOREADDONS_ADD_PLUGIN(skrooge_categories SOURCES ${skrooge_categories_SRCS} INSTALL_NAMESPACE "skg_gui")
TARGET_LINK_LIBRARIES(skrooge_categories KF6::Parts skgbasemodeler KF6::ItemViews skgbasegui skgbankmodeler skgbankgui)

########### install files ###############
INSTALL(FILES ${PROJECT_SOURCE_DIR}/skrooge_categories.rc  DESTINATION  ${KDE_INSTALL_KXMLGUIDIR}/skrooge_categories )
INSTALL(DIRECTORY . DESTINATION ${KDE_INSTALL_DATADIR}/skrooge/categories/ FILES_MATCHING PATTERN "*.qif"
PATTERN ".svn" EXCLUDE
PATTERN "CMakeFiles" EXCLUDE
PATTERN "Testing" EXCLUDE)

