/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTIPOFDAYBOARDWIDGET_H
#define SKGTIPOFDAYBOARDWIDGET_H
/** @file
 * This file is a plugin for tip of day.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgboardwidget.h"
#include "ui_skgtipofdayboardwidget.h"

/**
 * This file is a plugin for tip of day
 */
class SKGTipOfDayBoardWidget : public SKGBoardWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGTipOfDayBoardWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGTipOfDayBoardWidget() override;

private Q_SLOTS:
    void onModified();

private:
    Q_DISABLE_COPY(SKGTipOfDayBoardWidget)

    Ui::skgtipofdayboardwidget ui{};
};

#endif // SKGTIPOFDAYBOARDWIDGET_H
