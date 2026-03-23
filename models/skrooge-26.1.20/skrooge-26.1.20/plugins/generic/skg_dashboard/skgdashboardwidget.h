/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDASHBOARDWIDGET_H
#define SKGDASHBOARDWIDGET_H
/** @file
 * A dashboard widget
 *
 * @author Stephane MANKOWSKI
 */
#include <qlist.h>
#include <qstringlist.h>
#include <qtimer.h>

#include "skgwidget.h"

class QMenu;
class SKGInterfacePlugin;
class SKGFlowLayout;
class SKGBoardWidget;

/**
 * A dashboard
 */
class SKGDashboardWidget : public SKGWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     * @param iMenu the menu where to add options
     */
    explicit SKGDashboardWidget(QWidget *iParent, SKGDocument *iDocument, QMenu *iMenu = nullptr);

    /**
     * Default Destructor
     */
    ~SKGDashboardWidget() override;

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
     * Get the printable widgets.
     * The default implementation returns the main widget.
     * @return the printable widgets.
     */
    QList<QWidget *> printableWidgets();

protected:
    /**
     * Event filtering
     * @param iObject object
     * @param iEvent event
     * @return In your reimplementation of this function, if you want to filter the event out, i.e. stop it being handled further, return true; otherwise return
     * false.
     */
    bool eventFilter(QObject *iObject, QEvent *iEvent) override;

Q_SIGNALS:
    /**
     * When an applet id added
     */
    void appletAdded(const QString & /*_t1*/);

private Q_SLOTS:
    void showHeaderMenu(QPoint iPos);
    void onAddWidget();
    void onChangeLayout();
    void onRemoveWidget();
    void onMoveWidget(int iMove);

private:
    Q_DISABLE_COPY(SKGDashboardWidget)

    void addItem(SKGInterfacePlugin *iDashboard, int iIndex, int iZoom = -10, const QString &iState = QString());
    void moveItem(int iFrom, int iTo);
    static SKGBoardWidget *parentBoardWidget(QWidget *iWidget);

    QLayout *m_flowLayout;

    QStringList m_items;
    QList<SKGBoardWidget *> m_itemsPointers;
    QMenu *m_menu;
    QMenu *m_addMenu;
    QAction *m_layoutF;
    QAction *m_layout1;
    QAction *m_layout2;
    QAction *m_layout3;
    QAction *m_layout4;
    QWidget *m_content;
    QTimer m_timer;
    int m_layout;

    QPoint m_clickedPoint;
    QPoint m_lastPoint;
};

#endif // SKGDASHBOARDWIDGET_H
