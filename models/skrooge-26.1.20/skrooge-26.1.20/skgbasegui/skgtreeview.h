/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTREEVIEW_H
#define SKGTREEVIEW_H
/** @file
 * A tree view with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qmenu.h>
#include <qstringlist.h>
#include <qtreeview.h>

#include "skgbasegui_export.h"
#include "skgmainpanel.h"
#include "skgobjectbase.h"
#include "skgservices.h"
#include "skgtraces.h"

class SKGObjectModelBase;
class SKGDocument;
class QTextBrowser;
class SKGSortFilterProxyModel;
class QTimer;

/**
 * This file is a tab widget used by plugins
 */
class SKGBASEGUI_EXPORT SKGTreeView : public QTreeView
{
    Q_OBJECT

    /**
     * Text resizable by CTRL+wheel
     */
    Q_PROPERTY(bool textResizable READ isTextResizable WRITE setTextResizable NOTIFY modified)
    /**
     * Auto resize mode of the view
     */
    Q_PROPERTY(bool autoResized READ isAutoResized CONSTANT)

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGTreeView(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGTreeView() override;

    /**
     * Get the current state
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    virtual QString getState();

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    virtual void setState(const QString &iState);

    /**
     * To know if the autoresized mode is enable
     */
    virtual bool isAutoResized();

    /**
     * Set parameter to activate and save default state of this table
     * @param iDocument document pointer
     * @param iParameterName parameter name in this document
     */
    virtual void setDefaultSaveParameters(SKGDocument *iDocument, const QString &iParameterName);

    /**
     * Get the table content as QTextBrowser
     * @return the table content (MUST BE DELETED)
     */
    virtual QTextBrowser *getTextBrowser() const;

    /**
     * Get the table content
     * @param iIndex the line index
     * @return the table content
     */
    virtual SKGStringListList getTable(const QModelIndex &iIndex = QModelIndex()) const;

    /**
     * Get the current selection
     * @return selected objects
     */
    virtual SKGObjectBase::SKGListSKGObjectBase getSelectedObjects();

    /**
     * Get the first selected object
     * @return first selected object
     */
    virtual SKGObjectBase getFirstSelectedObject();

    /**
     * Get the number of selected object
     * @return number of selected objects
     */
    virtual int getNbSelectedObjects();

    /**
     * Sets the current selection model to the given selectionModel.
     * @param iSelectionModel the selection model
     */
    void setSelectionModel(QItemSelectionModel *iSelectionModel) override;

    /**
     * Insert a registered action
     * @see SKGMainPanel::registerGlobalAction
     * @param iRegisteredAction the registered action. "" means separator
     */
    virtual void insertGlobalAction(const QString &iRegisteredAction = QString());

    /**
     * @brief Set model
     *
     * @param iModel the model
     * @return void
     **/
    void setModel(QAbstractItemModel *iModel) override;

    /**
     * @brief Get the header menu
     * @return the header menu
     **/
    virtual QMenu *getHeaderMenu() const;

    /**
     * Export to a file
     * @param iFileName the file name
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError exportInFile(const QString &iFileName);

    /**
     * Get the current schema
     * @return the current schema
     */
    virtual QStringList getCurrentSchema() const;

    /**
     * Get the property to know if text is resizable
     * @return the property
     */
    virtual bool isTextResizable() const;

public Q_SLOTS:
    /**
     * This property holds whether to authorize text size modification by CTRL+wheel.
     * @param resizable true of false
     */
    virtual void setTextResizable(bool resizable);

    /**
     * Set the zoom position.
     * @param iZoomPosition zoom position (-10<=zoom position<=10)
     */
    virtual void setZoomPosition(int iZoomPosition);

    /**
     * Get zoom position
     * @return zoom position (-10<=zoom position<=10)
     */
    virtual int zoomPosition();

    /**
     * Save the selection
     */
    virtual void saveSelection();

    /**
     * Reset the selection
     */
    virtual void resetSelection();

    /**
     * Scroll on selected lines.
     */
    virtual void scroolOnSelection();

    /**
     * Select an object and focus on it
     * @param iUniqueID unique ID of the object
     */
    virtual void selectObject(const QString &iUniqueID);

    /**
     * Select objects and focus on the first one
     * @param iUniqueIDs unique IDs of objects
     * @param iFocusOnFirstOne set the focus on the first one
     */
    virtual void selectObjects(const QStringList &iUniqueIDs, bool iFocusOnFirstOne = false);

    /**
     * This property holds whether to draw the background using alternating colors.
     * @param enable true of false
     */
    virtual void setAlternatingRowColors(bool enable);

    /**
     * Reset columns order
     */
    virtual void resetColumnsOrder();

    /**
     * Resizes all columns based on the size hints of the delegate used to render each item in the columns.
     */
    virtual void resizeColumnsToContents();

    /**
     * Resizes all columns based on the size hints of the delegate used to render each item in the columns.
     */
    virtual void resizeColumnsToContentsDelayed();

    /**
     * When the selection changed
     */
    virtual void onSelectionChanged();

    /**
     * Expand all and resize columns if needed
     */
    virtual void expandAll();

    /**
     * Copy selection in clipboard
     */
    virtual void copy();

    /**
     * Switch auto resize
     */
    virtual void switchAutoResize();

protected:
    /**
     * This function is called with the given event when a mouse button is pressed while the cursor is inside the widget.
     * If a valid item is pressed on it is made into the current item. This function emits the pressed() signal.
     * @param iEvent the event
     */
    void mousePressEvent(QMouseEvent *iEvent) override;

Q_SIGNALS:
    /**
     * Emitted when the empty area
     */
    void clickEmptyArea();

    /**
     * Emitted 300ms after selection changed
     */
    void selectionChangedDelayed();

    /**
     * Emitted when zoom changed
     * @param iZoomPosition zoom position (-10<=zoom position<=10)
     */
    void zoomChanged(int iZoomPosition);

    /**
     * When properties are modified
     */
    void modified();

protected:
    /**
     * Event filtering
     * @param iObject object
     * @param iEvent event
     * @return In your reimplementation of this function, if you want to filter the event out, i.e. stop it being handled further, return true; otherwise return
     * false.
     */
    bool eventFilter(QObject *iObject, QEvent *iEvent) override;

private Q_SLOTS:
    void onExport();
    void setupHeaderMenu();
    void showHeaderMenu();
    void showHeaderMenu(QPoint iPos);
    void showHideColumn();
    void respanFirstColumns();
    void refreshExpandCollapse();
    void changeSchema();
    void saveDefaultClicked();
    void groupByChanged(QAction * /*iAction*/);

    void onSortChanged(int /*iIndex*/, Qt::SortOrder /*iOrder*/);

    void onExpand(const QModelIndex &index);
    void onCollapse(const QModelIndex &index);
    void onClick(const QModelIndex &index);
    void onActionTriggered(int action);
    void onRangeChanged();

    void rebuildContextualMenu();

private:
    Q_DISABLE_COPY(SKGTreeView)

    QMenu *m_headerMenu;
    bool m_autoResize;
    bool m_autoResizeDone;
    QAction *m_actAutoResize;
    QAction *m_actCopy;
    QAction *m_actExpandAll;
    QAction *m_actCollapseAll;
    SKGDocument *m_document;
    QString m_parameterName;
    QStringList m_selection;
    QStringList m_expandedNodes;
    QString m_groupby;

    QTimer m_timerDelayedResize;
    QTimer m_timerSelectionChanged;
    QTimer m_timerScrollSelection;
    bool m_textResizable;
    int m_fontOriginalPointSize;
    int m_iconOriginalSize;

    SKGObjectModelBase *m_model;
    SKGSortFilterProxyModel *m_proxyModel;

    QAction *m_actGroupByNone;

    SKGObjectBase::SKGListSKGObjectBase m_lastSelection;
    SKGObjectBase::SKGListSKGObjectBase m_lastSelection_if_deleted;
    QString m_lastSelection_previous;

    bool stickH;
    bool stickV;
};

#endif // SKGTREEVIEW_H
