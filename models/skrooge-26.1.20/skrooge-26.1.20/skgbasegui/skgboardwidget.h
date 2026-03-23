/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBOARDWIDGET_H
#define SKGBOARDWIDGET_H
/** @file
 * This file is a generic for board widget.
 * @see SKGReport
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbasegui_export.h"
#include "skgwidget.h"

class QAction;
class QMenu;
class QToolButton;
class QFrame;
class QGridLayout;
class QLabel;
class QAction;
class QMenu;
class SKGZoomSelector;

/**
 * This file is a generic for board widget
 */
class SKGBASEGUI_EXPORT SKGBoardWidget : public SKGWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     * @param iTitle the title
     * @param iZoomable for a zoomable widget
     */
    explicit SKGBoardWidget(QWidget *iParent, SKGDocument *iDocument, const QString &iTitle, bool iZoomable = false);

    /**
     * Default Destructor
     */
    ~SKGBoardWidget() override;

    /**
     * Get the current state
     * MUST BE OVERWRITTEN
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    QString getState() override;

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    void setState(const QString &iState) override;

    /**
     * Get the widget for drag and drop
     * MUST BE OVERWRITTEN
     * @return the widget.
     */
    virtual QWidget *getDragWidget();

    /**
     * Set the main widget
     * @param iWidget the main widget
     */
    virtual void setMainWidget(QWidget *iWidget);

    /**
     * Set the main title
     * @param iTitle the main title
     */
    virtual void setMainTitle(const QString &iTitle);

    /**
     * Get the main title
     * @return the main title
     */
    virtual QString getMainTitle();

    /**
     * Get the original title
     * @return the original title
     */
    virtual QString getOriginalTitle();

    /**
     * Set the zoom ratio
     * @param iRatio the zoom ratio (1 <= iRatio <=5)
     */
    virtual void setZoomRatio(double iRatio);

    /**
     * Get the zoom ratio
     * @return the zoom ratio
     */
    virtual double getZoomRatio();

    /**
     * Add an action
     * @param iAction the action to add
     */
    virtual void addAction(QAction *iAction);

    /**
     * Add a sub menu
     * @param iMenu the menu to add
     */

    virtual void addMenu(QMenu *iMenu);

    /**
     * Get the menu
     */
    virtual QMenu *getMenu();

    /**
     * Insert an action
     * @param iPos the position of the insertion
     * @param iAction the action to add
     */
    virtual void insertAction(int iPos, QAction *iAction);

    /**
     * Hide title and configuration widget
     */
    virtual void hideTitle();

Q_SIGNALS:

    /**
     * Sent when the remove is requested
     */
    void requestRemove();

    /**
     * Sent when the move is requested
     * @param iMove the move to apply
     */
    void requestMove(int iMove);

private Q_SLOTS:
    void onZoom(int iZoom);
    void requestMoveBefore();
    void requestMoveAfter();
    void requestMoveFirst();
    void requestMoveLast();
    void onRenameTitle();

private:
    Q_DISABLE_COPY(SKGBoardWidget)

    QLabel *m_Title{};
    QGridLayout *m_gridLayout{};
    QFrame *m_frame;
    QToolButton *m_toolButton;
    QMenu *m_menu;
    SKGZoomSelector *m_zoomMenu;
    double m_zoomRatio;
    QSize m_initialSize;
    QFrame *m_line;
    QAction *m_menuRename;
    QString m_title;
    QString m_titleDefault;
};

#endif // SKGBOARDWIDGET_H
