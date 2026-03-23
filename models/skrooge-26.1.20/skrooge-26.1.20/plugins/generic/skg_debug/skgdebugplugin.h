/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDEBUGPLUGIN_H
#define SKGDEBUGPLUGIN_H
/** @file
 * This file is a plugin for debug.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skginterfaceplugin.h"

/**
 * This file is a plugin for debug
 */
class SKGDebugPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGDebugPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGDebugPlugin() override;

    /**
     * Called to initialise the plugin
     * @param iDocument the main document
     * @return true if the plugin is compatible with the document
     */
    bool setupActions(SKGDocument *iDocument) override;

    /**
     * The page widget of the plugin.
     * @return The page widget of the plugin
     */
    SKGTabPage *getWidget() override;

    /**
     * The title of the plugin.
     * @return The title of the plugin
     */
    QString title() const override;

    /**
     * The icon of the plugin.
     * @return The icon of the plugin
     */
    QString icon() const override;

    /**
     * The toolTip of the plugin.
     * @return The toolTip of the plugin
     */
    QString toolTip() const override;

    /**
     * Must be implemented to know if a plugin must be display in pages chooser.
     * @return true of false (default = false)
     */
    bool isInPagesChooser() const override;

    /**
     * Must be implemented to know if this plugin is enabled
     * @return true of false (default = true)
     */
    bool isEnabled() const override;

private Q_SLOTS:
    void onRestartProfiling();
    void onOpenProfiling();

private:
    Q_DISABLE_COPY(SKGDebugPlugin)

    SKGDocument *m_currentDocument;
};

#endif // SKGDEBUGPLUGIN_H
