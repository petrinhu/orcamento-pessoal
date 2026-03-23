/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A table widget with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtablewidget.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qevent.h>
#include <qscrollbar.h>

#include <algorithm>

#include "skgtraces.h"
#include <utility>

SKGTableWidget::SKGTableWidget(QWidget *iParent)
    : QTableWidget(iParent)
    , stickH(false)
    , stickV(false)
{
    this->installEventFilter(this);
    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &SKGTableWidget::onActionTriggered);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &SKGTableWidget::onActionTriggered);
    connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &SKGTableWidget::onRangeChanged);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &SKGTableWidget::onRangeChanged);
}

SKGTableWidget::~SKGTableWidget() = default;

void SKGTableWidget::onRangeChanged()
{
    auto *scrollb = qobject_cast<QScrollBar *>(sender());
    if ((stickH && scrollb == horizontalScrollBar()) || (stickV && scrollb == verticalScrollBar())) {
        scrollb->setValue(scrollb->maximum());
    }
}

void SKGTableWidget::onActionTriggered()
{
    auto *scrollb = qobject_cast<QScrollBar *>(sender());
    if (scrollb != nullptr) {
        if (scrollb == horizontalScrollBar()) {
            stickH = (scrollb->value() == scrollb->maximum());
        }
        if (scrollb == verticalScrollBar()) {
            stickV = (scrollb->value() == scrollb->maximum());
        }
    }
}

void SKGTableWidget::setStickHorizontal(bool iStick)
{
    stickH = iStick;
}

bool SKGTableWidget::stickHorizontal() const
{
    return stickH;
}

void SKGTableWidget::setStickVertical(bool iStick)
{
    stickV = iStick;
}
bool SKGTableWidget::stickVertical() const
{
    return stickV;
}

bool SKGTableWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if (iObject == this && iEvent != nullptr && iEvent->type() == QEvent::KeyPress) {
        auto *kevent = dynamic_cast<QKeyEvent *>(iEvent);
        if (kevent != nullptr) {
            if (kevent->key() == Qt::Key_Delete && state() != QAbstractItemView::EditingState) {
                QList<QTableWidgetItem *> listOfSelectedItems = this->selectedItems();
                int nb = listOfSelectedItems.count();
                if (nb > 0) {
                    // Build list of indexes
                    QList<int> listIndex;
                    listIndex.reserve(nb);
                    for (int i = 0; i < nb; ++i) {
                        QModelIndex mIndex = this->indexFromItem(listOfSelectedItems.at(i));
                        if (!listIndex.contains(mIndex.row())) {
                            listIndex.append(mIndex.row());
                        }
                    }

                    // Sort reverse
                    std::sort(listIndex.begin(), listIndex.end(), std::greater<int>());

                    // Emit events
                    nb = listIndex.count();
                    for (int i = 0; i < nb; ++i) {
                        Q_EMIT removeLine(listIndex.at(i));
                    }

                    if (iEvent != nullptr) {
                        iEvent->accept();
                    }
                    return true; // stop the process
                }
            } else if (kevent->matches(QKeySequence::Copy) && this->state() != QAbstractItemView::EditingState) {
                copy();
                if (iEvent != nullptr) {
                    iEvent->accept();
                }
                return true; // stop the process
            }
        }
    }

    return QTableWidget::eventFilter(iObject, iEvent);
}

void SKGTableWidget::copy()
{
    QItemSelectionModel *selection = selectionModel();
    if (selection != nullptr) {
        QModelIndexList indexes = selection->selectedIndexes();

        if (indexes.empty()) {
            return;
        }

        std::sort(indexes.begin(), indexes.end());

        // You need a pair of indexes to find the row changes
        QModelIndex previous = indexes.first();
        indexes.removeFirst();
        QString header_text;
        bool header_done = false;
        QString selected_text;
        for (const auto &current : std::as_const(indexes)) {
            selected_text.append(model()->data(previous).toString());
            if (!header_done) {
                header_text.append(model()->headerData(previous.column(), Qt::Horizontal).toString());
            }
            if (current.row() != previous.row()) {
                selected_text.append(u'\n');
                header_done = true;
            } else {
                selected_text.append(u';');
                if (!header_done) {
                    header_text.append(u';');
                }
            }
            previous = current;
        }

        // add last element
        selected_text.append(model()->data(previous).toString());
        selected_text.append(u'\n');
        QApplication::clipboard()->setText(header_text + '\n' + selected_text);
    }
}
