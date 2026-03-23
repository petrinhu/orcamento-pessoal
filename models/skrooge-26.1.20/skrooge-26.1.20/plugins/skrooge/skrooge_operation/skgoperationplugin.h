/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGOPERATIONPLUGIN_H
#define SKGOPERATIONPLUGIN_H
/** @file
 * This file is Skrooge plugin for transaction management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skginterfaceplugin.h"
#include "ui_skgoperationpluginwidget_pref.h"

class QMenu;
class SKGDocumentBank;

/**
 * This file is Skrooge plugin for transaction management
 */
class SKGOperationPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGOperationPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGOperationPlugin() override;

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
     * This function is called when preferences have been modified. Must be used to save some parameters into the document.
     * A transaction is already opened
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError savePreferences() const override;

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

    /**
     * Must be implemented to execute the automatic correction for the advice.
     * @param iAdviceIdentifier the identifier of the advice
     * @param iSolution the identifier of the possible solution
     * @return an object managing the error. MUST return ERR_NOTIMPL if iAdviceIdentifier is not known
     *   @see SKGError
     */
    SKGError executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution) override;

private Q_SLOTS:
    void onDuplicate();
    void onCreateTemplate();
    void onSwitchToMarked();
    void onSwitchToChecked();
    void onOpenOperations();
    void onGroupOperation();
    void onUngroupOperation();
    void onMergeSubOperations();
    void onAlignComment();
    void onAlignComment2();
    void onAlignDate();
    void onAlignWithCategoryOfPayee();
    void onRemoveGroupWithOneOperation();
    void onShowApplyTemplateMenu();
    void onApplyTemplate();
    void onShowOpenWithMenu();

private:
    Q_DISABLE_COPY(SKGOperationPlugin)

    static SKGError checkReconciliation(SKGDocument * /*iDocument*/);
    static SKGError checkImport(SKGDocument * /*iDocument*/);

    QMenu *m_applyTemplateMenu;
    QMenu *m_openOperationsWithMenu;
    QMenu *m_openSubOperationsWithMenu;
    SKGDocumentBank *m_currentBankDocument;

    Ui::skgoperationplugin_pref ui{};
};

#endif // SKGOPERATIONPLUGIN_H
