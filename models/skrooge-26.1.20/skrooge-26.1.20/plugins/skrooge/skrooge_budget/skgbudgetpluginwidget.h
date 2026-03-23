/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBUDGETPLUGINWIDGET_H
#define SKGBUDGETPLUGINWIDGET_H
/** @file
 * A skrooge plugin to manage budgets
 *
 * @author Stephane MANKOWSKI
 */
#include "skgtabpage.h"
#include "ui_skgbudgetpluginwidget_base.h"
#include <qtimer.h>

class SKGObjectModel;
class SKGBudgetObject;
class SKGBudgetRuleObject;

/**
 * A skrooge plugin to manage budgets
 */
class SKGBudgetPluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGBudgetPluginWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGBudgetPluginWidget() override;

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
     * To know if this page contains an editor. MUST BE OVERWRITTEN
     * @return the editor state
     */
    bool isEditor() override;

    /**
     * To activate the editor by setting focus on right widget. MUST BE OVERWRITTEN
     */
    void activateEditor() override;

public Q_SLOTS:
    /**
     * Refresh the content.
     */
    virtual void refresh();

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
    void dataModified(const QString &iTableName, int iIdTransaction, bool iLightTransaction = false);
    void onCreatorModified();
    void onAddClicked();
    void onUpdateClicked();
    void onSelectionChanged();
    void onBtnModeClicked(int mode);
    void onTop();
    void onUp();
    void onDown();
    void onBottom();

    void refreshInfoZone();

private:
    Q_DISABLE_COPY(SKGBudgetPluginWidget)

    Ui::skgbudgetplugin_base ui{};

    SKGError updateBudget(SKGBudgetObject &iBudget, int iMonth = -1);
    SKGError updateBudgetRule(SKGBudgetRuleObject &iRule);

    SKGObjectModel *m_objectModel;
    QString m_viewBudget;
    QString m_viewRule;

    QTimer m_timer;
};

#endif // SKGBUDGETPLUGINWIDGET_H
