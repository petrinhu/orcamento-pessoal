/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGCALCULATORPLUGINWIDGET_H
#define SKGCALCULATORPLUGINWIDGET_H
/** @file
 * A skrooge plugin to calculate
 *
 * @author Stephane MANKOWSKI
 */
#include "skgtabpage.h"
#include "ui_skgcalculatorpluginwidget_base.h"
#include <qtimer.h>

class SKGObjectModel;

/**
 * A skrooge plugin to calculate
 */
class SKGCalculatorPluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGCalculatorPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument);

    /**
     * Default Destructor
     */
    ~SKGCalculatorPluginWidget() override;

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
     * Get the current selection
     * @return selected objects
     */
    SKGObjectBase::SKGListSKGObjectBase getSelectedObjects() override;

    /**
     * Get the number of selected object
     * @return number of selected objects
     */
    int getNbSelectedObjects() override;

    /**
     * Get the zoomable widget.
     * The default implementation returns the main widget.
     * @return the zoomable widget.
     */
    QWidget *zoomableWidget() override;

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
    void dataModified(const QString &iTableName, int iIdTransaction);
    void onBtnModeClicked(int mode);
    void onAmortizationComputationDelayed();
    void onAmortizationComputation();
    //         void onCalculatorReturnPressed(const QString& iText);
    //         void onCalculatorListClicked(const QString& iText);
    void onSelectionChanged();
    void onAdd();
    void onUpdate();
    void onFilterChanged();
    void onSelectedInterestChanged();

private:
    Q_DISABLE_COPY(SKGCalculatorPluginWidget)
    void computeInterest();

    Ui::skgcalculatorplugin_base ui{};
    SKGObjectModel *m_objectModel;
    QTimer m_timer;
};

#endif // SKGCALCULATORPLUGINWIDGET_H
