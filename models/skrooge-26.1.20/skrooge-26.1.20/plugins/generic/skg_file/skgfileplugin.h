/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGFILEPLUGIN_H
#define SKGFILEPLUGIN_H
/** @file
 * This file is a plugin for file operation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qurl.h>

#include "skginterfaceplugin.h"
#include "ui_skgfilepluginwidget_pref.h"

class QAction;
class KRecentFilesAction;

/**
 * This file is a plugin for file operation
 */
class SKGFilePlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGFilePlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGFilePlugin() override;

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
     * The tips list of the plugin.
     * @return The tips list of the plugin
     */
    QStringList tips() const override;

    /**
     * The advice list of the plugin.
     * @return The advice list of the plugin
     */
    SKGAdviceList advice(const QStringList &iIgnoredAdvice) override;

Q_SIGNALS:
    /**
     * request to load a file
     * @param iFile file name
     */
    void loadFile(const QUrl &iFile);

private Q_SLOTS:
    /**
     * @brief Open a Url
     * @param iUrl The url to open.
     * If empty, then the URL is searched on the property "filename" of the QAction sending
     * If still empty, an open panel is displayed
     * @return void
     */
    void onOpen(const QUrl &iUrl = QUrl());
    void onSave();
    void onSaveAs();
    void onReOpen();
    void onRecover();
    void onNew();
    void onChangePassword();

private:
    Q_DISABLE_COPY(SKGFilePlugin)

    QAction *m_saveAction;
    KRecentFilesAction *m_recentFiles;

    SKGDocument *m_currentDocument;

    Ui::skgfileplugin_pref ui{};
};

#endif // SKGFILEPLUGIN_H
