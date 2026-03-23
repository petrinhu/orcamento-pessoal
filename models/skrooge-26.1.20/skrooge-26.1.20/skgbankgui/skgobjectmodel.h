/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGOBJECTMODEL_H
#define SKGOBJECTMODEL_H
/** @file
 * This file defines classes SKGObjectModel.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qcolor.h>
#include <qfont.h>

#include "skgbankgui_export.h"
#include "skgdocumentbank.h"
#include "skgobjectbase.h"
#include "skgobjectmodelbase.h"

/**
 * The Table model managing SKGObjectBase
 */
class SKGBANKGUI_EXPORT SKGObjectModel : public SKGObjectModelBase
{
    Q_OBJECT
public:
    /**
     * Default constructor
     * @param iDocument the document where to search
     * @param iTable the table where to search
     * @param iWhereClause the where clause
     * @param iParent parent QT object
     * @param iParentAttribute the attribute to find the parent of an object clause to find children
     * @param iResetOnCreation to reset data during creation
     */
    explicit SKGObjectModel(SKGDocumentBank *iDocument,
                            const QString &iTable,
                            const QString &iWhereClause,
                            QWidget *iParent,
                            const QString &iParentAttribute = QString(),
                            bool iResetOnCreation = true);

    /**
     * Destructor
     */
    ~SKGObjectModel() override;

    /**
     * Returns the data stored under the given role for the item referred to by the index.
     * @param iIndex the index
     * @param iRole the role
     * @return the returned value
     */
    QVariant computeData(const QModelIndex &iIndex, int iRole = Qt::DisplayRole) const override;

    /**
     * Sets the role data for the item at index to value. Returns true if successful; otherwise returns false.
     * @param iIndex index of the object
     * @param iValue value
     * @param iRole role
     * @return
     */
    bool setData(const QModelIndex &iIndex, const QVariant &iValue, int iRole = Qt::EditRole) override;

    /**
     * Returns the data for the given role and section in the header with the specified orientation.
     * @param section the section
     * @param orientation the orientation
     * @param role the role
     * @return the header data
     */
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * Returns the item flags for the given index.
     * @param iIndex index of the object
     * @return flags of the given index
     */
    Qt::ItemFlags flags(const QModelIndex &iIndex) const override;

    /**
     * Returns the actions supported by the data in this model.
     * @return Qt::DropActions
     */
    Qt::DropActions supportedDragActions() const override;

    /**
     * Returns the actions supported by the data in this model.
     * @return Qt::DropActions
     */
    Qt::DropActions supportedDropActions() const override;

    /**
     * Handles the data supplied by a drag and drop transaction that ended with the given action.
     * Returns true if the data and action can be handled by the model; otherwise returns false.
     * Although the specified row, column and parent indicate the location of an item in the model where the transaction ended,
     *it is the responsibility of the view to provide a suitable location for where the data should be inserted.
     * @param iData mime data
     * @param iAction action
     * @param iRow row
     * @param iColumn column
     * @param iParent parent
     * @return true if the dropping was successful otherwise false.
     */
    bool dropMimeData(const QMimeData *iData, Qt::DropAction iAction, int iRow, int iColumn, const QModelIndex &iParent) override;

protected:
    /**
     * Get the attribute value for grouping
     * @param iObject the object
     * @param iAttribute the attribute name
     * @return the value of the attribute
     */
    QString getAttributeForGrouping(const SKGObjectBase &iObject, const QString &iAttribute) const override;

    /**
     * Get the string of an amount
     * @param iValue the value
     * @return the string
     */
    QString formatMoney(double iValue) const override;

protected Q_SLOTS:
    /**
     * This method is called by refresh to build the cache (to improve performance)
     */
    void buidCache() override;

    /**
     * data are modified
     * @param iTableName table name
     * @param iIdTransaction the id of the transaction for direct modifications of the table (update of modify objects is enough)
     *or 0 in case of modifications by impact (full table must be refreshed)
     */
    void dataModified(const QString &iTableName, int iIdTransaction) override;

private:
    Q_DISABLE_COPY(SKGObjectModel)

    SKGServices::SKGUnitInfo m_cacheUnit;
    bool m_operationTable;
    bool m_recurrentoperationTable;
    bool m_trackerTable;
    bool m_accountTable;
    bool m_unitTable;
    bool m_unitvalueTable;
    bool m_suboperationTable;
    bool m_categoryTable;
    bool m_ruleTable;
    bool m_interestTable;
    bool m_interestResultTable;
    bool m_payeeTable;
    bool m_budgetTable;
    bool m_budgetRuleTable;

    QVariant m_fontDisabledScheduleColor;
    QVariant m_fontFutureOperationsColor;
    QVariant m_fontNotValidatedOperationsColor;
    QVariant m_fontSubOperationsColor;

    QVariant m_iconTransfer;
    QVariant m_iconGroup;
    QVariant m_iconSplit;
    QVariant m_iconMuchMore;
    QVariant m_iconMuchLess;
    QVariant m_iconMore;
    QVariant m_iconLess;
    QVariant m_iconClosed;
    QVariant m_iconImported;
    QVariant m_iconImportedChecked;
    QVariant m_iconRecurrent;
    QVariant m_iconRecurrentMaster;
    QVariant m_iconFavorite;
    QVariant m_iconCategory;
    QVariant m_iconCategoryPlus;
    QVariant m_iconCategoryMoins;
    QVariant m_iconSearch;
    QVariant m_iconUpdate;
    QVariant m_iconAlarm;
    QVariant m_iconTemplate;

    QVariant m_iconGreen;
    QVariant m_iconRed;
    QVariant m_iconAnber;
};

#endif
