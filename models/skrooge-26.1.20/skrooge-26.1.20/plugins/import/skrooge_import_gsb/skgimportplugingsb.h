/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINGSB_H
#define SKGIMPORTPLUGINGSB_H
/** @file
 * This file is Skrooge plugin for GSB import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugin.h"

#include <qdom.h>

/**
 * This file is Skrooge plugin for GSB import / export.
 */
class SKGImportPluginGsb : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginGsb(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginGsb() override;

    /**
     * To know if import is possible with this plugin
     */
    bool isImportPossible() override;

    /**
     * Import a file
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError importFile() override;

    /**
     * Return the mime type filter
     * @return the mime type filter. Example: "*.csv|CSV file"
     */
    QString getMimeTypeFilter() const override;

private:
    Q_DISABLE_COPY(SKGImportPluginGsb)

    static QString getAttribute(const QDomElement &iElement, const QString &iAttribute);
};

#endif // SKGIMPORTPLUGINGSB_H
