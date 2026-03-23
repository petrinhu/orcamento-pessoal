/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGCALCULATOREDIT_H
#define SKGCALCULATOREDIT_H
/** @file
 * A QLineEdit with calculator included.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbasegui_export.h"
#include <qlineedit.h>
/**
 * This file is a QLineEdit with calculator included
 */
class SKGBASEGUI_EXPORT SKGCalculatorEdit : public QLineEdit
{
    Q_OBJECT
    /**
     * Value of the calculator editor
     */
    Q_PROPERTY(double value READ value WRITE setValue USER true NOTIFY modified)

    /**
     * Mode of the calculator editor
     */
    Q_PROPERTY(SKGCalculatorEdit::Mode mode READ mode WRITE setMode NOTIFY modified)

    /**
     * Sign of the calculator editor
     */
    Q_PROPERTY(double sign READ sign NOTIFY modified)

    /**
     * To know if the text is valid
     */
    Q_PROPERTY(bool valid READ valid NOTIFY modified)

public:
    /**
     * This enumerate defines type of calculator
     */
    enum Mode {
        CALCULATOR, /**< simple calculator */
        EXPRESSION /**< expression is evaluated */
    };
    /**
     * Mode of the calculator editor
     */
    Q_ENUM(Mode)

    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGCalculatorEdit(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGCalculatorEdit() override;

    /**
     * Get the mode
     * @return the mode
     */
    virtual Mode mode() const;

    /**
     * Set the mode
     * @param iMode the mode
     */
    virtual void setMode(Mode iMode);

    /**
     * Get the value
     * @return the value
     */
    virtual double value();

    /**
     * Set the value
     * @param iValue the value
     */
    virtual void setValue(double iValue);

    /**
     * Set the text
     * @param iText the text
     */
    virtual void setText(const QString &iText);

    /**
     * Get the sign of the value.
     * 1 if the value is write like "+5"
     * -1 if the value is write like "-5"
     * 0 if the value is write like "5"
     * @return the sign
     */
    virtual int sign() const;

    /**
     * To know if the text is a valid value
     * @return validation
     */
    virtual bool valid();

    /**
     * To formula
     * @return the formula
     */
    virtual QString formula();

    /**
     * Add a parameter
     * @param iParameter the parameter name
     * @param iValue the value
     */
    virtual void addParameterValue(const QString &iParameter, double iValue);

Q_SIGNALS:
    /**
     * This signal is launched when the object is modified
     */
    void modified();

protected:
    /**
     * This event handler, for event event, can be reimplemented in a subclass to receive key press events for the widget.
     * @param iEvent the event
     */
    void keyPressEvent(QKeyEvent *iEvent) override;

    /**
     * This event handler can be reimplemented in a subclass to receive keyboard focus events (focus lost) for the widget. The events is passed in the event
     * parameter.
     * @param iEvent the event
     */
    void focusOutEvent(QFocusEvent *iEvent) override;

private:
    Q_DISABLE_COPY(SKGCalculatorEdit)

    void keyPressEvent(int key);
    double getEvaluatedValue(bool &iOk);
    double m_lastValue;
    int m_lastOperator;
    Mode m_currentMode;

    QColor m_fontColor;
    QMap<QString, double> m_parameters;
    QString m_formula;
};

#endif // SKGCALCULATOREDIT_H
