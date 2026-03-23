/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A dashboard widget
 *
 * @author Stephane MANKOWSKI
 */
#include "skgdashboardwidget.h"

#include <kaboutdata.h>
#include <ktitlewidget.h>

#include <qactiongroup.h>
#include <qdom.h>
#include <qdrag.h>
#include <qevent.h>
#include <qgridlayout.h>
#include <qmenu.h>
#include <qmimedata.h>

#include "skgboardwidget.h"
#include "skgdashboardplugin.h"
#include "skgdocument.h"
#include "skgflowlayout.h"
#include "skginterfaceplugin.h"
#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgzoomselector.h"
#include <utility>

SKGDashboardWidget::SKGDashboardWidget(QWidget *iParent, SKGDocument *iDocument, QMenu *iMenu)
    : SKGWidget(iParent, iDocument)
    , m_flowLayout(nullptr)
    , m_menu(nullptr)
    , m_addMenu(nullptr)
    , m_layoutF(nullptr)
    , m_layout1(nullptr)
    , m_layout2(nullptr)
    , m_layout3(nullptr)
    , m_layout4(nullptr)
    , m_content(nullptr)
    , m_layout(0)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    if (iMenu) {
        m_addMenu = iMenu;
    } else {
        // Create a context menu for adding widgets
        setContextMenuPolicy(Qt::CustomContextMenu);

        m_menu = new QMenu(this);
        setContextMenuPolicy(Qt::CustomContextMenu);
        m_menu = new QMenu(this);

        connect(this, &SKGDashboardWidget::customContextMenuRequested, this, &SKGDashboardWidget::showHeaderMenu);
        m_addMenu = m_menu->addMenu(SKGServices::fromTheme(QStringLiteral("configure")), i18nc("Verb", "Configure the dashboard"));
    }

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(2);

    if (!iMenu) {
        auto horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(2);

        auto kTitle = new KTitleWidget(this);
        kTitle->setIcon(QApplication::windowIcon(), KTitleWidget::ImageLeft);
        kTitle->setIconSize(QSize(32, 32));

        kTitle->setText(i18nc("Message", "Welcome to %1", KAboutData::applicationData().displayName()));
        kTitle->setLevel(1);

        horizontalLayout->addWidget(kTitle);

        auto kAddWidget = new QToolButton(this);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(kAddWidget->sizePolicy().hasHeightForWidth());
        kAddWidget->setSizePolicy(sizePolicy);
        kAddWidget->setFocusPolicy(Qt::NoFocus);
        kAddWidget->setCheckable(false);
        kAddWidget->setChecked(false);
        kAddWidget->setAutoRaise(true);

        horizontalLayout->addWidget(kAddWidget);

        verticalLayout->addLayout(horizontalLayout);

        auto kContainer = new QScrollArea(this);
        kContainer->setFrameShape(QFrame::NoFrame);
        kContainer->setWidgetResizable(true);
        kContainer->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);
        m_content = new QWidget();
        m_content->setGeometry(QRect(0, 0, 601, 237));
        kContainer->setWidget(m_content);

        verticalLayout->addWidget(kContainer);

        // Plug buttons with menus
        if ((m_addMenu != nullptr) && (kAddWidget != nullptr)) {
            kAddWidget->setIcon(m_addMenu->icon());
            kAddWidget->setMenu(m_addMenu);
            kAddWidget->setPopupMode(QToolButton::InstantPopup);
        }
    } else {
        m_content = new QWidget();
        m_content->setGeometry(QRect(0, 0, 601, 237));

        verticalLayout->addWidget(m_content);
    }

    // Drag and drop
    m_clickedPoint = QPoint(-1, -1);

    // Build menu
    if (m_addMenu != nullptr) {
        // Add layouts sub menu
        auto layoutMenu = m_addMenu->addMenu(SKGServices::fromTheme(QStringLiteral("labplot-editbreaklayout")), i18nc("Verb", "Choose layout …"));
        auto alignmentGroup = new QActionGroup(this);
        {
            m_layoutF = layoutMenu->addAction(i18nc("Verb", "Flow"));
            if (m_layoutF != nullptr) {
                m_layoutF->setIcon(SKGServices::fromTheme(QStringLiteral("text-flow-into-frame")));
                m_layoutF->setCheckable(true);
                m_layoutF->setData(0);
                connect(m_layoutF, &QAction::triggered, this, &SKGDashboardWidget::onChangeLayout);

                alignmentGroup->addAction(m_layoutF);
            }
        }
        {
            m_layout1 = layoutMenu->addAction(i18nc("Verb", "1 column"));
            if (m_layout1 != nullptr) {
                m_layout1->setIcon(SKGServices::fromTheme(QStringLiteral("object-columns")));
                m_layout1->setCheckable(true);
                m_layout1->setData(1);
                connect(m_layout1, &QAction::triggered, this, &SKGDashboardWidget::onChangeLayout);

                alignmentGroup->addAction(m_layout1);
            }
        }
        {
            m_layout2 = layoutMenu->addAction(i18nc("Verb", "2 columns"));
            if (m_layout2 != nullptr) {
                m_layout2->setIcon(SKGServices::fromTheme(QStringLiteral("object-columns")));
                m_layout2->setCheckable(true);
                m_layout2->setData(2);
                connect(m_layout2, &QAction::triggered, this, &SKGDashboardWidget::onChangeLayout);

                alignmentGroup->addAction(m_layout2);
            }
        }
        {
            m_layout3 = layoutMenu->addAction(i18nc("Verb", "3 columns"));
            if (m_layout3 != nullptr) {
                m_layout3->setIcon(SKGServices::fromTheme(QStringLiteral("object-columns")));
                m_layout3->setCheckable(true);
                m_layout3->setData(3);
                connect(m_layout3, &QAction::triggered, this, &SKGDashboardWidget::onChangeLayout);

                alignmentGroup->addAction(m_layout3);
            }
        }
        {
            m_layout4 = layoutMenu->addAction(i18nc("Verb", "4 columns"));
            if (m_layout4 != nullptr) {
                m_layout4->setIcon(SKGServices::fromTheme(QStringLiteral("object-columns")));
                m_layout4->setCheckable(true);
                m_layout4->setData(4);
                connect(m_layout4, &QAction::triggered, this, &SKGDashboardWidget::onChangeLayout);

                alignmentGroup->addAction(m_layout4);
            }
        }
        if (m_layoutF) {
            m_layoutF->setChecked(true);
        }
        auto sep = new QAction(m_addMenu);
        sep->setSeparator(true);
        m_addMenu->addAction(sep);

        int index = 0;
        int added = 0;
        auto currentMenu = m_addMenu;
        while (index >= 0) {
            SKGInterfacePlugin *plugin = SKGMainPanel::getMainPanel()->getPluginByIndex(index);
            if (plugin != nullptr) {
                int nbdbw = plugin->getNbDashboardWidgets();
                for (int j = 0; j < nbdbw; ++j) {
                    added++;
                    if (added % 10 == 0) {
                        currentMenu = currentMenu->addMenu(SKGServices::fromTheme(QStringLiteral("list-add")), i18nc("Verb", "More …"));
                    }

                    // Create menu
                    QAction *act = currentMenu->addAction(plugin->getDashboardWidgetTitle(j));
                    if (act != nullptr) {
                        act->setIcon(SKGServices::fromTheme(plugin->icon()));
                        act->setData(QString(plugin->objectName() % u'-' % SKGServices::intToString(j)));

                        connect(act, &QAction::triggered, this, &SKGDashboardWidget::onAddWidget);
                    }
                }
            } else {
                index = -2;
            }
            ++index;
        }
    }

    // Build layout
    m_flowLayout = new SKGFlowLayout(m_content, 0, 0, 0);
}

