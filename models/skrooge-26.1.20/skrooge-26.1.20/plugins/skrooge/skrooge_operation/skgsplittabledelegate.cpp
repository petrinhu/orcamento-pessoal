/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for transaction management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgsplittabledelegate.h"

#include <kformat.h>
#include <ksqueezedtextlabel.h>

#include <qheaderview.h>
#include <qpainter.h>

#include <utility>

#include "skgbasegui_settings.h"
#include "skgcalculatoredit.h"
#include "skgcombobox.h"
#include "skgdateedit.h"
#include "skgmainpanel.h"
#include "skgtablewidget.h"

SKGSplitTableDelegate::SKGSplitTableDelegate(QObject *iParent, SKGDocument *iDoc, QStringList iListAttribut)
    : QItemDelegate(iParent)
    , m_document(iDoc)
    , m_listAttributes(std::move(iListAttribut))
    , m_table(qobject_cast<SKGTableWidget *>(iParent))
{
}

SKGSplitTableDelegate::~SKGSplitTableDelegate()
{
    m_document = nullptr;
}

QWidget *SKGSplitTableDelegate::createEditor(QWidget *iParent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSizePolicy newSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    newSizePolicy.setHorizontalStretch(0);
    newSizePolicy.setVerticalStretch(0);

    if (index.column() == m_listAttributes.indexOf(QStringLiteral("t_category"))) {
        auto editor = new SKGComboBox(iParent);
        editor->setEditable(true);
        if (m_document != nullptr) {
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << editor,
                                                m_document,
                                                QStringLiteral("category"),
                                                QStringLiteral("t_fullname"),
                                                QStringLiteral("t_close='N'"));
        }
        editor->setSizePolicy(newSizePolicy);
        editor->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_table->setColumnWidth(index.column(), editor->sizeHint().width());

        return editor;
    }
    if (index.column() == m_listAttributes.indexOf(QStringLiteral("t_comment"))) {
        auto editor = new SKGComboBox(iParent);
        editor->setEditable(true);
        if (m_document != nullptr) {
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << editor,
                                                m_document,
                                                QStringLiteral("v_operation_all_comment"),
                                                QStringLiteral("t_comment"),
                                                QString());
        }
        editor->setSizePolicy(newSizePolicy);
        editor->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_table->setColumnWidth(index.column(), editor->sizeHint().width());

        return editor;
    }
    if (index.column() == m_listAttributes.indexOf(QStringLiteral("f_value"))) {
        auto editor = new SKGCalculatorEdit(iParent);
        editor->setMode(SKGCalculatorEdit::EXPRESSION);
        QMapIterator<QString, double> i(m_parameters);
        while (i.hasNext()) {
            i.next();
            editor->addParameterValue(i.key(), i.value());
        }
        editor->setSizePolicy(newSizePolicy);
        m_table->setColumnWidth(index.column(), editor->sizeHint().width());

        return editor;
    }
    if (index.column() == m_listAttributes.indexOf(QStringLiteral("t_refund"))) {
        auto editor = new SKGComboBox(iParent);
        editor->setEditable(true);
        if (m_document != nullptr) {
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << editor,
                                                m_document,
                                                QStringLiteral("refund"),
                                                QStringLiteral("t_name"),
                                                QStringLiteral("t_close='N'"));
        }
        editor->setSizePolicy(newSizePolicy);
        editor->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_table->setColumnWidth(index.column(), editor->sizeHint().width());

        return editor;
    }
    if (index.column() == m_listAttributes.indexOf(QStringLiteral("d_date"))) {
        auto editor = new SKGDateEdit(iParent);
        editor->setSizePolicy(newSizePolicy);
        editor->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_table->setColumnWidth(index.column(), editor->sizeHint().width());

        return editor;
    }

    return QItemDelegate::createEditor(iParent, option, index);
}

void SKGSplitTableDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == m_listAttributes.indexOf(QStringLiteral("f_value"))) {
        auto *calculator = qobject_cast<SKGCalculatorEdit *>(editor);
        if (calculator != nullptr) {
            calculator->setText(index.model()->data(index, Qt::ToolTipRole).toString());
        }
    } else if (index.column() == m_listAttributes.indexOf(QStringLiteral("d_date"))) {
        auto *date = qobject_cast<SKGDateEdit *>(editor);
        if (date != nullptr) {
            date->setDate(SKGServices::stringToDate(index.model()->data(index, Qt::ToolTipRole).toString()));
        }
    } else {
        QItemDelegate::setEditorData(editor, index);
    }
}

void SKGSplitTableDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (index.column() == m_listAttributes.indexOf(QStringLiteral("f_value"))) {
        auto *calculator = qobject_cast<SKGCalculatorEdit *>(editor);
        if ((calculator != nullptr) && (model != nullptr)) {
            QString f = calculator->formula();
            QString t = SKGServices::doubleToString(calculator->value());
            if (f.isEmpty()) {
                f = t;
            }
            bool previous = model->blockSignals(true);
            model->setData(index, f, Qt::ToolTipRole);
            model->setData(index, calculator->value(), 101);
            model->blockSignals(previous);
            model->setData(index, t, Qt::DisplayRole);
        }
    } else if (index.column() == m_listAttributes.indexOf(QStringLiteral("d_date"))) {
        auto *date = qobject_cast<SKGDateEdit *>(editor);
        if ((date != nullptr) && (model != nullptr)) {
            QString dateDisplay = SKGMainPanel::dateToString(date->date());
            QString dateInternal = SKGServices::dateToSqlString(date->date());

            model->setData(index, dateInternal, Qt::ToolTipRole);
            model->setData(index, dateDisplay, Qt::DisplayRole);
        }
    } else {
        QItemDelegate::setModelData(editor, model, index);
    }
    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setStretchLastSection(true);
}

void SKGSplitTableDelegate::addParameterValue(const QString &iParameter, double iValue)
{
    m_parameters.insert(iParameter, iValue);
}
