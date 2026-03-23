/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPROPERTIESPLUGIN_H
#define SKGPROPERTIESPLUGIN_H
/** @file
 * A plugin to manage properties on objects.
 *
 * @author Stephane MANKOWSKI
 */
#include <qprocess.h>

#include "skginterfaceplugin.h"

class SKGPropertiesPluginDockWidget;
class QMenu;
class QDockWidget;

/**
 * A plugin to manage properties on objects
 */
class SKGPropertiesPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGPropertiesPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGPropertiesPlugin() override;

    /**
     * Called to initialise the plugin
     * @param iDocument the main document
     * @return true if the plugin is compatible with the document
     */
    bool setupActions(SKGDocument *iDocument) override;

    /**
     * Must be modified to refresh widgets after a modification.
     */
    void refresh() override;

    /**
     * The dock widget of the plugin.
     * @return The dock widget of the plugin
     */
    QDockWidget *getDockWidget() override;

    /**
     * The icon of the plugin.
     * @return The icon of the plugin
     */
    QString icon() const override;

    /**
     * The title of the plugin.
     * @return The title of the plugin
     */
    QString title() const override;

    /**
     * The tips list of the plugin.
     * @return The tips list of the plugin
     */
    QStringList tips() const override;

    /**
     * Must be implemented to set the position of the plugin.
     * @return integer value between 0 and 999 (default = 999)
     */
    int getOrder() const override;

private Q_SLOTS:
    void onAddProperty();
    void onDownloadAndAddBills();
    void onShowAddPropertyMenu();
    void onBillsRetreived();

private:
    Q_DISABLE_COPY(SKGPropertiesPlugin)

    QProcess m_billsProcess;
    QStringList m_bills;

    SKGDocument *m_currentDocument;
    QDockWidget *m_dockWidget;
    SKGPropertiesPluginDockWidget *m_dockContent;
    QMenu *m_addPropertyMenu;
};

#endif // SKGPROPERTIESPLUGIN_H
