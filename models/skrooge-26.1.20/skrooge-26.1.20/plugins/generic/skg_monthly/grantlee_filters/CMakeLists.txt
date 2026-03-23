#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE GRANTLEE_FILTERS ::..")

PROJECT(grantlee_filters)

ADD_LIBRARY(grantlee_skgfilters MODULE
    skggrantleefilters.cpp
    skgdocumentfilter.cpp
    skgobjectfilter.cpp)
SET_PROPERTY(TARGET grantlee_skgfilters PROPERTY EXPORT_NAME skggrantleefilters)
TARGET_LINK_LIBRARIES(grantlee_skgfilters KF6::CoreAddons KF6::TextTemplate Qt6::Core skgbasemodeler skgbasegui)

########### install files ###############
INSTALL(TARGETS grantlee_skgfilters LIBRARY DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/kf6/ktexttemplate )