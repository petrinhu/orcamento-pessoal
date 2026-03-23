/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINBACKEND_H
#define SKGIMPORTPLUGINBACKEND_H
/** @file
 * This file is Skrooge plugin for BACKEND import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugin.h"

#include <kpluginmetadata.h>

#include <qtemporarydir.h>

/**
 * This file is Skrooge plugin for BACKEND import / export.
 */
class SKGImportPluginBackend : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginBackend(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginBackend() override;

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
    Q_DISABLE_COPY(SKGImportPluginBackend)

    const KPluginMetaData *getService() const;
    QString getParameter(const QString &iAttribute);

    QVector<KPluginMetaData> m_listBackends;
    QTemporaryDir m_tempDir;
};

#endif // SKGIMPORTPLUGINBACKEND_H
