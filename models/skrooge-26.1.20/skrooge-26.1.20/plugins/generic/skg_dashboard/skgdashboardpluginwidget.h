/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDASHBOARDPLUGINWIDGET_H
#define SKGDASHBOARDPLUGINWIDGET_H
/** @file
 * A dashboard
 *
 * @author Stephane MANKOWSKI
 */

#include "skgtabpage.h"

class SKGInterfacePlugin;
class SKGDashboardWidget;

/**
 * A dashboard
 */
class SKGDashboardPluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGDashboardPluginWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGDashboardPluginWidget() override;

    /**
     * Get the current state
     * MUST BE OVERWRITTEN
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    QString getState() override;

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    void setState(const QString &iState) override;

    /**
     * Get attribute name to save the default state
     * MUST BE OVERWRITTEN
     * @return attribute name to save the default state.
     */
    QString getDefaultStateAttribute() override;

    /**
     * Get the zoomable widget.
     * The default implementation returns the main widget.
     * @return the zoomable widget.
     */
    QWidget *zoomableWidget() override;

    /**
     * Get the printable widgets.
     * The default implementation returns the main widget.
     * @return the printable widgets.
     */
    QList<QWidget *> printableWidgets() override;

public Q_SLOTS:
    /**
     * Refresh the content.
     */
    virtual void refresh();

private:
    Q_DISABLE_COPY(SKGDashboardPluginWidget)

    SKGDashboardWidget *m_widget;
};

#endif // SKGDASHBOARDPLUGINWIDGET_H
