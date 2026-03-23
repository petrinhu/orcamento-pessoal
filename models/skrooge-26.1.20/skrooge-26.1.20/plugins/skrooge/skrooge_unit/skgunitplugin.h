/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGUNITPLUGIN_H
#define SKGUNITPLUGIN_H
/** @file
 * This file is Skrooge plugin for unit management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skginterfaceplugin.h"
#include "ui_skgunitpluginwidget_pref.h"

class SKGDocumentBank;

/**
 * This file is Skrooge plugin for unit management
 */
class SKGUnitPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGUnitPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGUnitPlugin() override;

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
     * The number of dashboard widgets of the plugin.
     * @return The number of dashboard widgets of the plugin
     */
    int getNbDashboardWidgets() override;

    /**
     * Get a dashboard widget title of the plugin.
     * @param iIndex the index of the widget
     * @return The title
     */
    QString getDashboardWidgetTitle(int iIndex) override;

    /**
     * Get a dashboard widget of the plugin.
     * @param iIndex the index of the widget
     * @return The dashboard widget of the plugin
     */
    SKGBoardWidget *getDashboardWidget(int iIndex) override;

    /**
     * Must be modified to refresh widgets after a modification.
     */
    void refresh() override;

    /**
     * The preference widget of the plugin.
     * @return The preference widget of the plugin
     */
    QWidget *getPreferenceWidget() override;

    /**
     * The preference skeleton of the plugin.
     * @return The preference skeleton of the plugin
     */
    KConfigSkeleton *getPreferenceSkeleton() override;

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
     * The sub plugins services types list of the plugin.
     * This will be used to display authors in the "About" of the application
     * @return The sub plugins list of the plugin
     */
    QStringList subPlugins() const override;

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

    /**
     * Must be implemented to execute the automatic correction for the advice.
     * @param iAdviceIdentifier the identifier of the advice
     * @param iSolution the identifier of the possible solution
     * @return an object managing the error. MUST return ERR_NOTIMPL if iAdviceIdentifier is not known
     *   @see SKGError
     */
    SKGError executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution) override;

private Q_SLOTS:
    void onSplitShare();
    void deleteUnusedUnits() const;

private:
    Q_DISABLE_COPY(SKGUnitPlugin)

    SKGDocumentBank *m_currentBankDocument;
    QString m_docUniqueIdentifier;

    Ui::skgunitplugin_pref ui{};
};

#endif // SKGUNITPLUGIN_H
