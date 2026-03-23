/***************************************************************************
 * SPDX-FileCopyrightText: 2023 N. KRUPENKO krnekit@gmail.com
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a widget that could be used in QWidgetAction and looks like a menu item.
 *
 * @author Nikita KRUPENKO
 */
#include "skgmenuitem.h"

#include <qapplication.h>
#include <qpainter.h>
#include <qstyleoption.h>

#include "skgtraces.h"

SKGMenuitem::SKGMenuitem(QWidget *iParent)
    : QWidget(iParent)
{
    SKGTRACEINFUNC(5);

    setMouseTracking(true);
}

SKGMenuitem::~SKGMenuitem(){SKGTRACEINFUNC(5)}

QString SKGMenuitem::getText() const
{
    return m_text;
}

void SKGMenuitem::setText(const QString &iText)
{
    if (m_text == iText) {
        return;
    }

    m_text = iText;

    Q_EMIT textChanged(iText);

    update();
}

QIcon SKGMenuitem::getIcon() const
{
    return m_icon;
}

void SKGMenuitem::setIcon(const QIcon &iIcon)
{
    m_icon = iIcon;

    Q_EMIT iconChanged(iIcon);

    update();
}

QColor SKGMenuitem::getColor() const
{
    return m_color;
}

void SKGMenuitem::setColor(const QColor &iColor)
{
    if (m_color == iColor) {
        return;
    }

    m_color = iColor;

    Q_EMIT colorChanged(iColor);

    update();
}

bool SKGMenuitem::getIsBold() const
{
    return m_isBold;
}

void SKGMenuitem::setIsBold(bool isBold)
{
    if (m_isBold == isBold) {
        return;
    }

    m_isBold = isBold;

    Q_EMIT isBoldChanged(isBold);

    update();
}

QSize SKGMenuitem::minimumSizeHint() const
{
    QStyleOptionMenuItem opt;
    initStyleOption(&opt);

    QSize contentSize = fontMetrics().size(Qt::TextSingleLine /* | Qt::TextShowMnemonic*/, m_text);
    return style()->sizeFromContents(QStyle::CT_MenuItem, &opt, contentSize, this);
}

void SKGMenuitem::paintEvent(QPaintEvent * /*e*/)
{
    QPainter p(this);
    QStyleOptionMenuItem opt;
    initStyleOption(&opt);

    style()->drawControl(QStyle::CE_MenuItem, &opt, &p, this);
}

void SKGMenuitem::initStyleOption(QStyleOptionMenuItem *option) const
{
    option->initFrom(this);
    option->text = m_text;
    option->menuHasCheckableItems = false;
    option->icon = m_icon;
    option->maxIconWidth = style()->pixelMetric(QStyle::PM_SmallIconSize, option, this) + 4; // Same as in QMenuPrivate
    option->menuItemType = QStyleOptionMenuItem::Normal;
    if (m_color.isValid()) {
        option->palette.setColor(QPalette::WindowText, m_color);
    }
    option->font.setBold(m_isBold);

    if (underMouse()) {
        option->state |= QStyle::State_Selected;
        if (QApplication::mouseButtons() != Qt::NoButton) {
            option->state |= QStyle::State_Sunken;
        }
    }
}
