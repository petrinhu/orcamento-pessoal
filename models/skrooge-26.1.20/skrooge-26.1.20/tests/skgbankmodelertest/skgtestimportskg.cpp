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
        // Test import SKG
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_SKG"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.skg")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportskg/all_types.skg"));
            SKGTESTERROR(QStringLiteral("SKG.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("SKG.setName"), account.setName(QStringLiteral("FRANC")), true)
            SKGTESTERROR(QStringLiteral("SKG.load"), account.load(), true)
            SKGTEST(QStringLiteral("SKG:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1.524490172"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("SKG.setName"), account.setName(QStringLiteral("COURANT")), true)
            SKGTESTERROR(QStringLiteral("SKG.load"), account.load(), true)
            SKGTEST(QStringLiteral("SKG:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-14055"))

            auto properties = account.getProperties();
            SKGTEST(QStringLiteral("SKG:getProperties"), SKGServices::intToString(properties.length()), QStringLiteral("1"))

            SKGPropertyObject p(account.getPropertyObject(properties.at(0)));
            SKGTEST(QStringLiteral("SKG:getPropertyBlob FALSE"),
                    static_cast<unsigned int>(account.getPropertyBlob(properties.at(0)).isNull()
                                              || account.getPropertyBlob(properties.at(0)).toByteArray().length() == 0),
                    static_cast<unsigned int>(true))
            SKGTEST(QStringLiteral("SKG:getPropertyObject.value"), p.getValue(), "PROP_CPT")
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("SKG.setName"), account.setName(QStringLiteral("EPARGNE")), true)
            SKGTESTERROR(QStringLiteral("SKG.load"), account.load(), true)
            SKGTEST(QStringLiteral("SKG:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("5622.224686"))
        }

        // test multi import
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_SKG"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportskg/all_types.skg"));
            SKGTESTERROR(QStringLiteral("SKG.importFile"), imp1.importFile(), true)
        }

        // Export xml
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_XML"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types.xml"));
            SKGTESTERROR(QStringLiteral("SKG.exportFile"), imp1.exportFile(), true)
        }

        // Export json
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_JSON"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types.json"));
            SKGTESTERROR(QStringLiteral("SKG.exportFile"), imp1.exportFile(), true)
        }

        // Export json
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_JSON"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types.json"));
            SKGTESTERROR(QStringLiteral("SKG.importFile"), imp1.importFile(), false)
        }
    }
    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        // Export skg from memory
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_SKG"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_exp_from_mem.skg"));
            SKGTESTERROR(QStringLiteral("SKG.exportFile from memory"), imp1.exportFile(), true)
            SKGTESTERROR(QStringLiteral("SKG.importFile from file"), imp1.importFile(), true)
        }
        SKGTESTERROR(QStringLiteral("SKG.saveAs"),
                     document1.saveAs(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_save.skg", true),
                     true)

        // Export skg from file
        SKGTESTERROR(QStringLiteral("SKG.load"), document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_save.skg"), true)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_SKG"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_exp_from_file.skg"));
            SKGTESTERROR(QStringLiteral("SKG.exportFile from file"), imp1.exportFile(), true)
            SKGTESTERROR(QStringLiteral("SKG.importFile from file"), imp1.importFile(), true)
        }
    }
    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        // Export sqlite from memory
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_SQLITE"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_exp_from_mem.sqlite"));
            SKGTESTERROR(QStringLiteral("SQLITE.exportFile from memory"), imp1.exportFile(), true)
            SKGTESTERROR(QStringLiteral("SQLITE.importFile from file"), imp1.importFile(), true)
        }

        // Export skg from file
        SKGTESTERROR(QStringLiteral("SKG.load"), document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_save.skg"), true)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_SQLITE"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_exp_from_file.sqlite"));
            SKGTESTERROR(QStringLiteral("SQLITE.exportFile from file"), imp1.exportFile(), true)
            SKGTESTERROR(QStringLiteral("SQLITE.importFile from file"), imp1.importFile(), true)
        }
    }
    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        // Export sqlcipher from memory
        {
            // Scope of the transaction
            SKGTESTERROR(QStringLiteral("SQLCIPHER.changePassword"), document1.changePassword(QStringLiteral("password")), true)

            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_SQLCIPHER"), err)
            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_exp_from_mem.sqlcipher"));
            SKGTESTERROR(QStringLiteral("SQLCIPHER.exportFile from memory"), imp1.exportFile(), true)
            QMap<QString, QString> parameters;
            parameters[QStringLiteral("password")] = QStringLiteral("password");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("SQLCIPHER.importFile from file"), imp1.importFile(), true)
        }

        // Export skg from file
        SKGTESTERROR(QStringLiteral("SKG.load"), document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_save.skg"), true)
        {
            // Scope of the transaction
            SKGTESTERROR(QStringLiteral("SQLCIPHER.changePassword"), document1.changePassword(QStringLiteral("password")), true)

            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_SQLCIPHER"), err)
            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportskg/all_types_exp_from_file.sqlcipher"));
            SKGTESTERROR(QStringLiteral("SQLCIPHER.exportFile from file"), imp1.exportFile(), true)
            QMap<QString, QString> parameters;
            parameters[QStringLiteral("password")] = QStringLiteral("password");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("SQLCIPHER.importFile from file"), imp1.importFile(), true)
        }
    }
    {
        // Test import encrypted SKG
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_SKG"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportskg/encrypted.skg"));
            QMap<QString, QString> parameters;
            parameters[QStringLiteral("password")] = QStringLiteral("password");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("SKG.importFile"), imp1.importFile(), false) // Since 2.33.0, no more supported
        }
    }

    {
        // Test import SKG
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_SKG"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportskg/euro_bitcoin_dollar.skg"));
            SKGTESTERROR(QStringLiteral("SKG.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import SKG
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_SKG"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/advice.skg"));
            SKGTESTERROR(QStringLiteral("SKG.importFile"), imp1.importFile(), true)
        }
    }
    // End test
    SKGENDTEST()
}
