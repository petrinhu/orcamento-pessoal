/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A tree view with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtreeview.h"

#include <klocalizedstring.h>
#include <kstandardaction.h>

#include <QtCore5Compat/QTextCodec>
#include <qactiongroup.h>
#include <qapplication.h>
#include <qbasictimer.h>
#include <qclipboard.h>
#include <qdesktopservices.h>
#include <qdom.h>
#include <qevent.h>
#include <qfileinfo.h>
#include <qheaderview.h>
#include <qicon.h>
#include <qmenu.h>
#include <qpainter.h>
#include <qprinter.h>
#include <qpushbutton.h>
#include <qsavefile.h>
#include <qscrollbar.h>
#include <qsvggenerator.h>
#include <qtextbrowser.h>
#include <qtextdocumentwriter.h>
#include <qtextobject.h>
#include <qtexttable.h>
#include <qtimer.h>

#include <algorithm>

#include "skgdocument.h"
#include "skgerror.h"
#include "skgmainpanel.h"
#include "skgobjectbase.h"
#include "skgobjectmodelbase.h"
#include "skgpropertyobject.h"
#include "skgservices.h"
#include "skgsortfilterproxymodel.h"
#include "skgtableview.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include <utility>

SKGTreeView::SKGTreeView(QWidget *iParent)
    : QTreeView(iParent)
    , m_autoResize(true)
    , m_autoResizeDone(false)
    , m_actAutoResize(nullptr)
    , m_document(nullptr)
    , m_textResizable(true)
    , m_model(nullptr)
    , m_proxyModel(nullptr)
    , m_actGroupByNone(nullptr)
    , stickH(false)
    , stickV(false)
{
    // Initialize
    setTextElideMode(Qt::ElideMiddle);
    setAutoExpandDelay(300);
    setAnimated(true);

    m_timerDelayedResize.setSingleShot(true);
    connect(&m_timerDelayedResize, &QTimer::timeout, this, &SKGTreeView::resizeColumnsToContents, Qt::QueuedConnection);

    m_timerSelectionChanged.setSingleShot(true);
    connect(&m_timerSelectionChanged, &QTimer::timeout, this, &SKGTreeView::selectionChangedDelayed, Qt::QueuedConnection);

    m_timerScrollSelection.setSingleShot(true);
    connect(&m_timerScrollSelection, &QTimer::timeout, this, &SKGTreeView::scroolOnSelection, Qt::QueuedConnection);

    // Menu
    QHeaderView *hori = header();
    hori->setContextMenuPolicy(Qt::CustomContextMenu);
    m_headerMenu = new QMenu(this);

    setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(hori, &QHeaderView::customContextMenuRequested, this, static_cast<void (SKGTreeView::*)(const QPoint)>(&SKGTreeView::showHeaderMenu));
    connect(hori, &QHeaderView::sortIndicatorChanged, this, &SKGTreeView::onSortChanged);

    //
    m_actCopy = KStandardAction::copy(this, SLOT(copy()), nullptr);
    m_actCopy->setProperty("isShortcutConfigurable", false);
    m_actCopy->setShortcutContext(Qt::WidgetShortcut);

    m_actExpandAll = new QAction(SKGServices::fromTheme(QStringLiteral("format-indent-more")), i18nc("Noun, user action", "Expand all"), this);
    m_actExpandAll->setShortcut(Qt::ALT | Qt::Key_Plus);
    m_actExpandAll->setProperty("isShortcutConfigurable", false);
    m_actExpandAll->setShortcutContext(Qt::WidgetShortcut);
    connect(m_actExpandAll, &QAction::triggered, this, &SKGTreeView::expandAll);

    m_actCollapseAll = new QAction(SKGServices::fromTheme(QStringLiteral("format-indent-less")), i18nc("Noun, user action", "Collapse all"), this);
    m_actCollapseAll->setShortcut(Qt::ALT | Qt::Key_Minus);
    m_actCollapseAll->setProperty("isShortcutConfigurable", false);
    m_actCollapseAll->setShortcutContext(Qt::WidgetShortcut);
    connect(m_actCollapseAll, &QAction::triggered, this, &SKGTreeView::collapseAll);
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGMainPanel::getMainPanel()->registerGlobalAction(QStringLiteral("edit_copy"), m_actCopy);
        SKGMainPanel::getMainPanel()->registerGlobalAction(QStringLiteral("edit_expandall"), m_actExpandAll);
        SKGMainPanel::getMainPanel()->registerGlobalAction(QStringLiteral("edit_collapseall"), m_actCollapseAll);
    }

    // Scroll bars
    connect(horizontalScrollBar(), &QScrollBar::actionTriggered, this, &SKGTreeView::onActionTriggered);
    connect(verticalScrollBar(), &QScrollBar::actionTriggered, this, &SKGTreeView::onActionTriggered);
    connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &SKGTreeView::onRangeChanged);
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &SKGTreeView::onRangeChanged);

    // Headers
    hori->setSectionsMovable(true);
    hori->setSectionResizeMode(QHeaderView::Fixed);
    setWordWrap(false);

    connect(header(), &QHeaderView::sectionMoved, this, &SKGTreeView::setupHeaderMenu, Qt::QueuedConnection);

    connect(this, &SKGTreeView::clicked, this, &SKGTreeView::onClick);
    connect(this, &SKGTreeView::collapsed, this, &SKGTreeView::onCollapse);
    connect(this, &SKGTreeView::expanded, this, &SKGTreeView::onExpand);

    QWidget *vp = this->viewport();
    if (vp != nullptr) {
        vp->installEventFilter(this);
        this->installEventFilter(this);
    }

    // Save original size
    m_fontOriginalPointSize = this->font().pointSize();
    m_iconOriginalSize = this->iconSize().height();
    if (m_iconOriginalSize <= 0) {
        m_iconOriginalSize = 16;
    }
}

SKGTreeView::~SKGTreeView()
{
    m_document = nullptr;
    m_headerMenu = nullptr;
    m_proxyModel = nullptr;
    m_model = nullptr;
    m_actExpandAll = nullptr;
    m_actCollapseAll = nullptr;
}

bool SKGTreeView::isAutoResized()
{
    return m_autoResize;
}

