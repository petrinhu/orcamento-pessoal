/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGDocumentPrivate.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdocumentprivate.h"

/**
 * Last error.
 */
SKGError SKGDocumentPrivate::m_lastCallbackError;

/**
 * Unique identifier.
 */
int SKGDocumentPrivate::m_databaseUniqueIdentifier = 0;

SKGDocumentPrivate::SKGDocumentPrivate()
    : m_currentFileName(QString())
{
    m_cacheSql = new QHash<QString, SKGStringListList>();
}

SKGDocumentPrivate::~SKGDocumentPrivate()
{
    delete m_cacheSql;
    m_cacheSql = nullptr;
}
