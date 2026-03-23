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

    QStringList extensions;
    extensions << QStringLiteral("csv") << QStringLiteral("cfo") << QStringLiteral("gnucash") << QStringLiteral("gsb") << QStringLiteral("kmy")
               << QStringLiteral("mmb") << QStringLiteral("mt940") << QStringLiteral("ofx") << QStringLiteral("qif") << QStringLiteral("skg")
               << QStringLiteral("xhb");

    int nb = extensions.count();
    for (int i = 0; i < nb; ++i) {
        const QString &ext = extensions.at(i);
        QString filename = "https://skrooge.org/skgtestimporturl/test." % ext;
        SKGTRACE << i + 1 << "/" << nb << ": Import " << filename << Qt::endl;

        // Test import
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromUserInput(filename));
            SKGTESTERROR(ext % ".importFile", imp1.importFile(), true)
        }
    }

    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(QStringLiteral("https://skrooge.org/skgtestimporturl/test.skg")), true)
    }

    // End test
    SKGENDTEST()
}