QString SKGTreeView::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    QHeaderView *hHeader = header();
    if ((hHeader != nullptr) && (m_model != nullptr)) {
        if (isSortingEnabled()) {
            root.setAttribute(QStringLiteral("sortOrder"), SKGServices::intToString(static_cast<int>(hHeader->sortIndicatorOrder())));
            root.setAttribute(QStringLiteral("sortColumn"), m_model->getAttribute(hHeader->sortIndicatorSection()));

            if (m_proxyModel != nullptr) {
                root.setAttribute(QStringLiteral("sortPreviousColumn"), SKGServices::intToString(m_proxyModel->getPreviousSortColumn()));
            }
        }
        root.setAttribute(QStringLiteral("groupBy"), m_groupby);

        // Memorize order
        int nb = hHeader->count();
        if (nb != 0) {
            QString columns;
            QString columnsSize;
            QString columnsVisibility;
            for (int i = 0; i < nb; ++i) {
                int idx = hHeader->logicalIndex(i);
                if (i != 0) {
                    columns += ';';
                }
                columns += m_model->getAttribute(idx);

                if (i != 0) {
                    columnsSize += ';';
                }
                columnsSize += SKGServices::intToString(hHeader->sectionSize(idx));

                if (i != 0) {
                    columnsVisibility += ';';
                }
                columnsVisibility += (hHeader->isSectionHidden(idx) ? QStringLiteral("N") : QStringLiteral("Y"));
            }
            root.setAttribute(QStringLiteral("columns"), columns);
            if (!m_autoResize) {
                root.setAttribute(QStringLiteral("columnsSize"), columnsSize);
            }
            root.setAttribute(QStringLiteral("columnsVisibility"), columnsVisibility);
            root.setAttribute(QStringLiteral("columnsAutoResize"), m_autoResize ? QStringLiteral("Y") : QStringLiteral("N"));
        }

        // Memorize expanded groups
        if (!m_groupby.isEmpty()) {
            root.setAttribute(QStringLiteral("expandedGroups"), m_expandedNodes.join(QStringLiteral(";")));
        }
    }
    root.setAttribute(QStringLiteral("alternatingRowColors"), alternatingRowColors() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("zoomPosition"), SKGServices::intToString(zoomPosition()));

    QScrollBar *scroll2 = horizontalScrollBar();
    if ((scroll2 != nullptr) && scroll2->value() == scroll2->maximum() && scroll2->value() != scroll2->minimum()) {
        root.setAttribute(QStringLiteral("stickH"), QStringLiteral("Y"));
    }
    scroll2 = verticalScrollBar();
    if ((scroll2 != nullptr) && scroll2->value() == scroll2->maximum() && scroll2->value() != scroll2->minimum()) {
        root.setAttribute(QStringLiteral("stickV"), QStringLiteral("Y"));
    }
    return doc.toString(-1);
}

void SKGTreeView::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    resetColumnsOrder();

    QDomDocument doc(QStringLiteral("SKGML"));

    QString viewState = iState;
    if (viewState.isEmpty() && (m_document != nullptr)) {
        // Get default state
        viewState = m_document->getParameter(m_parameterName);
    }

    Qt::SortOrder qtsortorder = Qt::AscendingOrder;
    if (doc.setContent(viewState)) {
        QDomElement root = doc.documentElement();

        QString sortOrder = root.attribute(QStringLiteral("sortOrder"));
        QString sortColumn = root.attribute(QStringLiteral("sortColumn"));
        QString sortPreviousColumn = root.attribute(QStringLiteral("sortPreviousColumn"));
        m_groupby = root.attribute(QStringLiteral("groupBy"));
        QString columns = root.attribute(QStringLiteral("columns"));
        QString columnsSize = root.attribute(QStringLiteral("columnsSize"));
        QString columnsVisibility = root.attribute(QStringLiteral("columnsVisibility"));
        QString columnsAutoResize = root.attribute(QStringLiteral("columnsAutoResize"));
        QString alternatingRowColors2 = root.attribute(QStringLiteral("alternatingRowColors"));
        QString zoomPositionString = root.attribute(QStringLiteral("zoomPosition"));
        QString expandedGroups = root.attribute(QStringLiteral("expandedGroups"));
        stickH = (root.attribute(QStringLiteral("stickH")) == QStringLiteral("Y"));
        stickV = (root.attribute(QStringLiteral("stickV")) == QStringLiteral("Y"));

        // Set column order
        QStringList listAtt;
        if (!columns.isEmpty()) {
            listAtt = SKGServices::splitCSVLine(columns, ';');
            QStringList sizes = SKGServices::splitCSVLine(columnsSize, ';');
            QStringList visibilities = SKGServices::splitCSVLine(columnsVisibility, ';');

            int nb = listAtt.count();
            int nbvisibilities = visibilities.count();
            int nbsizes = sizes.count();
            for (int i = 0; i < nb; ++i) {
                if (nbvisibilities == nb) {
                    listAtt[i] = listAtt.at(i) % u'|' % visibilities.at(i);
                    if (nbsizes == nb) {
                        listAtt[i] = listAtt.at(i) % u'|' % sizes.at(i);
                    }
                }
            }
        }
        if (m_model != nullptr) {
            m_model->setSupportedAttributes(listAtt);
        }

        // Set autoResize
        if (!columnsAutoResize.isEmpty()) {
            m_autoResize = (columnsAutoResize == QStringLiteral("Y"));
            header()->setSectionResizeMode(m_autoResize ? QHeaderView::Fixed : QHeaderView::Interactive);
            if (!m_autoResize) {
                m_timerDelayedResize.stop();
                m_autoResizeDone = false;
            }
        }

        // Set sort
        if ((m_proxyModel != nullptr) && !sortPreviousColumn.isEmpty()) {
            m_proxyModel->setPreviousSortColumn(SKGServices::stringToInt(sortPreviousColumn));
        }
        if ((m_model != nullptr) && isSortingEnabled() && !sortOrder.isEmpty() && !sortColumn.isEmpty()) {
            int index = SKGServices::splitCSVLine(columns, ';').indexOf(sortColumn);
            if (index == -1) {
                index = m_model->getIndexAttribute(sortColumn);
            }
            if (index == -1) {
                index = 0;
            }
            qtsortorder = static_cast<Qt::SortOrder>(SKGServices::stringToInt(sortOrder));
            this->sortByColumn(index, qtsortorder);
        }

        if (m_model != nullptr) {
            QString att = m_groupby;
            if (att == QStringLiteral("#")) {
                att = std::move(sortColumn);
            }
            m_model->setGroupBy(att);
            m_model->dataModified();

            refreshExpandCollapse();
        }

        // Set alternatingRowColors
        if (!alternatingRowColors2.isEmpty()) {
            setAlternatingRowColors(alternatingRowColors2 == QStringLiteral("Y"));
        }
        if (!zoomPositionString.isEmpty()) {
            setZoomPosition(SKGServices::stringToInt(zoomPositionString));
        }

        // Set expanded groups
        m_expandedNodes = SKGServices::splitCSVLine(expandedGroups);
        resetSelection();
    } else {
        if (m_model != nullptr) {
            m_model->setSupportedAttributes(QStringList());
            m_groupby = QString();
            m_model->setGroupBy(m_groupby);
            m_model->dataModified();

            refreshExpandCollapse();
        }

        this->sortByColumn(0, qtsortorder);
    }
}

