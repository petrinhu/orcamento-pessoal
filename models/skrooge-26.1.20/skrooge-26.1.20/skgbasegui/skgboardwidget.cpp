/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a generic for board widget.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgboardwidget.h"

#include <qdom.h>
#include <qinputdialog.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qwidgetaction.h>

#include <klocalizedstring.h>

#include "skgmainpanel.h"
#include "skgtraces.h"
#include "skgzoomselector.h"

SKGBoardWidget::SKGBoardWidget(QWidget *iParent, SKGDocument *iDocument, const QString &iTitle, bool iZoomable)
    : SKGWidget(iParent, iDocument)
    , m_menu(nullptr)
    , m_zoomMenu(nullptr)
    , m_zoomRatio(1.0)
    , m_title(iTitle)
    , m_titleDefault(iTitle)
{
    SKGTRACEINFUNC(10)

    // Created widgets
    auto horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setSpacing(0);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    m_frame = new QFrame(this);
    m_frame->setObjectName(QStringLiteral("frame"));
    m_frame->setFrameShape(QFrame::StyledPanel);
    m_frame->setFrameShadow(QFrame::Raised);
    m_gridLayout = new QGridLayout(m_frame);
    m_gridLayout->setSpacing(2);
    m_gridLayout->setContentsMargins(0, 0, 0, 0);

    m_toolButton = new QToolButton(m_frame);
    m_toolButton->setIconSize(QSize(16, 16));
    m_toolButton->setMaximumSize(QSize(22, 22));
    m_toolButton->setPopupMode(QToolButton::InstantPopup);
    m_toolButton->setAutoRaise(true);
    m_toolButton->hide();

    m_gridLayout->addWidget(m_toolButton, 0, 0, 1, 1);

    m_Title = new QLabel(m_frame);
    QFont boldFont = m_Title->font();
    boldFont.setBold(true);
    m_Title->setFont(boldFont);
    m_Title->setAlignment(Qt::AlignCenter);
    m_Title->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    getDragWidget()->setCursor(QCursor(Qt::OpenHandCursor));

    m_gridLayout->addWidget(m_Title, 0, 1, 1, 1);

    m_line = new QFrame(m_frame);
    m_line->setFrameShape(QFrame::HLine);
    m_line->setFrameShadow(QFrame::Sunken);

    m_gridLayout->addWidget(m_line, 1, 0, 1, 2);

    horizontalLayout->addWidget(m_frame);

    // Add default actions
    auto w = new QWidget(this);
    auto hlayoutMove = new QHBoxLayout(w);
    hlayoutMove->setSpacing(2);
    hlayoutMove->setContentsMargins(0, 0, 0, 0);

    auto pbFirst = new QPushButton(w);
    pbFirst->setToolTip(i18nc("Move tooltip", "Move first"));
    pbFirst->setIcon(SKGServices::fromTheme(QStringLiteral("go-first-view")));
    pbFirst->setMaximumSize(QSize(22, 22));
    pbFirst->setFlat(true);
    connect(pbFirst, &QPushButton::clicked, this, &SKGBoardWidget::requestMoveFirst);
    hlayoutMove->addWidget(pbFirst);

    auto pbBefore = new QPushButton(w);
    pbBefore->setToolTip(i18nc("Move tooltip", "Move before"));
    pbBefore->setIcon(SKGServices::fromTheme(QStringLiteral("go-previous-view")));
    pbBefore->setMaximumSize(QSize(22, 22));
    pbBefore->setFlat(true);
    connect(pbBefore, &QPushButton::clicked, this, &SKGBoardWidget::requestMoveBefore);
    hlayoutMove->addWidget(pbBefore);

    auto pbDelete = new QPushButton(w);
    pbDelete->setToolTip(i18nc("Move tooltip", "Delete"));
    pbDelete->setIcon(SKGServices::fromTheme(QStringLiteral("edit-delete")));
    pbDelete->setMaximumSize(QSize(22, 22));
    pbDelete->setFlat(true);
    connect(pbDelete, &QPushButton::clicked, this, &SKGBoardWidget::requestRemove);
    hlayoutMove->addWidget(pbDelete);

    auto pbAfter = new QPushButton(w);
    pbAfter->setToolTip(i18nc("Move tooltip", "Move after"));
    pbAfter->setIcon(SKGServices::fromTheme(QStringLiteral("go-next-view")));
    pbAfter->setMaximumSize(QSize(22, 22));
    pbAfter->setFlat(true);
    connect(pbAfter, &QPushButton::clicked, this, &SKGBoardWidget::requestMoveAfter);
    hlayoutMove->addWidget(pbAfter);

    auto pbLast = new QPushButton(w);
    pbLast->setToolTip(i18nc("Move tooltip", "Move last"));
    pbLast->setIcon(SKGServices::fromTheme(QStringLiteral("go-last-view")));
    pbLast->setMaximumSize(QSize(22, 22));
    pbLast->setFlat(true);
    connect(pbLast, &QPushButton::clicked, this, &SKGBoardWidget::requestMoveLast);
    hlayoutMove->addWidget(pbLast);

    auto moveWidget = new QWidgetAction(this);
    moveWidget->setDefaultWidget(w);
    addAction(moveWidget);

    if (iZoomable) {
        m_zoomMenu = new SKGZoomSelector(this);
        m_zoomMenu->setResetValue(-10);
        m_zoomMenu->setValue(-10, false);
        connect(m_zoomMenu, &SKGZoomSelector::changed, this, &SKGBoardWidget::onZoom);

        auto zoomWidget = new QWidgetAction(this);
        zoomWidget->setDefaultWidget(m_zoomMenu);
        addAction(zoomWidget);
    }

    m_menuRename = new QAction(SKGServices::fromTheme(QStringLiteral("edit-rename")), i18nc("Verb, change the name of an item", "Rename"), this);
    connect(m_menuRename, &QAction::triggered, this, &SKGBoardWidget::onRenameTitle);
    addAction(m_menuRename);

    auto sep = new QAction(this);
    sep->setSeparator(true);
    addAction(sep);

    // Set main title
    setMainTitle(iTitle);

    // Set default icon
    m_toolButton->setIcon(SKGServices::fromTheme(QStringLiteral("configure")));
}

