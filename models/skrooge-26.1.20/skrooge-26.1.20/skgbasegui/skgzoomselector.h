/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGZOOMSELECTOR_H
#define SKGZOOMSELECTOR_H
/** @file
 * A zoom selector.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbasegui_export.h"
#include "ui_skgzoomselector.h"
#include <qtimer.h>

/**
 * This file is a zoom selector.
 */
class SKGBASEGUI_EXPORT SKGZoomSelector : public QWidget
{
    Q_OBJECT
    /**
     * Value of the zoom
     */
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY changed USER true)

    /**
     * Value of the zoom
     */
    Q_PROPERTY(int resetValue READ resetValue WRITE setResetValue NOTIFY changed)

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGZoomSelector(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGZoomSelector() override;

    /**
     * Get the value of the zoom
     * @return the value
     */
    virtual int value() const;

    /**
     * Get the value of the zoom when reset
     * @return the value
     */
    virtual int resetValue() const;

public Q_SLOTS:
    /**
     * Set the value of the zoom
     * @param iValue the value
     * @param iEmitEvent to emit delayed event
     */
    virtual void setValue(int iValue, bool iEmitEvent = true);

    /**
     * Set the value of the zoom when reset
     * @param iValue the value
     */
    virtual void setResetValue(int iValue);

    /**
     * Reinitialize zoom
     */
    virtual void initializeZoom();

    /**
     * Zoom in
     */
    virtual void zoomIn();

    /**
     * Zoom out
     */
    virtual void zoomOut();

Q_SIGNALS:
    /**
     * Emitted when the value is changed
     * @param iValue the value
     */
    void changed(int iValue);

private Q_SLOTS:
    void onZoomChanged();
    void onZoomChangedDelayed();

private:
    Ui::skgzoomselector ui{};
    QTimer m_timer;

    int m_resetValue;
};

#endif // SKGZOOMSELECTOR_H
