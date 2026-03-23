/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A color button with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgcolorbutton.h"

SKGColorButton::SKGColorButton(QWidget *iParent)
    : QWidget(iParent)
{
    ui.setupUi(this);
    connect(ui.button, &KColorButton::changed, this, &SKGColorButton::changed);
}

SKGColorButton::~SKGColorButton() = default;

QString SKGColorButton::text() const
{
    return m_text;
}

void SKGColorButton::setText(const QString &iText)
{
    m_text = iText;
    ui.label->setText(iText);
}

QColor SKGColorButton::color() const
{
    return ui.button->color();
}

void SKGColorButton::setColor(const QColor &iColor)
{
    ui.button->setColor(iColor);
}

QColor SKGColorButton::defaultColor() const
{
    return ui.button->defaultColor();
}

void SKGColorButton::setDefaultColor(const QColor &iColor)
{
    ui.button->setDefaultColor(iColor);
}
