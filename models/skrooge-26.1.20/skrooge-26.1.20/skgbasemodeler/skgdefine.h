/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDEFINE_H
#define SKGDEFINE_H
/** @file
 * This file defines some macros.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qstring.h>
#include <qstringbuilder.h>

#include <klocalizedstring.h>

#include "skgbasemodeler_export.h"

/**
 * @var OBJECTSEPARATOR
 * Define the separator between object and subobject
 */
#define OBJECTSEPARATOR QStringLiteral(" > ")

/**
 * @var DUMPSQLITE
 * To display SQLITE internals (tables, views, indexes, …)
 * @see dump
 */
#define DUMPSQLITE (2 << 0)

/**
 * @var DUMPPARAMETERS
 * To display parameters
 * @see dump
 */
#define DUMPPARAMETERS (2 << 1)

/**
 * @var DUMPTRANSACTIONS
 * To display transactions
 * @see dump
 */
#define DUMPTRANSACTIONS (2 << 2)

/**
 * @var DUMPNODES
 * To display nodes
 * @see dump
 */
#define DUMPNODES (2 << 3)

/**
 * @var DUMPALL
 * To display display all=@p DUMPSQLITE +@p DUMPPARAMETERS +@p DUMPTRANSACTIONS
 * @see dump
 */

#define DUMPALL ((2 << 10) - 1)

/**
 * @var SKG_UNDO_MAX_DEPTH
 * Default value for the max depth for the undo / redo mechanism
 */
#define SKG_UNDO_MAX_DEPTH 50

/**
 * @var ERR_NOTIMPL
 * Error number
 */
#define ERR_NOTIMPL 1

/**
 * @var ERR_NOINTERFACE
 * Error number
 */
#define ERR_NOINTERFACE 2

/**
 * @var ERR_POINTER
 * Error number
 */
#define ERR_POINTER 3

/**
 * @var ERR_ABORT
 * Error number
 */
#define ERR_ABORT 4

/**
 * @var ERR_FAIL
 * Error number
 */
#define ERR_FAIL 5

/**
 * @var ERR_HANDLE
 * Error number
 */
#define ERR_HANDLE 6

/**
 * @var ERR_OUTOFMEMORY
 * Error number
 */
#define ERR_OUTOFMEMORY 7

/**
 * @var ERR_INVALIDARG
 * Error number
 */
#define ERR_INVALIDARG 8

/**
 * @var ERR_UNEXPECTED
 * Error number
 */
#define ERR_UNEXPECTED 9

/**
 * @var ERR_WRITEACCESS
 * Error number
 */
#define ERR_WRITEACCESS 10

/**
 * @var ERR_FORCEABLE
 * Error number
 */
#define ERR_FORCEABLE 11

/**
 * @var ERR_ENCRYPTION
 * Error number
 */
#define ERR_ENCRYPTION 12

/**
 * @var ERR_INSTALL
 * Error number
 */
#define ERR_INSTALL 13

/**
 * @var ERR_CORRUPTION
 * Error number
 */
#define ERR_CORRUPTION 14

/**
 * @var ERR_READACCESS
 * Error number
 */
#define ERR_READACCESS 15

/**
 * @brief For a not modified field
 **/
#define NOUPDATE QStringLiteral("-------")

/**
 * @var EPSILON
 * Epsilon (for comparison)
 */
#define EPSILON 0.00001

/**
 * @def DELETECASCADE
 * Define a standard trigger for cascaded delete
 */
#define DELETECASCADE(TABLEPARENT, ATTPARENT, TABLECHILD, ATTCHILD)                                                                                            \
    (QStringList() << QString()%"DROP TRIGGER IF EXISTS fkdc_"%(TABLEPARENT)%"_"%(TABLECHILD)%"_"%(ATTPARENT)%"_"%(ATTCHILD) \
     << QString()%"CREATE TRIGGER fkdc_"%(TABLEPARENT)%"_"%(TABLECHILD)%"_"%(ATTPARENT)%"_"%(ATTCHILD)%" "\
     "BEFORE DELETE ON "%(TABLEPARENT)%" "\
     "FOR EACH ROW BEGIN "\
     "    DELETE FROM "%(TABLECHILD)%" WHERE "%(TABLECHILD)%"."%(ATTCHILD)%" = OLD."%(ATTPARENT)%"; "\
     "END")

/**
 * @def INSERTUPDATECONSTRAINT
 * Define a standard trigger for foreign constraint
 */

