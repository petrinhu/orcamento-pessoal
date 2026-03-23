/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSELECTALLPLUGIN_H
#define SKGSELECTALLPLUGIN_H
/** @file
 * A plugin to select all.
 *
 * @author Stephane MANKOWSKI
 */
#include "skginterfaceplugin.h"

class QLabel;

/**
 * A plugin to select all
 */
class SKGSelectAllPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGSelectAllPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGSelectAllPlugin() override;

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
    void onSelectAll();
    void onSelectionChanged();

private:
    Q_DISABLE_COPY(SKGSelectAllPlugin)

    SKGDocument *m_currentDocument;
    QLabel *m_selectionMessage;
};

#endif // SKGSELECTALLPLUGIN_H
