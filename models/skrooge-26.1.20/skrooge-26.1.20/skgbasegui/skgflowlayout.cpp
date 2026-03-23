/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGFlowLayout.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgflowlayout.h"
#include "skgdefine.h"

#include <qlayout.h>
#include <qwidget.h>
#include <utility>

SKGFlowLayout::SKGFlowLayout(QWidget *iParent, int iMargin, int hSpacing, int vSpacing)
    : QLayout(iParent)
    , m_hSpace(hSpacing)
    , m_vSpace(vSpacing)
{
    setContentsMargins(iMargin, iMargin, iMargin, iMargin);
}

SKGFlowLayout::SKGFlowLayout(int iMargin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing)
    , m_vSpace(vSpacing)
{
    setContentsMargins(iMargin, iMargin, iMargin, iMargin);
}

SKGFlowLayout::~SKGFlowLayout()
{
    while (count() != 0) {
        QLayoutItem *child = takeAt(0);
        if (child != nullptr) {
            QWidget *w = child->widget();
            delete w;
            delete child;
        }
    }
}

void SKGFlowLayout::addItem(QLayoutItem *item)
{
    m_itemList.append(item);
}

void SKGFlowLayout::setSpacing(int space)
{
    m_hSpace = space;
    m_vSpace = space;
}

int SKGFlowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) {
        return m_hSpace;
    }
    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int SKGFlowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) {
        return m_vSpace;
    }
    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

int SKGFlowLayout::count() const
{
    return m_itemList.size();
}

QLayoutItem *SKGFlowLayout::itemAt(int index) const
{
    return m_itemList.value(index);
}

QLayoutItem *SKGFlowLayout::takeAt(int index)
{
    if (index >= 0 && index < m_itemList.size()) {
        return m_itemList.takeAt(index);
    }
    return nullptr;
}

Qt::Orientations SKGFlowLayout::expandingDirections() const
{
    return Qt::Orientations();
}

bool SKGFlowLayout::hasHeightForWidth() const
{
    return true;
}

int SKGFlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

void SKGFlowLayout::setGeometry(const QRect &rect) // clazy:exclude=function-args-by-value
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize SKGFlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize SKGFlowLayout::minimumSize() const
{
    QSize size;
    for (auto item : std::as_const(m_itemList)) {
        size = size.expandedTo(item->minimumSize());
    }
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    size += QSize(left + right, top + left);
    return size;
}

int SKGFlowLayout::doLayout(const QRect rect, bool testOnly) const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;
    int x1 = x;
    int y1 = y;

    for (auto item : std::as_const(m_itemList)) {
        QWidget *wid = item->widget();
        if (wid != nullptr) {
            // Get spaces
            int spaceX = horizontalSpacing();
            if (spaceX == -1) {
                spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
            }

            int spaceY = verticalSpacing();
            if (spaceY == -1) {
                spaceY = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);
            }

            // Try option
            bool optionUsed = false;
            if ((lineHeight != 0) && (x1 != 0)) {
                int nextX = x1 + item->sizeHint().width() + spaceX;
                if (nextX <= x && y1 - y + item->sizeHint().height() <= lineHeight) {
                    optionUsed = true;

                    // Position item
                    if (!testOnly) {
                        item->setGeometry(QRect(QPoint(x1, y1), item->sizeHint()));
                    }

                    x1 = nextX;
                }
            }

            if (!optionUsed) {
                int nextX = x + item->sizeHint().width() + spaceX;
                if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
                    x = effectiveRect.x();
                    y = y + lineHeight + spaceY;
                    nextX = x + item->sizeHint().width() + spaceX;
                    lineHeight = 0;
                }

                // Position item
                if (!testOnly) {
                    item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
                }

                x1 = x;
                y1 = y + item->sizeHint().height() + spaceY;
                x = nextX;
                lineHeight = qMax(lineHeight, item->sizeHint().height());
            }
        }
    }
    return y + lineHeight - rect.y() + bottom;
}

int SKGFlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *p = this->parent();
    if (p == nullptr) {
        return -1;
    }
    if (p->isWidgetType()) {
        auto *pw = qobject_cast<QWidget *>(p);
        return pw->style()->pixelMetric(pm, nullptr, pw);
    }
    return qobject_cast<QLayout *>(p)->spacing();
}
