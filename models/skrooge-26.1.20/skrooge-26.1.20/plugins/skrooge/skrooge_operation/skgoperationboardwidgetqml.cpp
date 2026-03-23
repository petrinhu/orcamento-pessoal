/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for transaction management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgoperationboardwidgetqml.h"

#include <qdom.h>

#include <qaction.h>
#include <qqmlcontext.h>
#include <qquickwidget.h>
#include <qwidgetaction.h>

#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgperiodedit.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGOperationBoardWidgetQml::SKGOperationBoardWidgetQml(QWidget *iParent, SKGDocument *iDocument)
    : SKGHtmlBoardWidget(iParent,
                         iDocument,
                         i18nc("Dashboard widget title", "Income & Expenditure"),
                         QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/income_vs_expenditure.qml")),
                         QStringList() << QStringLiteral("v_operation"),
                         SKGSimplePeriodEdit::NONE,
                         QStringList() << QStringLiteral("t_ACCOUNTTYPE") << QStringLiteral("t_ACCOUNT") << QStringLiteral("t_BANK")
                                       << QStringLiteral("t_CATEGORY"))
    , m_periodEdit1(nullptr)
    , m_periodEdit2(nullptr)
{
    SKGTRACEINFUNC(10)

    QStringList overlayopen;
    overlayopen.push_back(QStringLiteral("quickopen"));
    m_menuOpen = new QAction(SKGServices::fromTheme(QStringLiteral("view-statistics"), overlayopen), i18nc("Verb", "Open report…"), this);
    connect(m_menuOpen, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    addAction(m_menuOpen);

    {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        addAction(sep);
    }
    m_menuGroup = new QAction(i18nc("Noun, a type of transactions", "Grouped"), this);
    m_menuGroup->setCheckable(true);
    m_menuGroup->setChecked(false);
    connect(m_menuGroup, &QAction::triggered, this, &SKGOperationBoardWidgetQml::settingsModified);
    addAction(m_menuGroup);
    m_Quick->rootContext()->setContextProperty(QStringLiteral("groupedWidget"), m_menuGroup);

    m_menuTransfer = new QAction(i18nc("Noun, a type of transactions", "Transfers"), this);
    m_menuTransfer->setCheckable(true);
    m_menuTransfer->setChecked(false);
    connect(m_menuTransfer, &QAction::triggered, this, &SKGOperationBoardWidgetQml::settingsModified);
    addAction(m_menuTransfer);
    m_Quick->rootContext()->setContextProperty(QStringLiteral("transferWidget"), m_menuTransfer);

    connect(m_menuGroup, &QAction::toggled, m_menuTransfer, &QAction::setEnabled);

    m_menuTracked = new QAction(i18nc("Noun, a type of transactions", "Tracked"), this);
    m_menuTracked->setCheckable(true);
    m_menuTracked->setChecked(true);
    connect(m_menuTracked, &QAction::triggered, this, &SKGOperationBoardWidgetQml::settingsModified);
    addAction(m_menuTracked);
    m_Quick->rootContext()->setContextProperty(QStringLiteral("trackerWidget"), m_menuTracked);

    m_menuSuboperation = new QAction(i18nc("Noun, a type of transactions", "On subtransactions"), this);
    m_menuSuboperation->setCheckable(true);
    m_menuSuboperation->setChecked(false);
    connect(m_menuSuboperation, &QAction::triggered, this, &SKGOperationBoardWidgetQml::settingsModified);
    addAction(m_menuSuboperation);
    m_Quick->rootContext()->setContextProperty(QStringLiteral("suboperationsWidget"), m_menuSuboperation);

    {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        addAction(sep);
    }

    m_periodEdit1 = new SKGPeriodEdit(this, true);
    m_periodEdit1->setObjectName(QStringLiteral("m_periodEdit1"));
    {
        // Set default
        QDomDocument doc(QStringLiteral("SKGML"));
        QDomElement root = doc.createElement(QStringLiteral("parameters"));
        doc.appendChild(root);
        root.setAttribute(QStringLiteral("period"), SKGServices::intToString(static_cast<int>(SKGPeriodEdit::CURRENT)));
        m_periodEdit1->setState(doc.toString());

        // Add widget in menu
        auto periodEditWidget = new QWidgetAction(this);
        periodEditWidget->setObjectName(QStringLiteral("m_periodEdit1Action"));
        periodEditWidget->setDefaultWidget(m_periodEdit1);

        addAction(periodEditWidget);
    }
    m_Quick->rootContext()->setContextProperty(QStringLiteral("period1Widget"), m_periodEdit1);

    {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        addAction(sep);
    }

    m_periodEdit2 = new SKGPeriodEdit(this, true);
    m_periodEdit2->setObjectName(QStringLiteral("m_periodEdit2"));
    {
        // Set default
        QDomDocument doc(QStringLiteral("SKGML"));
        QDomElement root = doc.createElement(QStringLiteral("parameters"));
        doc.appendChild(root);
        root.setAttribute(QStringLiteral("period"), SKGServices::intToString(static_cast<int>(SKGPeriodEdit::PREVIOUS)));
        m_periodEdit2->setState(doc.toString());

        // Add widget in menu
        auto periodEditWidget = new QWidgetAction(this);
        periodEditWidget->setObjectName(QStringLiteral("m_periodEdit2Action"));
        periodEditWidget->setDefaultWidget(m_periodEdit2);

        addAction(periodEditWidget);
    }
    m_Quick->rootContext()->setContextProperty(QStringLiteral("period2Widget"), m_periodEdit2);

    // Refresh
    connect(m_periodEdit1, &SKGPeriodEdit::changed, this, &SKGOperationBoardWidgetQml::settingsModified);
    connect(m_periodEdit2, &SKGPeriodEdit::changed, this, &SKGOperationBoardWidgetQml::settingsModified);

    settingsModified();
}

SKGOperationBoardWidgetQml::~SKGOperationBoardWidgetQml()
{
    SKGTRACEINFUNC(10)
    m_menuTransfer = nullptr;
    m_menuTracked = nullptr;
}

QString SKGOperationBoardWidgetQml::getState()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGHtmlBoardWidget::getState());
    QDomElement root = doc.documentElement();

    root.setAttribute(QStringLiteral("menuGroup"), (m_menuGroup != nullptr) && m_menuGroup->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuTransfert"), (m_menuTransfer != nullptr) && m_menuTransfer->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuTracked"), (m_menuTracked != nullptr) && m_menuTracked->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuSuboperation"),
                      (m_menuSuboperation != nullptr) && m_menuSuboperation->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("period1"), m_periodEdit1 != nullptr ? m_periodEdit1->getState() : QString());
    root.setAttribute(QStringLiteral("period2"), m_periodEdit2 != nullptr ? m_periodEdit2->getState() : QString());
    return doc.toString();
}