SKGDashboardWidget::~SKGDashboardWidget()
{
    SKGTRACEINFUNC(1)
    m_menu = nullptr;
    m_addMenu = nullptr;
    m_flowLayout = nullptr;
    m_layoutF = nullptr;
    m_layout1 = nullptr;
    m_layout2 = nullptr;
    m_layout3 = nullptr;
    m_layout4 = nullptr;
    m_content = nullptr;
}

QString SKGDashboardWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);
    root.setAttribute(QStringLiteral("layout"), m_layout);

    int nb = m_items.count();
    for (int i = 0; i < nb; ++i) {
        QDomElement element = doc.createElement("ITEM-" % SKGServices::intToString(i + 1));
        root.appendChild(element);

        QStringList param = SKGServices::splitCSVLine(m_items.at(i), '-');
        SKGBoardWidget *item = m_itemsPointers.at(i);
        if (item != nullptr) {
            element.setAttribute(QStringLiteral("name"), param.at(0));
            element.setAttribute(QStringLiteral("index"), param.at(1));
            element.setAttribute(QStringLiteral("state"), item->getState());
            element.setAttribute(QStringLiteral("zoom"), SKGServices::intToString(item->getZoomRatio() * 5 - 15));
        }
    }
    return doc.toString();
}

void SKGDashboardWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString layout = root.attribute(QStringLiteral("layout"));
    if (!layout.isEmpty()) {
        m_layout = SKGServices::stringToInt(layout);
    }

    // Initialisation
    int nb = (m_flowLayout != nullptr ? m_flowLayout->count() : 0);
    for (int i = 0; i < nb; ++i) {
        SKGBoardWidget *item = m_itemsPointers.at(0);
        if (item != nullptr) {
            m_flowLayout->removeWidget(item);
            item->hide();

            m_items.removeAt(0);
            m_itemsPointers.removeAt(0);

            item->deleteLater();
        }
    }

    // Create new layout
    if (m_flowLayout) {
        delete m_flowLayout;
    }
    if (m_layout == 0) {
        m_flowLayout = new SKGFlowLayout(m_content, 0, 0, 0);
    } else {
        m_flowLayout = new QGridLayout(m_content);
    }
    m_content->setLayout(m_flowLayout);

    int index = 1;
    while (index > 0) {
        QDomElement element = root.firstChildElement("ITEM-" % SKGServices::intToString(index));
        if (!element.isNull()) {
            SKGInterfacePlugin *plugin = SKGMainPanel::getMainPanel()->getPluginByName(element.attribute(QStringLiteral("name")));
            QString indexString = element.attribute(QStringLiteral("index"));
            if (indexString.isEmpty()) {
                indexString = '0';
            }
            QString zoom = element.attribute(QStringLiteral("zoom"));
            if (zoom.isEmpty()) {
                zoom = '0';
            }
            if (plugin != nullptr) {
                addItem(plugin, SKGServices::stringToInt(indexString), SKGServices::stringToInt(zoom), element.attribute(QStringLiteral("state")));
            }
        } else {
            index = -1;
        }
        ++index;
    }

    // In case of reset
    if (m_items.isEmpty() && root.attribute(QStringLiteral("zoomPosition")).isEmpty()) {
        int index2 = 0;
        while (index2 >= 0) {
            SKGInterfacePlugin *plugin = SKGMainPanel::getMainPanel()->getPluginByIndex(index2);
            if (plugin != nullptr && dynamic_cast<SKGDashboardPlugin *>(plugin) == nullptr) {
                int nb2 = plugin->getNbDashboardWidgets();
                for (int j = 0; j < nb2; ++j) {
                    addItem(plugin, j);
                }
            } else {
                index2 = -2;
            }
            ++index2;
        }
    }
}

