/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGHIGHLIGHTPLUGIN_H
#define SKGHIGHLIGHTPLUGIN_H
/** @file
 * A plugin to highlight objects.
 *
 * @author Stephane MANKOWSKI
 */
#include "skginterfaceplugin.h"

/**
 * A plugin to highlight objects
 */
class SKGHighlightPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGHighlightPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGHighlightPlugin() override;

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
     * Must be implemented to set the position of the plugin.
     * @return integer value between 0 and 999 (default = 999)
     */
    int getOrder() const override;

private Q_SLOTS:
    void onSwitchHighLight();

    void onSwitchClose();

private:
    Q_DISABLE_COPY(SKGHighlightPlugin)

    SKGDocument *m_currentDocument;
};

#endif // SKGHIGHLIGHTPLUGIN_H
