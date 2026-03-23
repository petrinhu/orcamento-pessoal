/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGMONTHLYPLUGINWIDGET_H
#define SKGMONTHLYPLUGINWIDGET_H
/** @file
 * A plugin for monthly report
 *
 * @author Stephane MANKOWSKI
 */
#include "skginterfaceplugin.h"
#include "skgtabpage.h"
#include "ui_skgmonthlypluginwidget_base.h"

/**
 * A plugin for monthly report
 */
class SKGMonthlyPluginWidget : public SKGTabPage
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGMonthlyPluginWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGMonthlyPluginWidget() override;

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
     * Get attribute name to save the default state
     * MUST BE OVERWRITTEN
     * @return attribute name to save the default state.
     */
    QString getDefaultStateAttribute() override;

    /**
     * Get the main widget
     * @return a widget
     */
    QWidget *mainWidget() override;

    /**
     * Get the current month
     * @return the current month
     */
    virtual QString getPeriod();

private Q_SLOTS:
    void dataModified(const QString &iTableName, int iIdTransaction);
    void onPeriodChanged();
    void onGetNewHotStuff();
    void onPutNewHotStuff();
    void onTemplateChanged();
    void onAddTemplate();
    void onDeleteTemplate();

    QString getReport();

private:
    Q_DISABLE_COPY(SKGMonthlyPluginWidget)
    void fillTemplateList();

    Ui::skgmonthlyplugin_base ui{};

    QAction *m_upload;
};

#endif // SKGMonthlyPLUGINWIDGET_H
