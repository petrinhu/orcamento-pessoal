/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDASHBOARDBOARDWIDGET_H
#define SKGDASHBOARDBOARDWIDGET_H
/** @file
 * A dashboard widget
 *
 * @author Stephane MANKOWSKI
 */

#include "skgboardwidget.h"

class SKGInterfacePlugin;
class SKGDashboardWidget;

/**
 * A dashboard
 */
class SKGDashboardboardWidget : public SKGBoardWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGDashboardboardWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGDashboardboardWidget() override;

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

private:
    Q_DISABLE_COPY(SKGDashboardboardWidget)

    SKGDashboardWidget *m_widget;
};

#endif // SKGDASHBOARDBOARDWIDGET_H
