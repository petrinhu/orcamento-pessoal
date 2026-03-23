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
#include "skgdashboardboardwidget.h"

#include <qdom.h>

#include "skgdashboardwidget.h"
#include "skgdocument.h"
#include "skginterfaceplugin.h"
#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGDashboardboardWidget::SKGDashboardboardWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGBoardWidget(iParent, iDocument, i18nc("Dashboard widget title", "Sub dashboard"))
    , m_widget(nullptr)
{
    SKGTRACEINFUNC(1)
    m_widget = new SKGDashboardWidget(this, iDocument, getMenu());
    setMainWidget(m_widget);
}

SKGDashboardboardWidget::~SKGDashboardboardWidget()
{
    SKGTRACEINFUNC(1)
    m_widget = nullptr;
}

QString SKGDashboardboardWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGBoardWidget::getState());
    QDomElement root = doc.documentElement();

    // Get state
    if (m_widget != nullptr) {
        root.setAttribute(QStringLiteral("widget"), m_widget->getState());
    }
    return doc.toString();
}

void SKGDashboardboardWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    SKGBoardWidget::setState(iState);

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();
    QString widget = root.attribute(QStringLiteral("widget"));
    if (widget.isEmpty()) {
        m_widget->setState(iState);
    } else {
        m_widget->setState(widget);
    }
}
