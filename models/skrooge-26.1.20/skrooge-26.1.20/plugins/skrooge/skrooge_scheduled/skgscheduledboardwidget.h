/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSCHEDULEDBOARDWIDGET_H
#define SKGSCHEDULEDBOARDWIDGET_H
/** @file
 * This file is Skrooge plugin for scheduled transaction management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skghtmlboardwidget.h"

/**
 * This file is Skrooge plugin for scheduled transaction management
 */
class SKGScheduledBoardWidget : public SKGHtmlBoardWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGScheduledBoardWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGScheduledBoardWidget() override;

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

protected Q_SLOTS:
    void dataModified(const QString &iTableName = QString(), int iIdTransaction = 0) override;

private:
    Q_DISABLE_COPY(SKGScheduledBoardWidget)
    SKGComboBox *m_daysmax;
};

#endif // SKGSCHEDULEDBOARDWIDGET_H
