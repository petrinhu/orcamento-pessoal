/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGUNDOREDOPLUGIN_H
#define SKGUNDOREDOPLUGIN_H
/** @file
 * This file is a plugin for undo and redo operation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skginterfaceplugin.h"
#include "ui_skgundoredopluginwidget_pref.h"

class QAction;
class KToolBarPopupAction;
class QMenu;

/**
 * This file is a plugin for undo and redo operation
 */
class SKGUndoRedoPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGUndoRedoPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGUndoRedoPlugin() override;

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
     * The dock widget of the plugin.
     * @return The dock widget of the plugin
     */
    QDockWidget *getDockWidget() override;

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
    void onUndoSave();
    void onUndo();
    void onRedo();
    void onClearHistory();

    void onShowUndoMenu();
    void onShowRedoMenu();

private:
    Q_DISABLE_COPY(SKGUndoRedoPlugin)

    QAction *m_undoSaveAction;
    KToolBarPopupAction *m_undoAction;
    KToolBarPopupAction *m_redoAction;

    QMenu *m_undoMenu;
    QMenu *m_redoMenu;

    SKGDocument *m_currentDocument;
    QDockWidget *m_dockWidget;

    Ui::skgundoredoplugin_pref ui{};
};

#endif // SKGDEBUGPLUGIN_H
