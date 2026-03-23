/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINGNC_H
#define SKGIMPORTPLUGINGNC_H
/** @file
 * This file is Skrooge plugin for GNC import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qdom.h>

#include "skgimportplugin.h"

/**
 * This file is Skrooge plugin for GNC import / export.
 */
class SKGImportPluginGnc : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginGnc(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginGnc() override;

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
    Q_DISABLE_COPY(SKGImportPluginGnc)

    struct SubOpInfo {
        QDomElement subOp;
        QDomElement account;
        double value{};
    };
};

#endif // SKGIMPORTPLUGINGNC_H
