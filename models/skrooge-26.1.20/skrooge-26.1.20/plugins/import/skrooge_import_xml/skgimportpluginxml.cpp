/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for XML import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginxml.h"

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include <qdir.h>
#include <qfile.h>
#include <qprocess.h>
#include <qsavefile.h>
#include <qstandardpaths.h>
#include <quuid.h>

#include "skgbankincludes.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginXml, "metadata.json")

SKGImportPluginXml::SKGImportPluginXml(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginXml::~SKGImportPluginXml() = default;

bool SKGImportPluginXml::isImportPossible()
{
    SKGTRACEINFUNC(10)
    if (m_importer->getDocument() == nullptr) {
        return true;
    }
    if (m_importer->getFileNameExtension() != QStringLiteral("XML")) {
        return false;
    }
    return true;
}

SKGError SKGImportPluginXml::getXMLDocument(const QString &iFileName, QDomElement &oDocument)
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Open file
    QFile file(iFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", iFileName));
    } else {
        QDomDocument doc;

        // Set the file without uncompression
        QString errorMsg;
        int errorLine = 0;
        int errorCol = 0;
        bool contentOK = false;
        const auto &content = QString::fromUtf8(file.readAll());
        const auto &result = doc.setContent(content);

        contentOK = !!result;
        errorLine = result.errorLine;
        errorCol = result.errorColumn;
        errorMsg = result.errorMessage;
        file.close();

        if (!contentOK) {
            err.setReturnCode(ERR_ABORT)
                .setMessage(i18nc("Error message", "%1-%2: '%3'", errorLine, errorCol, errorMsg))
                .addError(ERR_INVALIDARG, i18nc("Error message", "Invalid XML content in file '%1'", iFileName));
        } else {
            // Get root
            oDocument = doc.documentElement();
        }
    }

    return err;
}

SKGError SKGImportPluginXml::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Determine the XML format
    QString format;
    QDomElement docElem;
    IFOKDO(err, getXMLDocument(m_importer->getLocalFileName(), docElem))
    IFOK(err)
    {
        if (docElem.tagName() == "skrooge") {
            // SKROOGE XML FORMAT
            format = QStringLiteral("XML");
        } else if (docElem.tagName() == "Document") {
            // Check install
            QString saxonb_bin;
            if (QStandardPaths::findExecutable(QStringLiteral("saxonb-xslt")).isEmpty()) {
                if (QStandardPaths::findExecutable(QStringLiteral("saxon-xslt")).isEmpty()) {
                    err.setReturnCode(ERR_ABORT).setMessage(
                        i18nc("Error message",
                              "The saxon-xslt application is not installed. You must manually install it. This could be by the package libsaxonb-java."));
                    return err;
                } else {
                    saxonb_bin = QStringLiteral("saxon-xslt");
                }
            } else {
                saxonb_bin = QStringLiteral("saxonb-xslt");
            }

            // ISO20022 XML FORMAT
            QString uniqueId = QUuid::createUuid().toString();
            QString temporaryPath = QDir::tempPath() % "/" % uniqueId % ".xml";
            QString cmd = "sed -E 's/<Document .*>/<Document>/g' \"" % m_importer->getLocalFileName() % "\" > \"" % temporaryPath % "\";saxonb-xslt -xsl:\""
                % QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/ISO20022.xslt")) % "\" -s:\"" % temporaryPath % "\" -o:\""
                % temporaryPath % "\"";
            SKGTRACEL(10) << "Execution of :" << cmd << Qt::endl;

            QProcess p;
            p.start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << cmd);
            if (p.waitForFinished(1000 * 60 * 5) && p.exitCode() == 0) {
                IFOKDO(err, getXMLDocument(temporaryPath, docElem))
                IFOK(err)
                {
                    // Get root
                    format = QStringLiteral("ISO20022");
                }
            } else {
                err.setReturnCode(ERR_FAIL)
                    .setMessage(i18nc("Error message", "The execution of '%1' failed", cmd))
                    .addError(ERR_FAIL, i18nc("Error message", "The conversion in xml of '%1' failed", m_importer->getFileName().toDisplayString()));
            }
        } else {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Unknown XML format. Only Skrooge and ISO20022 XML formats are supported"));
        }
    }

    // Build list of items to import
    QList<QString> types;
    types.append(QStringLiteral("unit"));
    types.append(QStringLiteral("unitvalue"));
    types.append(QStringLiteral("bank"));
    types.append(QStringLiteral("account"));
    types.append(QStringLiteral("payee"));
    types.append(QStringLiteral("refund"));
    types.append(QStringLiteral("rule"));
    types.append(QStringLiteral("category"));
    types.append(QStringLiteral("budget"));
    types.append(QStringLiteral("budgetrule"));
    types.append(QStringLiteral("operation"));
    types.append(QStringLiteral("suboperation"));
    types.append(QStringLiteral("recurrentoperation"));
    types.append(QStringLiteral("interest"));

    auto nb = types.length();
    IFOKDO(err, m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", format), nb))
    for (int i = 0; !err && i < nb; ++i) {
        auto type = types.at(i);
        QDomElement itemsList = docElem.firstChildElement(QStringLiteral("list_") + type);
        IFOKDO(err, m_importer->importItems(itemsList));

        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1));
    }

    SKGENDTRANSACTION(m_importer->getDocument(), err)

    IFOKDO(err, m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE")))

    return err;
}

bool SKGImportPluginXml::isExportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("XML"));
}

SKGError SKGImportPluginXml::exportFile()
{
    SKGError err;
    QDomDocument doc;
    QVector<QString> ignore;
    ignore.append(QStringLiteral("parameters"));
    ignore.append(QStringLiteral("doctransaction"));
    ignore.append(QStringLiteral("doctransactionitem"));
    ignore.append(QStringLiteral("doctransactionmsg"));
    ignore.append(QStringLiteral("operationbalance"));
    ignore.append(QStringLiteral("budgetsuboperation"));
    ignore.append(QStringLiteral("node"));
    ignore.append(QStringLiteral("interest_result"));
    ignore.append(QStringLiteral("category.t_fullname"));
    ignore.append(QStringLiteral("operation.i_tmp"));
    ignore.append(QStringLiteral("suboperation.i_tmp"));
    ignore.append(QStringLiteral("unit.f_CURRENTAMOUNT_CACHE"));
    err = SKGServices::copySqliteDatabaseToXml(*(m_importer->getDocument()->getMainDatabase()), doc, &ignore);
    IFOK(err)
    {
        QSaveFile file(m_importer->getLocalFileName(false));
        if (!file.open(QIODevice::WriteOnly)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", m_importer->getFileName().toDisplayString()));
        } else {
            QTextStream stream(&file);
            if (!m_importer->getCodec().isEmpty()) {
                stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
            }
            stream << doc.toString() << Qt::endl;

            // Close file
            file.commit();
        }
    }
    return err;
}

QString SKGImportPluginXml::getMimeTypeFilter() const
{
    return "*.xml|" % i18nc("A file format", "XML file");
}

#include <skgimportpluginxml.moc>
