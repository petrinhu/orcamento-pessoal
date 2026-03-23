/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGOBJECTMODELBASE_H
#define SKGOBJECTMODELBASE_H
/** @file
 * This file defines classes skgbjectmodelbase.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qabstractitemmodel.h>
#include <qstringlist.h>

#include "skgbasegui_export.h"
#include "skgdocument.h"
#include "skgobjectbase.h"

class QWidget;
/**
 * The Table model managing SKGObjectBase
 */
class SKGBASEGUI_EXPORT SKGObjectModelBase : public QAbstractItemModel
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
    SKGObjectModelBase(SKGDocument *iDocument,
                       const QString &iTable,
                       QString iWhereClause,
                       QWidget *iParent,
                       QString iParentAttribute = QString(),
                       bool iResetOnCreation = true);

    /**
     * Destructor
     */
    ~SKGObjectModelBase() override;

    /**
     * Allows to block a refresh
     * @param iBlocked blocking status
     * @return previous value
     */
    virtual bool blockRefresh(bool iBlocked);

    /**
     * To know if the refresh is blocked
     * @return true or false
     */
    virtual bool isRefreshBlocked();

    /**
     * Returns true if parent has at least one child.
     * @param iParent the parent
     * @return true or false
     */
    bool hasChildren(const QModelIndex &iParent = QModelIndex()) const override;

    /**
     * Returns the number of row for the given parent.
     * @param iParent the parent
     * @return the number of row
     */
    int rowCount(const QModelIndex &iParent = QModelIndex()) const override;

    /**
     * Returns the index of the item in the model specified by the given row, column and parent index.
     * @param row row
     * @param column column
     * @param iParent parent
     * @return the index
     */
    QModelIndex index(int row, int column, const QModelIndex &iParent = QModelIndex()) const override;

    /**
     * Returns the parent of the model item with the given index, or QModelIndex() if it has no parent.
     * @param iIndex index
     * @return the parent
     */
    QModelIndex parent(const QModelIndex &iIndex) const override;

    /**
     * Returns the number of columns for the given parent.
     * @param iParent the parent
     * @return the number of column
     */
    int columnCount(const QModelIndex &iParent = QModelIndex()) const override;

    /**
     * Returns the data stored under the given role for the item referred to by the index.
     * @param iIndex the index
     * @param iRole the role
     * @return the returned value
     */
    QVariant data(const QModelIndex &iIndex, int iRole = Qt::DisplayRole) const override;

    /**
     * Returns the data stored under the given role for the item referred to by the index.
     * @param iIndex the index
     * @param iRole the role
     * @return the returned value
     */
    virtual QVariant computeData(const QModelIndex &iIndex, int iRole = Qt::DisplayRole) const;

    /**
     * Returns the SKGObjectBase for the item referred to by the index.
     * @param iIndex the index
     * @return the returned SKGObjectBase
     */
    virtual SKGObjectBase getObject(const QModelIndex &iIndex) const;

    /**
     * Returns the SKGObjectBase for the item referred to by the index.
     * @param iIndex the index
     * @return the returned SKGObjectBase. Do not delete it !
     */
    virtual SKGObjectBase *getObjectPointer(const QModelIndex &iIndex) const;

    /**
     * Returns the data for the given role and section in the header with the specified orientation.
     * @param iSection the section
     * @param iOrientation the orientation
     * @param iRole the role
     * @return the header data
     */
    QVariant headerData(int iSection, Qt::Orientation iOrientation, int iRole = Qt::DisplayRole) const override;

    /**
     * Returns the item flags for the given index.
     * @param iIndex index of the object
     * @return flags of the given index
     */
    Qt::ItemFlags flags(const QModelIndex &iIndex) const override;

    /**
     * Sets the role data for the item at index to value. Returns true if successful; otherwise returns false.
     * @param iIndex index of the object
     * @param iValue value
     * @param iRole role
     * @return
     */
    bool setData(const QModelIndex &iIndex, const QVariant &iValue, int iRole = Qt::EditRole) override;

    /**
     * Return the index of the attribute, -1 if not found
     * @param iAttributeName the attribute name
     * @return index of this attribute
     */
    virtual int getIndexAttribute(const QString &iAttributeName) const;

    /**
     * Return the attribute for an index
     * @param iIndex the index
     * @return attribute
     */
    virtual QString getAttribute(int iIndex) const;

    /**
     * Return the type of attribute for an index
     * @param iIndex the index
     * @return type of attribute
     */
    virtual SKGServices::AttributeType getAttributeType(int iIndex) const;

    /**
     * Set the table. Do not forget to do a reset after that.
     * @param iTable the table name
     */
    void setTable(const QString &iTable);

    /**
     * Get table name
     * @return table name
     */
    virtual QString getTable() const;

    /**
     * Set the attribute used for grouping
     * @param iAttribute the attribute name
     */
    virtual void setGroupBy(const QString &iAttribute = QString());

    /**
     * Get the attribute used for grouping
     * @return attribute name
     */
    virtual QString getGroupBy() const;

    /**
     * Get the attribute used for parent / child relationship
     * @return attribute name
     */
    virtual QString getParentChildAttribute() const;

    /**
     * Get real table name
     * @return real table name
     */
    virtual QString getRealTable() const;

    /**
     * Get where clause
     * @return where clause
     */
    virtual QString getWhereClause() const;

    /**
     * Get document
     * @return document
     */
    virtual SKGDocument *getDocument() const;

    /**
     * Set the list of supported attributes. Do not forget to do a reset after that.
     * @param iListAttribute the list of attributes. If the list is empty then the default list is set.
     * The format of this string is the following one: attribute name[|visibility Y or N[|size]];attribute name[|visibility Y or N[|size]];…
     */
    virtual void setSupportedAttributes(const QStringList &iListAttribute);

    /**
     * Set the filter. Do not forget to do a reset after that.
     * @param iWhereClause the where clause
     * @return true is the filter is really changed
     */
    virtual bool setFilter(const QString &iWhereClause);

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
     * Get list of supported schemas
     * @return list of schemas @see SKGModelTemplate
     */
    virtual SKGDocument::SKGModelTemplateList getSchemas() const;

    /**
     * Returns a list of MIME types that can be used to describe a list of model indexes.
     * @return list of mime types
     */
    QStringList mimeTypes() const override;

    /**
     * Returns an object that contains serialized items of data corresponding to the list of indexes specified.
     *The formats used to describe the encoded data is obtained from the mimeTypes() function.
     * If the list of indexes is empty, or there are no supported MIME types, 0 is returned rather than a serialized empty list.
     * @param iIndexes the index object
     * @return the mime data
     */
    QMimeData *mimeData(const QModelIndexList &iIndexes) const override;

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
Q_SIGNALS:
    /**
     * Emitted before reset
     */
    void beforeReset();

    /**
     * Emitted after reset
     */
    void afterReset();

