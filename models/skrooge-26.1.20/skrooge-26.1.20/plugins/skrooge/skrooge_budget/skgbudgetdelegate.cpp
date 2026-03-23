/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a delegate for budget.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbudgetdelegate.h"

#include <kcolorscheme.h>
#include <kstatefulbrush.h>

#include <qpainter.h>
#include <qsortfilterproxymodel.h>

#include "skgobjectmodelbase.h"
#include "skgprogressbar.h"
#include "skgtraces.h"

SKGBudgetDelegate::SKGBudgetDelegate(QObject *iParent, SKGDocument *iDoc)
    : QStyledItemDelegate(iParent)
    , m_document(iDoc)
{
}

SKGBudgetDelegate::~SKGBudgetDelegate()
{
    m_document = nullptr;
}

void SKGBudgetDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool done = false;
    if (index.isValid()) {
        const auto *m = qobject_cast<const SKGObjectModelBase *>(index.model());
        const auto *p = qobject_cast<const QSortFilterProxyModel *>(index.model());
        if (p != nullptr) {
            m = qobject_cast<SKGObjectModelBase *>(p->sourceModel());
        }
        if (m != nullptr) {
            QString att = m->getAttribute(index.column());

            // Compute percent of time
            QModelIndex idxs = index;
            if (p != nullptr) {
                idxs = p->mapToSource(index);
            }
            SKGObjectBase obj = m->getObject(idxs);

            int year = SKGServices::stringToDouble(obj.getAttribute(QStringLiteral("i_year")));
            int month = SKGServices::stringToDouble(obj.getAttribute(QStringLiteral("i_month")));
            QDate today = QDate::currentDate();
            double pourcent = 1;
            bool actif = true;
            if (year == today.year()) {
                if (month == 0) {
                    QDate d1(year, 1, 1);
                    pourcent = static_cast<double>(d1.daysTo(today)) / static_cast<double>(today.daysInYear());
                } else if (month == today.month()) {
                    QDate d1(year, month, 1);
                    pourcent = static_cast<double>(d1.daysTo(today)) / static_cast<double>(today.daysInMonth());
                } else if (month > today.month()) {
                    pourcent = 0;
                    actif = false;
                }
            } else if (year > today.year()) {
                pourcent = 0;
                actif = false;
            }

            if ((att == QStringLiteral("f_budgeted") || att == QStringLiteral("f_budgeted_modified")) && actif) {
                double budgeted = SKGServices::stringToDouble(obj.getAttribute(att));
                double amount = SKGServices::stringToDouble(obj.getAttribute(QStringLiteral("f_CURRENTAMOUNT")));

                KColorScheme scheme(QPalette::Normal);
                QColor negativeC = scheme.foreground(KColorScheme::NegativeText).color().toHsv();
                QColor positiveC = scheme.foreground(KColorScheme::PositiveText).color().toHsv();
                QColor backC = scheme.foreground(KColorScheme::LinkText).color().toHsv();

                double coef = 0.3;
                negativeC.setHsv(negativeC.hue(), negativeC.saturation() * coef, negativeC.value());
                positiveC.setHsv(positiveC.hue(), positiveC.saturation() * coef, positiveC.value());
                backC.setHsv(backC.hue(), backC.saturation() * coef, backC.value());

                QBrush negative(negativeC);
                QBrush positive(positiveC);
                QBrush back(backC);

                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);

                QStyleOptionViewItem opt = option;
                QStyledItemDelegate::initStyleOption(&opt, index);
                QRect rect = opt.rect.adjusted(1, 1, -1, -1);

                // handle selection
                if ((option.state & QStyle::State_Selected) != 0u) {
                    KStatefulBrush sb(KColorScheme::View, KColorScheme::NormalBackground);

                    QBrush selectionBrush(sb.brush(QPalette::Active));
                    painter->setBrush(selectionBrush);
                    painter->drawRect(rect);
                }

                rect = opt.rect.adjusted(1, 1, -1, -1);

                painter->setPen(Qt::NoPen);

                if (budgeted > 0) {
                    if (amount < 0) {
                        amount = 0;
                    }
                    // Income
                    if (amount < budgeted) {
                        // Draw red zone
                        painter->setBrush(negative);
                        painter->drawRect(rect);

                        // Draw blue zone
                        painter->setBrush(back);
                        QRect r2(rect.left(), rect.top(), rect.width() * (budgeted == 0 ? 1 : amount / budgeted), rect.height());
                        painter->drawRect(r2);
                    } else {
                        // Draw green zone
                        painter->setBrush(positive);
                        painter->drawRect(rect);

                        // Draw blue zone
                        painter->setBrush(back);
                        QRect r2(rect.left(), rect.top(), rect.width() * (amount == 0 ? 0 : budgeted / amount), rect.height());
                        painter->drawRect(r2);
                    }
                } else {
                    if (amount > 0) {
                        amount = 0;
                    }
                    // Expenditure
                    if (amount < budgeted) {
                        // Draw red zone
                        painter->setBrush(negative);
                        painter->drawRect(rect);

                        // Draw blue zone
                        painter->setBrush(back);
                        QRect r2(rect.left(), rect.top(), rect.width() * (amount == 0 ? 0 : budgeted / amount), rect.height());
                        painter->drawRect(r2);
                    } else {
                        // Draw green zone
                        painter->setBrush(positive);
                        painter->drawRect(rect);

                        // Draw blue zone
                        painter->setBrush(back);
                        QRect r2(rect.left(), rect.top(), rect.width() * (budgeted == 0 ? 1 : amount / budgeted), rect.height());
                        painter->drawRect(r2);
                    }
                }

                // Draw time progress
                painter->setPen(Qt::black);
                QLine r2(rect.left() + rect.width() * pourcent, rect.top() + 1, rect.left() + rect.width() * pourcent, rect.top() + rect.height() - 1);
                painter->drawLine(r2);

                // Draw text
                painter->setPen(m->data(idxs, Qt::ForegroundRole).value<QColor>());

                QTextOption to;
                to.setAlignment(static_cast<Qt::AlignmentFlag>(m->data(idxs, Qt::TextAlignmentRole).toInt()));
                painter->drawText(rect, m->data(idxs).toString(), to);

                painter->restore();
                done = true;
            }
        }
    }
    if (!done) {
        QStyledItemDelegate::paint(painter, option, index);
    }
}
