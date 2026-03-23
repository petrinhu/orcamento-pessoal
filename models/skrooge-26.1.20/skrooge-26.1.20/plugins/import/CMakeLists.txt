#***************************************************************************
#* SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
#* SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
#* SPDX-License-Identifier: GPL-3.0-or-later
#***************************************************************************
#Correction bug 223848 vvvv
FIND_PACKAGE( LibOfx )

ADD_SUBDIRECTORY(skrooge_import_afb120)
IF(WIN32)
    MESSAGE( STATUS "     WARNING:Backend module not available on windows")
ELSE(WIN32)
    ADD_SUBDIRECTORY(skrooge_import_backend)
ENDIF(WIN32)
ADD_SUBDIRECTORY(skrooge_import_iif)
ADD_SUBDIRECTORY(skrooge_import_qif)
ADD_SUBDIRECTORY(skrooge_import_csv)
ADD_SUBDIRECTORY(skrooge_import_gnc)
ADD_SUBDIRECTORY(skrooge_import_gsb)
ADD_SUBDIRECTORY(skrooge_import_json)
ADD_SUBDIRECTORY(skrooge_import_kmy)
ADD_SUBDIRECTORY(skrooge_import_ledger)
ADD_SUBDIRECTORY(skrooge_import_mmb)
ADD_SUBDIRECTORY(skrooge_import_mny)
ADD_SUBDIRECTORY(skrooge_import_mt940)
IF(LIBOFX_FOUND)
  ADD_SUBDIRECTORY(skrooge_import_ofx)
ENDIF(LIBOFX_FOUND)
ADD_SUBDIRECTORY(skrooge_import_pdf)
ADD_SUBDIRECTORY(skrooge_import_skg)

ADD_SUBDIRECTORY(skrooge_import_xhb)
ADD_SUBDIRECTORY(skrooge_import_xml)
