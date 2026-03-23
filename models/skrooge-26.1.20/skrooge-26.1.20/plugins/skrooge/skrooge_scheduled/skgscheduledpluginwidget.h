/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSCHEDULEDPLUGINWIDGET_H
#define SKGSCHEDULEDPLUGINWIDGET_H
/** @file
 * A skrooge plugin to manage scheduled transactions
 *
 * @author Stephane MANKOWSKI
 */
#include "skgtabpage.h"
#include "ui_skgscheduledpluginwidget_base.h"

class SKGDocumentBank;

/**
 * A skrooge plugin to manage scheduled transactions
 */
class SKGScheduledPluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGScheduledPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument);

    /**
     * Default Destructor
     */
    ~SKGScheduledPluginWidget() override;

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
    void onSelectionChanged();
    void onUpdate();
    void onProcessImmediately();
    void onProcess(bool iImmediately = false);
    void onNbOccurrenceChanged();
    void onJumpToTheOperation();

private:
    Q_DISABLE_COPY(SKGScheduledPluginWidget)

    Ui::skgscheduledplugin_base ui{};
};

#endif // SKGSCHEDULEDPLUGINWIDGET_H
