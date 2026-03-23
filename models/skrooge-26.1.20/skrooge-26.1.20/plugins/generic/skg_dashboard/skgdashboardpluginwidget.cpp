/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A dashboard.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgdashboardpluginwidget.h"

#include <qdom.h>

#include "skgdashboardwidget.h"
#include "skgdocument.h"
#include "skginterfaceplugin.h"
#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGDashboardPluginWidget::SKGDashboardPluginWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGTabPage(iParent, iDocument)
    , m_widget(nullptr)
{
    SKGTRACEINFUNC(1)
    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(2);
    verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
    m_widget = new SKGDashboardWidget(this, iDocument);
    verticalLayout->addWidget(m_widget);
}

SKGDashboardPluginWidget::~SKGDashboardPluginWidget()
{
    SKGTRACEINFUNC(1)
    m_widget = nullptr;
}

QString SKGDashboardPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("zoomPosition"), SKGServices::intToString(zoomPosition()));
    root.setAttribute(QStringLiteral("board"), m_widget->getState());
    return doc.toString();
}

void SKGDashboardPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    // Compliance with old mode
    auto board = root.attribute(QStringLiteral("board"));
    if (board.isEmpty()) {
        board = iState;
    }

    m_widget->setState(board);

    QString zoomPositionS = root.attribute(QStringLiteral("zoomPosition"));
    if (zoomPositionS.isEmpty()) {
        zoomPositionS = '0';
    }
    setZoomPosition(SKGServices::stringToInt(zoomPositionS));
}

QString SKGDashboardPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGDASHBOARD_DEFAULT_PARAMETERS");
}

void SKGDashboardPluginWidget::refresh(){SKGTRACEINFUNC(1)}

QWidget *SKGDashboardPluginWidget::zoomableWidget()
{
    return SKGTabPage::zoomableWidget();
}

QList<QWidget *> SKGDashboardPluginWidget::printableWidgets()
{
    return m_widget->printableWidgets();
}
