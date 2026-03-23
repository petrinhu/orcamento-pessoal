/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPAYEEPLUGINWIDGET_H
#define SKGPAYEEPLUGINWIDGET_H
/** @file
 * A skrooge plugin to payee transactions
 *
 * @author Stephane MANKOWSKI
 */
#include "skgtabpage.h"
#include "ui_skgpayeepluginwidget_base.h"

/**
 * A skrooge plugin to track transactions
 */
class SKGPayeePluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGPayeePluginWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGPayeePluginWidget() override;

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
    void onSelectionChanged();
    void onEditorModified();
    void onAddPayee();
    void onModifyPayee();
    void cleanEditor();
    void onDeleteUnused();

private:
    Q_DISABLE_COPY(SKGPayeePluginWidget)

    Ui::skgpayeeplugin_base ui{};
};

#endif // SKGPAYEEPLUGINWIDGET_H
