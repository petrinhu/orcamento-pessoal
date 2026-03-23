/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTRANSACTIONMNG_H
#define SKGTRANSACTIONMNG_H
/** @file
 * This file defines classes SKGTransactionMng and a macro.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdefine.h"
#include "skgtraces.h"
#include <qstring.h>

/**
 * @def SKGBEGINTRANSACTION(Document, Name, Error)
 * Macro to start a new transaction
 * Example of usage:
 * @code
 * { // BEGIN OF THE TRANSACTION SCOPE
 *  // Error management of the scope
 *  SKGError err;
 *
 *  // Begin transaction
 *   SKGBEGINTRANSACTION(Your_SKGDocument, QStringLiteral("The_Name_Of_The_Transaction"), err)
 *
 *  // Check the error
 *  if(!err)
 *  {
 *   // You code here
 *   // At the end the err variable must contain the result of your operation
 *  }
 *
 *  // A commit or rollback is done here because the scope is close
 * } // END OF THE TRANSACTION
 * @endcode
 */
#define SKGBEGINTRANSACTION(Document, Name, Error) SKGTransactionMng TOKENPASTE2(transaction_, __LINE__)(&(Document), Name, &(Error));

/**
 * @def SKGBEGINLIGHTTRANSACTION(Document, Name, Error)
 * Macro to start a new light transaction (without views computation)
 */
#define SKGBEGINLIGHTTRANSACTION(Document, Name, Error) SKGTransactionMng transactionlight_(&(Document), Name, &(Error), 1, false);
/**
 * @def SKGBEGINPROGRESSTRANSACTION(Document, Name, Error)
 * Macro to start a new transaction
 * Example of usage:
 * @code
 * { // BEGIN OF THE TRANSACTION SCOPE
 *  // Error management of the scope
 *  SKGError err;
 *
 *  // Begin transaction
 *   SKGBEGINPROGRESSTRANSACTION(Your_SKGDocument, "The_Name_Of_The_Transaction", err, 5)
 *
 *  // Check the error
 *  if(!err)
 *  {
 *   // You code here
 *   // At the end the err variable must contain the result of your operation
 *  Your_SKGDocument.stepForward(1);
 *  …
 *  Your_SKGDocument.stepForward(2);
 *  …
 *  Your_SKGDocument.stepForward(3);
 *  …
 *  }
 *
 *  // A commit or rollback is done here because the scope is close
 * } // END OF THE TRANSACTION
 * @endcode
 */
#define SKGBEGINPROGRESSTRANSACTION(Document, Name, Error, Nb) SKGTransactionMng TOKENPASTE2(transaction_, __LINE__)(&(Document), Name, &(Error), Nb);

/**
 * @def SKGBEGINLIGHTPROGRESSTRANSACTION(Document, Name, Error)
 * Macro to start a new light transaction (without views computation)
 */
#define SKGBEGINLIGHTPROGRESSTRANSACTION(Document, Name, Error, Nb)                                                                                            \
    SKGTransactionMng TOKENPASTE2(transactionlight_, __LINE__)(&(Document), Name, &(Error), Nb, false);

/**
 * @def SKGENDTRANSACTION(Document,  Error)
 * End correctly a transaction
 */
#define SKGENDTRANSACTION(Document, Error)                                                                                                                     \
    if (!(Error)) {                                                                                                                                            \
        (Error) = (Document)->endTransaction(true);                                                                                                            \
    } else {                                                                                                                                                   \
        (Document)->endTransaction(false);                                                                                                                     \
    }

class SKGError;
class SKGDocument;

/**
 * Facilitate the transaction management
 */
class SKGBASEMODELER_EXPORT SKGTransactionMng
{
public:
    /**
     * Constructor.
     * This class must be used with the macro
     * @see SKGBEGINTRANSACTION
     * @param iDocument The parent document of the transaction
     * @param iName The message of the transaction
     * @param iError A pointer of the error object of the calling scope
     * @param iNbStep the number of step in this transaction.
     * @param iRefreshViews at the end of the transaction, computed views will be recomputed.
     * It is used to call the progress callback.
     * @see The SKGError
     */
    explicit SKGTransactionMng(SKGDocument *iDocument, const QString &iName, SKGError *iError, int iNbStep = 1, bool iRefreshViews = true);

    /**
     * Destructor
     */
    virtual ~SKGTransactionMng();

private:
    Q_DISABLE_COPY(SKGTransactionMng)

    SKGError *m_error;
    SKGDocument *m_parentDocument;
    bool m_errorInBeginTransaction;
};

#endif
