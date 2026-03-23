/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgreportboardwidget.h"

#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgreportpluginwidget.h"
#include "skgtraces.h"

SKGReportBoardWidget::SKGReportBoardWidget(QWidget *iParent, SKGDocumentBank *iDocument)
    : SKGBoardWidget(iParent, iDocument, i18nc("Dashboard widget title", "Report"), true)
{
    SKGTRACEINFUNC(10)

    // This must be done at the beginning
    this->setMinimumSize(200, 200);

    // Create menu
    setContextMenuPolicy(Qt::ActionsContextMenu);

    QStringList overlayopen;
    overlayopen.push_back(QStringLiteral("quickopen"));
    auto open = new QAction(SKGServices::fromTheme(QStringLiteral("view-statistics"), overlayopen), i18nc("Verb", "Open report…"), this);
    connect(open, &QAction::triggered, this, &SKGReportBoardWidget::onOpen);
    addAction(open);

    m_graph = new SKGReportPluginWidget(iParent, iDocument, true);
    setMainWidget(m_graph);

    // Refresh
    connect(getDocument(), &SKGDocument::tableModified, this, &SKGReportBoardWidget::dataModified, Qt::QueuedConnection);
}

SKGReportBoardWidget::~SKGReportBoardWidget()
{
    SKGTRACEINFUNC(10)
    m_graph = nullptr;
}

QString SKGReportBoardWidget::getState()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGBoardWidget::getState());
    QDomElement root = doc.documentElement();

    if (m_graph != nullptr) {
        root.setAttribute(QStringLiteral("graph"), m_graph->getState());
    }
    return doc.toString();
}

void SKGReportBoardWidget::setState(const QString &iState)
{
    SKGBoardWidget::setState(iState);

    QDomDocument doc(QStringLiteral("SKGML"));
    if (doc.setContent(iState)) {
        QDomElement root = doc.documentElement();

        QString title = root.attribute(QStringLiteral("title"));
        if (!title.isEmpty()) {
            setMainTitle(title);
        }

        QString graphS = root.attribute(QStringLiteral("graph"));
        if (m_graph != nullptr) {
            if (graphS.isEmpty()) {
                m_graph->setState(iState);
            } else {
                m_graph->setState(graphS);
            }
        }
    }

    dataModified(QString(), 0);
}

void SKGReportBoardWidget::dataModified(const QString &iTableName, int iIdTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)
    if (iTableName == QStringLiteral("operation") || iTableName.isEmpty()) {
        bool exist = false;
        getDocument()->existObjects(QStringLiteral("account"), QString(), exist);
        if (parentWidget() != nullptr) {
            setVisible(exist);
        }
    }
}

void SKGReportBoardWidget::onOpen()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    QString graphS;
    if (doc.setContent(getState())) {
        QDomElement root = doc.documentElement();
        graphS = root.attribute(QStringLiteral("graph"));

        QDomDocument doc2(QStringLiteral("SKGML"));
        if (doc2.setContent(graphS)) {
            QDomElement root2 = doc2.documentElement();

            QString currentPage = root2.attribute(QStringLiteral("currentPage"));
            if (SKGServices::stringToInt(currentPage) < -1) {
                root2.setAttribute(QStringLiteral("currentPage"), QStringLiteral("-1"));
                graphS = doc2.toString();
            }
        }
    }
    SKGMainPanel::getMainPanel()->openPage(SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge report plugin")), -1, graphS);
}
