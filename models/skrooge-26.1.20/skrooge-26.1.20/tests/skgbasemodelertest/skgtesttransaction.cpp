/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test script.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestmacro.h"

/**
 * To check the progress
 */
static int previousProgress = 0;

/**
 * To test progress
 * @param iPos the current position
 * @return 0
 */
int progress1(int iPos, qint64 /*iTime*/, const QString & /*iName*/, void * /*iData*/)
{
    if (previousProgress > iPos) {
        return 1;
    }
    previousProgress = iPos;
    return 0;
}

/**
 * To test progress
 * @param iPos the current position
 * @return 1
 */
int progress2(int iPos, qint64 /*iTime*/, const QString & /*iName*/, void * /*iData*/)
{
    if (iPos > 50) {
        return 1;
    }
    return 0;
}
/**
 * The main function of the unit test
 * @param argc the number of arguments
 * @param argv the list of arguments
 */
int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Init test
    SKGINITTEST(true)

    // test class SKGDocument / COMMIT / ROLLBACK
    {
        SKGDocument document1;
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 0)
        SKGTESTERROR(QStringLiteral("TRANS.beginTransaction(transaction1)"), document1.beginTransaction(QStringLiteral("transaction1")), false)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 0)
        SKGTESTERROR(QStringLiteral("TRANS.initialize"), document1.initialize(), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 0)
        SKGTESTERROR(QStringLiteral("TRANS.beginTransaction(transaction1)"), document1.beginTransaction(QStringLiteral("transaction1")), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 1)
        SKGTESTERROR(QStringLiteral("TRANS.endTransaction"), document1.endTransaction(false), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 0)
        SKGTESTERROR(QStringLiteral("TRANS.beginTransaction(transaction2)"), document1.beginTransaction(QStringLiteral("transaction2")), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 1)
        SKGTESTERROR(QStringLiteral("TRANS.setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1")), true)
        SKGTESTERROR(QStringLiteral("TRANS.endTransaction"), document1.endTransaction(true), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 0)
        SKGTESTERROR(QStringLiteral("TRANS.endTransaction TOO MANY !"), document1.endTransaction(true), false)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 0)

        // Test rollback
        SKGTESTERROR(QStringLiteral("TRANS.beginTransaction"), document1.beginTransaction(QStringLiteral("transaction3")), true)
        SKGTESTERROR(QStringLiteral("TRANS.setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL2")), true)
        SKGTESTERROR(QStringLiteral("TRANS.endTransaction"), document1.endTransaction(false), true)
        SKGTEST(QStringLiteral("TRANS.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))

        // Test multi transaction in cascade
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 0)
        SKGTESTERROR(QStringLiteral("TRANS.beginTransaction"), document1.beginTransaction(QStringLiteral("transaction4")), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 1)
        SKGTESTERROR(QStringLiteral("TRANS.beginTransaction"), document1.beginTransaction(QStringLiteral("transaction'5")), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 2)
        SKGTESTERROR(QStringLiteral("TRANS.setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL2")), true)
        SKGTESTERROR(QStringLiteral("TRANS.endTransaction"), document1.endTransaction(false), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 1)
        SKGTEST(QStringLiteral("TRANS.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL2"))
        SKGTESTERROR(QStringLiteral("TRANS.endTransaction"), document1.endTransaction(false), true)
        SKGTEST(QStringLiteral("TRANS.getDepthTransaction"), document1.getDepthTransaction(), 0)
        SKGTEST(QStringLiteral("TRANS.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))

        SKGPropertyObject prop(&document1);
        SKGTESTERROR(QStringLiteral("PROP.getParameter"), prop.setName(QStringLiteral("ATT1")), true)
        SKGTESTERROR(QStringLiteral("PROP.load"), prop.load(), true)
        SKGTEST(QStringLiteral("PROP.getValue"), prop.getValue(), QStringLiteral("VAL1"))
        prop.getParentId();

        SKGPropertyObject prop2(prop);
        SKGPropertyObject prop3(static_cast<SKGObjectBase>(prop));
        SKGPropertyObject prop4;
        prop4 = static_cast<SKGObjectBase>(prop);
    }

    // test class SKGDocument / COMMIT / ROLLBACK with SKGTransactionMng
    {
        // Test with a succeeded transaction
        SKGError err;
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("TRANSMNG.initialize()"), document1.initialize(), true)
        SKGTEST(QStringLiteral("TRANSMNG.getNbTransaction"), document1.getNbTransaction(), 0)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("T1"), err)

            // The code here
            IFOK(err)
            {
                SKGTEST(QStringLiteral("TRANSMNG.getDepthTransaction"), document1.getDepthTransaction(), 1)

                // Example; an error is generated
                err = SKGError(1, QStringLiteral("Error1"));
            }

            // A rollback is done here because the scope is close
        }
        SKGTESTERROR(QStringLiteral("TRANSMNG.err"), err, false)
        SKGTEST(QStringLiteral("TRANSMNG.getNbTransaction"), document1.getNbTransaction(), 0)
        SKGTEST(QStringLiteral("TRANSMNG.getDepthTransaction"), document1.getDepthTransaction(), 0)
    }

    {
        // Test with a succeeded transaction
        SKGError err;
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("TRANSMNG.initialize()"), document1.initialize(), true)
        SKGTEST(QStringLiteral("TRANSMNG.getNbTransaction"), document1.getNbTransaction(), 0)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("T1"), err)

            // The code here
            IFOK(err)
            {
                SKGTEST(QStringLiteral("TRANSMNG.getDepthTransaction"), document1.getDepthTransaction(), 1)

                // Example; transaction succeeded
                SKGTESTERROR(QStringLiteral("TRANS.setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL2")), true)
            }

            // A commit is done here because the scope is close
        }
        SKGTESTERROR(QStringLiteral("TRANSMNG.err"), err, true)
        SKGTEST(QStringLiteral("TRANSMNG.getNbTransaction"), document1.getNbTransaction(), 1)
        SKGTEST(QStringLiteral("TRANSMNG.getDepthTransaction"), document1.getDepthTransaction(), 0)
    }

    {
        // Test undo on parameters
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("TRANSUNDO.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGTEST(QStringLiteral("TRANSUNDO.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QString())

            {// In a scope to the then endTransaction automatically
             SKGBEGINTRANSACTION(document1, QStringLiteral("T1"), err)
                 SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                              document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1_1")),
                              true) // To test undo on more than one modification of an object in a transaction
             SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                          document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1_2")),
                          true) // To test undo on more than one modification of an object in a transaction
             SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                          document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1")),
                          true)} SKGTEST(QStringLiteral("TRANSUNDO.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
                SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction"), document1.getNbTransaction(), 1)
                    SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction(SKGDocument::REDO))"), document1.getNbTransaction(SKGDocument::REDO), 0)

                        {// In a scope to the then endTransaction automatically
                         SKGBEGINTRANSACTION(document1, QStringLiteral("T2"), err)
                             SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                                          document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL2_1")),
                                          true) // To test undo on more than one modification of an object in a transaction
                         SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                                      document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL2_2")),
                                      true) // To test undo on more than one modification of an object in a transaction

                         SKGTESTERROR(
                             QStringLiteral("TRANSUNDO.setParameter"),
                             document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL2")),
                             true)} SKGTEST(QStringLiteral("TRANSUNDO.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL2"))
                            SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction"), document1.getNbTransaction(), 2)
                                SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction(SKGDocument::REDO))"), document1.getNbTransaction(SKGDocument::REDO), 0)

                                    QStringList oResult;
        int nbitemexpected = 6;
        SKGTESTERROR(QStringLiteral("TRANSUNDO.getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("doctransactionitem"), QStringLiteral("id"), oResult),
                     true)
        SKGTEST(QStringLiteral("TRANSUNDO.oResult.size"), oResult.size(), nbitemexpected)

        SKGTESTERROR(QStringLiteral("TRANSUNDO.undoRedoTransaction(T2, SKGDocument::UNDO)"), document1.undoRedoTransaction(), true)
        SKGTEST(QStringLiteral("TRANSUNDO.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
        SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction"), document1.getNbTransaction(), 1)
        SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction(SKGDocument::REDO))"), document1.getNbTransaction(SKGDocument::REDO), 1)

        SKGTESTERROR(QStringLiteral("TRANSUNDO.getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("doctransactionitem"), QStringLiteral("id"), oResult),
                     true)
        SKGTEST(QStringLiteral("TRANSUNDO.oResult.size"), oResult.size(), nbitemexpected)

        SKGTESTERROR(QStringLiteral("TRANSUNDO.undoRedoTransaction(T1, SKGDocument::UNDO)"), document1.undoRedoTransaction(), true)
        SKGTEST(QStringLiteral("TRANSUNDO.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QString())
        SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction"), document1.getNbTransaction(), 0)
        SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction(SKGDocument::REDO))"), document1.getNbTransaction(SKGDocument::REDO), 2)

        SKGTESTERROR(QStringLiteral("TRANSUNDO.getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("doctransactionitem"), QStringLiteral("id"), oResult),
                     true)
        SKGTEST(QStringLiteral("TRANSUNDO.oResult.size"), oResult.size(), nbitemexpected)

        SKGTESTERROR(QStringLiteral("TRANSUNDO.undoRedoTransaction(T1, SKGDocument::REDO)"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTEST(QStringLiteral("TRANSUNDO.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
        SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction"), document1.getNbTransaction(), 1)
        SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction(SKGDocument::REDO))"), document1.getNbTransaction(SKGDocument::REDO), 1)

        SKGTESTERROR(QStringLiteral("TRANSUNDO.getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("doctransactionitem"), QStringLiteral("id"), oResult),
                     true)
        SKGTEST(QStringLiteral("TRANSUNDO.oResult.size"), oResult.size(), nbitemexpected)

        SKGTESTERROR(QStringLiteral("TRANSUNDO.undoRedoTransaction(T2, SKGDocument::REDO)"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTEST(QStringLiteral("TRANSUNDO.getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL2"))
        SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction"), document1.getNbTransaction(), 2)
        SKGTEST(QStringLiteral("TRANSUNDO.getNbTransaction(SKGDocument::REDO))"), document1.getNbTransaction(SKGDocument::REDO), 0)
        SKGTESTERROR(QStringLiteral("TRANSUNDO.getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("doctransactionitem"), QStringLiteral("id"), oResult),
                     true)
        SKGTEST(QStringLiteral("TRANSUNDO.oResult.size"), oResult.size(), nbitemexpected)
    }

    {
        // Test max depth for transaction
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("MAXDEPTH.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGTEST(QStringLiteral("MAXDEPTH.getNbTransaction"), document1.getNbTransaction(), 0){
            SKGBEGINTRANSACTION(document1, QStringLiteral("MAXDEPTH 1"), err)
                SKGTESTERROR(QStringLiteral("MAXDEPTH.setParameter"),
                             document1.setParameter(QStringLiteral("SKG_UNDO_MAX_DEPTH"), QStringLiteral("3")),
                             true)} SKGTEST(QStringLiteral("MAXDEPTH.getNbTransaction"), document1.getNbTransaction(), 1){
            SKGBEGINTRANSACTION(document1, QStringLiteral("MAXDEPTH 2"), err)
                SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                             document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")),
                             true)} SKGTESTERROR(QStringLiteral("MAXDEPTH.endTransaction"), err, true)
            SKGTEST(QStringLiteral("MAXDEPTH.getNbTransaction"), document1.getNbTransaction(), 2){
                SKGBEGINTRANSACTION(document1, QStringLiteral("MAXDEPTH 3"), err)
                    SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                                 document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")),
                                 true)} SKGTESTERROR(QStringLiteral("MAXDEPTH.endTransaction"), err, true)
                SKGTEST(QStringLiteral("MAXDEPTH.getNbTransaction"), document1.getNbTransaction(), 3){
                    SKGBEGINTRANSACTION(document1, QStringLiteral("MAXDEPTH 4"), err)
                        SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                                     document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")),
                                     true)} SKGTESTERROR(QStringLiteral("MAXDEPTH.endTransaction"), err, true)
                    SKGTEST(QStringLiteral("MAXDEPTH.getNbTransaction"), document1.getNbTransaction(), 3){
                        SKGBEGINTRANSACTION(document1, QStringLiteral("MAXDEPTH 5"), err)
                            SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)
                                SKGTESTERROR(QStringLiteral("TRANSUNDO.setParameter"),
                                             document1.setParameter(QStringLiteral("ATT5"), QStringLiteral("VAL")),
                                             true)} SKGTESTERROR(QStringLiteral("MAXDEPTH.endTransaction"), err, true)
                        SKGTEST(QStringLiteral("MAXDEPTH.getNbTransaction"), document1.getNbTransaction(), 3)

                            SKGDocument::SKGObjectModificationList oModifications;
        SKGTESTERROR(QStringLiteral("MAXDEPTH.getModifications"),
                     document1.getModifications(document1.getTransactionToProcess(SKGDocument::UNDO), oModifications),
                     true)
        SKGTEST(QStringLiteral("MAXDEPTH.oModifications.count"), oModifications.count(), 2)
        SKGTEST(QStringLiteral("MAXDEPTH.oModifications.id"), oModifications[0].id, 7)
        SKGTEST(QStringLiteral("MAXDEPTH.oModifications.table"), oModifications[0].table, QStringLiteral("parameters"))
        SKGTEST(QStringLiteral("MAXDEPTH.oModifications.type"), static_cast<unsigned int>(oModifications[0].type), static_cast<unsigned int>(SKGDocument::U))
        SKGTEST(QStringLiteral("MAXDEPTH.oModifications.id"), oModifications[1].id, 8)
        SKGTEST(QStringLiteral("MAXDEPTH.oModifications.table"), oModifications[1].table, QStringLiteral("parameters"))
        SKGTEST(QStringLiteral("MAXDEPTH.oModifications.type"), static_cast<unsigned int>(oModifications[1].type), static_cast<unsigned int>(SKGDocument::I))
    }

    {
        // Redo delete after new transaction
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("REDO.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(), 0)
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("SKGDocument::REDO 1"), err)
            SKGTESTERROR(QStringLiteral("REDO.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)
        }
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("SKGDocument::REDO 2"), err)
            SKGTESTERROR(QStringLiteral("REDO.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)
        }
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("SKGDocument::REDO 3"), err)
            SKGTESTERROR(QStringLiteral("REDO.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)
        }
        {SKGBEGINTRANSACTION(document1, QStringLiteral("SKGDocument::REDO 4"), err)
             SKGTESTERROR(QStringLiteral("REDO.setParameter"),
                          document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")),
                          true)} SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(), 4)
            SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 0)

                SKGTESTERROR(QStringLiteral("REDO.undoRedoTransaction"), document1.undoRedoTransaction(), true)
                    SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(), 3)
                        SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 1)

                            SKGTESTERROR(QStringLiteral("REDO.undoRedoTransaction"), document1.undoRedoTransaction(), true)
                                SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(), 2)
                                    SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 2)

                                        SKGTESTERROR(QStringLiteral("REDO.undoRedoTransaction"), document1.undoRedoTransaction(SKGDocument::REDO), true)
                                            SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(), 3)
                                                SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 1)

                                                    SKGTESTERROR(QStringLiteral("REDO.undoRedoTransaction"), document1.undoRedoTransaction(), true)
                                                        SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(), 2)
                                                            SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 2)

                                                                {SKGBEGINTRANSACTION(document1, QStringLiteral("SKGDocument::REDO 5"), err)
                                                                     SKGTESTERROR(QStringLiteral("REDO.setParameter"),
                                                                                  document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")),
                                                                                  true)}

        SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(), 3)
            SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 0)

                SKGTESTERROR(QStringLiteral("REDO.undoRedoTransaction"), document1.undoRedoTransaction(SKGDocument::UNDOLASTSAVE), true)
                    SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(), 0)
                        SKGTEST(QStringLiteral("REDO.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 1)
    }

    {
        // Test progress
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("PROGRESS.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.setProgressCallback"), document1.setProgressCallback(&progress1, nullptr), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.beginTransaction"), document1.beginTransaction(QStringLiteral("PROGRESS1"), 5), true)
        for (int i = 1; i <= 5; ++i) {
            SKGTESTERROR(QStringLiteral("PROGRESS.stepForward"), document1.stepForward(i), true)
            SKGTESTERROR(QStringLiteral("PROGRESS.beginTransaction"), document1.beginTransaction(QStringLiteral("PROGRESS2"), 3), true)
            for (int j = 1; j <= 3; j++) {
                SKGTESTERROR(QStringLiteral("PROGRESS.stepForward"), document1.stepForward(j), true)
            }
            SKGTESTERROR(QStringLiteral("PROGRESS.endTransaction"), document1.endTransaction(true), true)
        }
        SKGTESTERROR(QStringLiteral("PROGRESS.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("PROGRESS.setProgressCallback"), document1.setProgressCallback(&progress2, nullptr), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.beginTransaction"), document1.beginTransaction(QStringLiteral("PROGRESS1"), 5), true)
        for (int i = 1; i <= 5; ++i) {
            SKGTESTERROR(QStringLiteral("PROGRESS.stepForward"), document1.stepForward(i), (i < 3))
            SKGTESTERROR(QStringLiteral("PROGRESS.beginTransaction"), document1.beginTransaction(QStringLiteral("PROGRESS2"), 3), (i < 3))
            for (int j = 1; j <= 3; j++) {
                SKGTESTERROR(QStringLiteral("PROGRESS.stepForward"), document1.stepForward(j), (3 * i + j - 3 < 5))
            }
            SKGTESTERROR(QStringLiteral("PROGRESS.endTransaction"), document1.endTransaction(true), true)
        }
        SKGTESTERROR(QStringLiteral("PROGRESS.endTransaction"), document1.endTransaction(true), true)
    }

    {
        // Test progress
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("PROGRESS.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.beginTransaction"), document1.beginTransaction(QStringLiteral("T1")), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)

        SKGTESTERROR(QStringLiteral("PROGRESS.endTransaction"), document1.endTransaction(true), true)
        SKGTEST(QStringLiteral("PROGRESS.getNbTransaction"), document1.getNbTransaction(), 1)

        SKGTESTERROR(QStringLiteral("PROGRESS.beginTransaction"), document1.beginTransaction(QStringLiteral("T2")), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.endTransaction"), document1.endTransaction(true), true)
        SKGTEST(QStringLiteral("PROGRESS.getNbTransaction"), document1.getNbTransaction(), 2)

        SKGTESTERROR(QStringLiteral("PROGRESS.beginTransaction"), document1.beginTransaction(QStringLiteral("T3")), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.endTransaction"), document1.endTransaction(false), true)
        SKGTEST(QStringLiteral("PROGRESS.getNbTransaction"), document1.getNbTransaction(), 2)

        // Transaction failed with existing name
        SKGTESTERROR(QStringLiteral("PROGRESS.beginTransaction"), document1.beginTransaction(QStringLiteral("T2")), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)
        SKGTESTERROR(QStringLiteral("PROGRESS.endTransaction"), document1.endTransaction(false), true)
        SKGTEST(QStringLiteral("PROGRESS.getNbTransaction"), document1.getNbTransaction(), 2)
    }

    {
        // Test group of transactions on U
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("GROUP.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T1")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL1")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T2")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL2")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T3")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL3")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T4")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL4")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.groupTransactions("), document1.groupTransactions(3, 5), true)
        SKGTEST(QStringLiteral("GROUP.getNbTransaction"), document1.getNbTransaction(), 2)
        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(), true)
        SKGTEST(QStringLiteral("GROUP.getParameter"), document1.getParameter(QStringLiteral("ATT")), QStringLiteral("VAL1"))

        // Remove all transaction
        SKGTEST(QStringLiteral("GROUP.getNbTransaction"), document1.getNbTransaction(SKGDocument::UNDO), 1)
        SKGTEST(QStringLiteral("GROUP.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 1)
        SKGTESTERROR(QStringLiteral("GROUP.removeAllTransactions"), document1.removeAllTransactions(), true)
        SKGTEST(QStringLiteral("GROUP.getNbTransaction"), document1.getNbTransaction(SKGDocument::UNDO), 0)
        SKGTEST(QStringLiteral("GROUP.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 0)
    }

    {
        // Test group of transactions on R
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("GROUP.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T1")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL1")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T2")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL2")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T3")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL3")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T4")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL4")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(), true)
        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(), true)
        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(), true)
        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(), true)

        SKGTESTERROR(QStringLiteral("GROUP.groupTransactions("), document1.groupTransactions(7, 9), true)
        SKGTEST(QStringLiteral("GROUP.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 2)
        //      document1.dump(DUMPTRANSACTIONS);
        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTEST(QStringLiteral("GROUP.getParameter"), document1.getParameter(QStringLiteral("ATT")), QStringLiteral("VAL3"))
    }

    {
        // Test group of transactions on U and R
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("GROUP.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T1")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL1")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T2")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL2")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T3")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL3")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T4")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL4")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(), true)
        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(), true)

        SKGTESTERROR(QStringLiteral("GROUP.groupTransactions("), document1.groupTransactions(3, 6), false)
        //      document1.dump(DUMPTRANSACTIONS);
        SKGTEST(QStringLiteral("GROUP.getNbTransaction"), document1.getNbTransaction(SKGDocument::UNDO), 2)
        SKGTEST(QStringLiteral("GROUP.getNbTransaction"), document1.getNbTransaction(SKGDocument::REDO), 2)
    }

    {
        // Test group of transactions on R
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("GROUP.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T1")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL1")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T2")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL2")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        QString filename1 = SKGTest::getTestPath(QStringLiteral("OUT")) % "/filename1.skg";
        SKGTESTERROR(QStringLiteral("GROUP.saveAs"), document1.saveAs(filename1, true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T3")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL3")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("GROUP.beginTransaction"), document1.beginTransaction(QStringLiteral("T4")), true)
        SKGTESTERROR(QStringLiteral("GROUP.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL4")), true)
        SKGTESTERROR(QStringLiteral("GROUP.endTransaction"), document1.endTransaction(true), true)

        document1.dump(DUMPTRANSACTIONS);
        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(SKGDocument::UNDOLASTSAVE), true)
        SKGTEST(QStringLiteral("GROUP.getParameter"), document1.getParameter(QStringLiteral("ATT")), QStringLiteral("VAL2"))
        SKGTESTERROR(QStringLiteral("GROUP.undoRedoTransaction"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTEST(QStringLiteral("GROUP.getParameter"), document1.getParameter(QStringLiteral("ATT")), QStringLiteral("VAL4"))

        QStringList oResult;
        SKGTESTERROR(QStringLiteral("SKGServices::dumpSelectSqliteOrder"),
                     document1.dumpSelectSqliteOrder(QStringLiteral("SELECT * from doctransaction"), oResult),
                     true)
        SKGTESTERROR(QStringLiteral("SKGServices::dumpSelectSqliteOrder"),
                     document1.dumpSelectSqliteOrder(QStringLiteral("SELECT * from doctransaction")),
                     true)
    }

    {
        // Test error
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("ERROR.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("ERROR.undoRedoTransaction"), document1.undoRedoTransaction(SKGDocument::UNDO), false) // No transaction

        SKGTESTERROR(QStringLiteral("ERROR.beginTransaction"), document1.beginTransaction(QStringLiteral("T1")), true)
        SKGTEST(QStringLiteral("ERROR.getCurrentTransaction"), document1.getCurrentTransaction(), 2)

        SKGTESTERROR(QStringLiteral("ERROR.groupTransactions"), document1.groupTransactions(1, 1), false) // Not authorized into a transaction
        SKGTESTERROR(QStringLiteral("ERROR.undoRedoTransaction"), document1.undoRedoTransaction(SKGDocument::UNDO), false) // Not authorized into a transaction
        SKGTESTERROR(QStringLiteral("ERROR.saveAs"),
                     document1.saveAs(SKGTest::getTestPath(QStringLiteral("OUT")) % "/filename1.skg", true),
                     false) // Not authorized into a transaction

        SKGTESTERROR(QStringLiteral("ERROR.endTransaction"), document1.endTransaction(true), true)
    }

    {
        // Test messages
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("MSG.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("MSG.sendMessage"), document1.sendMessage(QStringLiteral("Information"), SKGDocument::Information), true)
        SKGTESTERROR(QStringLiteral("MSG.sendMessage"), document1.sendMessage(QStringLiteral("Warning"), SKGDocument::Warning), true)
        SKGTESTERROR(QStringLiteral("MSG.sendMessage"), document1.sendMessage(QStringLiteral("Positive"), SKGDocument::Positive), true)
        SKGTESTERROR(QStringLiteral("MSG.sendMessage"), document1.sendMessage(QStringLiteral("Error"), SKGDocument::Error), true)
        SKGTESTERROR(QStringLiteral("MSG.sendMessage"), document1.sendMessage(QStringLiteral("Hidden"), SKGDocument::Hidden), true)

        SKGTESTERROR(QStringLiteral("MSG.beginTransaction"), document1.beginTransaction(QStringLiteral("T1")), true)
        SKGTESTERROR(QStringLiteral("MSG.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL")), true)
        SKGTESTERROR(QStringLiteral("MSG.sendMessage"), document1.sendMessage(QStringLiteral("Hello")), true)
        SKGTESTERROR(QStringLiteral("MSG.sendMessage"), document1.sendMessage(QStringLiteral("World")), true)
        SKGTESTERROR(QStringLiteral("MSG.endTransaction"), document1.endTransaction(true), true)

        SKGDocument::SKGMessageList msg;
        SKGTESTERROR(QStringLiteral("MSG.getMessages"), document1.getMessages(document1.getTransactionToProcess(SKGDocument::UNDO), msg), true)
        SKGTEST(QStringLiteral("MSG.msg.count"), msg.count(), 6)
        SKGTEST(QStringLiteral("MSG.msg"), msg.at(0).Text, QStringLiteral("Information"));
        SKGTEST(QStringLiteral("MSG.type"), static_cast<unsigned int>(msg.at(0).Type), static_cast<unsigned int>(SKGDocument::Information))
        SKGTEST(QStringLiteral("MSG.msg"), msg.at(1).Text, QStringLiteral("Warning"));
        SKGTEST(QStringLiteral("MSG.type"), static_cast<unsigned int>(msg.at(1).Type), static_cast<unsigned int>(SKGDocument::Warning))
        SKGTEST(QStringLiteral("MSG.msg"), msg.at(2).Text, QStringLiteral("Positive"));
        SKGTEST(QStringLiteral("MSG.type"), static_cast<unsigned int>(msg.at(2).Type), static_cast<unsigned int>(SKGDocument::Positive))
        SKGTEST(QStringLiteral("MSG.msg"), msg.at(3).Text, QStringLiteral("Error"));
        SKGTEST(QStringLiteral("MSG.type"), static_cast<unsigned int>(msg.at(3).Type), static_cast<unsigned int>(SKGDocument::Error))
        SKGTEST(QStringLiteral("MSG.msg"), msg.at(4).Text, QStringLiteral("Hello"));
        SKGTEST(QStringLiteral("MSG.type"), static_cast<unsigned int>(msg.at(4).Type), static_cast<unsigned int>(SKGDocument::Information))
        SKGTEST(QStringLiteral("MSG.msg"), msg.at(5).Text, QStringLiteral("World"));
        SKGTEST(QStringLiteral("MSG.type"), static_cast<unsigned int>(msg.at(5).Type), static_cast<unsigned int>(SKGDocument::Information))
    }

    {
        // Test clean after save
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("CLEAN.initialize()"), document1.initialize(), true)

        SKGTESTERROR(QStringLiteral("CLEAN.beginTransaction"), document1.beginTransaction(QStringLiteral("T1")), true)
        SKGTESTERROR(QStringLiteral("CLEAN.setParameter"), document1.setParameter(QStringLiteral("SKG_UNDO_CLEAN_AFTER_SAVE"), QStringLiteral("Y")), true)
        SKGTESTERROR(QStringLiteral("CLEAN.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL1")), true)
        SKGTESTERROR(QStringLiteral("CLEAN.endTransaction"), document1.endTransaction(true), true)

        SKGTESTERROR(QStringLiteral("CLEAN.beginTransaction"), document1.beginTransaction(QStringLiteral("T2")), true)
        SKGTESTERROR(QStringLiteral("CLEAN.setParameter"), document1.setParameter(QStringLiteral("ATT"), QStringLiteral("VAL2")), true)
        SKGTESTERROR(QStringLiteral("CLEAN.endTransaction"), document1.endTransaction(true), true)

        SKGTEST(QStringLiteral("CLEAN.getNbTransaction"), document1.getNbTransaction(SKGDocument::UNDO), 2)

        QString filename1 = SKGTest::getTestPath(QStringLiteral("OUT")) % "/filename1.skg";
        SKGTESTERROR(QStringLiteral("CLEAN.saveAs"), document1.saveAs(filename1, true), true)
        SKGTEST(QStringLiteral("CLEAN.getNbTransaction"), document1.getNbTransaction(SKGDocument::UNDO), 0)
    }
    // End test
    SKGENDTEST()
}
