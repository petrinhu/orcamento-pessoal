/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGCOMBOBOX_H
#define SKGCOMBOBOX_H
/** @file
 * A combo box with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbasegui_export.h"
#include <kcombobox.h>
/**
 * This file is a combo box with more features.
 */
class SKGBASEGUI_EXPORT SKGComboBox : public KComboBox
{
    Q_OBJECT
    /**
     * Text of the combobox
     */
    Q_PROPERTY(QString text READ text WRITE setText USER true) // clazy:exclude=qproperty-without-notify

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGComboBox(QWidget *iParent = nullptr);

    /**
     * Default Destructor
     */
    ~SKGComboBox() override;

    /**
     * Get the text for the combo
     * @return the text
     */
    virtual QString text() const;

    /**
     * Set the text for the combo
     * @param iText the text
     */
    virtual void setText(const QString &iText);

    /**
     * Set the Palette for the combobox.
     * Reimplemented since the base method does
     * not apply the Palette to the underlying
     * QlineEdit
     * @param iPalette the new palette
     */
    virtual void setPalette(const QPalette &iPalette);
};

#endif // SKGCOMBOBOX_H
