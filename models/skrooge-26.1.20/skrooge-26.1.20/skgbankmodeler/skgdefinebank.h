/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDEFINEBANK_H
#define SKGDEFINEBANK_H
/** @file
 * This file defines some macros and constants.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdefine.h"

/**
 * @var DUMPUNIT
 * To display units and values
 * @see dump
 */
static const int DUMPUNIT = (2u << 10);

/**
 * @var DUMPACCOUNT
 * To display accounts
 * @see dump
 */
static const int DUMPACCOUNT = (2u << 11);

/**
 * @var DUMPOPERATION
 * To display accounts
 * @see dump
 */
static const int DUMPOPERATION = (2u << 12);

/**
 * @var DUMPCATEGORY
 * To display categories
 * @see dump
 */
static const int DUMPCATEGORY = (2u << 13);

/**
 * @var DUMPPAYEE
 * To display payees
 * @see dump
 */
static const int DUMPPAYEE = (2u << 14);

/**
 * @var DUMPBUDGET
 * To display payees
 * @see dump
 */
static const int DUMPBUDGET = (2u << 15);

/**
 * @var DUMPBANKOBJECT
 * To display categories
 * @see dump
 */
static const int DUMPBANKOBJECT = DUMPUNIT | DUMPACCOUNT | DUMPOPERATION | DUMPCATEGORY | DUMPPAYEE | DUMPBUDGET;
#endif
