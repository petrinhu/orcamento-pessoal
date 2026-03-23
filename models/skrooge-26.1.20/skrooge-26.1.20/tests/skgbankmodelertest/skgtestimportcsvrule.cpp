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
 * The main function of the import rule test
 * @param argc the number of arguments
 * @param argv the list of arguments
 */
int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Init test
    SKGINITTEST(true)
    SKGError err;
    {
        // Test import SKGImportExportManager::CSV skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            {
                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsvrule/notfound.csv"));
                QMap<QString, QString> parameters = imp1.getImportParameters();
                parameters[QStringLiteral("mode_csv_rule")] = 'Y';
                imp1.setImportParameters(parameters);
                SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), false) // FILE NOT FOUND
            }
            {
                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsvrule/test1.csv"));
                QMap<QString, QString> parameters = imp1.getImportParameters();
                parameters[QStringLiteral("mode_csv_rule")] = 'Y';
                imp1.setImportParameters(parameters);
                SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            }
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("imp1.getNbObjects"), document1.getNbObjects(QStringLiteral("rule"), QString(), nb), true)
        SKGTEST(QStringLiteral("imp1.nb"), nb, 3)
    }

    // End test
    SKGENDTEST()
}
