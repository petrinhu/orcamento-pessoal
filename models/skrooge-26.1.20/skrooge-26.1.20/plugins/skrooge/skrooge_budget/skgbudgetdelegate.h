/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBUDGETDELEGATE_H
#define SKGBUDGETDELEGATE_H
/** @file
 * This file is a delegate for budget.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qstyleditemdelegate.h>

class SKGDocument;

/**
 * This file is a delegate for budget
 */
class SKGBudgetDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    explicit SKGBudgetDelegate(QObject *iParent, SKGDocument *iDoc);

    /**
     * Default Destructor
     */
    ~SKGBudgetDelegate() override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    Q_DISABLE_COPY(SKGBudgetDelegate)

    SKGDocument *m_document;
    QString m_negativeStyleSheet;
    QString m_neutralStyleSheet;
    QString m_positiveStyleSheet;
};

#endif // SKGBUDGETDELEGATE_H
