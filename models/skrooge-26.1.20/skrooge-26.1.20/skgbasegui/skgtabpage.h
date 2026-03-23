/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTABPAGE_H
#define SKGTABPAGE_H
/** @file
 * This file is a class managing widget.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qlist.h>
#include <qwidget.h>

#include "skgbasegui_export.h"
#include "skgwidget.h"

/**
 * This file is a tab widget used by plugins
 */
class SKGBASEGUI_EXPORT SKGTabPage : public SKGWidget
{
    Q_OBJECT

public:
    /**
     * Describe a history item
     */
    struct SKGPageHistoryItem {
        QString plugin; /**< The plugin name */
        QString name; /**< The name */
        QString state; /**< The state */
        QString icon; /**< The icon */
        QString bookmarkID; /**< The bookmarkID */
    };

    /**
     * List of history item
     */
    using SKGPageHistoryItemList = QVector<SKGTabPage::SKGPageHistoryItem>;

    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGTabPage(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGTabPage() override;

    /**
     * Set the bookmark id
     * @param iId bookmark id
     */
    virtual void setBookmarkID(const QString &iId);

    /**
     * Get the bookmark id
     * @return bookmark id
     */
    virtual QString getBookmarkID();

    /**
     * To know if an overwrite is needed
     */
    virtual bool isOverwriteNeeded();

    /**
     * Overwrite bookmark if page is opened from a bookmark
     * Overwrite context if page is opened from a context
     * @param iUserConfirmation to display a confirmation panel
     */
    virtual void overwrite(bool iUserConfirmation = true);

    /**
     * Get previous pages
     * @return the list
     */
    virtual SKGTabPage::SKGPageHistoryItemList getPreviousPages();

    /**
     * Set previous pages
     * @param iPages the list
     */
    virtual void setPreviousPages(const SKGTabPage::SKGPageHistoryItemList &iPages);

    /**
     * Get next pages
     * @return the list
     */
    virtual SKGTabPage::SKGPageHistoryItemList getNextPages();

    /**
     * Set next pages
     * @param iPages the list
     */
    virtual void setNextPages(const SKGTabPage::SKGPageHistoryItemList &iPages);

    /**
     * To know if this page contains an editor. MUST BE OVERWRITTEN
     * @return the editor state
     */
    virtual bool isEditor();

    /**
     * To activate the editor by setting focus on right widget. MUST BE OVERWRITTEN
     */
    virtual void activateEditor();

    /**
     * Get the zoomable widget.
     * The default implementation returns the main widget.
     * @return the zoomable widget.
     */
    virtual QWidget *zoomableWidget();

    /**
     * Get the printable widgets.
     * The default implementation returns the main widget.
     * @return the printable widgets.
     */
    virtual QList<QWidget *> printableWidgets();

    /**
     * To know if this page is zoomable. MUST BE OVERWRITTEN
     * @return true or false
     */
    virtual bool isZoomable();

    /**
     * Get the zoom position. MUST BE OVERWRITTEN
     * @return the position (-10<=value<=10)
     */
    virtual int zoomPosition();

    /**
     * Set the zoom position. MUST BE OVERWRITTEN
     * @param iValue the position (-10<=value<=10)
     */
    virtual void setZoomPosition(int iValue);

    /**
     * Get the tab page of a widget
     * @param iWidget the widget
     * @return the iParent tab page or nullptr
     */
    static SKGTabPage *parentTabPage(QWidget *iWidget);

    /**
     * Get the pin status
     * @return the pin status
     */
    virtual bool isPin() const;

public Q_SLOTS:
    /**
     * Set the pin status
     * @param iPin the pin status
     */
    virtual void setPin(bool iPin);

    /**
     * Closes this widget
     * @param iForce to close pinned pages too
     * @return true if the widget was closed; otherwise returns false.
     */
    virtual bool close(bool iForce = false);

private:
    Q_DISABLE_COPY(SKGTabPage)

    QString m_bookmarkID;
    SKGTabPage::SKGPageHistoryItemList m_previousPages;
    SKGTabPage::SKGPageHistoryItemList m_nextPages;
    int m_fontOriginalPointSize;
    bool m_pin;
};
#endif // SKGTABPAGE_H
