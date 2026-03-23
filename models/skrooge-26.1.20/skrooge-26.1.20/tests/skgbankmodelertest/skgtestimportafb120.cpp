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
        // Test import AFB120
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            SKGTESTERROR(QStringLiteral("DOC:changePassword"), document1.changePassword(QStringLiteral("test")), true)

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_AFB120"), err)

            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.cfo")));
            SKGTESTERROR(QStringLiteral("AFB120.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportafb120/gs_01139_021239S.cfo"));
            SKGTESTERROR(QStringLiteral("AFB120.importFile"), imp1.importFile(), true)

            document1.dump(DUMPOPERATION | DUMPACCOUNT);
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("AFB120.setName"), account.setName(QStringLiteral("0000021239S")), true)
            SKGTESTERROR(QStringLiteral("AFB120.load"), account.load(), true)
            SKGTEST(QStringLiteral("AFB120:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("2555.48"))
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_AFB120"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportafb120/gs_01139_021239S.cfo"));
            SKGTESTERROR(QStringLiteral("AFB120.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("AFB120.setName"), account.setName(QStringLiteral("0000021239S")), true)
            SKGTESTERROR(QStringLiteral("AFB120.load"), account.load(), true)
            SKGTEST(QStringLiteral("AFB120:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("2555.48"))
        }

        QString fano;
        {
            QString f = SKGTest::getTestPath(QStringLiteral("OUT")) % QStringLiteral("/skgtestimportafb120/anonymize.skg");
            QFile(f).remove();
            SKGImportExportManager imp1(&document1);
            SKGTESTERROR(QStringLiteral("AFB120:anonymize"), imp1.anonymize(QString()), false)
            SKGTESTERROR(QStringLiteral("AFB120.saveAs"), document1.saveAs(f), true)
            SKGTESTERROR(QStringLiteral("AFB120:anonymize"), imp1.anonymize(QString()), true)
            fano = document1.getCurrentFileName();
            SKGTEST(QStringLiteral("AFB120:getCurrentFileName"), static_cast<unsigned int>(fano != f), static_cast<unsigned int>(true))

            document1.close();
        }

        {
            // Load anonymized file
            SKGDocumentBank document2;
            SKGTESTERROR(QStringLiteral("DOC:load"), document2.load(fano), true)

            SKGImportExportManager imp1(&document2);
            SKGTESTERROR(QStringLiteral("AFB120:anonymize"), imp1.anonymize(QStringLiteral("KEY")), true)
        }
    }
    // End test
    SKGENDTEST()
}
