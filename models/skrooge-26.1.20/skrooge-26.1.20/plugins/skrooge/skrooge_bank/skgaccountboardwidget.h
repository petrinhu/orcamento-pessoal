/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGACCOUNTBOARDWIDGET_H
#define SKGACCOUNTBOARDWIDGET_H
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgboardwidget.h"

class QAction;

/**
 * This file is Skrooge plugin for bank management
 */
class SKGAccountBoardWidget : public SKGBoardWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGAccountBoardWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGAccountBoardWidget() override;

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
    void pageChanged();
    void dataModified(const QString &iTableName = QString(), int iIdTransaction = 0);

private:
    Q_DISABLE_COPY(SKGAccountBoardWidget)

    QAction *m_menuAssets;
    QAction *m_menuCurrent;
    QAction *m_menuCreditCard;
    QAction *m_menuSaving;
    QAction *m_menuInvestment;
    QAction *m_menuWallet;
    QAction *m_menuLoan;
    QAction *m_menuPension;
    QAction *m_menuOther;
    QAction *m_menuFavorite;
    QAction *m_menuPastOperations;
    bool m_refreshNeeded;
    QLabel *m_label;
};

#endif // SKGACCOUNTBOARDWIDGET_H
