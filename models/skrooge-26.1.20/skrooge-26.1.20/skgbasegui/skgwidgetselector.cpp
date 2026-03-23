/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A widget selector.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgwidgetselector.h"
#include "skgdefine.h"

#include <qtoolbutton.h>
#include <utility>

SKGWidgetSelector::SKGWidgetSelector(QWidget *iParent)
    : QWidget(iParent)
    , m_currentMode(-1)
    , m_alwaysOneOpen(false)
{
    ui.setupUi(this);
}

SKGWidgetSelector::~SKGWidgetSelector() = default;

void SKGWidgetSelector::addButton(const QIcon &iIcon, const QString &iTitle, const QString &iToolTip, QWidget *iWidgets)
{
    SKGListQWidget list;
    list.push_back(iWidgets);
    addButton(iIcon, iTitle, iToolTip, list);
}

void SKGWidgetSelector::addButton(const QIcon &iIcon,
                                  const QString &iTitle,
                                  const QString &iToolTip,
                                  const SKGWidgetSelector::SKGListQWidget &iListOfShownWidgets)
{
    // Create button
    auto btn = new QToolButton(this);
    btn->setCheckable(true);
    btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btn->setAutoRaise(true);
    btn->setText(iTitle);
    btn->setToolTip(iToolTip);
    btn->setIcon(iIcon);

    ui.horizontalLayout->insertWidget(m_listButton.count(), btn);

    connect(btn, &QToolButton::clicked, this, &SKGWidgetSelector::onButtonClicked);

    // Memorize items
    m_listButton.push_back(btn);

    // Hide widgets
    SKGWidgetSelector::SKGListQWidget list;
    for (auto w : std::as_const(iListOfShownWidgets)) {
        if (w != nullptr) {
            // Addition of an intermediate widget
            /*auto widget = new QWidget(w->parentWidget());
                widget->setObjectName(w->objectName()+"_intermediate");
                auto horizontalLayout = new QHBoxLayout(widget);
                horizontalLayout->setSpacing(0);
                horizontalLayout->setContentsMargins(0, 0, 0, 0);
                horizontalLayout->setObjectName(w->objectName()+"_horizontalLayout");
            w->setParent(widget);
                horizontalLayout->addWidget(w);*/

            list.push_back(w);
            w->hide();
        }
    }
    m_listWidgets.push_back(list);
}

int SKGWidgetSelector::getSelectedMode() const
{
    return m_currentMode;
}

void SKGWidgetSelector::setSelectedMode(int iMode)
{
    if (iMode != m_currentMode) {
        // Hide current widgets
        if (m_currentMode >= 0) {
            m_listButton.at(m_currentMode)->setChecked(false);
            SKGListQWidget list = m_listWidgets.at(m_currentMode);

            for (auto w : std::as_const(list)) {
                if (w != nullptr) {
                    /*if (iMode == -1) {
                        auto anim1 = new QPropertyAnimation(w, "geometry");
                    QRect r=w->geometry();
                         m_originGeometries[w]=r;
                                         // anim1->setStartValue(m_originGeometries[w]);
                         r.setY(r.y()+r.height());
                         m_endGeometries[w]=r;
                                         anim1->setEndValue(m_endGeometries[w]);
                                         anim1->setEasingCurve(QEasingCurve::InOutSine);
                                         anim1->setDuration(500);
                                         anim1->start(QAbstractAnimation::DeleteWhenStopped);
                                         connect(anim1, &QPropertyAnimation::finished, w, &QWidget::hide);

                                     } else */
                    w->hide();
                }
            }
        }

        // Set current mode
        // int previousmode = m_currentMode;
        m_currentMode = iMode;
        if (m_currentMode >= m_listWidgets.count()) {
            m_currentMode = -1;
        }

        // Show widgets
        if (m_currentMode >= 0) {
            m_listButton.at(m_currentMode)->setChecked(true);
            for (auto w : std::as_const(m_listWidgets.at(m_currentMode))) {
                if (w != nullptr) {
                    w->show();
                    /*if (previousmode == -1) {
                        auto anim1 = new QPropertyAnimation(w, "geometry");
                        // anim1->setStartValue(m_endGeometries[w]);
                        anim1->setEndValue(m_originGeometries[w]);
                        anim1->setEasingCurve(QEasingCurve::InOutSine);
                        anim1->setDuration(500);
                        anim1->start(QAbstractAnimation::DeleteWhenStopped);
                    }*/
                }
            }
        }

        if (m_currentMode < -1) {
            this->hide();
        }

        Q_EMIT selectedModeChanged(m_currentMode);
    }
}

void SKGWidgetSelector::setEnabledMode(int iMode, bool iEnabled)
{
    if (iMode >= 0 && iMode < m_listButton.count()) {
        m_listButton.at(iMode)->setEnabled(iEnabled);
    }
}

void SKGWidgetSelector::onButtonClicked()
{
    auto *clickedButton = qobject_cast<QToolButton *>(sender());
    int newMode = m_listButton.indexOf(clickedButton);
    if (m_currentMode == newMode) {
        if (getAlwaysOneOpen()) {
            --newMode;
            if (newMode == -1) {
                newMode = m_listButton.count() - 1;
            }
        } else {
            newMode = -1;
        }
    }

    setSelectedMode(newMode);
}

bool SKGWidgetSelector::getAlwaysOneOpen() const
{
    return m_alwaysOneOpen;
}
void SKGWidgetSelector::setAlwaysOneOpen(bool iMode)
{
    if (m_alwaysOneOpen != iMode) {
        m_alwaysOneOpen = iMode;
        Q_EMIT alwaysOneOpenChanged();
    }
}
