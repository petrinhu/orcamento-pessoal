/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTABWIDGET_H
#define SKGTABWIDGET_H
/** @file
 * A QTabWidget with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qhash.h>
#include <qtabwidget.h>
#include <qtimer.h>

#include "skgbasegui_export.h"

class QPushButton;

/**
 * A QTabWidget with more features.
 */
class SKGBASEGUI_EXPORT SKGTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGTabWidget(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGTabWidget() override;

public Q_SLOTS:
    /**
     * Remove a tab
     * @param index the tab index
     */
    virtual void removeTab(int index);

private Q_SLOTS:
    void onCurrentChanged();
    void onRefreshSaveIcon();
    void onSaveRequested();
    void onMoveTab(int oldPos, int newPos);

private:
    QTimer m_timerSave;

    QHash<QWidget *, QPushButton *> m_tabIndexSaveButton;
};

#endif // SKGTABWIDGET_H
