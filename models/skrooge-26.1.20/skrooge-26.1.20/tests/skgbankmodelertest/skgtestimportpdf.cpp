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
#include <utility>

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
        // Test import PDF skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_PDF"), err)
            QString dir = SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportpdf/";
            auto listFiles = QDir(dir).entryList(QStringList() << QStringLiteral("*.pdf"), QDir::Files, QDir::Name);
            for (const auto &file : std::as_const(listFiles)) {
                SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(dir % file));
                SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            }

            document1.dump(DUMPOPERATION | DUMPACCOUNT);
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("Facture allopneus"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-2041.24"))

        int nb = 0;
        SKGTESTERROR(QStringLiteral("DOC:getNbObjects"), document1.getNbObjects(QStringLiteral("operation"), QString(), nb), true)
        SKGTEST(QStringLiteral("DOC:getNbObjects"), nb, 11)

        SKGTESTERROR(
            QStringLiteral("DOC:getNbObjects"),
            document1.getNbObjects(QStringLiteral("operation"), QStringLiteral("d_date='") + SKGServices::dateToSqlString(QDate::currentDate()) + '\'', nb),
            true)
        SKGTEST(QStringLiteral("DOC:getNbObjects"), nb, 0)
    }
    // End test
    SKGENDTEST()
}
