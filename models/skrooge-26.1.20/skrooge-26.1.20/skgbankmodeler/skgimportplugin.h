/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGIN_H
#define SKGIMPORTPLUGIN_H
/** @file
 * This file is a plugin interface definition.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbankmodeler_export.h"
#include "skgerror.h"
#include "skgimportexportmanager.h"

/**
 * This file is a plugin interface definition.
 */
class SKGBANKMODELER_EXPORT SKGImportPlugin : public QObject
{
    Q_OBJECT
public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     */
    explicit SKGImportPlugin(QObject *iImporter = nullptr);

    /**
     * Default destructor
     */
    ~SKGImportPlugin() override;

    /**
     * Get parameters for Import
     * @return the parameters
     */
    virtual inline QMap<QString, QString> getImportParameters()
    {
        return m_importParameters;
    }

    /**
     * Set parameters for Import
     * @param iParameters the parameters
     */
    virtual inline void setImportParameters(const QMap<QString, QString> &iParameters)
    {
        m_importParameters = iParameters;
    }

    /**
     * Get parameters for Export
     * @return the parameters
     */
    virtual inline QMap<QString, QString> getExportParameters()
    {
        return m_exportParameters;
    }

    /**
     * Set parameters for Export
     * @param iParameters the parameters
     */
    virtual inline void setExportParameters(const QMap<QString, QString> &iParameters)
    {
        m_exportParameters = iParameters;
    }

    /**
     * To know if import is possible with this plugin
     * @return true or false
     */
    virtual inline bool isImportPossible()
    {
        return false;
    }

    /**
     * Import a file
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual inline SKGError importFile()
    {
        return SKGError(ERR_NOTIMPL, QString());
    }

    /**
     * To know if export is possible with this plugin
     * @return true or false
     */
    virtual inline bool isExportPossible()
    {
        return false;
    }

    /**
     * Export a file
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual inline SKGError exportFile()
    {
        return SKGError(ERR_NOTIMPL, QString());
    }

    /**
     * Return the mime type filter
     * @return the mime type filter. Example: "*.csv|CSV file"
     */
    virtual QString getMimeTypeFilter() const
    {
        return QString();
    }

private:
    Q_DISABLE_COPY(SKGImportPlugin)

protected:
    SKGImportExportManager *m_importer;
    QMap<QString, QString> m_importParameters;
    QMap<QString, QString> m_exportParameters;
};

/**
 * This plugin interface definition.
 */
Q_DECLARE_INTERFACE(SKGImportPlugin, "skrooge.com.SKGImportPlugin/1.0")

#endif // SKGIMPORTPLUGIN_H
