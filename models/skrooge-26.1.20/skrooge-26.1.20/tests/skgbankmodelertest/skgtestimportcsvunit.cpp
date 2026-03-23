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
    SKGError err;
    {
        // Test import SKGImportExportManager::CSV skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            {
                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsvunit/notfound.csv"));
                QMap<QString, QString> parameters = imp1.getImportParameters();
                parameters[QStringLiteral("mode_csv_unit")] = 'Y';
                imp1.setImportParameters(parameters);
                SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), false) // FILE NOT FOUND
            }
            {
                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsvunit/test.csv"));
                QMap<QString, QString> parameters = imp1.getImportParameters();
                parameters[QStringLiteral("mode_csv_unit")] = 'Y';
                imp1.setImportParameters(parameters);
                SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            }
            {
                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsvunit/test2.csv"));
                QMap<QString, QString> parameters = imp1.getImportParameters();
                parameters[QStringLiteral("mode_csv_unit")] = 'Y';
                imp1.setImportParameters(parameters);
                SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            }
        }
        document1.dump(DUMPUNIT);
        {
            SKGUnitObject unit(&document1);
            SKGTESTERROR(QStringLiteral("UNIT.setName"), unit.setName(QStringLiteral("test")), true)
            SKGTESTERROR(QStringLiteral("UNIT.load"), unit.load(), true)
            SKGUnitValueObject UnitValue;
            SKGTESTERROR(QStringLiteral("UNIT.getLastUnitValue"), unit.getLastUnitValue(UnitValue), true)
            SKGTEST(QStringLiteral("UNIT:getQuantity"), SKGServices::doubleToString(UnitValue.getQuantity()), QStringLiteral("50"))
        }
        {
            SKGUnitObject unit(&document1);
            SKGTESTERROR(QStringLiteral("UNIT.setName"), unit.setName(QStringLiteral("test2")), true)
            SKGTESTERROR(QStringLiteral("UNIT.load"), unit.load(), true)
            SKGUnitValueObject UnitValue;
            SKGTESTERROR(QStringLiteral("UNIT.getLastUnitValue"), unit.getLastUnitValue(UnitValue), true)
            SKGTEST(QStringLiteral("UNIT:getQuantity"), SKGServices::doubleToString(UnitValue.getQuantity()), QStringLiteral("25.16"))
        }
    }

    {
        // Test import SKGImportExportManager::CSV skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            {
                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsvunit/coma.csv"));
                QMap<QString, QString> parameters = imp1.getImportParameters();
                parameters[QStringLiteral("mode_csv_unit")] = 'Y';
                imp1.setImportParameters(parameters);
                SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            }
        }
    }
    // End test
    SKGENDTEST()
}
