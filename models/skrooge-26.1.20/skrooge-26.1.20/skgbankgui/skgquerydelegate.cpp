/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This is a delegate for query creator
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgquerydelegate.h"

#include <qpainter.h>
#include <qtablewidget.h>

#include "skgmainpanel.h"
#include "skgpredicatcreator.h"
#include "skgtraces.h"

SKGQueryDelegate::SKGQueryDelegate(QObject *iParent, SKGDocument *iDoc, bool iModeUpdate, QStringList iListAtt)
    : QItemDelegate(iParent)
    , m_document(iDoc)
    , m_updateMode(iModeUpdate)
    , m_listAtt(std::move(iListAtt))
{
}

SKGQueryDelegate::~SKGQueryDelegate()
{
    m_document = nullptr;
}

QWidget *SKGQueryDelegate::createEditor(QWidget *iParent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    SKGTRACEINFUNC(1)
    Q_UNUSED(option)

    QTableWidgetItem *it_h = (qobject_cast<QTableWidget *>(this->parent()))->horizontalHeaderItem(index.column());
    QString attname = it_h->data(Qt::UserRole).toString();

    auto editor = new SKGPredicatCreator(iParent, m_document, attname, m_updateMode, m_listAtt);
    connect(editor, &SKGPredicatCreator::editingFinished, this, &SKGQueryDelegate::commitAndCloseEditor);
    return editor;
}

void SKGQueryDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    SKGTRACEINFUNC(1)
    auto *pred = qobject_cast<SKGPredicatCreator *>(editor);
    if (pred != nullptr) {
        pred->setXmlDescription(index.model()->data(index, Qt::UserRole).toString());
    } else {
        QItemDelegate::setEditorData(editor, index);
    }
}

void SKGQueryDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    SKGTRACEINFUNC(1)
    auto *pred = qobject_cast<SKGPredicatCreator *>(editor);
    if ((pred != nullptr) && (model != nullptr)) {
        QString xml = pred->xmlDescription();
        model->setData(index, pred->text(), Qt::DisplayRole);
        model->setData(index, xml, Qt::UserRole);
    } else {
        QItemDelegate::setModelData(editor, model, index);
    }
}

void SKGQueryDelegate::commitAndCloseEditor()
{
    auto *editor = qobject_cast<SKGPredicatCreator *>(sender());
    Q_EMIT commitData(editor);
    Q_EMIT closeEditor(editor);
}
