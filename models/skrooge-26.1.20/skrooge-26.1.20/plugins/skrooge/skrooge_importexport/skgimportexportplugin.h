/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTEXPORTPLUGIN_H
#define SKGIMPORTEXPORTPLUGIN_H
/** @file
 * This file is Skrooge plugin for import and export operation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportexportmanager.h"
#include "skginterfaceplugin.h"
#include "ui_skgimportexportpluginwidget_pref.h"

class QAction;
class SKGDocumentBank;

/**
 * This file is Skrooge plugin for import and export operation
 */
class SKGImportExportPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGImportExportPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportExportPlugin() override;

    /**
     * Called to initialise the plugin
     * @param iDocument the main document
     * @return true if the plugin is compatible with the document
     */
    bool setupActions(SKGDocument *iDocument) override;

    /**
     * This function is called when the application is launched again with new arguments
     * @param iArgument the arguments
     * @return the rest of arguments to treat
     */
    QStringList processArguments(const QStringList &iArgument) override;

    /**
     * Must be modified to refresh widgets after a modification.
     */
    void refresh() override;

    /**
     * Initialize the preferences. This is use full for settings stored in the document
     */
    virtual void initPreferences() override;

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

Q_SIGNALS:
    /**
     * Request to import a file
     * @param iFile the file
     */
    void importFileName(const QString &iFile);

private Q_SLOTS:
    void importFile(const QString &iFile = QString(), bool iBlockOpenLastModified = false);
    void importFiles(const QList<QUrl> &iFiles = QList<QUrl>(), int mode = 1, bool iBlockOpenLastModified = false); // 1=transactions, 2=unit, 3=rules
    SKGError importbackends();
    void exportFile();
    void findTransfers();
    void anonymize();
    void cleanBanks();
    void swithvalidationImportedOperations();
    void validateAllOperations();
    void mergeImportedOperation();
    void onInstall();

private:
    Q_DISABLE_COPY(SKGImportExportPlugin)
    void openLastModifiedIfSetting();

    SKGDocumentBank *m_currentBankDocument;
    QString m_docUniqueIdentifier;

    Ui::skgimportexportplugin_pref ui{};
    bool m_install;
};

#endif // SKGDEBUGPLUGIN_H
