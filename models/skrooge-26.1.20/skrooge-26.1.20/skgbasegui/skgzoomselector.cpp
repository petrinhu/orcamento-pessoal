/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A zoom selector.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgzoomselector.h"
#include "skgservices.h"

SKGZoomSelector::SKGZoomSelector(QWidget *iParent)
    : QWidget(iParent)
    , m_resetValue(0)
{
    ui.setupUi(this);

    // Set icons
    ui.kZoomIn->setIcon(SKGServices::fromTheme(QStringLiteral("zoom-in")));
    ui.kZoomOut->setIcon(SKGServices::fromTheme(QStringLiteral("zoom-out")));
    ui.kZoomOriginal->setIcon(SKGServices::fromTheme(QStringLiteral("zoom-original")));

    connect(ui.kZoomOriginal, &QToolButton::clicked, this, &SKGZoomSelector::initializeZoom);
    connect(ui.kZoomOut, &QToolButton::clicked, this, &SKGZoomSelector::zoomOut);
    connect(ui.kZoomIn, &QToolButton::clicked, this, &SKGZoomSelector::zoomIn);
    connect(ui.kZoomSlider, &QSlider::valueChanged, this, &SKGZoomSelector::onZoomChangedDelayed);

    // Init timer
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGZoomSelector::onZoomChanged, Qt::QueuedConnection);
}

SKGZoomSelector::~SKGZoomSelector() = default;

void SKGZoomSelector::setValue(int iValue, bool iEmitEvent)
{
    bool previous = ui.kZoomSlider->blockSignals(!iEmitEvent);
    ui.kZoomSlider->setValue(iValue);
    if (ui.kZoomOriginal != nullptr) {
        ui.kZoomOriginal->setChecked(iValue == m_resetValue);
    }
    ui.kZoomSlider->blockSignals(previous);
}

int SKGZoomSelector::value() const
{
    return ui.kZoomSlider->value();
}

int SKGZoomSelector::resetValue() const
{
    return m_resetValue;
}

void SKGZoomSelector::setResetValue(int iValue)
{
    m_resetValue = iValue;
}

void SKGZoomSelector::zoomIn()
{
    ui.kZoomSlider->setValue(ui.kZoomSlider->value() + 1);
}

void SKGZoomSelector::zoomOut()
{
    ui.kZoomSlider->setValue(ui.kZoomSlider->value() - 1);
}

void SKGZoomSelector::initializeZoom()
{
    ui.kZoomSlider->setValue(m_resetValue);
}

void SKGZoomSelector::onZoomChangedDelayed()
{
    m_timer.start(300);
}

void SKGZoomSelector::onZoomChanged()
{
    int val = ui.kZoomSlider->value();
    if (ui.kZoomIn != nullptr) {
        ui.kZoomIn->setEnabled(val < 10);
    }
    if (ui.kZoomOut != nullptr) {
        ui.kZoomOut->setEnabled(val > -10);
    }
    if (ui.kZoomOriginal != nullptr) {
        ui.kZoomOriginal->setChecked(val == m_resetValue);
    }

    Q_EMIT changed(val);
}