// Should these strings be translated ??? They look far too SQLish for that purpose
#define INSERTUPDATECONSTRAINT(TABLEPARENT, ATTPARENT, TABLECHILD, ATTCHILD)                                                                                   \
    (QStringList() << QString()%"DROP TRIGGER IF EXISTS fki_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT) \
     << QString()%"CREATE TRIGGER fki_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT)%" "\
     "BEFORE INSERT ON "%(TABLECHILD)%" "\
     "FOR EACH ROW BEGIN "\
     "  SELECT RAISE(ABORT, '"%SKGServices::stringToSqlString(i18nc("Error message", "Impossible to insert object (%1 is used by %2).\nConstraint name: %3",TABLEPARENT, TABLECHILD, "fki_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT)))%"') "\
     "  WHERE NEW."%(ATTCHILD)%"!=0 AND NEW."%(ATTCHILD)%"!='' AND (SELECT "%(ATTPARENT)%" FROM "%(TABLEPARENT)%" WHERE "%(ATTPARENT)%" = NEW."%(ATTCHILD)%") IS NULL; "\
     "END"\
     << QString()%"DROP TRIGGER IF EXISTS fku_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT) \
     << QString()%"CREATE TRIGGER fku_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT)%" "\
     "BEFORE UPDATE ON "%(TABLECHILD)%" "\
     "FOR EACH ROW BEGIN "\
     "    SELECT RAISE(ABORT, '"%SKGServices::stringToSqlString(i18nc("Error message", "Impossible to update object (%1 is used by %2).\nConstraint name: %3",TABLEPARENT, TABLECHILD, "fku_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT)))%"') "\
     "      WHERE NEW."%(ATTCHILD)%"!=0 AND NEW."%(ATTCHILD)%"!='' AND (SELECT "%(ATTPARENT)%" FROM "%(TABLEPARENT)%" WHERE "%(ATTPARENT)%" = NEW."%(ATTCHILD)%") IS NULL; "\
     "END")

/**
 * @def FOREIGNCONSTRAINT
 * Define a standard trigger for foreign constraint
 */
#define FOREIGNCONSTRAINT(TABLEPARENT, ATTPARENT, TABLECHILD, ATTCHILD)                                                                                        \
    (INSERTUPDATECONSTRAINT(TABLEPARENT,ATTPARENT,TABLECHILD,ATTCHILD)\
     << QString()%"DROP TRIGGER IF EXISTS fkd_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT) \
     << QString()%"CREATE TRIGGER fkd_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT)%" "\
     "BEFORE DELETE ON "%(TABLEPARENT)%" "\
     "FOR EACH ROW BEGIN "\
     "    SELECT RAISE(ABORT, '"%SKGServices::stringToSqlString(i18nc("Error message", "Impossible to delete used object (%1 is used by %2).\nConstraint name: %3",TABLEPARENT, TABLECHILD, "fkd_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT)))%"') "\
     "    WHERE (SELECT "%(ATTCHILD)%" FROM "%(TABLECHILD)%" WHERE "%(ATTCHILD)%" = OLD."%(ATTPARENT)%") IS NOT NULL; "\
     "END")

/**
 * @def FOREIGNCONSTRAINTUPDATE
 * Define a standard trigger for foreign constraint
 */
#define FOREIGNCONSTRAINTUPDATE(TABLEPARENT, ATTPARENT, TABLECHILD, ATTCHILD)                                                                                  \
    (INSERTUPDATECONSTRAINT(TABLEPARENT,ATTPARENT,TABLECHILD,ATTCHILD)\
     << QString()%"DROP TRIGGER IF EXISTS fkd_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT) \
     << QString()%"CREATE TRIGGER fkd_"%(TABLECHILD)%"_"%(TABLEPARENT)%"_"%(ATTCHILD)%"_"%(ATTPARENT)%" "\
     "BEFORE DELETE ON "%(TABLEPARENT)%" "\
     "FOR EACH ROW BEGIN "\
     "    UPDATE "%(TABLECHILD)%" SET "%(ATTCHILD)%"=0 WHERE "%(ATTCHILD)%"=OLD."%(ATTPARENT)%"; "\
     "END")

/**
 * @def FOREIGNCONSTRAINTCASCADE
 * Define a standard trigger for foreign constraint cascade delete
 */
#define FOREIGNCONSTRAINTCASCADE(TABLEPARENT, ATTPARENT, TABLECHILD, ATTCHILD)                                                                                 \
    INSERTUPDATECONSTRAINT(TABLEPARENT, ATTPARENT, TABLECHILD, ATTCHILD) << DELETECASCADE(TABLEPARENT, ATTPARENT, TABLECHILD, ATTCHILD)

/**
 * @def DELETECASCADEPARAMETER
 * Define a cascaded delete to delete parameters associated with an object
 */
#define DELETECASCADEPARAMETER(TABLE)                                                                                                                          \
    (QStringList() << QString()%"DROP TRIGGER IF EXISTS fkdc_"%(TABLE)%"_parameters_uuid" \
     << QString()%"CREATE TRIGGER fkdc_"%(TABLE)%"_parameters_uuid "\
     "BEFORE DELETE ON "%(TABLE)%" "\
     "FOR EACH ROW BEGIN "\
     "    DELETE FROM parameters WHERE parameters.t_uuid_parent=OLD.id||'-'||'"%(TABLE)%"'; "\
     "END")

#endif
