/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for PDF import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginpdf.h"

#include <kpluginfactory.h>

#include <qdiriterator.h>
#include <qfileinfo.h>
#include <qprocess.h>
#include <qregularexpression.h>
#include <qstandardpaths.h>
#include <qtemporaryfile.h>

#include "skgbankincludes.h"
#include "skgdefine.h"
#include "skgerror.h"
#include "skgimportexportmanager.h"
#include "skgservices.h"
#include "skgtraces.h"
#include <utility>

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginPDF, "metadata.json")

SKGImportPluginPDF::SKGImportPluginPDF(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginPDF::~SKGImportPluginPDF() = default;

bool SKGImportPluginPDF::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("PDF"));
}

QString SKGImportPluginPDF::extract(const QStringList &iLine, const QString &iSyntax)
{
    QString output;
    int currentIndex = -1;
    bool setpossible = true;

    // Interpret the syntax
    auto items = SKGServices::splitCSVLine(iSyntax, '|', false);
    for (const auto &item : std::as_const(items)) {
        if (item.startsWith(QStringLiteral("REGEXPCAP:"))) {
            QRegularExpression regexp(item.right(item.length() - 10));
            if (output.isEmpty()) {
                for (const auto &line : iLine) {
                    auto martch = regexp.match(line);
                    if (martch.hasMatch()) {
                        output = martch.captured(1);
                        break;
                    }
                }
            } else {
                auto martch = regexp.match(output);
                if (martch.hasMatch()) {
                    output = martch.captured(1);
                }
            }
        } else if (item.startsWith(QStringLiteral("REGEXP:"))) {
            setpossible = false;
            QRegularExpression regexp(item.right(item.length() - 7));
            int nb = iLine.count();
            for (int i = 0; i < nb; ++i) {
                auto martch = regexp.match(iLine.at(i));
                if (martch.hasMatch()) {
                    currentIndex = i;
                    setpossible = true;
                    break;
                }
            }
        } else if (item.startsWith(QStringLiteral("LINEOFFSET:"))) {
            currentIndex += SKGServices::stringToInt(item.right(item.length() - 11));
            if (currentIndex >= 0 && currentIndex < iLine.count()) {
                output = iLine.at(currentIndex);
            }
        } else if (item.startsWith(QStringLiteral("SET:")) && setpossible) {
            QString s = item.right(item.length() - 4);
            if (s.contains(QStringLiteral("%1"))) {
                output = s.arg(output);
            } else {
                output = s;
            }
        }
    }
    return output;
}

