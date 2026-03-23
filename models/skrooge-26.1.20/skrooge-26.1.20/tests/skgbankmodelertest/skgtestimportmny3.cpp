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

struct test {
    QString fileName;
    QString password;
    QMap<QString, double> expectedAccountAmount;
};

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
        // Test import MNY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny1/A B/money2002.mny"));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)

            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MNY.setName"), account.setName(QStringLiteral("None Investment")), true)
            SKGTESTERROR(QStringLiteral("MNY.load"), account.load(), true)
            SKGTEST(QStringLiteral("MNY.getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1.49"))
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny1/A B/money2002.mny"));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)

            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MNY.setName"), account.setName(QStringLiteral("None Investment")), true)
            SKGTESTERROR(QStringLiteral("MNY.load"), account.load(), true)
            SKGTEST(QStringLiteral("MNY.getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1.49"))
        }
    }

    {
        // Test import MNY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny1/notreadable.mny"));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTEST(QStringLiteral("imp1.importFile"), imp1.importFile().getReturnCode(), ERR_READACCESS)
        }
    }

    {
        // Test import MNY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny3/transfer_and_mode.mny"));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        bool test2 = false;
        SKGTESTERROR(QStringLiteral("MNY.existObjects"),
                     document1.existObjects(QStringLiteral("operation"), QStringLiteral("t_comment='Mode should be ''text''' AND t_mode='text'"), test2),
                     true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))

        SKGTESTERROR(QStringLiteral("MNY.existObjects"),
                     document1.existObjects(QStringLiteral("operation"), QStringLiteral("t_comment='Number should be 123456' AND t_number='123456'"), test2),
                     true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))

        SKGTESTERROR(QStringLiteral("MNY.existObjects"),
                     document1.existObjects(QStringLiteral("operation"), QStringLiteral("t_comment='True transfert' AND i_group_id<>0"), test2),
                     true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))

        SKGTESTERROR(
            QStringLiteral("MNY.existObjects"),
            document1.existObjects(QStringLiteral("operation"), QStringLiteral("t_comment='Not a transfert, just same date/amount' AND i_group_id=0"), test2),
            true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))
    }

    {
        // Test import 507235
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny3/507235.mny"));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        bool test2 = false;
        SKGTESTERROR(
            QStringLiteral("MNY.existObjects"),
            document1.existObjects(QStringLiteral("v_unit_display"), QStringLiteral("t_name='PartSociale' AND f_QUANTITYOWNED=5 AND f_LASTVALUE=20"), test2),
            true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))

        SKGTESTERROR(QStringLiteral("MNY.existObjects"), document1.existObjects(QStringLiteral("unit"), QStringLiteral("t_symbol='SX5E'"), test2), true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))
    }

    {
        // Test import 510022 & 510027
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny3/510022.mny"));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        // 510022
        bool test2 = false;
        SKGTESTERROR(QStringLiteral("MNY.existObjects"), document1.existObjects(QStringLiteral("unitvalue"), QStringLiteral("f_quantity=0"), test2), true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(false))

        // 510027
        SKGTESTERROR(QStringLiteral("MNY.existObjects"), document1.existObjects(QStringLiteral("operation"), QStringLiteral("i_group_id!=0"), test2), true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))
    }

    {
        // Test import 510025
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny3/510025.mny"));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        bool test2 = false;
        SKGTESTERROR(QStringLiteral("MNY.existObjects"),
                     document1.existObjects(QStringLiteral("suboperation"), QStringLiteral("t_comment='ChildMemo1'"), test2),
                     true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))
    }

    {
        // Test import 510115
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny3/510115.mny"));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        bool test2 = false;
        SKGTESTERROR(QStringLiteral("MNY.existObjects"),
                     document1.existObjects(QStringLiteral("category"), QStringLiteral("t_name='Russia 2023'"), test2),
                     true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(false))

        SKGTESTERROR(QStringLiteral("MNY.existObjects"), document1.existObjects(QStringLiteral("refund"), QStringLiteral("t_name='Russia 2023'"), test2), true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))

        SKGStringListList result;
        SKGTESTERROR(QStringLiteral("MNY.executeSelectSqliteOrder"),
                     document1.executeSelectSqliteOrder(QStringLiteral("SELECT count(1) FROM refund"), result),
                     true)
        SKGTEST(QStringLiteral("MNY:tracker"), result.at(1).at(0), "8")

        SKGTESTERROR(QStringLiteral("MNY.existObjects"),
                     document1.existObjects(QStringLiteral("refund"), QStringLiteral("t_name='UserDefined-Sub-1 > Sub-sub-1'"), test2),
                     true)
        SKGTEST(QStringLiteral("MNY.existObjects-test"), static_cast<unsigned int>(test2), static_cast<unsigned int>(true))
    }
    // End test

    SKGENDTEST()
}