QList<QWidget *> SKGDashboardWidget::printableWidgets()
{
    QList<QWidget *> output;
    output.reserve(m_itemsPointers.count());
    for (auto w : std::as_const(m_itemsPointers)) {
        output.push_back(w);
    }

    return output;
}

bool SKGDashboardWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::HoverLeave) {
        // Leave widget
        m_timer.stop();
        return true;
    }

    if ((iEvent != nullptr) && (iObject != nullptr)
        && (iEvent->type() == QEvent::MouseButtonPress || iEvent->type() == QEvent::MouseButtonRelease || iEvent->type() == QEvent::MouseMove
            || iEvent->type() == QEvent::DragEnter || iEvent->type() == QEvent::DragMove || iEvent->type() == QEvent::Drop
            || iEvent->type() == QEvent::HoverMove)) {
        // Search SKGBoardWidget corresponding to this widget
        SKGBoardWidget *toMove = nullptr;
        int toMoveIndex = -1;
        int nb = m_itemsPointers.count();
        for (int i = 0; toMove == nullptr && i < nb; ++i) {
            SKGBoardWidget *w = m_itemsPointers.at(i);
            if ((w != nullptr) && w->getDragWidget() == iObject) {
                toMove = w;
                toMoveIndex = i;
            }
        }

        if (iEvent->type() == QEvent::MouseButtonPress) {
            // Drag
            auto *mevent = dynamic_cast<QMouseEvent *>(iEvent);
            if (mevent && mevent->button() == Qt::LeftButton) {
                m_clickedPoint = mevent->pos();
                m_timer.stop();
            }
        } else if (iEvent->type() == QEvent::MouseButtonRelease) {
            // Drag
            auto *mevent = dynamic_cast<QMouseEvent *>(iEvent);
            if (mevent && mevent->button() == Qt::LeftButton) {
                m_clickedPoint = QPoint(-1, -1);
            }
        } else if (iEvent->type() == QEvent::MouseMove) {
            // Drag
            if (m_clickedPoint != QPoint(-1, -1) && toMoveIndex != -1) {
                auto *mevent = dynamic_cast<QMouseEvent *>(iEvent);
                if (mevent) {
                    int distance = (mevent->pos() - m_clickedPoint).manhattanLength();
                    if (distance >= QApplication::startDragDistance()) {
                        auto mimeData = new QMimeData;
                        mimeData->setData(QStringLiteral("application/x-SKGDashboardWidget"), SKGServices::intToString(toMoveIndex).toLatin1());

                        auto drag = new QDrag(this);
                        drag->setMimeData(mimeData);
                        drag->exec(); // krazy:exclude=crashy

                        return true;
                    }
                }
            }
        } else if (iEvent->type() == QEvent::DragEnter) {
            // Drop move
            auto *devent = dynamic_cast<QDragEnterEvent *>(iEvent);
            if (devent && devent->mimeData()->hasFormat(QStringLiteral("application/x-SKGDashboardWidget"))) {
                devent->accept();

                return true;
            }
        } else if (iEvent->type() == QEvent::DragMove) {
            // Drop move
            auto *devent = dynamic_cast<QDragMoveEvent *>(iEvent);
            if (devent && devent->mimeData()->hasFormat(QStringLiteral("application/x-SKGDashboardWidget"))) {
                int oldPos = SKGServices::stringToInt(devent->mimeData()->data(QStringLiteral("application/x-SKGDashboardWidget")));
                if (oldPos != toMoveIndex) {
                    devent->accept();
                } else {
                    devent->ignore();
                }

                return true;
            }
        } else if (iEvent->type() == QEvent::Drop) {
            // Drop
            auto *devent = dynamic_cast<QDropEvent *>(iEvent);
            if (devent && devent->mimeData()->hasFormat(QStringLiteral("application/x-SKGDashboardWidget"))) {
                int oldPos = SKGServices::stringToInt(devent->mimeData()->data(QStringLiteral("application/x-SKGDashboardWidget")));

                if (oldPos + 1 == toMoveIndex) {
                    ++toMoveIndex;
                }

                // Move item
                if (toMoveIndex > oldPos) {
                    --toMoveIndex;
                }
                moveItem(oldPos, toMoveIndex);

                return true;
            }
        }
    }
    return SKGWidget::eventFilter(iObject, iEvent);
}

