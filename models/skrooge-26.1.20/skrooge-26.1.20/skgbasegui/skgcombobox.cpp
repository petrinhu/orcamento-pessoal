/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A combo box with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgcombobox.h"

#include <qlineedit.h>

SKGComboBox::SKGComboBox(QWidget *iParent)
    : KComboBox(iParent)
{
}

SKGComboBox::~SKGComboBox() = default;

QString SKGComboBox::text() const
{
    return currentText();
}

void SKGComboBox::setText(const QString &iText)
{
    int pos2 = findText(iText);
    if (pos2 == -1) {
        pos2 = 0;
        insertItem(pos2, iText);
    }
    setCurrentIndex(pos2);
}

void SKGComboBox::setPalette(const QPalette &iPalette)
{
    KComboBox::setPalette(iPalette);
    lineEdit()->setPalette(iPalette);
}
