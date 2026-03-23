/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPAYEEPLUGIN_H
#define SKGPAYEEPLUGIN_H
/** @file
 * A skrooge plugin to payee transactions.
 *
 * @author Stephane MANKOWSKI
 */
#include "skginterfaceplugin.h"
#include "ui_skgpayeepluginwidget_pref.h"

/**
 * A skrooge plugin to tracker transactions
 */
class SKGPayeePlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGPayeePlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGPayeePlugin() override;

    /**
     * Called to initialise the plugin
     * @return true if the plugin is compatible with the document
     */
    bool setupActions(SKGDocument *iDocument) override;

    /**
     * The preference skeleton of the plugin.
     * @return The preference skeleton of the plugin
     */
    KConfigSkeleton *getPreferenceSkeleton() override;

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
     * The tips list of the plugin.
     * @return The tips list of the plugin
     */
    QStringList tips() const override;

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

    /**
     * The advice list of the plugin.
     * @return The advice list of the plugin
     */
    SKGAdviceList advice(const QStringList &iIgnoredAdvice) override;

private Q_SLOTS:
    void deleteUnusedPayees() const;

private:
    Q_DISABLE_COPY(SKGPayeePlugin)

    SKGDocument *m_currentBankDocument;

    Ui::skgpayeeplugin_pref ui{};
};

#endif // SKGPAYEEPLUGIN_H
