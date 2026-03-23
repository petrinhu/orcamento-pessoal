/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBOOKMARKPLUGIN_H
#define SKGBOOKMARKPLUGIN_H
/** @file
 * This file is a plugin for bookmarks management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qvariant.h>

#include "skginterfaceplugin.h"
#include "ui_skgbookmarkpluginwidget_pref.h"

class QMenu;

/**
 * This file is a plugin for bookmarks management
 */
class SKGBookmarkPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGBookmarkPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGBookmarkPlugin() override;

    /**
     * Called to setupActions the plugin
     * @param iDocument the main document
     * @return true if the plugin is compatible with the document
     */
    bool setupActions(SKGDocument *iDocument) override;

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

private Q_SLOTS:
    void importStandardBookmarks();
    void goHome();

    void onOpenBookmark();
    void onAddBookmark();
    void onShowBookmarkMenu();

private:
    Q_DISABLE_COPY(SKGBookmarkPlugin)

    SKGDocument *m_currentDocument;
    QDockWidget *m_dockWidget;
    QString m_docUniqueIdentifier;
    QMenu *m_bookmarkMenu;

    Ui::skgbookmarkplugin_pref ui{};
};

#endif // SKGBOOKMARKPLUGIN_H
