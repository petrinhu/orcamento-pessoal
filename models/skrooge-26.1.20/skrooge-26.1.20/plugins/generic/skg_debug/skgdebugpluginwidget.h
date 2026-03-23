/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDEBUGPLUGINWIDGET_H
#define SKGDEBUGPLUGINWIDGET_H
/** @file
 * This file is a plugin for debug.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtabpage.h"
#include "ui_skgdebugpluginwidget_base.h"

/**
 * This file is a plugin for debug
 */
class SKGDebugPluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGDebugPluginWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGDebugPluginWidget() override;

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

private Q_SLOTS:
    void onExecuteSqlOrderInTransaction();
    void onExecuteSqlOrder(bool iInTransaction = false);
    void onTraceLevelModified();
    void onProfilingModeChanged();
    void onModeChanged();
    void onRefreshViewsAndIndexes();
    SKGError executeSqlOrders(const QStringList &iSQL, QString &oOutput);

private:
    Q_DISABLE_COPY(SKGDebugPluginWidget)

    Ui::skgdebugplugin_base ui{};
};

#endif // SKGDEBUGPLUGINWIDGET_H
