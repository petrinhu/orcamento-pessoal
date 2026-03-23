/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINXML_H
#define SKGIMPORTPLUGINXML_H
/** @file
 * This file is Skrooge plugin for XML import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugin.h"

class SKGObjectBase;

/**
 * This file is Skrooge plugin for XML import / export.
 */
class SKGImportPluginXml : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginXml(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginXml() override;

    /**
     * To know if import is possible with this plugin
     * @return true or false
     */
    bool isImportPossible() override;

    /**
     * Import a file
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError importFile() override;

    /**
     * To know if export is possible with this plugin
     * @return true or false
     */
    bool isExportPossible() override;

    /**
     * Export a file
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError exportFile() override;

    /**
     * Return the mime type filter
     * @return the mime type filter. Example: "*.csv|CSV file"
     */
    QString getMimeTypeFilter() const override;

    /**
     * Get the document as an XML object
     * @param iFileName File to load.
     * @param oDocument The output XML document.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getXMLDocument(const QString &iFileName, QDomElement &oDocument);

private:
    Q_DISABLE_COPY(SKGImportPluginXml)
};

#endif // SKGIMPORTPLUGINXML_H
