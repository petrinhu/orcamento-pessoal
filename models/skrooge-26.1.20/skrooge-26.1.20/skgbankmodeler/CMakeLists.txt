#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
MESSAGE( STATUS "..:: CMAKE SKGBANKMODELER ::..")

PROJECT(SKGBANKMODELER)

LINK_DIRECTORIES (${LIBRARY_OUTPUT_PATH})

SET(skgbankmodeler_SRCS
   skgbankobject.cpp
   skgbudgetobject.cpp
   skgbudgetruleobject.cpp
   skgaccountobject.cpp
   skgoperationobject.cpp
   skgrecurrentoperationobject.cpp
   skgtrackerobject.cpp
   skgpayeeobject.cpp
   skgsuboperationobject.cpp
   skgcategoryobject.cpp
   skgunitobject.cpp
   skgunitvalueobject.cpp
   skgruleobject.cpp
   skginterestobject.cpp
   skgdocumentbank.cpp
   skgimportexportmanager.cpp
   skgimportplugin.cpp
   skgreportbank.cpp
 )

#build a shared library
ADD_LIBRARY(skgbankmodeler SHARED ${skgbankmodeler_SRCS})

#need to link to some other libraries ? just add them here
TARGET_LINK_LIBRARIES(skgbankmodeler LINK_PUBLIC KF6::Parts Qt6::Xml skgbasemodeler)
SET_TARGET_PROPERTIES( skgbankmodeler PROPERTIES VERSION ${SKG_VERSION} SOVERSION ${SOVERSION} )

GENERATE_EXPORT_HEADER(skgbankmodeler BASE_NAME skgbankmodeler)

ADD_SUBDIRECTORY(currency)

########### install files ###############
INSTALL(TARGETS skgbankmodeler ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}  LIBRARY NAMELINK_SKIP)
INSTALL(DIRECTORY sources/. DESTINATION ${KDE_INSTALL_DATADIR}/skrooge_source FILES_MATCHING PATTERN "*.json")

INSTALL(PROGRAMS skrooge-cryptocompare.py DESTINATION ${KDE_INSTALL_DATADIR}/skrooge)
INSTALL(PROGRAMS skrooge-coinmarketcap.py DESTINATION ${KDE_INSTALL_DATADIR}/skrooge)
INSTALL(PROGRAMS skrooge-exchangerates.py DESTINATION ${KDE_INSTALL_DATADIR}/skrooge)
INSTALL(PROGRAMS skrooge-exchangerates_apilayer.py DESTINATION ${KDE_INSTALL_DATADIR}/skrooge)
INSTALL(PROGRAMS skrooge-boursorama.py DESTINATION ${KDE_INSTALL_DATADIR}/skrooge)
