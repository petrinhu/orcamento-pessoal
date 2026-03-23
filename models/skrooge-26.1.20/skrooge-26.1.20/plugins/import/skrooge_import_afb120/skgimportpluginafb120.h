/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINAFB120_H
#define SKGIMPORTPLUGINAFB120_H
/** @file
 * This file is Skrooge plugin for AFB120 import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugin.h"

/**
 * This file is Skrooge plugin for AFB120 import / export.
 */
class SKGImportPluginAFB120 : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginAFB120(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginAFB120() override;

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
    Q_DISABLE_COPY(SKGImportPluginAFB120)

    double toAmount(const QString &iAmount, int iNbDecimal);
};

#endif // SKGIMPORTPLUGINAFB120_H