void SKGDashboardWidget::showHeaderMenu(const QPoint iPos)
{
    // Display menu
    if (m_menu != nullptr) {
        m_menu->popup(mapToGlobal(iPos));
    }
}

void SKGDashboardWidget::onAddWidget()
{
    auto *send = qobject_cast<QAction *>(this->sender());
    if (send != nullptr) {
        QString id = send->data().toString();
        QStringList param = SKGServices::splitCSVLine(id, '-');

        SKGInterfacePlugin *db = SKGMainPanel::getMainPanel()->getPluginByName(param.at(0));
        if (db != nullptr) {
            addItem(db, SKGServices::stringToInt(param.at(1)));
        }
    }
}

void SKGDashboardWidget::onChangeLayout()
{
    auto *send = qobject_cast<QAction *>(this->sender());
    if (send != nullptr) {
        m_layout = send->data().toInt();
        setState(getState());
    }
}

void SKGDashboardWidget::onMoveWidget(int iMove)
{
    // Get current position
    QWidget *send = qobject_cast<QWidget *>(this->sender());
    if (send != nullptr) {
        int currentPos = m_itemsPointers.indexOf(parentBoardWidget(send));
        int newPos = currentPos + iMove;
        if (newPos < 0) {
            newPos = 0;
        } else if (newPos > m_items.count() - 1) {
            newPos = m_items.count() - 1;
        }

        moveItem(currentPos, newPos);
    }
}