SKGBoardWidget::~SKGBoardWidget()
{
    SKGTRACEINFUNC(10)
    m_menuRename = nullptr;
}

QWidget *SKGBoardWidget::getDragWidget()
{
    return m_Title;
}

double SKGBoardWidget::getZoomRatio()
{
    return m_zoomRatio;
}

void SKGBoardWidget::onZoom(int iZoom)
{
    setZoomRatio((iZoom + 15.0) / 5.0);
}

void SKGBoardWidget::requestMoveAfter()
{
    Q_EMIT requestMove(1);
}

void SKGBoardWidget::requestMoveBefore()
{
    Q_EMIT requestMove(-1);
}

void SKGBoardWidget::requestMoveFirst()
{
    Q_EMIT requestMove(-100000);
}

void SKGBoardWidget::requestMoveLast()
{
    Q_EMIT requestMove(100000);
}

void SKGBoardWidget::setZoomRatio(double iRatio)
{
    if (m_zoomMenu != nullptr) {
        if (m_zoomRatio == 1.0) {
            // Memorize initial size
            m_initialSize = minimumSize();
        }

        // Move zoom widget
        m_zoomRatio = iRatio;
        if (m_zoomRatio < 1.0) {
            m_zoomRatio = 1.0;
        } else if (m_zoomRatio > 5.0) {
            m_zoomRatio = 5.0;
        }
        m_zoomMenu->setValue(5.0 * iRatio - 15.0, false);

        // Resize widget
        QSize newSize(m_initialSize.width() * iRatio, m_initialSize.height() * iRatio);
        setMinimumSize(newSize);
    }
}

void SKGBoardWidget::setState(const QString &iState)
{
    QDomDocument doc(QStringLiteral("SKGML"));
    if (doc.setContent(iState)) {
        QDomElement root = doc.documentElement();

        QString title = root.attribute(QStringLiteral("title"));
        if (!title.isEmpty()) {
            m_title = title;
            setMainTitle(title);
        }
    }
}

QString SKGBoardWidget::getState()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("title"), m_title);
    return doc.toString();
}

void SKGBoardWidget::onRenameTitle()
{
    bool ok = false;
    QString newTitle = QInputDialog::getText(SKGMainPanel::getMainPanel(),
                                             i18nc("Question", "Title"),
                                             i18nc("Question", "New title (Empty to retrieve the default title):"),
                                             QLineEdit::Normal,
                                             getOriginalTitle(),
                                             &ok);
    if (ok) {
        m_title = newTitle;
        if (m_title.isEmpty()) {
            m_title = m_titleDefault;
        }
        setMainTitle(m_title);
    }
}

void SKGBoardWidget::setMainWidget(QWidget *iWidget)
{
    iWidget->setParent(m_frame);
    m_gridLayout->addWidget(iWidget, 2, 0, 1, 2);
}

void SKGBoardWidget::insertAction(int iPos, QAction *iAction)
{
    auto l = getMenu()->actions();
    getMenu()->insertAction(l[iPos], iAction);

    // Change icon
    if (!iAction->isCheckable() && !iAction->isSeparator()) {
        m_toolButton->setIcon(SKGServices::fromTheme(QStringLiteral("run-build-configure")));
    }
}

QMenu *SKGBoardWidget::getMenu()
{
    if (m_menu == nullptr) {
        m_menu = new QMenu(this);
        m_toolButton->show();
        m_toolButton->setMenu(m_menu);
    }

    return m_menu;
}

void SKGBoardWidget::addMenu(QMenu *iMenu)
{
    getMenu()->addMenu(iMenu);
}

void SKGBoardWidget::addAction(QAction *iAction)
{
    getMenu()->addAction(iAction);

    // Change icon
    if (!iAction->isCheckable() && !iAction->isSeparator()) {
        m_toolButton->setIcon(SKGServices::fromTheme(QStringLiteral("run-build-configure")));
    }
}

void SKGBoardWidget::hideTitle()
{
    m_toolButton->hide();
    m_Title->hide();
    m_line->hide();
}

void SKGBoardWidget::setMainTitle(const QString &iTitle)
{
    m_Title->setText(iTitle);
}

QString SKGBoardWidget::getMainTitle()
{
    return m_Title->text();
}

QString SKGBoardWidget::getOriginalTitle()
{
    return m_title;
}

#include "skgboardwidget.h"
