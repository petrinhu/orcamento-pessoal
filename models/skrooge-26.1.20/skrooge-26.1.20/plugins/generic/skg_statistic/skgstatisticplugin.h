/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSTATISTICPLUGIN_H
#define SKGSTATISTICPLUGIN_H
/** @file
 * A plugin to generate statistic.
 *
 * @author Stephane MANKOWSKI
 */
#include "skginterfaceplugin.h"

/**
 * A plugin to generate statistic
 */
class SKGStatisticPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGStatisticPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGStatisticPlugin() override;

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

    /**
     * Must be implemented to know if a plugin must be display in pages chooser.
     * @return true of false (default = false)
     */
    bool isInPagesChooser() const override;

private Q_SLOTS:

    void triggerAction();
    void pageChanged();
    void pageOpened();

private:
    Q_DISABLE_COPY(SKGStatisticPlugin)

    void readStats();
    void writeStats();

    SKGDocument *m_currentDocument;

    QVariantMap m_stats;
    QString m_file;
    QDateTime m_timeInit;
    QString m_docUniqueIdentifier;
};

#endif // SKGSTATISTICPLUGIN_H
