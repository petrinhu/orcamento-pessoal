/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGUNITBOARDWIDGET_H
#define SKGUNITBOARDWIDGET_H
/** @file
 * This file is Skrooge plugin for unit management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgboardwidget.h"

class QAction;

/**
 * This file is Skrooge plugin for unit management
 */
class SKGUnitBoardWidget : public SKGBoardWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGUnitBoardWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGUnitBoardWidget() override;

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
    void dataModified(const QString &iTableName = QString(), int iIdTransaction = 0);

private:
    Q_DISABLE_COPY(SKGUnitBoardWidget)

    QAction *m_menuFavorite;
    QAction *m_menuCurrencies;
    QAction *m_menuIndexes;
    QAction *m_menuShares;
    QAction *m_menuObjects;
    QAction *m_menuSharesOwnedOnly;

    QLabel *m_label;
};

#endif // SKGUNITBOARDWIDGET_H
