/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGGRAPHICSVIEW_H
#define SKGGRAPHICSVIEW_H
/** @file
 * A Graphic view with more functionalities.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qtimer.h>
#include <qwidget.h>

#include "skgbasegui_export.h"
#include "ui_skggraphicsview.h"

class QGraphicsScene;
class QGraphicsView;
class QMenu;
class QAction;

/**
 * This file is a Graphic view with more functionalities
 */
class SKGBASEGUI_EXPORT SKGGraphicsView : public QWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGGraphicsView(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGGraphicsView() override;

    /**
     * Set current scene
     * @param iScene The scene
     */
    void setScene(QGraphicsScene *iScene);

    /**
     * Get the internal Graphic view
     * @return the internal Graphic view
     */
    QGraphicsView *graphicsView();

    /**
     * Set the visibility of the ToolBar
     * @param iVisibility visibility
     */
    void setToolBarVisible(bool iVisibility);

    /**
     * Get the visibility of the ToolBar
     * @return visibility
     */
    bool isToolBarVisible() const;

    /**
     * Add a widget in the toolbar
     * @param iWidget widget
     */
    void addToolbarWidget(QWidget *iWidget);

    /**
     * Get the current state
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    QString getState() const;

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    void setState(const QString &iState);

    /**
     * Get a pointer on the contextual menu
     * @return contextual menu
     */
    QMenu *getContextualMenu() const;

public Q_SLOTS:
    /**
     * Set the zoom
     */
    void onZoom();

    /**
     * Print
     */
    void onPrint();

    /**
     * Export to a file
     * @param iFileName the file name
     */
    void exportInFile(const QString &iFileName);

    /**
     * Export
     */
    void onExport();

    /**
     * Copy
     */
    void onCopy();

    /**
     * Switch tool bar visibility
     */
    void onSwitchToolBarVisibility();

    /**
     * Set antialiasing
     * @param iAntialiasing enabled or disabled
     */
    void setAntialiasing(bool iAntialiasing);

    /**
     * Reinitialize zoom
     */
    void initializeZoom();

Q_SIGNALS:
    /**
     * View is resized
     */
    void resized();

    /**
     * The tool bar visibly changed
     */
    void toolBarVisiblyChanged();

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
    void showMenu(QPoint iPos);

private:
    Q_DISABLE_COPY(SKGGraphicsView)
    Ui::skggraphicview_base ui{};

    double m_oscale;

    QMenu *m_mainMenu{};
    QAction *m_actZoomOriginal{};
    QAction *m_actShowToolBar;
    QAction *m_actAntialiasing{};
    bool m_toolBarVisible;
    QTimer m_timer;
};

#endif // SKGGRAPHICSVIEW_H
