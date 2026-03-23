/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBOOKMARKPLUGINDOCKWIDGET_H
#define SKGBOOKMARKPLUGINDOCKWIDGET_H
/** @file
 * This file is a plugin for bookmarks management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgnodeobject.h"
#include "skgwidget.h"
#include "ui_skgbookmarkplugindockwidget_base.h"

class QMenu;
class SKGObjectModelBase;

/**
 * This file is a plugin for undoredo management
 */
class SKGBookmarkPluginDockWidget : public SKGWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGBookmarkPluginDockWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGBookmarkPluginDockWidget() override;

    /**
     * Get the main widget
     * @return a widget
     */
    QWidget *mainWidget() override;

    /**
     * Open a bookmark
     * @param iNode bookmark object to open
     * @param iFirstInNewPage to open first object in new page
     * @param iPin to pin opened page
     */
    static void openBookmark(const SKGNodeObject &iNode, bool iFirstInNewPage = false, bool iPin = false);

    /**
     * Bookmark a page
     * @param iPage the page to bookmark
     * @param iParentNode the parent node
     * @param oCreatedNode the created node
     */
    static SKGError createNodeFromPage(SKGTabPage *iPage, const SKGNodeObject &iParentNode, SKGNodeObject &oCreatedNode);

protected:
    /**
     * This event handler can be reimplemented in a subclass to receive widget resize events which are passed in the event parameter.
     * When resizeEvent() is called, the widget already has its new geometry. The old size is accessible through QResizeEvent::oldSize().
     * @param iEvent the event pointer
     */
    void resizeEvent(QResizeEvent *iEvent) override;

public Q_SLOTS:
    /**
     * Refresh the content.
     */
    virtual void refresh();

private Q_SLOTS:
    void initMenu();
    void showMenu(QPoint iPos);
    void onAddBookmarkGroup();
    void onAddBookmark();
    void onAddBookmarks();
    void onRemoveBookmark();
    void onRenameBookmark();
    void onChangeIconBookmark();
    void onBeforeOpenBookmark();
    void onOpenBookmark(const QModelIndex &index);
    void onOpenBookmarkFolder(const QModelIndex &index);
    void onBookmarkEditorChanged();
    void onSetAutostart();
    void onUnsetAutostart();
    void onPageChanged();

private:
    Q_DISABLE_COPY(SKGBookmarkPluginDockWidget)

    void setAutostart(const QString &value);

    Ui::skgbookmarkplugindockwidget_base ui{};
    QMenu *m_mainMenu;
    QAction *m_actDelete;
    QAction *m_actRename;
    QAction *m_actChangeIcon;
    QAction *m_actAddBookmark;
    QAction *m_actAddBookmarks;
    QAction *m_actAddBookmarkGroup;
    QAction *m_actSetAutostart;
    QAction *m_actUnsetAutostart;
    SKGObjectModelBase *m_modelview;
    static bool m_middleClick;
};

#endif // SKGBOOKMARKPLUGINDOCKWIDGET_H
