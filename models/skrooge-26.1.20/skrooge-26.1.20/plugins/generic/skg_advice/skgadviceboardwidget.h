/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGADVICEBOARDWIDGET_H
#define SKGADVICEBOARDWIDGET_H
/** @file
 * This file is plugin for advice.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgboardwidget.h"

class QFormLayout;
class QAction;
class QPushButton;

/**
 * This file is plugin for advice
 */
class SKGAdviceBoardWidget : public SKGBoardWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document*
     */
    explicit SKGAdviceBoardWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGAdviceBoardWidget() override;

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

Q_SIGNALS:
    void refreshNeeded();

private Q_SLOTS:
    void pageChanged();
    void dataModifiedNotForce();
    void dataModifiedForce();
    void dataModified(bool iForce = false);
    void adviceClicked();
    void activateAllAdvice();
    void moreAdvice();
    void lessAdvice();
    void applyRecommended();

private:
    Q_DISABLE_COPY(SKGAdviceBoardWidget)

    int m_maxAdvice;
    bool m_refreshNeeded;

    QAction *m_menuAuto;
    QPushButton *m_refresh;
    QFormLayout *m_layout;
    QList<QAction *> m_recommendedActions;
    bool m_inapplyall;
};

#endif // SKGADVICEBOARDWIDGET_H