SKGError SKGImportPluginPDF::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Begin transaction
    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "PDF"), 2);
    IFOK(err)
    {
        // Open file
        IFOK(err)
        {
            // Extract text from PDF
            QString file = m_importer->getLocalFileName();
            QTemporaryFile txtFile;
            if (txtFile.open()) {
                QStringList args = QStringList() << file << txtFile.fileName();

                QProcess p;
                p.start(QStringLiteral("pdftotext"), args);
                if (!p.waitForFinished(1000 * 60 * 2) || p.exitCode() != 0) {
                    QString cmd = "pdftotext " % args.join(QStringLiteral(" "));
                    err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "The following command line failed with code %2:\n'%1'", cmd, p.exitCode()));

                } else {
                    // Step 1 done
                    IFOKDO(err, m_importer->getDocument()->stepForward(1))

                    // Read the text file
                    QStringList lines;
                    QTextStream stream(&txtFile);
                    while (!stream.atEnd()) {
                        // Read line
                        lines.push_back(stream.readLine());
                    }

                    // Search extractors
                    QStringList listOfExtractors;
                    bool found = false;
                    QString a = QStringLiteral("skrooge/extractors");
                    const auto dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, a, QStandardPaths::LocateDirectory);
                    for (const auto &dir : dirs) {
                        QDirIterator it(dir, QStringList() << QStringLiteral("*.extractor"));
                        while (it.hasNext() && !found) {
                            // Read extractor
                            QString fileName = it.next();
                            QString extractor = QFileInfo(fileName).baseName().toUpper();
                            listOfExtractors.push_back(extractor);
                            QHash<QString, QString> properties;
                            err = SKGServices::readPropertyFile(fileName, properties);
                            IFOK(err)
                            {
                                // Check if this extractor is done for this file
                                QString payee = extract(lines, properties[QStringLiteral("payee")]);
                                if (!payee.isEmpty()) {
                                    // Search the date
                                    QString date = extract(lines, properties[QStringLiteral("date")]);
                                    QString dateFormat = properties[QStringLiteral("dateformat")];
                                    auto d = QDate::fromString(date, dateFormat);
                                    if (!d.isValid()) {
                                        d = QDate::fromString(date);
                                        if (!d.isValid()) {
                                            SKGTRACE << "WARNING: Impossible to parse the date [" << date << "] with [" << dateFormat << "]" << Qt::endl;
                                        }
                                    }
                                    if (!dateFormat.contains(QStringLiteral("yyyy")) && d.year() < 2000) {
                                        d = d.addYears(100);
                                    }

                                    // Search the amount
                                    double amount = SKGServices::stringToDouble(extract(lines, properties[QStringLiteral("amount")]));

                                    // Search the comment
                                    QString comment = extract(lines, properties[QStringLiteral("comment")]);

                                    // Search the number
                                    QString number = extract(lines, properties[QStringLiteral("number")]);

                                    // Search the mode
                                    QString mode = extract(lines, properties[QStringLiteral("mode")]);

                                    // Get account
                                    SKGAccountObject account;
                                    SKGOperationObject act;
                                    m_importer->getDocument()->getObject(QStringLiteral("v_account_display"),
                                                                         QStringLiteral("t_close='N' AND t_type='C' ORDER BY i_NBOPERATIONS DESC LIMIT 1"),
                                                                         act);
                                    if (act.exist()) {
                                        account = act;
                                        IFOKDO(err,
                                               m_importer->getDocument()->sendMessage(
                                                   i18nc("An information message", "Using account '%1' for import", account.getName())))
                                    } else {
                                        IFOKDO(err, m_importer->getDefaultAccount(account))
                                    }

                                    if (d.isValid() && !qFuzzyCompare(1 + amount, 1.0)) {
                                        // Get unit
                                        SKGUnitObject unit;
                                        IFOKDO(err, m_importer->getDefaultUnit(unit))

                                        // Create operation
                                        SKGOperationObject operation;
                                        IFOKDO(err, account.addOperation(operation, true))
                                        IFOKDO(err, operation.setDate(d))

                                        IFOKDO(err, operation.setUnit(unit))
                                        SKGPayeeObject payeeObj;
                                        IFOKDO(err, SKGPayeeObject::createPayee(m_importer->getDocument(), payee, payeeObj))
                                        IFOKDO(err, operation.setPayee(payeeObj))
                                        IFOKDO(err, operation.setComment(comment))
                                        IFOKDO(err, operation.setMode(mode))
                                        IFOKDO(err, operation.setImportID(QStringLiteral("PDF-") % extractor % QStringLiteral("-") % number))
                                        // This is normal. PDF import is for only one transaction, so no check if already imported
                                        IFOKDO(err, operation.setAttribute(QStringLiteral("t_imported"), QStringLiteral("Y")))
                                        IFOKDO(err, operation.save(false))

                                        SKGSubOperationObject subop;
                                        IFOKDO(err, operation.addSubOperation(subop))
                                        IFOKDO(err, subop.setComment(comment))
                                        IFOKDO(err, subop.setQuantity(-amount))
                                        IFOKDO(err, subop.save(false, false))

                                        // Add file
                                        IFOKDO(err, err = operation.setProperty(i18n("Invoice"), file, file))

                                        found = true;
                                    } else {
                                        SKGTRACE << "Invalid date = " << date << " or amount =  " << amount << '\n';
                                    }
                                }
                            }
                        }
                    }

                    if (!found) {
                        err = SKGError(ERR_FAIL,
                                       i18nc("An information message",
                                             "Invoice %1 has not been imported because it is not recognized (List of recognized extractors: %2).",
                                             file,
                                             listOfExtractors.join(u',')));
                    }

                    // Step 2 done
                    IFOKDO(err, m_importer->getDocument()->stepForward(2))
                }
            }
        }
    }
    SKGENDTRANSACTION(m_importer->getDocument(), err)

    return err;
}

QString SKGImportPluginPDF::getMimeTypeFilter() const
{
    return "*.pdf|" % i18nc("A file format", "PDF file (invoice)");
}

#include <skgimportpluginpdf.moc>