void SKGTreeView::refreshExpandCollapse()
{
    bool treeMode = !m_model->getParentChildAttribute().isEmpty();
    setRootIsDecorated(treeMode && m_groupby.isEmpty());
    if (m_actExpandAll != nullptr) {
        m_actExpandAll->setVisible(treeMode || !m_groupby.isEmpty());
    }
    if (m_actCollapseAll != nullptr) {
        m_actCollapseAll->setVisible(treeMode || !m_groupby.isEmpty());
    }
}

void SKGTreeView::respanFirstColumns()
{
    // Span groups
    int nbRow = m_model->rowCount();
    for (int row = 0; row < nbRow; ++row) {
        this->setFirstColumnSpanned(row, QModelIndex(), !m_groupby.isEmpty());
    }

    if (m_autoResize) {
        resizeColumnsToContentsDelayed();
    }
}

void SKGTreeView::onRangeChanged()
{
    auto *scroll2 = qobject_cast<QScrollBar *>(sender());
    if ((stickH && scroll2 == horizontalScrollBar()) || (stickV && scroll2 == verticalScrollBar())) {
        scroll2->setValue(scroll2->maximum());
    }
}

void SKGTreeView::onActionTriggered(int action)
{
    auto *scroll2 = qobject_cast<QScrollBar *>(sender());
    if ((scroll2 != nullptr) && action == QAbstractSlider::SliderToMaximum) {
        if (scroll2 == horizontalScrollBar()) {
            stickH = true;
        }
        if (scroll2 == verticalScrollBar()) {
            stickV = true;
        }
    } else {
        if (scroll2 == horizontalScrollBar()) {
            stickH = false;
        }
        if (scroll2 == verticalScrollBar()) {
            stickV = false;
        }
    }
}

void SKGTreeView::insertGlobalAction(const QString &iRegisteredAction)
{
    if (iRegisteredAction.isEmpty()) {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        this->insertAction(nullptr, sep);
    } else if (SKGMainPanel::getMainPanel() != nullptr) {
        QAction *act = SKGMainPanel::getMainPanel()->getGlobalAction(iRegisteredAction);
        this->insertAction(nullptr, act);
    }
}

void SKGTreeView::resizeColumnsToContentsDelayed()
{
    SKGTRACEINFUNC(10)
    m_timerDelayedResize.start(300);
}

void SKGTreeView::resizeColumnsToContents()
{
    SKGTRACEINFUNC(10)
    {
        SKGTRACEIN(10, "SKGTreeView::resizeColumnsToContents-respanFirstColumns")
        auto save = m_autoResize;
        m_autoResize = false; // To avoid loop
        respanFirstColumns();
        m_autoResize = save;
    }

    int nb = header()->count();
    for (int i = nb - 1; i >= 0; --i) {
        SKGTRACEIN(10, "SKGTreeView::resizeColumnsToContents-resizeColumnToContents(" + SKGServices::intToString(i) + ')')
        if (!isColumnHidden(i)) {
            resizeColumnToContents(i);
        }
    }
}

void SKGTreeView::showHeaderMenu()
{
    showHeaderMenu(header()->mapFromGlobal(QCursor::pos()));
}

void SKGTreeView::showHeaderMenu(const QPoint iPos)
{
    if (m_headerMenu != nullptr) {
        m_headerMenu->popup(header()->mapToGlobal(iPos));
    }
}

void SKGTreeView::setDefaultSaveParameters(SKGDocument *iDocument, const QString &iParameterName)
{
    m_document = iDocument;
    m_parameterName = iParameterName;
}

