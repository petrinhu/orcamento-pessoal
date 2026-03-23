/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGCOLORBUTTON_H
#define SKGCOLORBUTTON_H
/** @file
 * A color button with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbasegui_export.h"
#include "ui_skgcolorbutton.h"
/**
 * This file is a color selector box with more features.
 */
class SKGBASEGUI_EXPORT SKGColorButton : public QWidget
{
    Q_OBJECT
    /**
     * Text of the color selector
     */
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY changed USER true)
    /**
     * Color of the color selector
     */
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY changed)
    /**
     * Default color of the color selector
     */
    Q_PROPERTY(QColor defaultColor READ defaultColor WRITE setDefaultColor NOTIFY changed)

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGColorButton(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGColorButton() override;

    /**
     * Get the text for the color selector
     * @return the text
     */
    virtual QString text() const;

    /**
     * Set the text for the color selector
     * @param iText the text
     */
    virtual void setText(const QString &iText);

    /**
     * Get the color for the color selector
     * @return the text
     */
    virtual QColor color() const;

    /**
     * Set the color for the color selector
     * @param iColor the color
     */
    virtual void setColor(const QColor &iColor);

    /**
     * Get the default color for the color selector
     * @return the text
     */
    virtual QColor defaultColor() const;

    /**
     * Set the default color for the color selector
     * @param iColor the color
     */
    virtual void setDefaultColor(const QColor &iColor);

Q_SIGNALS:
    /**
     * Emitted when the color is changed
     * @param iColor the color
     */
    void changed(const QColor &iColor);

private:
    Ui::skgcolorbutton_base ui{};
    QString m_text{};
};

#endif // SKGCOLORBUTTON_H
