/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A QLineEdit with calculator included.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgcalculatoredit.h"

#include <kcolorscheme.h>

#include <qcompleter.h>
#include <qevent.h>
#include <qjsengine.h>
#include <qstringlistmodel.h>
#include <qvalidator.h>

#include "skgservices.h"
#include "skgtraces.h"
#include <utility>

SKGCalculatorEdit::SKGCalculatorEdit(QWidget *iParent)
    : QLineEdit(iParent)
    , m_lastValue(0)
    , m_lastOperator(0)
    , m_currentMode(EXPRESSION)
{
    setMode(CALCULATOR);
    m_fontColor = palette().color(QPalette::Text);
}

SKGCalculatorEdit::~SKGCalculatorEdit() = default;

double SKGCalculatorEdit::value()
{
    bool test;
    return getEvaluatedValue(test);
}

int SKGCalculatorEdit::sign() const
{
    QString t = text();
    if (!t.isEmpty() && t[0] == '+') {
        return 1;
    }
    if (!t.isEmpty() && t[0] == '-') {
        return -1;
    }
    return 0;
}

SKGCalculatorEdit::Mode SKGCalculatorEdit::mode() const
{
    return m_currentMode;
}

void SKGCalculatorEdit::setMode(Mode iMode)
{
    if (m_currentMode != iMode) {
        m_currentMode = iMode;
        if (iMode == CALCULATOR) {
            auto newValidator = new QDoubleValidator(this);
            setValidator(newValidator);
            setAlignment(Qt::AlignRight);
        } else {
            setValidator(nullptr);
        }
        Q_EMIT modified();
    }
}

void SKGCalculatorEdit::setValue(double iValue)
{
    setText(SKGServices::doubleToString(iValue));
}

void SKGCalculatorEdit::setText(const QString &iText)
{
    // Set default color
    QPalette field_palette = palette();
    field_palette.setColor(QPalette::Text, m_fontColor);
    setPalette(field_palette);

    // Set text (to be sure than keyPressEvent is able to get it)
    QLineEdit::setText(iText);

    // Simulate a validation
    if (mode() == EXPRESSION) {
        bool previous = this->blockSignals(true);
        keyPressEvent(Qt::Key_Return);
        this->blockSignals(previous);
    }

    // Set text (to display the input value)
    if (valid()) {
        QLineEdit::setText(iText);
    }
    Q_EMIT modified();
}

bool SKGCalculatorEdit::valid()
{
    bool test;
    getEvaluatedValue(test);
    return test;
}

QString SKGCalculatorEdit::formula()
{
    return m_formula;
}

void SKGCalculatorEdit::addParameterValue(const QString &iParameter, double iValue)
{
    m_parameters.insert(iParameter, iValue);
    QStringList list;
    auto keys = m_parameters.keys();
    list.reserve(keys.count());
    for (const auto &a : std::as_const(keys)) {
        list.append(u'=' % a);
    }

    // Refresh completion
    auto comp = new QCompleter(list);
    comp->setCaseSensitivity(Qt::CaseInsensitive);
    comp->setFilterMode(Qt::MatchContains);
    setCompleter(comp);
}

void SKGCalculatorEdit::keyPressEvent(QKeyEvent *iEvent)
{
    if (iEvent != nullptr) {
        int key = iEvent->key();
        if (mode() == CALCULATOR) {
            bool hasText = !text().isEmpty() && selectedText() != text();

            if (iEvent->count() == 1
                && ((key == Qt::Key_Plus && hasText) || (key == Qt::Key_Minus && hasText) || key == Qt::Key_Asterisk || key == Qt::Key_Slash
                    || key == Qt::Key_Return || key == Qt::Key_Enter)) {
                keyPressEvent(key);
                iEvent->accept();
            } else {
                QLineEdit::keyPressEvent(iEvent);
            }
        } else {
            // Set default color
            QPalette field_palette = palette();
            field_palette.setColor(QPalette::Text, m_fontColor);
            setPalette(field_palette);

            keyPressEvent(key);
            QLineEdit::keyPressEvent(iEvent);
        }
    }
}

void SKGCalculatorEdit::focusOutEvent(QFocusEvent *iEvent)
{
    if (iEvent->reason() != Qt::ActiveWindowFocusReason) {
        keyPressEvent(Qt::Key_Return);
    }
    QLineEdit::focusOutEvent(iEvent);
}

void SKGCalculatorEdit::keyPressEvent(int key)
{
    if (mode() == CALCULATOR) {
        if (m_lastOperator != 0) {
            if (m_lastOperator == Qt::Key_Plus) {
                m_lastValue += value();
                setValue(m_lastValue);
            } else if (m_lastOperator == Qt::Key_Minus) {
                m_lastValue -= value();
                setValue(m_lastValue);
            } else if (m_lastOperator == Qt::Key_Asterisk) {
                m_lastValue *= value();
                setValue(m_lastValue);
            } else if (m_lastOperator == Qt::Key_Slash && value() != 0) {
                m_lastValue /= value();
                setValue(m_lastValue);
            }

        } else {
            m_lastValue = value();
        }

        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            m_lastOperator = 0;
            m_lastValue = 0;
        } else {
            m_lastOperator = key;
            QLineEdit::setText(QString());
        }
    } else {
        if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            bool test;
            double v = getEvaluatedValue(test);
            if (test) {
                QString t = text();
                QLineEdit::setText((!t.isEmpty() && t[0] == '+' && v > 0 ? "+" : "") % SKGServices::doubleToString(v));
            } else {
                QPalette field_palette = palette();
                field_palette.setColor(QPalette::Text, KColorScheme(QPalette::Normal).foreground(KColorScheme::NegativeText).color());
                setPalette(field_palette);
            }
            Q_EMIT textChanged(text());
        }
    }
}

double SKGCalculatorEdit::getEvaluatedValue(bool &iOk)
{
    double output = 0;
    iOk = false;

    QString t = text().trimmed();
    if (!t.isEmpty()) {
        m_formula = t;
        t = t.replace(',', u'.'); // Replace comma by a point in case of typo
        t = t.remove(' '); // Remove space to support this kind of values 3 024,25
        if (!QLocale().groupSeparator().isNull()) {
            t = t.replace(QLocale().groupSeparator(), QStringLiteral("."));
        }

        // Remove double . in numbers
        int toRemoveIndex = -1;
        int nbc = t.size();
        for (int i = 0; i < nbc; ++i) {
            if (t.at(i) == '.') {
                if (toRemoveIndex != -1) {
                    t = t.remove(toRemoveIndex, 1);
                    --nbc;
                    --i;
                    toRemoveIndex = i;
                } else {
                    toRemoveIndex = i;
                }
            } else if (t.at(i) < '0' || t.at(i) > '9') {
                toRemoveIndex = -1;
            }
        }
        if (t.startsWith(QStringLiteral("="))) {
            t = t.right(t.length() - 1);
            QMapIterator<QString, double> i(m_parameters);
            while (i.hasNext()) {
                i.next();
                t.replace(i.key(), SKGServices::doubleToString(i.value()));
            }

        } else {
            m_formula = QString();
        }

        QJSEngine myEngine;
        const auto result = myEngine.evaluate(t);
        if (result.isNumber()) {
            output = result.toNumber();
            iOk = true;
        }
    }
    return output;
}