void SKGTreeView::setupHeaderMenu()
{
    SKGTRACEINFUNC(10)
    if ((m_model != nullptr) && m_model->isRefreshBlocked()) {
        return;
    }

    setCornerWidget(nullptr);
    if ((m_model != nullptr) && (m_headerMenu != nullptr)) {
        // Corner button on tables to show the contextual menu
        auto btn = new QPushButton(this);
        btn->setIcon(SKGServices::fromTheme(QStringLiteral("configure")));
        btn->setFocusPolicy(Qt::NoFocus);
        connect(btn, &QPushButton::clicked, this, static_cast<void (SKGTreeView::*)()>(&SKGTreeView::showHeaderMenu));
        setCornerWidget(btn);

        m_headerMenu->clear();

        // Processing
        QMenu *columns = m_headerMenu->addMenu(i18nc("Noun, Menu name", "Columns"));

        // Get current groupby column
        QMenu *groupbymenu = nullptr;
        QActionGroup *groupby = nullptr;
        if (!m_model->getWhereClause().contains(QStringLiteral("ORDER BY"))) {
            groupbymenu = m_headerMenu->addMenu(i18nc("Noun, Menu name", "Group by"));
            groupby = new QActionGroup(groupbymenu);
            m_actGroupByNone = groupbymenu->addAction(i18nc("Noun, grouping option", "None"));
            if (m_actGroupByNone != nullptr) {
                m_actGroupByNone->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-cancel")));
                m_actGroupByNone->setCheckable(true);
                m_actGroupByNone->setChecked(m_groupby.isEmpty());
                m_actGroupByNone->setData(QString());
                groupby->addAction(m_actGroupByNone);
            }
            if (m_proxyModel != nullptr) {
                QAction *actSort = groupbymenu->addAction(i18nc("Noun, grouping option", "Sorted column"));
                if (actSort != nullptr) {
                    actSort->setIcon(SKGServices::fromTheme(QStringLiteral("view-sort-ascending")));
                    actSort->setCheckable(true);
                    actSort->setChecked(m_groupby == QStringLiteral("#"));
                    actSort->setData(QStringLiteral("#"));
                    groupby->addAction(actSort);
                }
            }
            groupbymenu->addSeparator();
        }

        // Set right click menu
        SKGDocument::SKGModelTemplateList schemas = m_model->getSchemas();
        int nbSchemas = schemas.count();
        if (nbSchemas != 0) {
            QMenu *viewAppearanceMenu =
                columns->addMenu(SKGServices::fromTheme(QStringLiteral("view-file-columns")), i18nc("Noun, user action", "View appearance"));

            for (int i = 0; i < nbSchemas; ++i) {
                SKGDocument::SKGModelTemplate schema = schemas.at(i);
                QAction *act = viewAppearanceMenu->addAction(schema.name);
                if (!schema.icon.isEmpty()) {
                    act->setIcon(SKGServices::fromTheme(schema.icon));
                }
                act->setData(schema.schema);

                connect(act, &QAction::triggered, this, &SKGTreeView::changeSchema);
            }
        }

        QAction *actResize = columns->addAction(SKGServices::fromTheme(QStringLiteral("zoom-fit-width")), i18nc("Noun, user action", "Resize to content"));
        connect(actResize, &QAction::triggered, this, &SKGTreeView::resizeColumnsToContents);

        m_actAutoResize = columns->addAction(SKGServices::fromTheme(QStringLiteral("zoom-fit-width"), QStringList() << QStringLiteral("run-build")),
                                             i18nc("Noun, user action", "Auto resize"));
        m_actAutoResize->setCheckable(true);
        m_actAutoResize->setChecked(m_autoResize);
        connect(m_actAutoResize, &QAction::triggered, this, &SKGTreeView::switchAutoResize);

        QAction *actAlternatingRowColors = m_headerMenu->addAction(i18nc("Noun, user action", "Alternate row colors"));
        if (actAlternatingRowColors != nullptr) {
            actAlternatingRowColors->setCheckable(true);
            actAlternatingRowColors->setChecked(alternatingRowColors());
            connect(actAlternatingRowColors, &QAction::triggered, this, &SKGTreeView::setAlternatingRowColors);
        }

        if (m_document != nullptr) {
            QAction *actDefault =
                m_headerMenu->addAction(SKGServices::fromTheme(QStringLiteral("document-save")), i18nc("Noun, user action", "Save parameters"));
            connect(actDefault, &QAction::triggered, this, &SKGTreeView::saveDefaultClicked);
        }

        columns->addSeparator();

        // Build menus for columns
        QHeaderView *hHeader = header();
        int nbcol = hHeader->count();
        for (int i = 0; i < nbcol; ++i) {
            int idx = hHeader->logicalIndex(i);
            QString col = m_model->headerData(idx, Qt::Horizontal, Qt::UserRole).toString();
            QStringList values = col.split('|');

            if (!m_autoResizeDone) {
                if (values.count() > 1) {
                    hHeader->setSectionHidden(idx, values.at(1) == QStringLiteral("N"));
                }
                if (values.count() > 2) {
                    auto s = SKGServices::stringToInt(values.at(2));
                    if (s > 0) {
                        hHeader->resizeSection(idx, s);
                    }
                }
            }

            // Column menu
            QAction *act = columns->addAction(values.at(0));
            if (act != nullptr) {
                act->setCheckable(true);
                act->setChecked(!hHeader->isSectionHidden(idx));
                act->setIcon(m_model->headerData(idx, Qt::Horizontal, Qt::DecorationRole).value<QIcon>());
                act->setData(idx);
                act->setEnabled(i > 0);

                connect(act, &QAction::triggered, this, &SKGTreeView::showHideColumn);

                // Group by menu
                QString att = m_model->getAttribute(idx);
                if ((groupbymenu != nullptr) && (groupby != nullptr)) {
                    QAction *act2 = groupbymenu->addAction(values.at(0));
                    if (act2 != nullptr) {
                        act2->setCheckable(true);
                        act2->setChecked(att == m_groupby);
                        act2->setIcon(act->icon());
                        act2->setData(att);
                        groupby->addAction(act2);
                    }
                }
            }
        }
        m_autoResizeDone = true;
        if (groupby != nullptr) {
            connect(groupby, &QActionGroup::triggered, this, &SKGTreeView::groupByChanged);
        }
        m_headerMenu->addSeparator();

        QAction *actExport = m_headerMenu->addAction(SKGServices::fromTheme(QStringLiteral("document-export")), i18nc("Noun, user action", "Export…"));
        connect(actExport, &QAction::triggered, this, &SKGTreeView::onExport);

        if (m_autoResize) {
            resizeColumnsToContentsDelayed();
        }
    }
}

void SKGTreeView::setSelectionModel(QItemSelectionModel *iSelectionModel)
{
    if (this->selectionModel() != nullptr) {
        disconnect(this->selectionModel(), &QItemSelectionModel::selectionChanged, this, &SKGTreeView::onSelectionChanged);
    }
    QTreeView::setSelectionModel(iSelectionModel);
    if (iSelectionModel != nullptr) {
        connect(iSelectionModel, &QItemSelectionModel::selectionChanged, this, &SKGTreeView::onSelectionChanged);
    }
}

void SKGTreeView::onSelectionChanged()
{
    SKGTRACEINFUNC(10)
    SKGObjectBase::SKGListSKGObjectBase selection;
    SKGObjectBase::SKGListSKGObjectBase selectionparents;
    QItemSelectionModel *selModel = selectionModel();
    if (selModel != nullptr) {
        if (m_model != nullptr) {
            auto indexes = selModel->selectedRows();

            int nb = indexes.count();
            QVector<QModelIndex> indexesToSource;
            indexesToSource.reserve(indexes.count());
            for (const auto &index : std::as_const(indexes)) {
                indexesToSource.push_back(m_proxyModel != nullptr ? m_proxyModel->mapToSource(index) : index);
            }

            selection.reserve(indexesToSource.count());
            selectionparents.reserve(nb);
            for (int i = 0; i < nb; ++i) {
                QModelIndex index = indexes.at(i);
                QModelIndex idxs = indexesToSource.at(i);

                // Get above
                QModelIndex idxtmp = (m_proxyModel != nullptr ? m_proxyModel->mapToSource(indexAbove(index)) : indexAbove(index));
                auto *tmp = m_model->getObjectPointer(idxtmp);
                if (tmp != nullptr && idxtmp.parent() == idxs.parent() && !indexesToSource.contains(idxtmp)) {
                    selectionparents.push_back(*tmp);
                } else {
                    // Get below
                    idxtmp = (m_proxyModel != nullptr ? m_proxyModel->mapToSource(indexBelow(index)) : indexBelow(index));
                    tmp = m_model->getObjectPointer(idxtmp);
                    if (tmp != nullptr && idxtmp.parent() == idxs.parent() && !indexesToSource.contains(idxtmp)) {
                        selectionparents.push_back(*tmp);
                    } else {
                        // Get parent
                        tmp = m_model->getObjectPointer(idxs.parent());
                        if (tmp != nullptr) {
                            selectionparents.push_back(*tmp);
                        }
                    }
                }

                SKGObjectBase obj = m_model->getObject(idxs);
                selection.push_back(obj);
            }
        }
    }

    //    if (selection != m_lastSelection) {  // WARNING: selection can be equal but attributes of selected objects modified
    m_lastSelection = std::move(selection);
    m_lastSelection_if_deleted = std::move(selectionparents);
    m_timerSelectionChanged.start(300);
    //    }
}

