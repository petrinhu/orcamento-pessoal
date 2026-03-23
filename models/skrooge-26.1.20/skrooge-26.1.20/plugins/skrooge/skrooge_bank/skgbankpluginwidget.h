/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBANKPLUGINWIDGET_H
#define SKGBANKPLUGINWIDGET_H
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qstringlist.h>
#include <qtimer.h>

#include "skgaccountobject.h"
#include "skgtabpage.h"
#include "ui_skgbankpluginwidget_base.h"

class SKGBoardWidget;
class SKGDocumentBank;

/**
 * This file is Skrooge plugin for bank management
 */
class SKGBankPluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGBankPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument);

    /**
     * Default Destructor
     */
    ~SKGBankPluginWidget() override;

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
     * Get the printable widgets.
     * The default implementation returns the main widget.
     * @return the printable widgets.
     */
    QList<QWidget *> printableWidgets() override;

    /**
     * To know if this page contains an editor. MUST BE OVERWRITTEN
     * @return the editor state
     */
    bool isEditor() override;

    /**
     * To activate the editor by setting focus on right widget. MUST BE OVERWRITTEN
     */
    void activateEditor() override;

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
    void onIconChanged();
    void onAccountCreatorModified();
    void onAddAccountClicked();
    void onModifyAccountClicked();
    void onSelectionChanged();
    void onRefreshGraphDelayed();
    void onRefreshGraph();
    void cleanEditor();
    void refreshInfoZone();
    SKGError setInitialBalanceFromEditor(SKGAccountObject &iAccount);

private:
    Q_DISABLE_COPY(SKGBankPluginWidget)

    Ui::skgbankplugin_base ui{};
    QTimer m_timer;
    QTimer m_timer2;
    SKGBoardWidget *m_graph;
    QString m_graphState;
};

#endif // SKGDEBUGPLUGIN_H
