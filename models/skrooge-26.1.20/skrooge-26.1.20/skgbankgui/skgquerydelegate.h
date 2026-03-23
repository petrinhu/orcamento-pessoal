/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGQUERYDELEGATE_H
#define SKGQUERYDELEGATE_H
/** @file
 * This is a delegate for query creator.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qitemdelegate.h>

#include "skgbankgui_export.h"

class SKGDocument;

/**
 * This is a delegate for query creator
 */
class SKGBANKGUI_EXPORT SKGQueryDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     */
    explicit SKGQueryDelegate(QObject *iParent, SKGDocument *iDoc, bool iModeUpdate = false, QStringList iListAtt = QStringList());

    /**
     * Default Destructor
     */
    ~SKGQueryDelegate() override;

    /**
     * Returns the widget used to edit the item specified by index for editing.
     * The parent widget and style option are used to control how the editor widget appears.
     * @param iParent perant widget
     * @param option options
     * @param index index
     * @return the widget
     */
    QWidget *createEditor(QWidget *iParent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    /**
     * Sets the data to be displayed and edited by the editor from the data model item specified by the model index.
     * @param editor the editor
     * @param index the index
     */
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    /**
     * Gets data from the editor widget and stores it in the specified model at the item inde
     * @param editor the editor
     * @param model the model
     * @param index the index
     */
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private Q_SLOTS:
    void commitAndCloseEditor();

private:
    Q_DISABLE_COPY(SKGQueryDelegate)

    SKGDocument *m_document;
    bool m_updateMode;
    QStringList m_listAtt;
};

#endif // SKGQUERYDELEGATE_H