void SKGDashboardWidget::moveItem(int iFrom, int iTo)
{
    // Compute new position
    if (iTo != iFrom) {
        // Move item
        m_items.move(iFrom, iTo);
        m_itemsPointers.move(iFrom, iTo);

        // Build list of items in the right order
        QList<SKGBoardWidget *> listWidgets;
        int nb = m_itemsPointers.count();
        listWidgets.reserve(nb);
        for (int i = 0; i < nb; ++i) {
            SKGBoardWidget *wgt2 = m_itemsPointers.at(i);
            m_flowLayout->removeWidget(wgt2);
            listWidgets.push_back(wgt2);
        }

        // Add items
        nb = listWidgets.count();
        for (int i = 0; i < nb; ++i) {
            SKGBoardWidget *dbw = listWidgets.at(i);
            dbw->setParent(m_content);
            m_flowLayout->addWidget(dbw);
        }
    }
}

SKGBoardWidget *SKGDashboardWidget::parentBoardWidget(QWidget *iWidget)
{
    auto *output = qobject_cast<SKGBoardWidget *>(iWidget);
    if ((output == nullptr) && (iWidget != nullptr)) {
        QWidget *iParent = iWidget->parentWidget();
        if (iParent != nullptr) {
            output = SKGDashboardWidget::parentBoardWidget(iParent);
        }
    }

    return output;
}

void SKGDashboardWidget::onRemoveWidget()
{
    int p = -1;
    QWidget *send = qobject_cast<QWidget *>(this->sender());
    if (send != nullptr) {
        p = m_itemsPointers.indexOf(parentBoardWidget(send));
    }
    if (p >= 0) {
        // Get item
        SKGBoardWidget *wgt = m_itemsPointers.at(p);

        // Delete widget
        m_flowLayout->removeWidget(wgt);
        wgt->hide();
        wgt->deleteLater();

        // Remove item
        m_items.removeAt(p);
        m_itemsPointers.removeAt(p);
    }
}

void SKGDashboardWidget::addItem(SKGInterfacePlugin *iDashboard, int iIndex, int iZoom, const QString &iState)
{
    if ((iDashboard != nullptr) && (m_flowLayout != nullptr)) {
        SKGBoardWidget *dbw = iDashboard->getDashboardWidget(iIndex);
        if (dbw != nullptr) {
            // Add widget
            dbw->setParent(m_content);
            dbw->setState(iState);
            if (m_layout == 0) {
                m_flowLayout->addWidget(dbw);
            } else {
                auto *grid = qobject_cast<QGridLayout *>(m_flowLayout);
                if (grid) {
                    int nbItem = grid->count();
                    int row = nbItem / m_layout;
                    int col = nbItem % m_layout;
                    grid->addWidget(dbw, row, col);
                }
            }

            // Install filter
            QWidget *drag = dbw->getDragWidget();
            if (drag != nullptr) {
                drag->installEventFilter(this);
                drag->setAcceptDrops(true);
                drag->setAttribute(Qt::WA_Hover);
            }

            // Connect widget
            connect(dbw, &SKGBoardWidget::requestRemove, this, &SKGDashboardWidget::onRemoveWidget, Qt::QueuedConnection);
            connect(dbw, &SKGBoardWidget::requestMove, this, &SKGDashboardWidget::onMoveWidget, Qt::QueuedConnection);

            // Set size
            dbw->setZoomRatio((iZoom + 15.0) / 5.0);

            QString id = iDashboard->objectName() % u'-' % SKGServices::intToString(iIndex);
            m_items.push_back(id);
            m_itemsPointers.push_back(dbw);
        }
    }
}
