/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for JSON import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginjson.h"

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include <qsavefile.h>

#include "skgdocumentbank.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginJson, "metadata.json")

SKGImportPluginJson::SKGImportPluginJson(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginJson::~SKGImportPluginJson() = default;

bool SKGImportPluginJson::isExportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("JSON"));
}

SKGError SKGImportPluginJson::exportFile()
{
    SKGError err;
    QString doc;
    err = m_importer->getDocument()->copyToJson(doc);
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
            stream << doc << Qt::endl;

            // Close file
            file.commit();
        }
    }
    return err;
}

QString SKGImportPluginJson::getMimeTypeFilter() const
{
    return "*.json|" % i18nc("A file format", "JSON file");
}

#include <skgimportpluginjson.moc>
