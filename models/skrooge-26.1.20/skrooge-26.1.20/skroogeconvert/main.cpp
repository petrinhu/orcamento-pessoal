/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines the main of skroogeconvert.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdocumentbank.h"
#include "skgimportexportmanager.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

#include <kaboutdata.h>

#include <qapplication.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qcoreapplication.h>
#include <qfileinfo.h>
#include <qurl.h>

/**
 * To compute the version
 */
#define VER1_(x) #x
/**
 * To compute the version
 */
#define VER_(x) VER1_(x)
/**
 * To compute the version
 */
#define VER VER_(SKGVERSION)

/**
 * The main of the application
 * @param argc number of arguments
 * @param argv arguments
 * @return return code
 */
int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("skrooge");

    KAboutData about(QStringLiteral("skroogeconvert"),
                     i18nc("The name of the application", "Skrooge Convert"),
                     QStringLiteral(VER),
                     i18nc("The description of the application", "A conversion tool for financial files (KMyMoney, GnuCash, Skrooge, …)"),
                     KAboutLicense::GPL_V3,
                     i18nc("Fullname", "(c) 2007-%1 Stephane MANKOWSKI & Guillaume DE BURE", QDate::currentDate().toString(QStringLiteral("yyyy"))),
                     QString(),
                     QStringLiteral("https://skrooge.org"));

    about.addAuthor(i18nc("Fullname", "Stephane MANKOWSKI"),
                    i18nc("A job description", "Architect & Developer"),
                    QStringLiteral("stephane@mankowski.fr"),
                    QString(),
                    QStringLiteral("miraks"));
    about.addAuthor(i18nc("Fullname", "Guillaume DE BURE"),
                    i18nc("A job description", "Developer"),
                    QStringLiteral("guillaume.debure@gmail.com"),
                    QString(),
                    QStringLiteral("willy9"));
    about.setOtherText(i18nc("The description of the application",
                             "The application name is inspired by Charles Dicken's tale <i>A Christmas Carol</i>, where the main character, Ebenezer Scrooge, "
                             "a grumpy old narrow man, gets visited by three ghosts who change the way he sees the world, in a good way."));
    about.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    parser.addOption(
        QCommandLineOption(QStringList() << QStringLiteral("in"),
                           i18nc("Application argument", "Input file. Supported formats:\n%1", SKGImportExportManager().getImportMimeTypeFilter(false)),
                           QStringLiteral("file")));
    parser.addOption(
        QCommandLineOption(QStringList() << QStringLiteral("out"),
                           i18nc("Application argument", "Output file. Supported formats:\n%1", SKGImportExportManager().getExportMimeTypeFilter(false)),
                           QStringLiteral("file")));
    parser.addOption(
        QCommandLineOption(QStringList() << QStringLiteral("param"), i18nc("Application argument", "Name of a parameter"), QStringLiteral("name")));
    parser.addOption(
        QCommandLineOption(QStringList() << QStringLiteral("value"), i18nc("Application argument", "Value of a parameter"), QStringLiteral("value")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("param_export"),
                                        i18nc("Application argument", "Name of a parameter for export"),
                                        QStringLiteral("name_export")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("value_export"),
                                        i18nc("Application argument", "Value of a parameter for export"),
                                        QStringLiteral("value_export")));

    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    // Build list of arguments
    SKGError err;
    if (!parser.isSet(QStringLiteral("in"))) {
        err = SKGError(ERR_INVALIDARG, i18nc("Error message", "Missing -in option"));
    } else if (!parser.isSet(QStringLiteral("out"))) {
        err = SKGError(ERR_INVALIDARG, i18nc("Error message", "Missing -out option"));
    }

    // Initialisation of a bank document
    SKGDocumentBank document;
    document.setComputeBalances(false);
    IFOKDO(err, document.initialize())
    IFOK(err)
    {
        // Import
        QString file = parser.value(QStringLiteral("in"));
        QFileInfo fi(file);
        if (fi.exists()) {
            file = fi.absoluteFilePath();
        }
        SKGImportExportManager imp(&document, QUrl::fromLocalFile(file));
        QMap<QString, QString> parameters = imp.getImportParameters();
        QStringList params = parser.values(QStringLiteral("param"));
        QStringList values = parser.values(QStringLiteral("value"));
        int nb = qMin(params.count(), values.count());
        for (int i = 0; i < nb; ++i) {
            parameters[params.at(i)] = values.at(i);
        }
        imp.setImportParameters(parameters);

        SKGTRACESEPARATOR;
        SKGTRACE << i18nc("Title of a console trace section", " Import parameters") << Qt::endl;
        QMapIterator<QString, QString> i(imp.getImportParameters());
        while (i.hasNext()) {
            i.next();
            SKGTRACE << " " << i.key() << "=" << i.value() << Qt::endl;
        }
        SKGTRACESEPARATOR;
        SKGTRACE << i18nc("Title of a console trace section", " Imported file:") << imp.getFileName().toDisplayString() << Qt::endl;
        if (fi.suffix().toUpper() == QStringLiteral("SKG") || fi.suffix().toUpper() == QStringLiteral("SQLITE")
            || fi.suffix().toUpper() == QStringLiteral("SQLCIPHER")) {
            err = document.load(file, parameters[QStringLiteral("password")]);
        } else {
            SKGBEGINTRANSACTION(document, QStringLiteral("IMPORT"), err)
            IFOKDO(err, imp.importFile())
        }
    }
    IFOK(err)
    {
        // Export
        QString file = parser.value(QStringLiteral("out"));
        QFileInfo fi(file);
        if (fi.exists()) {
            file = fi.absoluteFilePath();
        }
        SKGImportExportManager exp(&document, QUrl::fromLocalFile(file));
        QMap<QString, QString> parameters = exp.getExportParameters();
        QStringList params = parser.values(QStringLiteral("param_export"));
        QStringList values = parser.values(QStringLiteral("value_export"));
        int nb = qMin(params.count(), values.count());
        for (int i = 0; i < nb; ++i) {
            parameters[params.at(i)] = values.at(i);
        }
        exp.setExportParameters(parameters);

        SKGTRACESEPARATOR;
        SKGTRACE << i18nc("Title of a console trace section", " Export parameters") << Qt::endl;
        QMapIterator<QString, QString> i(exp.getExportParameters());
        while (i.hasNext()) {
            i.next();
            SKGTRACE << " " << i.key() << "=" << i.value() << Qt::endl;
        }
        SKGTRACESEPARATOR;
        SKGTRACE << i18nc("Title of a console trace section", " Exported file:") << exp.getFileName().toDisplayString() << Qt::endl;
        if (fi.suffix().toUpper() == QStringLiteral("SKG")) {
            err = document.saveAs(file, true);
        } else {
            err = exp.exportFile();
        }
    }

    // Dump getMessages
    SKGDocument::SKGMessageList oMessages;
    IFOKDO(err, document.getMessages(document.getCurrentTransaction(), oMessages))
    int nb = oMessages.count();
    if (nb > 0) {
        SKGTRACESEPARATOR;
        for (int i = 0; i < nb; ++i) {
            SKGTRACE << oMessages.at(i).Text << Qt::endl;
        }
    }

    IFKO(err)
    {
        SKGTRACESUITE << err.getFullMessageWithHistorical() << Qt::endl;
        SKGTRACESEPARATOR;
        SKGTRACE << i18nc("Title of a console trace section", " FAILED") << Qt::endl;
        SKGTRACESEPARATOR;
    }
    else
    {
        SKGTRACESEPARATOR;
        SKGTRACE << i18nc("Title of a console trace section", " SUCCESSFUL") << Qt::endl;
        SKGTRACESEPARATOR;
    }

    SKGTraces::dumpProfilingStatistics();
    return err.getReturnCode();
}