void SKGTreeView::saveDefaultClicked()
{
    if (m_document != nullptr) {
        SKGError err;
        SKGBEGINTRANSACTION(*m_document, i18nc("Noun, name of the user action", "Save default parameters"), err)
        err = m_document->setParameter(m_parameterName, getState());
    }
}

void SKGTreeView::switchAutoResize()
{
    m_autoResize = m_actAutoResize->isChecked();
    header()->setSectionResizeMode(m_autoResize ? QHeaderView::Fixed : QHeaderView::Interactive);
    if (m_autoResize) {
        resizeColumnsToContentsDelayed();
    } else {
        m_timerDelayedResize.stop();
        m_autoResizeDone = false;
    }
}

void SKGTreeView::groupByChanged(QAction *iAction)
{
    if ((m_model != nullptr) && m_model->isRefreshBlocked()) {
        return;
    }

    if ((iAction != nullptr) && (m_model != nullptr)) {
        m_groupby = iAction->data().toString();
        QString att = m_groupby;
        if (att == QStringLiteral("#") && (m_proxyModel != nullptr)) {
            att = m_model->getAttribute(m_proxyModel->sortColumn());
        }
        m_model->setGroupBy(att);
        m_model->refresh();

        refreshExpandCollapse();
        respanFirstColumns();
    }
}

void SKGTreeView::onSortChanged(int iIndex, Qt::SortOrder iOrder)
{
    Q_UNUSED(iOrder)
    if (m_groupby == QStringLiteral("#") && (m_model != nullptr)) {
        m_model->setGroupBy(m_model->getAttribute(iIndex));
        m_model->refresh();
    }

    m_timerScrollSelection.start(300);
}

void SKGTreeView::showHideColumn()
{
    auto *send = qobject_cast<QAction *>(this->sender());
    if (send != nullptr) {
        QHeaderView *hHeader = header();

        int idx = send->data().toInt();
        bool hidden = !hHeader->isSectionHidden(idx);
        hHeader->setSectionHidden(idx, hidden);
    }
}

void SKGTreeView::resetColumnsOrder()
{
    QHeaderView *hHeader = header();
    int nbcol = hHeader->count();
    for (int i = 0; i < nbcol; ++i) {
        int idx = hHeader->visualIndex(i);
        if (idx != i) {
            hHeader->moveSection(idx, i);
        }
    }
}

void SKGTreeView::changeSchema()
{
    QStringList list;

    auto *send = qobject_cast<QAction *>(this->sender());
    if (send != nullptr) {
        list = SKGServices::splitCSVLine(send->data().toString(), ';');
    }

    if (m_model != nullptr) {
        // Reset column order
        resetColumnsOrder();

        m_model->setSupportedAttributes(list);
        bool tmp = m_autoResizeDone;
        m_autoResizeDone = false;
        m_model->dataModified();
        m_autoResizeDone = tmp;
        header()->setSortIndicator(0, Qt::AscendingOrder);
    }
}

QStringList SKGTreeView::getCurrentSchema() const
{
    QStringList list;
    QHeaderView *hHeader = header();
    if ((hHeader != nullptr) && (m_model != nullptr)) {
        int nb = hHeader->count();
        if (nb != 0) {
            QString att;
            for (int i = 0; i < nb; ++i) {
                int idx = hHeader->logicalIndex(i);
                att = m_model->getAttribute(idx);
                att += QStringLiteral("|") % (hHeader->isSectionHidden(idx) ? QStringLiteral("N") : QStringLiteral("Y"));
                att += QStringLiteral("|") % SKGServices::intToString(hHeader->sectionSize(idx));

                list.push_back(att);
            }
        }
    }
    return list;
}
void SKGTreeView::setAlternatingRowColors(bool enable)
{
    QTreeView::setAlternatingRowColors(enable);
}

SKGObjectBase SKGTreeView::getFirstSelectedObject()
{
    return m_lastSelection.value(0);
}

SKGObjectBase::SKGListSKGObjectBase SKGTreeView::getSelectedObjects()
{
    return m_lastSelection;
}

int SKGTreeView::getNbSelectedObjects()
{
    return m_lastSelection.count();
}

void SKGTreeView::saveSelection()
{
    SKGTRACEINFUNC(10)

    m_selection.clear();

    SKGObjectBase::SKGListSKGObjectBase objs = getSelectedObjects();
    int nb = objs.count();
    // We save the selection only if not too big
    if (nb <= 100) {
        for (int i = 0; i < nb; ++i) {
            QString id = objs.at(i).getUniqueID();
            m_selection.push_back(id);
        }
    }
    SKGTRACEL(10) << m_selection.count() << " objects saved" << Qt::endl;
}

void SKGTreeView::selectObject(const QString &iUniqueID)
{
    SKGTRACEINFUNC(10)
    QStringList tmp;
    tmp.push_back(iUniqueID);
    selectObjects(tmp, true);
}

