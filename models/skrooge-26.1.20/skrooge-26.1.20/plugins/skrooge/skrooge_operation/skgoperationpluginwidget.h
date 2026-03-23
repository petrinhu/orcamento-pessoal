/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGOPERATIONPLUGINWIDGET_H
#define SKGOPERATIONPLUGINWIDGET_H
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgoperationobject.h"
#include "skgsuboperationobject.h"
#include "skgtabpage.h"
#include "ui_skgoperationpluginwidget_base.h"
#include <qdom.h>

class SKGDocumentBank;
class SKGObjectModel;
class SKGSplitTableDelegate;
class SKGTreeView;
class QAction;

/**
 * This file is Skrooge plugin for transaction management
 */
class SKGOperationPluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGOperationPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument);

    /**
     * Default Destructor
     */
    ~SKGOperationPluginWidget() override;

    /**
     * Get the table view
     * @return the table view
     */
    virtual SKGTreeView *getTableView();

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
     * Set or not the template mode
     * @param iTemplate the template mode
     */
    virtual void setTemplateMode(bool iTemplate);

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
    void onOperationCreatorModified();
    void onPayeeChanged();
    void onAddOperationClicked();
    void onUpdateOperationClicked();
    void onFilterChanged();
    void onAccountChanged();
    void onSelectionChanged();
    void onFocusChanged();
    void onFastEdition();
    void onDoubleClick();
    void onQuantityChanged();
    // cppcheck-suppress passedByValue
    void onDateChanged(QDate iDate);
    void onSubopCellChanged(int row, int column);
    void onRemoveSubOperation(int iRow);
    void onRefreshInformationZone();
    void onRefreshInformationZoneDelayed();
    void onRotateAccountTools();
    void onValidateMarkedOperations();
    void onBtnModeClicked(int mode);
    void onAutoPoint();
    void onAddFakeOperation();
    void onFreeze();
    void refreshSubOperationAmount();
    void cleanEditor();
    void displayReconciliationInfo();
    void displayBalance();
    void fillTargetAccount();

private:
    Q_DISABLE_COPY(SKGOperationPluginWidget)

    SKGError getSelectedOperation(SKGOperationObject &operation);

    bool isWidgetEditionEnabled(QWidget *iWidget);
    void setWidgetEditionEnabled(QWidget *iWidget, bool iEnabled);
    void setAllWidgetsEnabled();

    bool isTemplateMode();
    void displaySubOperations();
    // cppcheck-suppress passedByValue
    void displaySubOperations(const SKGOperationObject &iOperation, bool iKeepId = true, QDate iSubOperationsDate = QDate());
    double getRemainingQuantity();
    // cppcheck-suppress passedByValue
    void addSubOperationLine(int row,
                             QDate date,
                             const QString &category,
                             const QString &tracker,
                             const QString &comment,
                             double quantity,
                             const QString &formula,
                             int id = 0);
    SKGError updateSelection(const SKGObjectBase::SKGListSKGObjectBase &iSelection, bool iForceCreation = false);
    void fillNumber();
    QString getAttributeOfSelection(const QString &iAttribute);
    QString currentAccount();

    Ui::skgoperationplugin_base ui{};
    SKGObjectModel *m_objectModel;
    QString m_operationWhereClause;
    QString m_previousValue;
    QDomDocument m_lastState;

    QAction *m_fastEditionAction;

    QString m_lastFastEditionWhereClause;
    int m_lastFastEditionOperationFound;
    bool m_showClosedAccounts;

    bool m_numberFieldIsNotUptodate;
    int m_modeInfoZone;
    QKeySequence m_deleteShortcut;

    SKGSplitTableDelegate *m_tableDelegate;
    QTimer m_timer;

    QStringList m_attributesForSplit;
    QDate m_previousDate;
};

#endif // SKGOPERATIONPLUGINWIDGET_H
