/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINSKG_H
#define SKGIMPORTPLUGINSKG_H
/** @file
 * This file is Skrooge plugin for SKG import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugin.h"

class SKGObjectBase;

/**
 * This file is Skrooge plugin for SKG import / export.
 */
class SKGImportPluginSkg : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginSkg(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginSkg() override;

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

private:
    Q_DISABLE_COPY(SKGImportPluginSkg)

    static SKGError copyParameters(const SKGObjectBase &iFrom, const SKGObjectBase &iTo);
};

#endif // SKGIMPORTPLUGINSKG_H