public Q_SLOTS:
    /**
     * Refresh the model.
     */
    virtual void refresh();

    /**
     * data are modified
     * @param iTableName table name
     * @param iIdTransaction the id of the transaction for direct modifications of the table (update of modify objects is enough)
     *or 0 in case of modifications by impact (full table must be refreshed)
     */
    virtual void dataModified(const QString &iTableName = QString(), int iIdTransaction = 0);

protected Q_SLOTS:
    /**
     * This method is called by refresh to build the cache (to improve performance)
     */
    virtual void buidCache();

protected:
    /**
     *list of attributes
     */
    QStringList m_listAttibutes;

    /**
     *list of attributes
     */
    QList<SKGServices::AttributeType> m_listAttributeTypes;

    /**
     *list of supported schemas
     */
    SKGDocument::SKGModelTemplateList m_listSchema;

    /**
     *To disable/enable reset
     */
    bool m_isResetRealyNeeded;

    /**
     *A cache for value computed in data
     */
    QMap<QString, QVariant> *m_cache;

    /**
     * Get the attribute value for grouping
     * @param iObject the object
     * @param iAttribute the attribute name
     * @return the value of the attribute
     */
    virtual QString getAttributeForGrouping(const SKGObjectBase &iObject, const QString &iAttribute) const;

    /**
     *The negative color
     */
    QVariant m_fontNegativeColor;

    /**
     * Get the string of an amount
     * @param iValue the value
     * @return the string
     */
    virtual QString formatMoney(double iValue) const;

private Q_SLOTS:
    void pageChanged();

private:
    Q_DISABLE_COPY(SKGObjectModelBase)
    void clear();

    SKGDocument *m_document;
    QString m_table;
    QString m_realTable;
    QString m_whereClause;
    QString m_parentAttribute;
    QString m_groupby;

    SKGObjectBase::SKGListSKGObjectBase m_listObjects;
    QHash<int, SKGIntList> m_parentChildRelations;
    QHash<int, int> m_childParentRelations;
    QHash<int, SKGObjectBase *> m_objectsHashTable;
    QHash<int, int> m_objectsHashTableRows;

    QStringList m_listSupported;
    QList<bool> m_listVisibility;
    QList<int> m_listSize;
    bool m_doctransactionTable;
    bool m_nodeTable;
    bool m_parametersTable;
    bool m_refreshBlocked;
};

#endif