void SKGTreeView::selectObjects(const QStringList &iUniqueIDs, bool iFocusOnFirstOne)
{
    SKGTRACEINFUNC(10)
    SKGTRACEL(10) << iUniqueIDs.count() << " objects to select" << Qt::endl;
    int nbset = 0;
    QItemSelectionModel *selModel = selectionModel();
    if (selModel != nullptr) {
        bool previous = selModel->blockSignals(true);

        selModel->clearSelection();

        if (m_model != nullptr) {
            // Get all indexes
            QVector<QModelIndex> items;
            items.reserve(items.count() * 2);
            items.push_back(QModelIndex());
            for (int i = 0; i < items.count(); ++i) { // Dynamic size because the list is modified
                QModelIndex mi = items.at(i);
                int nbRows = m_model->rowCount(mi);
                for (int j = 0; j < nbRows; ++j) {
                    items.push_back(m_model->index(j, 0, mi));
                }
            }
            items.removeAt(0);

            int nbRows = items.count();
            if (nbRows != 0) {
                // Expand nodes
                bool previousForThis = this->blockSignals(true);
                for (int i = 0; i < nbRows; ++i) {
                    QModelIndex index = items.at(i);
                    SKGObjectBase obj = m_model->getObject(index);
                    QString id = obj.getUniqueID();
                    if (obj.getTable().isEmpty()) {
                        // This is a group
                        id = obj.getAttribute(QStringLiteral("t_title"));
                    }
                    if (m_expandedNodes.contains(id)) {
                        QModelIndex idxs = (m_proxyModel != nullptr ? m_proxyModel->mapFromSource(index) : index);
                        setExpanded(idxs, true);
                    }
                }
                this->blockSignals(previousForThis);

                // Set selection
                bool focusDone = false;
                for (int i = 0; i < nbRows; ++i) {
                    QModelIndex index = items.at(i);
                    SKGObjectBase obj = m_model->getObject(index);
                    if (iUniqueIDs.contains(obj.getUniqueID())) {
                        QModelIndex idxs = (m_proxyModel != nullptr ? m_proxyModel->mapFromSource(index) : index);
                        selModel->select(idxs, QItemSelectionModel::Select | QItemSelectionModel::Rows);
                        selModel->setCurrentIndex(idxs, QItemSelectionModel::NoUpdate);
                        ++nbset;
                        if (iFocusOnFirstOne && !focusDone) {
                            scrollTo(idxs);
                            focusDone = true;
                        }
                    }
                }
            }
        }
        selModel->blockSignals(previous);
    }

    SKGTRACEL(10) << nbset << " objects selected" << Qt::endl;

    onSelectionChanged();
}

void SKGTreeView::resetSelection()
{
    SKGTRACEINFUNC(10)

    const auto &lastSelection_parents_save = m_lastSelection_if_deleted; // because selectObjects will modify m_lastSelection_if_deleted
    selectObjects(m_selection);
    if ((lastSelection_parents_save.count() != 0) && (getSelectedObjects().count() == 0)) {
        // Try to select parent objects
        int nb = lastSelection_parents_save.count();
        // We save the selection only if not too big
        if (nb <= 100) {
            QStringList sel;
            sel.reserve(nb);
            for (int i = 0; i < nb; ++i) {
                QString id = lastSelection_parents_save.at(i).getUniqueID();
                sel.push_back(id);
            }

            selectObjects(sel);
        }
    }
}

void SKGTreeView::scroolOnSelection()
{
    QItemSelectionModel *selModel = selectionModel();
    if (selModel != nullptr) {
        if (m_model != nullptr) {
            QModelIndexList indexes = selModel->selectedRows();
            if (!indexes.isEmpty()) {
                scrollTo(indexes.at(0));
            }
        }
    }
}

void SKGTreeView::onExpand(const QModelIndex &index)
{
    SKGTRACEINFUNC(10)
    if (index.isValid() && (m_model != nullptr)) {
        QModelIndex idxs = (m_proxyModel != nullptr ? m_proxyModel->mapToSource(index) : index);

        SKGObjectBase obj = m_model->getObject(idxs);
        QString id = obj.getUniqueID();
        if (obj.getTable().isEmpty()) {
            // This is a group
            id = obj.getAttribute(QStringLiteral("t_title"));
        }
        m_expandedNodes.push_back(id);
    }

    if (m_autoResize) {
        resizeColumnsToContentsDelayed();
    }
}

void SKGTreeView::expandAll()
{
    SKGTRACEINFUNC(10)
    QTreeView::expandAll();

    if (m_autoResize) {
        resizeColumnsToContentsDelayed();
    }
}

void SKGTreeView::onCollapse(const QModelIndex &index)
{
    SKGTRACEINFUNC(10)
    if (index.isValid() && (m_model != nullptr)) {
        QModelIndex idxs = (m_proxyModel != nullptr ? m_proxyModel->mapToSource(index) : index);

        SKGObjectBase obj = m_model->getObject(idxs);

        QString id = obj.getUniqueID();
        if (obj.getTable().isEmpty()) {
            // This is a group
            id = obj.getAttribute(QStringLiteral("t_title"));
        }
        m_expandedNodes.removeOne(id);
    }

    if (m_autoResize) {
        resizeColumnsToContentsDelayed();
    }
}

void SKGTreeView::onClick(const QModelIndex &index)
{
    SKGTRACEINFUNC(10)
    if (index.isValid() && (m_actExpandAll != nullptr) && m_actExpandAll->isVisible()) {
        this->setExpanded(index, !this->isExpanded(index));
    }
}

void SKGTreeView::copy()
{
    QItemSelectionModel *selection = selectionModel();
    if (selection != nullptr) {
        QModelIndexList indexes = selection->selectedIndexes();

        if (indexes.empty()) {
            return;
        }

        std::sort(indexes.begin(), indexes.end());

        QStringList colums_title;
        colums_title.reserve(2 * indexes.count());
        SKGStringListList columns_values;
        auto initial_r = -1;
        for (const auto &current : std::as_const(indexes)) {
            const auto c = current.column();
            const auto r = current.row();
            if (initial_r == -1) {
                initial_r = r;
            }
            auto header_title = model()->headerData(c, Qt::Horizontal).toString();
            if (header_title.isEmpty()) {
                header_title = SKGServices::splitCSVLine(model()->headerData(c, Qt::Horizontal, Qt::UserRole).toString(), u'|').at(0);
            }

            if (initial_r == r) {
                colums_title.append(header_title);
                colums_title.append(header_title + QStringLiteral("_raw"));

                columns_values.append(QStringList());
                columns_values.append(QStringList());
            }

            columns_values[2 * c].append(model()->data(current).toString());
            columns_values[2 * c + 1].append(model()->data(current, Qt::UserRole).toString());
        }

        // Remove useless raw columns (when all raw values = displayed values)
        int nbCols = columns_values.count();
        int nbRows = 0;
        if (nbCols > 0) {
            nbRows = columns_values.at(0).count();
            for (int c = nbCols - 1; c >= 0; c = c - 2) {
                bool allEqual = true;
                bool allDisplayedEmpty = true;
                const auto &displayedValues = columns_values.at(c - 1);
                const auto &rawValues = columns_values.at(c);
                for (int r = 0; r < nbRows; ++r) {
                    if (!displayedValues.at(r).isEmpty()) {
                        allDisplayedEmpty = false;
                    }
                    if (rawValues.at(r) != displayedValues.at(r)) {
                        allEqual = false;
                    }
                }

                // Remove the raw column
                if (allEqual) {
                    // Remove the useless raw column
                    colums_title.removeAt(c);
                    columns_values.removeAt(c);
                    nbCols--;
                } else if (allDisplayedEmpty) {
                    // Remove the empty column and keep the raw version
                    colums_title.removeAt(c - 1);
                    columns_values.removeAt(c - 1);
                    nbCols--;
                }
            }
        }

        // Build text
        QString text = colums_title.join(u';') + u'\n';
        if (nbCols > 0) {
            for (int r = 0; r < nbRows; ++r) {
                for (int c = 0; c < nbCols; ++c) {
                    if (c != 0) {
                        text += u';';
                    }
                    text += columns_values.at(c).at(r);
                }
                text += u'\n';
            }
        }

        auto clipBoard = QApplication::clipboard();
        if (clipBoard != nullptr) {
            clipBoard->setText(text);
        }
    }
}