void SKGOperationBoardWidgetQml::setState(const QString &iState)
{
    SKGHtmlBoardWidget::setState(iState);

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();
    if (m_menuGroup != nullptr) {
        QString val = root.attribute(QStringLiteral("menuGroup"));
        if (val.isEmpty()) {
            val = root.attribute(QStringLiteral("menuTransfert"));
        }
        m_menuGroup->setChecked(val == QStringLiteral("Y"));
    }
    if (m_menuTransfer != nullptr) {
        m_menuTransfer->setChecked(root.attribute(QStringLiteral("menuTransfert")) == QStringLiteral("Y"));
    }
    if (m_menuTracked != nullptr) {
        m_menuTracked->setChecked(root.attribute(QStringLiteral("menuTracked")) != QStringLiteral("N"));
    }
    if (m_menuSuboperation != nullptr) {
        m_menuSuboperation->setChecked(root.attribute(QStringLiteral("menuSuboperation")) == QStringLiteral("Y"));
    }
    QString period1 = root.attribute(QStringLiteral("period1"));
    if ((m_periodEdit1 != nullptr) && !period1.isEmpty()) {
        m_periodEdit1->setState(period1);
    }
    QString period2 = root.attribute(QStringLiteral("period2"));
    if ((m_periodEdit2 != nullptr) && !period2.isEmpty()) {
        m_periodEdit2->setState(period2);
    }

    SKGHtmlBoardWidget::dataModified(QString(), 0);
    settingsModified();
}

void SKGOperationBoardWidgetQml::settingsModified()
{
    SKGTRACEINFUNC(10)
    if (m_menuOpen != nullptr) {
        auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
        if (doc != nullptr) {
            QString url = QStringLiteral("skg://skrooge_report_plugin/?grouped=")
                % ((m_menuGroup != nullptr) && m_menuGroup->isChecked() ? QStringLiteral("Y") : QStringLiteral("N")) % "&transfers="
                % ((m_menuTransfer != nullptr) && m_menuTransfer->isChecked() ? QStringLiteral("Y") : QStringLiteral("N")) % "&tracked="
                % ((m_menuTracked != nullptr) && m_menuTracked->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"))
                % "&expenses=Y&incomes=Y&lines2=t_TYPEEXPENSENLS&columns=d_DATEMONTH&currentPage=-1" % "&mode=0&interval=3&period=3"
                % "&tableAndGraphState.graphMode=1&tableAndGraphState.allPositive=Y&tableAndGraphState.show=graph&title="
                % SKGServices::encodeForUrl(i18nc("Noun, the title of a section", "Income & Expenditure"));
            m_menuOpen->setData(url);
        }
    }
}
