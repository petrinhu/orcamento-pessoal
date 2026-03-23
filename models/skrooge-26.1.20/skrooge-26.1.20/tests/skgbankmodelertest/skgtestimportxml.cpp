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
#include "skgaccountobject.h"
#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgtestmacro.h"

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

    {
        // Test import XML
        SKGDocumentBank document1;
        SKGError err;
        {
            SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

            for (int i = 1; !err && i <= 2; ++i) {
                {
                    // Scope of the transaction
                    SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_XML"), err)
                    SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("/not-existing/missingfile.xml")));
                    SKGTESTERROR(QStringLiteral("imp1.importFile") + SKGServices::intToString(i), impmissing.importFile(), false)

                    SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxml/input.xml"));
                    SKGTESTERROR(QStringLiteral("XML.importFile") + SKGServices::intToString(i), imp1.importFile(), true)
                }

                {
                    SKGAccountObject account(&document1);
                    SKGTESTERROR(QStringLiteral("XML.setName") + SKGServices::intToString(i), account.setName(QStringLiteral("COURANT")), true)
                    SKGTESTERROR(QStringLiteral("XML.load") + SKGServices::intToString(i), account.load(), true)
                    SKGTEST(QStringLiteral("XML:getCurrentAmount") + SKGServices::intToString(i),
                            SKGServices::doubleToString(account.getCurrentAmount()),
                            QStringLiteral("-15255"))
                }

                {
                    SKGAccountObject account(&document1);
                    SKGTESTERROR(QStringLiteral("XML.setName") + SKGServices::intToString(i), account.setName(QStringLiteral("EPARGNE")), true)
                    SKGTESTERROR(QStringLiteral("XML.load") + SKGServices::intToString(i), account.load(), true)
                    SKGTEST(QStringLiteral("XML:getCurrentAmount") + SKGServices::intToString(i),
                            SKGServices::doubleToString(account.getCurrentAmount()),
                            QStringLiteral("197.8444982"))
                }
            }
        }
        {
            SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
            {
                SKGTESTERROR(QStringLiteral("DOC:changePassword"), document1.changePassword(QStringLiteral("test")), true)

                // Scope of the transaction
                SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_ISO20022"), err)

                SKGImportExportManager imp1(
                    &document1,
                    QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxml/camt_053_ver_2_extended_uk_account.xml"));
                SKGTESTERROR(QStringLiteral("ISO20022.importFile"), imp1.importFile(), true)
            }

            {
                SKGAccountObject account(&document1);
                SKGTESTERROR(QStringLiteral("ISO20022.setName"), account.setName(QStringLiteral("GB87HAND40516218000025")), true)
                SKGTESTERROR(QStringLiteral("ISO20022.load"), account.load(), true)
                SKGTEST(QStringLiteral("ISO20022:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-0.1"))
            }
        }
        {
            SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
            {
                // Scope of the transaction
                SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_UNKNOWN"), err)

                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxml/unknown.xml"));
                SKGTESTERROR(QStringLiteral("ISO20022.importFile"), imp1.importFile(), false)
            }
        }
        {
            SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
            {
                // Scope of the transaction
                SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_UNKNOWN"), err)

                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxml/paket_2.xml"));
                SKGTESTERROR(QStringLiteral("ISO20022.importFile"), imp1.importFile(), true)
            }
            {
                SKGAccountObject account(&document1);
                SKGTESTERROR(QStringLiteral("ISO20022.setName"), account.setName(QStringLiteral("SI56123456789012345")), true)
                SKGTESTERROR(QStringLiteral("ISO20022.load"), account.load(), true)
                SKGTEST(QStringLiteral("ISO20022:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1999.01"))
            }
        }

        {
            SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
            {
                // Scope of the transaction
                SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_1"), err)

                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxml/472392_1.xml"));
                SKGTESTERROR(QStringLiteral("ISO20022.importFile"), imp1.importFile(), true)

                // 472390
                SKGAccountObject account(&document1);
                SKGTESTERROR(QStringLiteral("ACT:setName"), account.setName(QStringLiteral("SI5___3")), true)
                SKGTESTBOOL("ACT:exist", account.exist(), false)
            }

            {
                // Scope of the transaction
                SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_2"), err)

                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxml/472392_2.xml"));
                SKGTESTERROR(QStringLiteral("ISO20022.importFile"), imp1.importFile(), true)
            }
        }
    }

    // End test
    SKGENDTEST()
}