void SKGTreeView::setZoomPosition(int iZoomPosition)
{
    int newZoomPos = qMax(qMin(iZoomPosition, 10), -10);
    if (newZoomPos != zoomPosition() && m_fontOriginalPointSize + newZoomPos > 1) {
        QFont newFont = this->font();
        newFont.setPointSize(m_fontOriginalPointSize + newZoomPos);
        int newIconSize = qMax(m_iconOriginalSize + newZoomPos, 1);

        this->setFont(newFont);
        this->setIconSize(QSize(newIconSize, newIconSize));
        header()->setIconSize(QSize(newIconSize, newIconSize));

        if (m_autoResize) {
            resizeColumnsToContentsDelayed();
        }

        Q_EMIT zoomChanged(newZoomPos);
    }
}

int SKGTreeView::zoomPosition()
{
    return this->font().pointSize() - m_fontOriginalPointSize;
}

bool SKGTreeView::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if (iObject == this && iEvent != nullptr && iEvent->type() == QEvent::Wheel) {
        auto *e = dynamic_cast<QWheelEvent *>(iEvent);
        if (m_textResizable && (e != nullptr) && ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u)) {
            int numDegrees = e->angleDelta().y() / 8;
            int numTicks = numDegrees / 15;

            setZoomPosition(zoomPosition() + (numTicks > 0 ? 1 : -1));
            e->setAccepted(true);
            return true;
        }
    }
    if (iObject == this && iEvent != nullptr && iEvent->type() == QEvent::KeyPress) {
        auto *kevent = dynamic_cast<QKeyEvent *>(iEvent);
        if (kevent != nullptr) {
            if (kevent->matches(QKeySequence::Copy) && this->state() != QAbstractItemView::EditingState) {
                copy();
                if (iEvent != nullptr) {
                    iEvent->accept();
                }
                return true; // stop the process
            }
        }
    }
    return QTreeView::eventFilter(iObject, iEvent);
}

void SKGTreeView::mousePressEvent(QMouseEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->button() == Qt::LeftButton && !(this->indexAt(iEvent->pos()).isValid())) {
        Q_EMIT clickEmptyArea();
        clearSelection();
    }

    if ((iEvent != nullptr) && iEvent->button() == Qt::LeftButton && (m_proxyModel != nullptr) && (m_model != nullptr)) {
        int propertyUUID = m_proxyModel->data(indexAt(iEvent->pos()), 101).toInt();
        if (propertyUUID != 0) {
            SKGPropertyObject prop(m_model->getDocument(), propertyUUID);
            QDesktopServices::openUrl(prop.getUrl(true));
        }
    }

    QTreeView::mousePressEvent(iEvent);
}

bool SKGTreeView::isTextResizable() const
{
    return m_textResizable;
}

void SKGTreeView::setTextResizable(bool resizable)
{
    if (m_textResizable != resizable) {
        m_textResizable = resizable;
        Q_EMIT modified();
    }
}

QTextBrowser *SKGTreeView::getTextBrowser() const
{
    auto output = new QTextBrowser();
    QTextCursor tcursor = output->textCursor();
    tcursor.beginEditBlock();

    // Create table format
    QTextTableFormat tableFormat;
    tableFormat.setAlignment(Qt::AlignHCenter);
    tableFormat.setAlignment(Qt::AlignLeft);
    tableFormat.setBackground(QColor(255, 255, 255));
    tableFormat.setCellPadding(5);
    tableFormat.setCellSpacing(5);

    // Create table
    SKGStringListList table = getTable();
    int nbRows = table.count();
    int nbCol = table.at(0).count();

    QTextTable *tableau = tcursor.insertTable(nbRows, nbCol, tableFormat);

    // Create frame
    QTextFrame *frame = tcursor.currentFrame();
    QTextFrameFormat frameFormat = frame->frameFormat();
    frameFormat.setBorder(0);
    frame->setFrameFormat(frameFormat);

    // Create header table format
    QTextCharFormat headerFormat;
    headerFormat.setFontPointSize(6);
    headerFormat.setFontWeight(QFont::Bold);

    // Create text format
    QTextCharFormat textFormat;
    textFormat.setFontPointSize(6);

    // Create header
    for (int r = 0; r < nbRows; ++r) {
        const QStringList &line = table.at(r);
        for (int c = 0; c < nbCol; ++c) {
            QTextCursor cellCursor = tableau->cellAt(r, c).firstCursorPosition();
            cellCursor.insertText(line.at(c), (r == 0 ? headerFormat : textFormat));
        }
    }

    // End
    tcursor.endEditBlock();

    return output;
}

SKGStringListList SKGTreeView::getTable(const QModelIndex &iIndex) const
{
    // Build table
    SKGStringListList table;

    // Get header names
    if (m_model != nullptr) {
        // Header
        int nb = m_model->columnCount();
        int nb2 = m_model->rowCount(iIndex);
        table.reserve(1 + nb2 * 2);
        if (!iIndex.isValid()) {
            QStringList cols;
            cols.reserve(nb);
            for (int i = 0; i < nb; ++i) {
                cols.append(m_model->headerData(i, Qt::Horizontal, Qt::UserRole).toString().split('|').at(0));
            }
            table.append(cols);
        }

        // Get content
        for (int i = 0; i < nb2; ++i) {
            QStringList row;
            row.reserve(nb);
            for (int j = 0; j < nb; j++) {
                // We have to check the type for 214849
                QModelIndex idx = m_model->index(i, j, iIndex);

                SKGServices::AttributeType type = m_model->getAttributeType(j);
                QString display =
                    m_model->data(idx, type == SKGServices::FLOAT || m_model->getObject(idx).getTable().isEmpty() ? Qt::DisplayRole : Qt::UserRole).toString();
                if (display.isEmpty()) {
                    display = m_model->data(idx, Qt::DisplayRole).toString();
                }
                row.append(display);
            }

            table.append(row);

            QModelIndex idx0 = m_model->index(i, 0, iIndex);
            if (m_model->hasChildren(idx0)) {
                table.append(getTable(idx0));
            }
        }
    }
    return table;
}

