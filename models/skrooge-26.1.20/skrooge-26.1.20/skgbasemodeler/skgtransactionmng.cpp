/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGTransactionMng.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtransactionmng.h"
#include "skgdocument.h"
#include "skgerror.h"

SKGTransactionMng::SKGTransactionMng(SKGDocument *iDocument, const QString &iName, SKGError *iError, int iNbStep, bool iRefreshViews)
{
    m_parentDocument = iDocument;
    m_error = iError;
    m_errorInBeginTransaction = false;
    if ((m_parentDocument != nullptr) && (m_error != nullptr)) {
        *m_error = m_parentDocument->beginTransaction(iName, iNbStep, QDateTime::currentDateTime(), iRefreshViews);
        m_errorInBeginTransaction = (m_error->isFailed());
    }
}

SKGTransactionMng::~SKGTransactionMng()
{
    if ((m_parentDocument != nullptr) && (m_error != nullptr)) {
        // close the transaction based on error
        if (!m_errorInBeginTransaction) {
            if (m_error->isSucceeded()) {
                SKGError opError = *m_error; // In case of the message is not empty
                *m_error = m_parentDocument->endTransaction(true);
                if (m_error->isSucceeded()) {
                    *m_error = opError;
                }
            } else {
                m_parentDocument->endTransaction(false);
            }
        }
        m_parentDocument = nullptr;
        m_error = nullptr;
    }
}
