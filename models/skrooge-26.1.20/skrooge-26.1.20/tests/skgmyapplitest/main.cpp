/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines the main of SKGMyAppliTest.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdocument.h"
#include "skgtraces.h"
#include "skguniqueapplication.h"

#include <kaboutdata.h>

#include <qcommandlineoption.h>
#include <qcommandlineparser.h>

/**
 * The main of the application
 * @param argc number of arguments
 * @param argv arguments
 * @return return code
 */
int main(int argc, char **argv)
{
    KAboutData about("skgmyapplitest",
                     0,
                     ki18nc("The name of the application", "SKGMyAppliTest"),
                     "0.1.0",
                     ki18nc("The description of the application", "Blablabla"),
                     KAboutLicense::GPL_V3,
                     i18nc("Fullname", "(c) 2007-%1 Stephane MANKOWSKI & Guillaume DE BURE", QDate::currentDate().toString(QStringLiteral("yyyy"))),
                     "",
                     "https://skrooge.org");

    about.addAuthor(ki18nc("Fullname", "Stephane MANKOWSKI"), ki18nc("A job description", "Architect & Developer"), "stephane@mankowski.fr");
    about.setOtherText(ki18nc("The description of the application", "An application test."));

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    app.setApplicationName(aboutData.componentName());
    app.setApplicationDisplayName(aboutData.displayName());
    app.setOrganizationDomain(aboutData.organizationDomain());
    app.setApplicationVersion(aboutData.version());
    parser.addVersionOption();
    parser.addHelpOption();
    // PORTING SCRIPT: adapt aboutdata variable if necessary
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("+[URL]"), i18nc("Application argument", "Document to open")));

    int rc = 0;
    if (!SKGUniqueApplication::start()) {
        fprintf(stderr, "SKGMyAppliTest is already running!\n");
    } else {
        // Creating a main panel on a generic document
        SKGDocument doc;
        SKGUniqueApplication kApp(&doc);

        rc = kApp.exec(); // krazy:exclude=crashy
    }
    SKGTraces::dumpProfilingStatistics();
    return rc;
}