SKGError SKGTreeView::exportInFile(const QString &iFileName)
{
    SKGError err;
    _SKGTRACEINFUNC(10)
    QString codec = QTextCodec::codecForLocale()->name();
    QString extension = QFileInfo(iFileName).suffix().toUpper();
    if (extension == QStringLiteral("CSV")) {
        // Write file
        QSaveFile file(iFileName);
        if (!file.open(QIODevice::WriteOnly)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", iFileName));
        } else {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::encodingForName(codec.toLatin1().constData()).value_or(QStringConverter::Utf8));
            QStringList dump = SKGServices::tableToDump(getTable(), SKGServices::DUMP_CSV);
            int nbl = dump.count();
            for (int i = 0; i < nbl; ++i) {
                out << dump.at(i) << Qt::endl;
            }

            // Close file
            file.commit();
        }
    } else if (extension == QStringLiteral("PDF")) {
        QImage image(this->size(), QImage::Format_ARGB32);
        QPainter painter(&image);
        this->render(&painter);
        painter.end();

        {
            QPrinter printer(QPrinter::HighResolution);
            printer.setOutputFileName(iFileName);
            QPainter newPainter(&printer);

            QRect painterRect = newPainter.viewport();
            QSize imageSize = image.size();
            imageSize.scale(painterRect.size(), Qt::KeepAspectRatio);
            newPainter.setViewport(painterRect.x(), painterRect.y(), imageSize.width(), imageSize.height());
            newPainter.setWindow(image.rect());
            newPainter.drawImage(0, 0, image);
            newPainter.end();
        }
    } else if (extension == QStringLiteral("SVG")) {
        QSvgGenerator generator;
        generator.setFileName(iFileName);
        generator.setTitle(i18nc("Title of the content SVG export", "Skrooge SVG export"));
        generator.setDescription(i18nc("Description of the content SVG export", "A SVG drawing created by the Skrooge."));

        QPainter painter(&generator);
        QWidget *w = this->viewport();
        w->render(&painter);
        generator.setSize(QSize(w->widthMM(), w->heightMM()));
        generator.setViewBox(QRect(0, 0, w->widthMM(), w->heightMM()));

        painter.end();
    } else if (extension == QStringLiteral("HTML")) {
        // Write file
        QSaveFile file(iFileName);
        if (!file.open(QIODevice::WriteOnly)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", iFileName));
        } else {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::encodingForName(codec.toLatin1().constData()).value_or(QStringConverter::Utf8));
            QTextBrowser *tb = getTextBrowser();
            if (tb != nullptr) {
                out << tb->toHtml().replace(QStringLiteral("<meta name=\"qrichtext\" content=\"1\" />"),
                                            QStringLiteral("<meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" />"))
                    << Qt::endl;

                delete tb;
            }

            // Close file
            file.commit();
        }
    } else if (extension == QStringLiteral("ODT")) {
        QTextBrowser *tb = getTextBrowser();
        if (tb != nullptr) {
            QTextDocument doc;
            doc.setHtml(tb->toHtml());

            QTextDocumentWriter docWriter(iFileName);
            docWriter.write(&doc);

            delete tb;
        }
    } else {
        // Write file
        QSaveFile file(iFileName);
        if (!file.open(QIODevice::WriteOnly)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", iFileName));
        } else {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::encodingForName(codec.toLatin1().constData()).value_or(QStringConverter::Utf8));
            QStringList dump = SKGServices::tableToDump(getTable(), SKGServices::DUMP_TEXT);
            int nbl = dump.count();
            for (int i = 0; i < nbl; ++i) {
                out << dump.at(i) << Qt::endl;
            }

            // Close file
            file.commit();
        }
    }

    return err;
}

void SKGTreeView::onExport()
{
    _SKGTRACEINFUNC(10)
    QString fileName =
        SKGMainPanel::getSaveFileName(QStringLiteral("kfiledialog:///IMPEXP"),
                                      QStringLiteral("text/csv text/plain text/html application/vnd.oasis.opendocument.text  image/svg+xml application/pdf"),
                                      this);
    if (!fileName.isEmpty()) {
        SKGError err = exportInFile(fileName);
        SKGMainPanel::displayErrorMessage(err);
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
    }
}

void SKGTreeView::setModel(QAbstractItemModel *iModel)
{
    if (iModel != this->model()) {
        m_model = qobject_cast<SKGObjectModelBase *>(iModel);
        m_proxyModel = qobject_cast<SKGSortFilterProxyModel *>(iModel);
        if (m_proxyModel != nullptr) {
            m_model = qobject_cast<SKGObjectModelBase *>(m_proxyModel->sourceModel());
        }

        if (m_model != nullptr) {
            connect(m_model, &SKGObjectModelBase::afterReset, this, &SKGTreeView::setupHeaderMenu);
            // connect(m_model, &SKGObjectModelBase::afterReset, this, &SKGTreeView::onSelectionChanged);
            connect(m_model, &SKGObjectModelBase::afterReset, this, &SKGTreeView::respanFirstColumns, Qt::QueuedConnection);
        }
        QTreeView::setModel(iModel);

        rebuildContextualMenu();
        refreshExpandCollapse();
    }
}

QMenu *SKGTreeView::getHeaderMenu() const
{
    return m_headerMenu;
}

void SKGTreeView::rebuildContextualMenu()
{
    // Remove all Actions
    const auto list = actions();
    for (auto act : list) {
        removeAction(act);
    }

    if (selectionMode() != NoSelection) {
        // Build contextual menu
        this->insertAction(nullptr, m_actCopy);
        this->insertAction(nullptr, m_actExpandAll);
        this->insertAction(nullptr, m_actCollapseAll);

        if ((m_model != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
            const auto list = SKGMainPanel::getMainPanel()->getActionsForContextualMenu(m_model->getRealTable());
            for (const auto &act : list) {
                if (act == nullptr) {
                    insertGlobalAction();
                } else {
                    insertAction(nullptr, act);
                }
            }
        }
    }
}
