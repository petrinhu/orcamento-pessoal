/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSPLITTABLEDELEGATE_H
#define SKGSPLITTABLEDELEGATE_H
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qitemdelegate.h>

class SKGDocument;
class SKGTableWidget;

/**
 * This file is Skrooge plugin for transaction management
 */
class SKGSplitTableDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     * @param iParent parent widget
     * @param iDoc the document
     * @param iListAttribut the list of corresponding attribute
     */
    explicit SKGSplitTableDelegate(QObject *iParent, SKGDocument *iDoc, QStringList iListAttribut = QStringList());

    /**
     * Default Destructor
     */
    ~SKGSplitTableDelegate() override;

    /**
     * Returns the widget used to edit the item specified by index for editing.
     * The parent widget and style option are used to control how the editor widget appears.
     * @param iParent parent widget
     * @param option options
     * @param index index
     * @return the widget
     */
    QWidget *createEditor(QWidget *iParent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    /**
     * Set the data to be shown in the delegate. For the quantity column,
     * We pass the text value. This allows the user to correct an invalid
     * expression without having to retype everything.
     * @param editor
     * @param index
     */
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    /**
     * Get the data to be shown in the model. For the quantity column,
     * We pass the text value. If the entered expression was incorrect,
     * we display it in red so that the user can see it is wrong and
     * correct it
     * @param editor
     * @param model
     * @param index
     */
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    /**
     * Add a parameter
     * @param iParameter the parameter name
     * @param iValue the value
     */
    virtual void addParameterValue(const QString &iParameter, double iValue);

private:
    Q_DISABLE_COPY(SKGSplitTableDelegate)

    SKGDocument *m_document;
    QMap<QString, double> m_parameters;
    QStringList m_listAttributes;
    SKGTableWidget *m_table;
};

#endif // SKGSPLITTABLEDELEGATE_H
