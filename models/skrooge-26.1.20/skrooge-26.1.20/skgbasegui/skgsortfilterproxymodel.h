/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSORTFILTERPROXYMODEL_H
#define SKGSORTFILTERPROXYMODEL_H
/** @file
 * This file is a proxy model with better filter mechanism.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbasegui_export.h"
#include <qsortfilterproxymodel.h>

class SKGSortFilterProxyModelPrivate;

/**
 * This class is a proxy model with better filter mechanism
 */
class SKGBASEGUI_EXPORT SKGSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    /**
     * The previous sort column
     */
    Q_PROPERTY(int previousSortColumn READ getPreviousSortColumn WRITE setPreviousSortColumn USER true NOTIFY previousSortColumnModified)

public:
    /**
     * Constructor
     * @param iParent parent widget
     */
    explicit SKGSortFilterProxyModel(QObject *iParent = nullptr);

    /**
     * Destructor
     */
    ~SKGSortFilterProxyModel() override;

    /**
     * @brief Set the previous sort column (-1 = none).
     *
     * @param iCol the column index
     */
    virtual void setPreviousSortColumn(int iCol);

    /**
     * @brief Get the previous sort column (-1 = none).
     * @return the column index
     */
    virtual int getPreviousSortColumn() const;

protected:
    /**
     * To know if a row must be displayed or not
     * @param source_row source row
     * @param source_parent prent
     * @return true of false
     */
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

    /**
     * To sort items
     * @param left left item
     * @param right right item
     * @return true of false
     */
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

Q_SIGNALS:
    /**
     * This signal is launched when the property is modified
     */
    void previousSortColumnModified();

private:
    bool filterAcceptsRowWords(int source_row, const QModelIndex &source_parent, const QStringList &iWords) const;

    bool lessThan(const QVariant &iLeftData, const QVariant &iRightData) const;
    bool moreThan(const QVariant &iLeftData, const QVariant &iRightData) const;

    SKGSortFilterProxyModelPrivate *const d;
};

#endif
