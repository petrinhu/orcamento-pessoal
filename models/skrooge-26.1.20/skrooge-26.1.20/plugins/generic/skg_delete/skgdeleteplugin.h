/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDELETEPLUGIN_H
#define SKGDELETEPLUGIN_H
/** @file
 * This file is a plugin for delete operation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skginterfaceplugin.h"

class QAction;

/**
 * This file is a plugin for delete operation
 */
class SKGDeletePlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGDeletePlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGDeletePlugin() override;

    /**
     * Called to initialise the plugin
     * @param iDocument the main document
     * @return true if the plugin is compatible with the document
     */
    bool setupActions(SKGDocument *iDocument) override;

    /**
     * The title of the plugin.
     * @return The title of the plugin
     */
    QString title() const override;

    /**
     * Must be implemented to set the position of the plugin.
     * @return integer value between 0 and 999 (default = 999)
     */
    int getOrder() const override;

private Q_SLOTS:
    void onDelete();

private:
    Q_DISABLE_COPY(SKGDeletePlugin)

    SKGDocument *m_currentDocument;
};

#endif // SKGDEBUGPLUGIN_H
