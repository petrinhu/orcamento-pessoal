#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGBASEMODELER ::..")

PROJECT(SKGBASEMODELER)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

INCLUDE_DIRECTORIES(${SQLCIPHER_INCLUDE_DIRS})

SET(skgbasemodeler_SRCS
   skgobjectbase.cpp
   skgnamedobject.cpp
   skgnodeobject.cpp
   skgpropertyobject.cpp
   skgdocument.cpp
   skgdocumentprivate.cpp
   skgtransactionmng.cpp
   skgservices.cpp
   skgerror.cpp
   skgtraces.cpp
   skgadvice.cpp
   skgreport.cpp
   skgtreemap.cpp
 )

#build a shared library
ADD_LIBRARY(skgbasemodeler SHARED ${skgbasemodeler_SRCS})

#need to link to some other libraries ? just add them here
SET_TARGET_PROPERTIES(skgbasemodeler PROPERTIES VERSION ${SKG_VERSION} SOVERSION ${SOVERSION} )

TARGET_LINK_LIBRARIES(skgbasemodeler PUBLIC KF6::I18n KF6::ConfigWidgets KF6::CoreAddons
    Qt6::Core Qt6::Gui Qt6::Sql Qt6::Qml Qt6::Xml Qt6::DBus Qt6::Concurrent
    KF6::KIOCore ${SQLCIPHER_LIBRARIES} KF6::IconThemes KF6::TextTemplate)

GENERATE_EXPORT_HEADER(skgbasemodeler BASE_NAME skgbasemodeler)

########### install files ###############
INSTALL(TARGETS skgbasemodeler ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}  LIBRARY NAMELINK_SKIP )
