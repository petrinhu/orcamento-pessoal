/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for scheduled transaction management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgscheduledboardwidget.h"

#include <qdom.h>

#include <qaction.h>
#include <qqmlcontext.h>
#include <qquickwidget.h>
#include <qwidgetaction.h>

#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgobjectbase.h"
#include "skgperiodedit.h"
#include "skgreportbank.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGScheduledBoardWidget::SKGScheduledBoardWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGHtmlBoardWidget(iParent,
                         iDocument,
                         i18nc("Noun, the title of a section", "Scheduled transactions"),
                         QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/scheduled_operations.qml")),
                         QStringList() << QStringLiteral("v_recurrentoperation_display"))
{
    SKGTRACEINFUNC(10)
    m_daysmax = new SKGComboBox(this);
    m_daysmax->addItem(i18nc("Item in a combo box", "For 5 next days"), "5");
    m_daysmax->addItem(i18nc("Item in a combo box", "For 10 next days"), "10");
    m_daysmax->addItem(i18nc("Item in a combo box", "For 15 next days"), "15");
    m_daysmax->addItem(i18nc("Item in a combo box", "For 30 next days"), "30");
    m_daysmax->addItem(i18nc("Item in a combo box", "For 60 next days"), "60");
    m_daysmax->addItem(i18nc("Item in a combo box", "For 90 next days"), "90");

    // Add widget in menu
    auto daysmaxWidget = new QWidgetAction(this);
    daysmaxWidget->setObjectName(QStringLiteral("daysmaxWidget"));
    daysmaxWidget->setDefaultWidget(m_daysmax);

    addAction(daysmaxWidget);

    connect(m_daysmax, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged), this, [this]() {
        this->dataModified();
    });
}

SKGScheduledBoardWidget::~SKGScheduledBoardWidget(){SKGTRACEINFUNC(10)}

QString SKGScheduledBoardWidget::getState()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("daysmax"), (m_daysmax != nullptr ? m_daysmax->currentData().toString() : QStringLiteral("30")));
    return doc.toString();
}

void SKGScheduledBoardWidget::setState(const QString &iState)
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString daysmax = root.attribute(QStringLiteral("daysmax"));
    if (daysmax.isEmpty()) {
        daysmax = QStringLiteral("30");
    }
    if (m_daysmax != nullptr && !daysmax.isEmpty()) {
        m_daysmax->setCurrentIndex(m_daysmax->findData(daysmax));
    }
}

void SKGScheduledBoardWidget::dataModified(const QString &iTableName, int iIdTransaction)
{
    m_Report->addParameter(QStringLiteral("scheduled_operation_days_max"), m_daysmax->currentData());
    SKGHtmlBoardWidget::dataModified(iTableName, iIdTransaction);
}
