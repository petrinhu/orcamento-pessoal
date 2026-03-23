/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A table with graph with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtablewithgraph.h"

#include <kcolorscheme.h>
#include <kstringhandler.h>

#include <QtCore5Compat/QTextCodec>
#include <qcollator.h>
#include <qdesktopservices.h>
#include <qdom.h>
#include <qfileinfo.h>
#include <qgraphicseffect.h>
#include <qgraphicsitem.h>
#include <qheaderview.h>
#include <qjsengine.h>
#include <qmath.h>
#include <qmenu.h>
#include <qpainter.h>
#include <qprinter.h>
#include <qsavefile.h>
#include <qscrollbar.h>
#include <qtablewidget.h>
#include <qtimer.h>
#include <qwidgetaction.h>

#include <algorithm>

#include "skgcolorbutton.h"
#include "skgcombobox.h"
#include "skggraphicsscene.h"
#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtreemap.h"
#include <utility>

/**
 * Data identifier for value
 */
static const int DATA_VALUE = 12;
/**
 * Data identifier for color
 */
static const int DATA_COLOR_H = 11;
/**
 * Data identifier for color
 */
static const int DATA_COLOR_S = 12;
/**
 * Data identifier for color
 */
static const int DATA_COLOR_V = 13;
/**
 * Data identifier for Z value
 */
static const int DATA_Z_VALUE = 14;
/**
 * Data identifier for mode
 */
static const int DATA_MODE = 15;
/**
 * Alpha value
 */
static const int ALPHA = 200;
/**
 * Box size
 */
static const double BOX_SIZE = 120.0;
/**
 * Box margin
 */
static const double BOX_MARGIN = 10.0;

#define cloneAction(MENU, ACTION, SLOTTOCALL)                                                                                                                  \
    auto act = (MENU)->addAction((ACTION)->icon(), (ACTION)->text());                                                                                          \
    act->setCheckable(true);                                                                                                                                   \
    act->setChecked((ACTION)->isChecked());                                                                                                                    \
    connect(act, &QAction::triggered, this, SLOTTOCALL);                                                                                                       \
    connect(act, &QAction::toggled, ACTION, &QAction::setChecked);                                                                                             \
    connect(ACTION, &QAction::toggled, act, &QAction::setChecked);

SKGTableWithGraph::SKGTableWithGraph()
    : SKGTableWithGraph(nullptr)
{
}

SKGTableWithGraph::SKGTableWithGraph(QWidget *iParent)
    : QWidget(iParent)
    , m_scene(nullptr)
    , m_additionalInformation(NONE)
    , m_nbVirtualColumns(0)
    , m_selectable(true)
    , m_toolBarVisible(true)
    , m_graphTypeVisible(true)
    , m_limitVisible(true)
    , m_averageVisible(true)
    , m_linearRegressionVisible(true)
    , m_paretoVisible(false)
    , m_legendVisible(false)
    , m_graphVisible(true)
    , m_tableVisible(true)
    , m_textVisible(false)
    , m_zeroVisible(true)
    , m_decimalsVisible(true)
    , m_shadow(true)
    , m_mainMenu(nullptr)
    , m_indexSum(-1)
    , m_indexAverage(-1)
    , m_indexMin(-1)
    , m_indexLinearRegression(-1)
    , m_sortOrder(Qt::AscendingOrder)
    , m_sortColumn(0)
{
    m_axisColor = Qt::gray;
    m_gridColor = Qt::lightGray;
    m_minColor = Qt::red;
    m_maxColor = Qt::green;
    m_paretoColor = Qt::darkRed;
    m_averageColor = Qt::blue;
    m_tendencyColor = Qt::darkYellow;
    m_backgroundColor = Qt::white;
    m_textColor = Qt::black;
    m_NegativeColor = KColorScheme(QPalette::Normal).foreground(KColorScheme::NegativeText);
    m_WhiteColor = QBrush(Qt::white);

    ui.setupUi(this);
    ui.kTextEdit->hide();

    ui.kFilterEdit->setPlaceholderText(i18n("Search"));

    m_displayMode = new SKGComboBox(this);
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-bar-stacked")),
                           i18nc("Noun, a type of graph, with bars stacked upon each other", "Stack of lines"),
                           static_cast<int>(STACK));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-bar-stacked")),
                           i18nc("Noun, a type of graph, with bars stacked upon each other", "Stack of columns"),
                           static_cast<int>(STACKCOLUMNS));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-bar")),
                           i18nc("Noun, a type of graph, with bars placed besides each other", "Histogram"),
                           static_cast<int>(HISTOGRAM));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-scatter")),
                           i18nc("Noun, a type of graph with only points", "Point"),
                           static_cast<int>(POINT));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-line")),
                           i18nc("Noun, a type of graph with only lines", "Line"),
                           static_cast<int>(LINE));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-area-stacked")),
                           i18nc("Noun, a type of graph, with lines stacked upon each other", "Stacked area"),
                           static_cast<int>(STACKAREA));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("skg-chart-bubble")),
                           i18nc("Noun, a type of graph, with bubbles", "Bubble"),
                           static_cast<int>(BUBBLE));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-pie")),
                           i18nc("Noun, a type of graph that looks like a sliced pie", "Pie"),
                           static_cast<int>(PIE));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-ring")),
                           i18nc("Noun, a type of graph that looks like concentric slices of a pie (a la filelight)", "Concentric pie"),
                           static_cast<int>(CONCENTRICPIE));
    m_displayMode->addItem(SKGServices::fromTheme(QStringLiteral("map-flat")),
                           i18nc("Noun, a type of graph that looks treemap", "Treemap"),
                           static_cast<int>(TREEMAP));

    ui.graphicView->addToolbarWidget(m_displayMode);

    ui.kShow->addItem(QStringLiteral("table"),
                      i18n("Table"),
                      QStringLiteral("view-list-details"),
                      QString(),
                      QString(),
                      QStringLiteral("text"),
                      QStringLiteral("graph"),
                      QString(),
                      Qt::META | Qt::Key_T);
    ui.kShow->addItem(QStringLiteral("graph"),
                      i18n("Graph"),
                      QStringLiteral("office-chart-pie"),
                      QString(),
                      QString(),
                      QStringLiteral("text"),
                      QStringLiteral("table"),
                      QString(),
                      Qt::META | Qt::Key_G);
    ui.kShow->addItem(QStringLiteral("text"),
                      i18n("Text"),
                      QStringLiteral("view-list-text"),
                      QString(),
                      QString(),
                      QStringLiteral("table;graph"),
                      QStringLiteral("table;graph"),
                      QString(),
                      Qt::META | Qt::Key_R);

    ui.kShow->setDefaultState(QStringLiteral("table;graph"));

    connect(ui.kShow, &SKGShow::stateChanged, this, &SKGTableWithGraph::onDisplayModeChanged, Qt::QueuedConnection);

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGTableWithGraph::refresh, Qt::QueuedConnection);

    m_timerRedraw.setSingleShot(true);
    connect(&m_timerRedraw, &QTimer::timeout, this, &SKGTableWithGraph::redrawGraph, Qt::QueuedConnection);

    // Build contextual menu
    ui.kTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.kTable, &SKGTableWidget::customContextMenuRequested, this, &SKGTableWithGraph::showMenu);

    m_mainMenu = new QMenu(ui.kTable);

    QAction *actExport = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("document-export")), i18nc("Noun, user action", "Export…"));
    connect(actExport, &QAction::triggered, this, &SKGTableWithGraph::onExport);

    // Add graph mode in menu
    getGraphContextualMenu()->addSeparator();
    auto displayModeMenu = new SKGComboBox(this);
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-bar-stacked")),
                             i18nc("Noun, a type of graph, with bars stacked upon each other", "Stack of lines"),
                             static_cast<int>(STACK));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-bar-stacked")),
                             i18nc("Noun, a type of graph, with bars stacked upon each other", "Stack of columns"),
                             static_cast<int>(STACKCOLUMNS));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-bar")),
                             i18nc("Noun, a type of graph, with bars placed besides each other", "Histogram"),
                             static_cast<int>(HISTOGRAM));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-scatter")),
                             i18nc("Noun, a type of graph with only points", "Point"),
                             static_cast<int>(POINT));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-line")),
                             i18nc("Noun, a type of graph with only lines", "Line"),
                             static_cast<int>(LINE));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-area-stacked")),
                             i18nc("Noun, a type of graph, with lines stacked upon each other", "Stacked area"),
                             static_cast<int>(STACKAREA));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("skg-chart-bubble")),
                             i18nc("Noun, a type of graph, with bubbles", "Bubble"),
                             static_cast<int>(BUBBLE));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-pie")),
                             i18nc("Noun, a type of graph that looks like a sliced pie", "Pie"),
                             static_cast<int>(PIE));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-ring")),
                             i18nc("Noun, a type of graph that looks like concentric slices of a pie (a la filelight)", "Concentric pie"),
                             static_cast<int>(CONCENTRICPIE));
    displayModeMenu->addItem(SKGServices::fromTheme(QStringLiteral("map-flat")),
                             i18nc("Noun, a type of graph that looks treemap", "Treemap"),
                             static_cast<int>(TREEMAP));

    m_displayModeWidget = new QWidgetAction(this);
    m_displayModeWidget->setDefaultWidget(displayModeMenu);
    getGraphContextualMenu()->addAction(m_displayModeWidget);

    connect(displayModeMenu, static_cast<void (SKGComboBox::*)(int)>(&SKGComboBox::currentIndexChanged), m_displayMode, &SKGComboBox::setCurrentIndex);
    connect(m_displayMode, static_cast<void (SKGComboBox::*)(int)>(&SKGComboBox::currentIndexChanged), displayModeMenu, &SKGComboBox::setCurrentIndex);

    // Reset Colors
    QAction *resetColorsAction =
        m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("format-fill-color")), i18nc("Noun, user action", "Reset default colors"));
    connect(resetColorsAction, &QAction::triggered, this, &SKGTableWithGraph::resetColors);
    getGraphContextualMenu()->addAction(resetColorsAction);

    // Add item in menu of the graph
    m_allPositiveMenu =
        getGraphContextualMenu()->addAction(SKGServices::fromTheme(QStringLiteral("go-up-search")), i18nc("Noun, user action", "All values in positive"));
    if (m_allPositiveMenu != nullptr) {
        m_allPositiveMenu->setCheckable(true);
    }
    connect(m_allPositiveMenu, &QAction::toggled, this, &SKGTableWithGraph::redrawGraphDelayed, Qt::QueuedConnection);

    // Add item in menu of the graph
    m_actShowLimits = getGraphContextualMenu()->addAction(i18nc("Noun, user action", "Show limits"));
    if (m_actShowLimits != nullptr) {
        m_actShowLimits->setCheckable(true);
        m_actShowLimits->setChecked(m_limitVisible);
        connect(m_actShowLimits, &QAction::triggered, this, &SKGTableWithGraph::switchLimitsVisibility);

        // Clone action on table contextual menu
        cloneAction(m_mainMenu, m_actShowLimits, &SKGTableWithGraph::switchLimitsVisibility)
    }

    // Add item in menu of the graph
    m_actShowAverage = getGraphContextualMenu()->addAction(i18nc("Noun, user action", "Show average"));
    if (m_actShowAverage != nullptr) {
        m_actShowAverage->setCheckable(true);
        m_actShowAverage->setChecked(m_averageVisible);
        connect(m_actShowAverage, &QAction::triggered, this, &SKGTableWithGraph::switchAverageVisibility);

        // Clone action on table contextual menu
        cloneAction(m_mainMenu, m_actShowAverage, &SKGTableWithGraph::switchAverageVisibility)
    }

    // Add item in menu of the graph
    m_actShowLinearRegression = getGraphContextualMenu()->addAction(i18nc("Noun, user action", "Show tendency line"));
    if (m_actShowLinearRegression != nullptr) {
        m_actShowLinearRegression->setCheckable(true);
        m_actShowLinearRegression->setChecked(m_linearRegressionVisible);
        connect(m_actShowLinearRegression, &QAction::triggered, this, &SKGTableWithGraph::switchLinearRegressionVisibility);

        // Clone action on table contextual menu
        cloneAction(m_mainMenu, m_actShowLinearRegression, &SKGTableWithGraph::switchLinearRegressionVisibility)
    }

    // Add item in menu of the graph
    m_actShowPareto = getGraphContextualMenu()->addAction(i18nc("Noun, user action", "Show Pareto curve"));
    if (m_actShowPareto != nullptr) {
        m_actShowPareto->setCheckable(true);
        m_actShowPareto->setChecked(m_paretoVisible);
        connect(m_actShowPareto, &QAction::triggered, this, &SKGTableWithGraph::switchParetoVisibility);
    }

    // Add item in menu of the graph
    m_actShowLegend = getGraphContextualMenu()->addAction(SKGServices::fromTheme(QStringLiteral("help-contents")), i18nc("Noun, user action", "Show legend"));
    if (m_actShowLegend != nullptr) {
        m_actShowLegend->setCheckable(true);
        m_actShowLegend->setChecked(m_legendVisible);
        connect(m_actShowLegend, &QAction::triggered, this, &SKGTableWithGraph::switchLegendVisibility);
    }

    // Add item in menu of the graph
    m_actShowZero = getGraphContextualMenu()->addAction(SKGServices::fromTheme(QStringLiteral("labplot-xy-plot-two-axes-centered-origin")),
                                                        i18nc("Noun, user action", "Show origin"));
    if (m_actShowZero != nullptr) {
        m_actShowZero->setCheckable(true);
        m_actShowZero->setChecked(m_zeroVisible);
        connect(m_actShowZero, &QAction::triggered, this, &SKGTableWithGraph::swithOriginVisibility);
    }

    // Add item in menu of the graph
    m_actShowDecimal =
        getGraphContextualMenu()->addAction(SKGServices::fromTheme(QStringLiteral("format-precision-less")), i18nc("Noun, user action", "Show decimals"));
    if (m_actShowDecimal != nullptr) {
        m_actShowDecimal->setCheckable(true);
        m_actShowDecimal->setChecked(m_decimalsVisible);
        connect(m_actShowDecimal, &QAction::triggered, this, &SKGTableWithGraph::swithDecimalsVisibility);

        // Clone action on table contextual menu
        cloneAction(m_mainMenu, m_actShowDecimal, &SKGTableWithGraph::swithDecimalsVisibility)
    }

    // Set headers parameters
    QHeaderView *verticalHeader = ui.kTable->verticalHeader();
    if (verticalHeader != nullptr) {
        verticalHeader->hide();
        verticalHeader->setDefaultSectionSize(verticalHeader->minimumSectionSize());
        // verticalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    ui.kTable->setSortingEnabled(false); // sort must be enable for refresh method
    QHeaderView *horizontalHeader = ui.kTable->horizontalHeader();
    if (horizontalHeader != nullptr) {
        horizontalHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        horizontalHeader->show();
        horizontalHeader->setSortIndicatorShown(true);
        horizontalHeader->setSortIndicator(m_sortColumn, m_sortOrder);
        connect(horizontalHeader, &QHeaderView::sortIndicatorChanged, this, &SKGTableWithGraph::refresh);
    }

    connect(ui.kTable->horizontalScrollBar(), &QScrollBar::valueChanged, this, &SKGTableWithGraph::onHorizontalScrollBarChanged);
    connect(m_displayMode,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGTableWithGraph::redrawGraphDelayed,
            Qt::QueuedConnection);
    connect(ui.graphicView, &SKGGraphicsView::resized, this, &SKGTableWithGraph::redrawGraphDelayed, Qt::QueuedConnection);

    connect(ui.kTable, &SKGTableWidget::cellDoubleClicked, this, &SKGTableWithGraph::onDoubleClick);
    connect(ui.kTable, &SKGTableWidget::itemSelectionChanged, this, &SKGTableWithGraph::onSelectionChanged);
    connect(ui.kFilterEdit, &QLineEdit::textChanged, this, &SKGTableWithGraph::onFilterModified);

#ifdef SKG_WEBENGINE
    connect(ui.kTextEdit, &SKGWebView::linkClicked, this, &SKGTableWithGraph::onLinkClicked);
#endif
}

SKGTableWithGraph::~SKGTableWithGraph()
{
    delete m_scene;
    m_scene = nullptr;

    m_mainMenu = nullptr;
    m_actShowLimits = nullptr;
    m_actShowLinearRegression = nullptr;
    m_actShowPareto = nullptr;
    m_displayModeWidget = nullptr;
    m_displayMode = nullptr;
}

void SKGTableWithGraph::onHorizontalScrollBarChanged(int iValue)
{
    QHeaderView *verticalHeader = ui.kTable->verticalHeader();
    if (verticalHeader != nullptr) {
        verticalHeader->setVisible(iValue > 0);
    }
}

bool SKGTableWithGraph::isGraphVisible() const
{
    return m_graphVisible;
}

bool SKGTableWithGraph::isTableVisible() const
{
    return m_tableVisible;
}

bool SKGTableWithGraph::isTextReportVisible() const
{
    return m_textVisible;
}

SKGShow *SKGTableWithGraph::getShowWidget() const
{
    return ui.kShow;
}
void SKGTableWithGraph::setAverageColor(const QColor &iColor)
{
    m_averageColor = iColor;
}

void SKGTableWithGraph::setAxisColor(const QColor &iColor)
{
    m_axisColor = iColor;
}

void SKGTableWithGraph::setGridColor(const QColor &iColor)
{
    m_gridColor = iColor;
}

void SKGTableWithGraph::setMaxColor(const QColor &iColor)
{
    m_maxColor = iColor;
}

void SKGTableWithGraph::setParetoColor(const QColor &iColor)
{
    m_paretoColor = iColor;
}

void SKGTableWithGraph::setMinColor(const QColor &iColor)
{
    m_minColor = iColor;
}

void SKGTableWithGraph::setTendencyColor(const QColor &iColor)
{
    m_tendencyColor = iColor;
}

void SKGTableWithGraph::setOutlineColor(const QColor &iColor)
{
    m_outlineColor = iColor;
}

void SKGTableWithGraph::setBackgroundColor(const QColor &iColor)
{
    m_backgroundColor = iColor;
}

void SKGTableWithGraph::setTextColor(const QColor &iColor)
{
    m_textColor = iColor;
}

void SKGTableWithGraph::setAntialiasing(bool iAntialiasing)
{
    ui.graphicView->setAntialiasing(iAntialiasing);
}

void SKGTableWithGraph::setGraphTypeSelectorVisible(bool iVisible)
{
    if (m_graphTypeVisible != iVisible) {
        m_graphTypeVisible = iVisible;
        if (m_displayMode != nullptr) {
            m_displayMode->setVisible(m_graphTypeVisible);
        }
        if (m_displayModeWidget != nullptr) {
            m_displayModeWidget->setVisible(m_graphTypeVisible);
        }
        Q_EMIT modified();
    }
}

bool SKGTableWithGraph::isGraphTypeSelectorVisible() const
{
    return m_graphTypeVisible;
}

bool SKGTableWithGraph::switchLimitsVisibility()
{
    m_limitVisible = !m_limitVisible;
    refresh();
    return m_limitVisible;
}

bool SKGTableWithGraph::switchAverageVisibility()
{
    m_averageVisible = !m_averageVisible;
    refresh();
    return m_averageVisible;
}

bool SKGTableWithGraph::switchLegendVisibility()
{
    m_legendVisible = !m_legendVisible;
    redrawGraphDelayed();
    return m_legendVisible;
}

bool SKGTableWithGraph::swithOriginVisibility()
{
    m_zeroVisible = !m_zeroVisible;
    redrawGraphDelayed();
    return m_zeroVisible;
}

bool SKGTableWithGraph::swithDecimalsVisibility()
{
    m_decimalsVisible = !m_decimalsVisible;
    refresh();
    return m_decimalsVisible;
}

bool SKGTableWithGraph::switchLinearRegressionVisibility()
{
    m_linearRegressionVisible = !m_linearRegressionVisible;
    refresh();
    return m_linearRegressionVisible;
}

bool SKGTableWithGraph::switchParetoVisibility()
{
    m_paretoVisible = !m_paretoVisible;
    redrawGraphDelayed();
    return m_paretoVisible;
}

QMenu *SKGTableWithGraph::getTableContextualMenu() const
{
    return m_mainMenu;
}

QMenu *SKGTableWithGraph::getGraphContextualMenu() const
{
    return ui.graphicView->getContextualMenu();
}

void SKGTableWithGraph::showMenu(const QPoint iPos)
{
    if (m_mainMenu != nullptr) {
        m_mainMenu->popup(ui.kTable->mapToGlobal(iPos));
    }
}

QTableWidget *SKGTableWithGraph::table() const
{
    return ui.kTable;
}

SKGGraphicsView *SKGTableWithGraph::graph() const
{
    return ui.graphicView;
}

SKGWebView *SKGTableWithGraph::textReport() const
{
    return ui.kTextEdit;
}

SKGStringListList SKGTableWithGraph::getTable()
{
    // Build table
    SKGStringListList output;

    // Get header names
    int nb2 = ui.kTable->rowCount();
    int nb = ui.kTable->columnCount();
    output.reserve(nb2 + 1);
    QStringList cols;
    cols.reserve(2 * nb);
    for (int i = 0; i < nb; ++i) {
        cols.append(ui.kTable->horizontalHeaderItem(i)->text());
        cols.append(i18n("%1 (raw)", ui.kTable->horizontalHeaderItem(i)->text()));
    }
    output.append(cols);

    // Get content
    for (int i = 0; i < nb2; ++i) {
        QStringList row;
        row.reserve(nb);
        for (int j = 0; j < nb; j++) {
            auto *button = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(i, j));
            if (button != nullptr) {
                row.append(button->text());
                row.append(button->color().toRgb().name());
            } else {
                row.append(ui.kTable->item(i, j)->text());
                QString raw = ui.kTable->item(i, j)->data(DATA_VALUE).toString();
                if (raw.isEmpty()) {
                    raw = ui.kTable->item(i, j)->text();
                }
                row.append(raw);
            }
        }
        output.append(row);
    }
    return output;
}
QString SKGTableWithGraph::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    if (ui.graphicView->isVisible() && ui.kTable->isVisible()) {
        root.setAttribute(QStringLiteral("splitterState"), QString(ui.splitter->saveState().toHex()));
    }
    root.setAttribute(QStringLiteral("graphMode"), SKGServices::intToString(static_cast<int>(getGraphType())));
    root.setAttribute(QStringLiteral("allPositive"), m_allPositiveMenu->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("filter"), ui.kFilterEdit->text());
    root.setAttribute(QStringLiteral("limitVisible"), m_limitVisible ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("averageVisible"), m_averageVisible ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("linearRegressionVisible"), m_linearRegressionVisible ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("paretoVisible"), m_paretoVisible ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("legendVisible"), m_legendVisible ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("zeroVisible"), m_zeroVisible ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("decimalsVisible"), m_decimalsVisible ? QStringLiteral("Y") : QStringLiteral("N"));
    QMapIterator<QString, QColor> i(m_mapTitleColor);
    while (i.hasNext()) {
        i.next();
        QDomElement color = doc.createElement(QStringLiteral("color"));
        root.appendChild(color);
        color.setAttribute(QStringLiteral("key"), i.key());
        color.setAttribute(QStringLiteral("value"), i.value().name());
    }

    QHeaderView *horizontalHeader = ui.kTable->horizontalHeader();
    root.setAttribute(QStringLiteral("sortOrder"), SKGServices::intToString(horizontalHeader->sortIndicatorOrder()));
    root.setAttribute(QStringLiteral("sortColumn"), SKGServices::intToString(horizontalHeader->sortIndicatorSection()));
    root.setAttribute(QStringLiteral("graphicViewState"), ui.graphicView->getState());
    root.setAttribute(QStringLiteral("web"), ui.kTextEdit->getState());
    root.setAttribute(QStringLiteral("show"), ui.kShow->getState());

    if (ui.kTable->stickHorizontal()) {
        root.setAttribute(QStringLiteral("stickH"), QStringLiteral("Y"));
    }
    if (ui.kTable->stickVertical()) {
        root.setAttribute(QStringLiteral("stickV"), QStringLiteral("Y"));
    }

    return doc.toString();
}

void SKGTableWithGraph::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    m_timer.stop();
    m_timerRedraw.stop();

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    m_mapTitleColor.clear();

    QDomNodeList colors = root.elementsByTagName(QStringLiteral("color"));
    int nb = colors.count();
    for (int i = 0; i < nb; ++i) {
        QDomElement c = colors.at(i).toElement();
        m_mapTitleColor[c.attribute(QStringLiteral("key"))] = QColor(c.attribute(QStringLiteral("value")));
    }

    QString splitterStateString = root.attribute(QStringLiteral("splitterState"));
    if (!splitterStateString.isEmpty()) {
        ui.splitter->restoreState(QByteArray::fromHex(splitterStateString.toLatin1()));
    }
    QString graphModeString = root.attribute(QStringLiteral("graphMode"));
    QString allPositiveString = root.attribute(QStringLiteral("allPositive"));
    QString limitVisibleString = root.attribute(QStringLiteral("limitVisible"));
    QString averageVisibleString = root.attribute(QStringLiteral("averageVisible"));
    QString legendVisibleString = root.attribute(QStringLiteral("legendVisible"));
    QString zeroVisibleString = root.attribute(QStringLiteral("zeroVisible"));
    QString decimalsVisibleString = root.attribute(QStringLiteral("decimalsVisible"));
    QString linearRegressionVisibleString = root.attribute(QStringLiteral("linearRegressionVisible"));
    QString paretorVisibleString = root.attribute(QStringLiteral("paretoVisible"));
    QString sortOrderString = root.attribute(QStringLiteral("sortOrder"));
    QString sortColumnString = root.attribute(QStringLiteral("sortColumn"));
    QString graphicViewStateString = root.attribute(QStringLiteral("graphicViewState"));
    QString webString = root.attribute(QStringLiteral("web"));
    QString showString = root.attribute(QStringLiteral("show"));
    ui.kTable->setStickHorizontal(root.attribute(QStringLiteral("stickH")) == QStringLiteral("Y"));
    ui.kTable->setStickVertical(root.attribute(QStringLiteral("stickV")) == QStringLiteral("Y"));

    // Default value in case of reset
    if (graphModeString.isEmpty()) {
        graphModeString = SKGServices::intToString(LINE);
    }
    if (allPositiveString.isEmpty()) {
        allPositiveString = 'N';
    }
    if (limitVisibleString.isEmpty()) {
        limitVisibleString = 'Y';
    }
    if (averageVisibleString.isEmpty()) {
        averageVisibleString = 'Y';
    }
    if (legendVisibleString.isEmpty()) {
        legendVisibleString = 'N';
    }
    if (linearRegressionVisibleString.isEmpty()) {
        linearRegressionVisibleString = 'Y';
    }
    if (paretorVisibleString.isEmpty()) {
        paretorVisibleString = 'N';
    }
    if (sortOrderString.isEmpty()) {
        sortOrderString = '0';
    }
    if (sortColumnString.isEmpty()) {
        sortColumnString = '0';
    }

    // Set
    setGraphType(SKGTableWithGraph::HISTOGRAM);
    if (m_displayMode != nullptr) {
        m_displayMode->setCurrentIndex(1);
    }
    setGraphType(static_cast<SKGTableWithGraph::GraphType>(SKGServices::stringToInt(graphModeString)));
    m_allPositiveMenu->setChecked(allPositiveString == QStringLiteral("Y"));
    ui.kFilterEdit->setText(root.attribute(QStringLiteral("filter")));
    m_limitVisible = (limitVisibleString == QStringLiteral("Y"));
    if (m_actShowLimits != nullptr) {
        m_actShowLimits->setChecked(m_limitVisible);
    }
    m_averageVisible = (averageVisibleString == QStringLiteral("Y"));
    if (m_actShowAverage != nullptr) {
        m_actShowAverage->setChecked(m_averageVisible);
    }
    m_legendVisible = (legendVisibleString == QStringLiteral("Y"));
    if (m_actShowLegend != nullptr) {
        m_actShowLegend->setChecked(m_legendVisible);
    }
    m_zeroVisible = (zeroVisibleString != QStringLiteral("N"));
    if (m_actShowZero != nullptr) {
        m_actShowZero->setChecked(m_zeroVisible);
    }
    m_decimalsVisible = (decimalsVisibleString != QStringLiteral("N"));
    if (m_actShowDecimal != nullptr) {
        m_actShowDecimal->setChecked(m_decimalsVisible);
    }
    m_linearRegressionVisible = (linearRegressionVisibleString == QStringLiteral("Y"));
    if (m_actShowLinearRegression != nullptr) {
        m_actShowLinearRegression->setChecked(m_linearRegressionVisible);
    }
    m_paretoVisible = (paretorVisibleString == QStringLiteral("Y"));
    if (m_actShowPareto != nullptr) {
        m_actShowPareto->setChecked(m_paretoVisible);
    }

    ui.kTable->setColumnCount(SKGServices::stringToInt(sortColumnString) + 1);
    QHeaderView *horizontalHeader = ui.kTable->horizontalHeader();
    if (horizontalHeader != nullptr) {
        bool previous = horizontalHeader->blockSignals(true);
        horizontalHeader->setSortIndicator(SKGServices::stringToInt(sortColumnString), static_cast<Qt::SortOrder>(SKGServices::stringToInt(sortOrderString)));
        horizontalHeader->blockSignals(previous);
    }
    ui.graphicView->setState(graphicViewStateString);
    ui.kTextEdit->setState(webString);
    if (!showString.isEmpty()) {
        ui.kShow->setState(showString);
    }
}

void SKGTableWithGraph::setFilterVisibility(bool iVisibility) const
{
    ui.kToolbar->setVisible(iVisibility);
}

void SKGTableWithGraph::onFilterModified()
{
    m_timerRedraw.stop();
    m_timer.start(300);
}

void SKGTableWithGraph::onDisplayModeChanged()
{
    QStringList mode = SKGServices::splitCSVLine(ui.kShow->getState());

    // Hide all
    if (m_scene != nullptr) {
        m_scene->clear();
        delete m_scene;
    }
    m_scene = new SKGGraphicsScene();
    ui.graphicView->setScene(m_scene);
    ui.graphicView->hide();
    ui.kTextEdit->hide();
    bool p = ui.kTable->blockSignals(true);
    ui.kTable->hide();
    ui.kTable->blockSignals(p);
    m_graphVisible = false;
    m_tableVisible = false;
    m_textVisible = false;
    m_mapItemGraphic.clear();

    // Show needed widget
    if (mode.contains(QStringLiteral("table"))) {
        ui.kTable->show();
        m_tableVisible = true;
    }
    if (mode.contains(QStringLiteral("graph"))) {
        ui.graphicView->show();
        m_graphVisible = true;
        redrawGraphDelayed();
    }
    if (mode.contains(QStringLiteral("text"))) {
        QTimer::singleShot(100, Qt::CoarseTimer, ui.kTextEdit, &SKGWebView::show);
        m_textVisible = true;
        redrawText();
    }
}

void SKGTableWithGraph::setData(const SKGStringListList &iData,
                                const SKGServices::SKGUnitInfo &iPrimaryUnit,
                                const SKGServices::SKGUnitInfo &iSecondaryUnit,
                                SKGTableWithGraph::DisplayAdditionalFlag iAdditionalInformation,
                                int iNbVirtualColumn)
{
    SKGTRACEINFUNC(10)
    m_data = iData;
    m_primaryUnit = iPrimaryUnit;
    m_secondaryUnit = iSecondaryUnit;
    m_additionalInformation = iAdditionalInformation;
    m_nbVirtualColumns = iNbVirtualColumn;

    onFilterModified();
}

SKGTableWithGraph::DisplayAdditionalFlag SKGTableWithGraph::getAdditionalDisplayMode() const
{
    return m_additionalInformation;
}

QStringList SKGTableWithGraph::getSumItems(const QString &iString) const
{
    QStringList output;
    QString current = iString;
    int index = -1;
    do {
        output.insert(0, current);
        index = current.lastIndexOf(OBJECTSEPARATOR);
        if (index != -1) {
            current = current.left(index);
        }
    } while (index != -1);
    return output;
}

void SKGTableWithGraph::addSums(SKGStringListList &ioTable, int &iNblines)
{
    SKGTRACEINFUNC(10)
    int nbCols = -1;
    if (!ioTable.isEmpty()) {
        nbCols = ioTable.at(0).count();
    }

    // Create a list of sums lines associated to the index where to add them
    QMap<double, QStringList> sums;
    for (int i = 1; i < nbCols; ++i) {
        QStringList previousHeaderSum;
        QList<double> sum;
        QList<int> nbElem;

        for (int j = 1; j < iNblines; ++j) {
            double indextoadd = j + 1 - 0.01;
            QStringList currentHeaderSum = getSumItems(ioTable.at(j).at(0));
            if (previousHeaderSum.isEmpty()) {
                previousHeaderSum = currentHeaderSum;
            }

            int nb = qMax(currentHeaderSum.count(), previousHeaderSum.count());
            for (int k = 0; k < nb; ++k) {
                QString chString = currentHeaderSum.value(k);
                QString phString = previousHeaderSum.value(k);
                if (chString != phString) {
                    // Add this sum
                    if (nbElem.value(k) > 1) {
                        QStringList thisSum;
                        if (i == 1) {
                            thisSum.push_back(phString);
                        } else {
                            thisSum = sums[indextoadd];
                        }
                        thisSum.push_back(SKGServices::doubleToString(sum.value(k)));

                        sums[indextoadd] = thisSum;

                        indextoadd -= 0.01;
                    }

                    // Reset sum
                    if (k < sum.count()) {
                        sum[k] = 0;
                    } else {
                        sum.push_back(0);
                    }
                    if (k < nbElem.count()) {
                        nbElem[k] = 0;
                    } else {
                        nbElem.push_back(0);
                    }
                }
                if (j < iNblines - 1) {
                    sum = sum.mid(0, previousHeaderSum.count());
                    nbElem = nbElem.mid(0, previousHeaderSum.count());

                    QString valstring = ioTable.at(j).at(i);
                    double v = 0.0;
                    if (!valstring.isEmpty()) {
                        v = SKGServices::stringToDouble(valstring);
                    }
                    if (k < sum.count()) {
                        sum[k] += v;
                    } else {
                        sum.push_back(v);
                    }
                    if (k < nbElem.count()) {
                        nbElem[k] = nbElem[k] + 1;
                    } else {
                        nbElem.push_back(1);
                    }
                }
            }

            previousHeaderSum = std::move(currentHeaderSum);
        }
    }

    // Add all sums in table
    QList<double> keys = sums.keys();
    std::sort(keys.begin(), keys.end());
    int nbkey = keys.count();
    for (int i = nbkey - 1; i >= 0; --i) {
        double key = keys.at(i);
        ioTable.insert(key, sums[key]);
        m_sumRows.insert(key, true);
        ++iNblines;
    }
}

void SKGTableWithGraph::refresh()
{
    SKGTRACEINFUNC(10)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Set parameters for static method
    QHeaderView *horizontalHeader = ui.kTable->horizontalHeader();
    m_sortOrder = horizontalHeader->sortIndicatorOrder();
    m_sortColumn = horizontalHeader->sortIndicatorSection();

    SKGStringListList groupedTable = m_data;
    int nbCols = -1;
    if (!groupedTable.isEmpty()) {
        nbCols = groupedTable.at(0).count();
    }
    int nbRealCols = nbCols;

    // Create filtered table
    {
        // Build list of criterias
        SKGServices::SKGSearchCriteriaList criterias = SKGServices::stringToSearchCriterias(ui.kFilterEdit->text());

        // Check all lines
        int nblists = criterias.count();
        if ((nblists != 0) && (nbCols != 0)) {
            for (int i = groupedTable.count() - 1; i >= 1; --i) { // The first line is not filtered because it is the title
                QStringList line = groupedTable.at(i);

                // Get title of the line
                const QString &val = line.at(0);

                // Filtered
                bool ok = false;
                for (int l = 0; l < nblists; ++l) {
                    QStringList words = criterias[l].words;
                    QChar mode = criterias[l].mode;
                    int nbwords = words.count();

                    bool validateAllWords = true;
                    for (int w = 0; validateAllWords && w < nbwords; ++w) {
                        validateAllWords = val.contains(words.at(w), Qt::CaseInsensitive);
                    }
                    if (mode == '+') {
                        ok |= validateAllWords;
                    } else if (mode == '-' && validateAllWords) {
                        ok = false;
                    }
                }

                if (!ok) {
                    groupedTable.removeAt(i);
                }
            }
        }
    }

    // Initialise sumRows
    int nblines = groupedTable.count();
    m_sumRows.clear();
    for (int j = 0; j < nblines; ++j) {
        m_sumRows.push_back(false);
    }

    // Compute sums line
    if ((m_additionalInformation & SUM) != 0u) {
        QStringList sums;
        sums.reserve(nbCols + 1);
        sums.push_back(QString());
        for (int i = 1; i < nbCols; ++i) {
            double sum = 0;
            for (int j = 1; j < nblines; ++j) {
                QString valstring = groupedTable.at(j).at(i);
                if (!valstring.isEmpty()) {
                    sum += SKGServices::stringToDouble(valstring);
                }
            }
            sums.push_back(SKGServices::doubleToString(sum));
        }
        groupedTable.push_back(sums);
        m_sumRows.push_back(true);
        ++nblines;
    }

    // Compute sub sums lines
    if ((m_additionalInformation & SUM) != 0u && m_sortColumn == 0 && m_sortOrder == Qt::AscendingOrder) {
        addSums(groupedTable, nblines);
    }

    // Compute sum and average column
    m_indexSum = -1;
    m_indexAverage = -1;
    m_indexMin = -1;
    m_indexLinearRegression = -1;
    if (nbCols > 2
        && ((m_additionalInformation & SUM) != 0u || (m_averageVisible && (m_additionalInformation & AVERAGE) != 0u)
            || (m_limitVisible && (m_additionalInformation & LIMITS) != 0u))) {
        SKGTRACEINFUNC(10)
        // Add title
        QStringList newLine = groupedTable.at(0);
        if ((m_additionalInformation & SUM) != 0u) {
            m_indexSum = newLine.count();
            newLine.push_back(i18nc("Noun, the numerical sum of a list of values", "Sum"));
        }
        if (m_averageVisible && (m_additionalInformation & AVERAGE) != 0u) {
            m_indexAverage = newLine.count();
            newLine.push_back(i18nc("Noun, the numerical average of a list of values", "Average"));
        }
        if (m_limitVisible && (m_additionalInformation & LIMITS) != 0u) {
            m_indexMin = newLine.count();
            newLine.push_back(i18nc("Noun, the minimum value of a list of values", "Min"));
            newLine.push_back(i18nc("Noun, the maximum value of a list of values", "Max"));
        }

        if (m_linearRegressionVisible) {
            m_indexLinearRegression = newLine.count();
            newLine.push_back(i18nc("Noun", "Tendency line"));
        }

        groupedTable.replace(0, newLine);

        for (int i = 1; i < nblines; ++i) {
            QStringList newLine2 = groupedTable.at(i);
            double sumy = 0;
            double sumx = 0;
            double sumx2 = 0;
            double sumxy = 0;
            double min = 10e20;
            double max = -10e20;
            int nbVals = 0;
            for (int j = 1; j < nbCols - m_nbVirtualColumns; ++j) {
                const QString &valstring = newLine2.at(j);
                if (!valstring.isEmpty()) {
                    double v = SKGServices::stringToDouble(valstring);
                    sumx += j;
                    sumx2 += j * j;
                    sumy += v;
                    sumxy += j * v;
                    min = qMin(min, v);
                    max = qMax(max, v);
                    ++nbVals;
                }
            }
            if ((m_additionalInformation & SUM) != 0u) {
                newLine2.push_back(SKGServices::doubleToString(sumy));
            }
            if (m_averageVisible && (m_additionalInformation & AVERAGE) != 0u) {
                if (nbVals != 0) {
                    newLine2.push_back(SKGServices::doubleToString(sumy / nbVals));
                } else {
                    newLine2.push_back(QStringLiteral("0"));
                }
            }
            if (m_limitVisible && (m_additionalInformation & LIMITS) != 0u) {
                if (nbVals != 0) {
                    newLine2.push_back(SKGServices::doubleToString(min));
                    newLine2.push_back(SKGServices::doubleToString(max));
                } else {
                    newLine2.push_back(QStringLiteral("0"));
                    newLine2.push_back(QStringLiteral("0"));
                }
            }

            if (nbVals != 0) {
                double s2x = sumx2 / nbVals - sumx * sumx / (nbVals * nbVals);
                double sxy = sumxy / nbVals - (sumx / nbVals) * (sumy / nbVals);

                double a = (s2x != 0.0 ? sxy / s2x : 0.0);
                double b = sumy / nbVals - a * sumx / nbVals;

                newLine2.push_back("y=" % SKGServices::doubleToString(a) % "*x+" % SKGServices::doubleToString(b));
            } else {
                newLine2.push_back(QStringLiteral("y=0"));
            }

            groupedTable.replace(i, newLine2);
        }
        if (m_linearRegressionVisible) {
            ++nbCols;
        }
        if ((m_additionalInformation & SUM) != 0u) {
            ++nbCols;
        }
        if (m_averageVisible && (m_additionalInformation & AVERAGE) != 0u) {
            ++nbCols;
        }
        if (m_limitVisible && (m_additionalInformation & LIMITS) != 0u) {
            nbCols += 2;
        }
    }

    // Sort lines
    if (m_sortColumn != 0 || m_sortOrder != Qt::AscendingOrder) {
        SKGTRACEINFUNC(10)
        // Extract title and sums
        if (!groupedTable.isEmpty()) {
            QStringList fist = groupedTable.takeFirst();
            QStringList last;
            if ((m_additionalInformation & SUM) != 0u && !groupedTable.isEmpty()) {
                last = groupedTable.takeLast();
            }

            // Sort
            QCollator comp;
            comp.setCaseSensitivity(Qt::CaseInsensitive);
            std::sort(groupedTable.begin(), groupedTable.end(), [&](const QStringList &s1, const QStringList &s2) {
                if (m_sortColumn >= s1.count()) {
                    m_sortColumn = s1.count() - 1;
                }
                if (m_sortColumn >= 0) {
                    const QString &v1 = s1.at(m_sortColumn);
                    const QString &v2 = s2.at(m_sortColumn);
                    if (m_sortColumn == 0) {
                        int v = comp.compare(v1, v2);
                        return (m_sortOrder != 0u ? v > 0 : v < 0);
                    }

                    double vd1 = SKGServices::stringToDouble(v1);
                    double vd2 = SKGServices::stringToDouble(v2);
                    return (m_sortOrder != 0u ? vd1 > vd2 : vd1 < vd2);
                }
                return false;
            });

            // Add title and sums
            groupedTable.insert(0, fist);
            if ((m_additionalInformation & SUM) != 0u) {
                groupedTable.push_back(last);
            }
        }
    }

    IFSKGTRACEL(10)
    {
        QStringList dump = SKGServices::tableToDump(groupedTable, SKGServices::DUMP_TEXT);
        int nbl = dump.count();
        for (int i = 0; i < nbl; ++i) {
            SKGTRACE << dump.at(i) << Qt::endl;
        }
    }

    // Fill table
    {
        SKGTRACEINFUNC(10)

        int nbRealColumns = nbRealCols - m_nbVirtualColumns;
        ui.kTable->hide();
        QHeaderView *hHeader = ui.kTable->horizontalHeader();
        if (hHeader != nullptr) {
            hHeader->setSectionResizeMode(QHeaderView::Fixed); // Needed to improve performances of setHorizontalHeaderLabels
        }
        ui.kTable->clear();
        ui.kTable->setRowCount(nblines - 1);
        ui.kTable->setColumnCount(nbCols);
        for (int i = 0; i < nblines; ++i) {
            const QStringList &line = groupedTable.at(i);
            if (i == 0) {
                SKGTRACEINFUNC(10)
                // Set header
                ui.kTable->setHorizontalHeaderLabels(line);
            } else {
                for (int j = 0; j < nbCols; ++j) {
                    const QString &val = line.at(j);

                    QTableWidgetItem *item;
                    if (j == 0) {
                        // Create the line header
                        if (m_sumRows.at(i)) {
                            item = new QTableWidgetItem(val.isEmpty() ? i18nc("Noun, the numerical sum of a list of values", "Sum")
                                                                      : i18nc("Noun, the numerical sum of a list of values", "Sum of '%1'", val));
                            item->setData(1, val);
                            QFont f = item->font();
                            f.setBold(true);
                            item->setFont(f);
                            if (m_indexLinearRegression != -1) {
                                item->setData(DATA_VALUE, line.at(m_indexLinearRegression));
                            }
                            ui.kTable->setItem(i - 1, j, item);

                            // Set header
                            ui.kTable->setVerticalHeaderItem(i - 1, new QTableWidgetItem(*item));
                        } else {
                            // New color selector
                            QColor defaultColor;
                            int color_h = (240 + 360 * (i - 1) / nblines) % 360; // First color is blue to be compliant with min (red) and max (green)
                            defaultColor = QColor::fromHsv(color_h, 255, 255);

                            QColor color;
                            if (m_mapTitleColor.contains(val)) {
                                color = m_mapTitleColor[val];
                            } else {
                                color = defaultColor;
                                m_mapTitleColor[val] = color;
                            }

                            auto colorSelector = new SKGColorButton(this);
                            colorSelector->setText(val);
                            colorSelector->setToolTip(val);
                            colorSelector->setColor(color);
                            colorSelector->setDefaultColor(defaultColor);
                            connect(colorSelector, &SKGColorButton::changed, this, &SKGTableWithGraph::onChangeColor);

                            ui.kTable->setCellWidget(i - 1, 0, colorSelector);

                            // Set header
                            auto itemTmp = new QTableWidgetItem(val);
                            itemTmp->setBackground(color);
                            itemTmp->setToolTip(val);
                            ui.kTable->setVerticalHeaderItem(i - 1, itemTmp);
                        }
                    } else {
                        // Add a value
                        QString tooltip = line.at(0) % '\n' % groupedTable.at(0).at(j);
                        if (!val.isEmpty()) {
                            if (j == m_indexLinearRegression) {
                                // A linear regression value
                                item = new QTableWidgetItem(val);
                            } else {
                                // A single value
                                double vald = SKGServices::stringToDouble(val);
                                QString vals = SKGServices::toCurrencyString(vald, m_primaryUnit.Symbol, m_decimalsVisible ? m_primaryUnit.NbDecimal : 0);
                                tooltip += '\n' % vals;

                                item = new QTableWidgetItem(vals);
                                item->setToolTip(tooltip);
                                if (!m_secondaryUnit.Symbol.isEmpty() && (m_secondaryUnit.Value != 0.0)) {
                                    item->setToolTip(tooltip % '\n'
                                                     % SKGServices::toCurrencyString(vald / m_secondaryUnit.Value,
                                                                                     m_secondaryUnit.Symbol,
                                                                                     m_decimalsVisible ? m_secondaryUnit.NbDecimal : 0));
                                }

                                item->setData(DATA_VALUE, vald);
                                item->setTextAlignment(Qt::AlignRight);
                                if (vald < 0) {
                                    item->setForeground(m_NegativeColor);
                                }
                                if (m_sumRows.at(i)) {
                                    QFont f = item->font();
                                    f.setBold(true);
                                    item->setFont(f);
                                }
                            }
                        } else {
                            // An empty value
                            item = new QTableWidgetItem(QString());
                            item->setToolTip(tooltip);
                            item->setData(DATA_VALUE, "");
                        }
                        item->setFlags((j >= nbRealColumns && j != m_indexSum) || ui.kTable->horizontalHeaderItem(j)->text() == QStringLiteral("0000")
                                           ? Qt::NoItemFlags
                                           : Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                        ui.kTable->setItem(i - 1, j, item);
                    }
                }
            }
        }

        // Refresh graphic view
        redrawGraph();

        // Refresh text area
        redrawText();

        if (hHeader != nullptr) {
            hHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
        }

        if (m_tableVisible) {
            ui.kTable->show();
        }

        if ((hHeader != nullptr) && (hHeader->count() != 0)) {
            hHeader->resizeSection(0, 100);
            hHeader->setSectionResizeMode(0, QHeaderView::Interactive);
        }
    }

    QApplication::restoreOverrideCursor();
}

void SKGTableWithGraph::onChangeColor()
{
    auto *colorButton = qobject_cast<SKGColorButton *>(sender());
    if (colorButton != nullptr) {
        m_mapTitleColor[colorButton->text()] = colorButton->color();
        refresh();
    }
}

void SKGTableWithGraph::resetColors()
{
    m_mapTitleColor.clear();
    refresh();
}

void SKGTableWithGraph::onSelectionChanged()
{
    _SKGTRACEINFUNC(10)
    if (m_graphVisible) {
        // Unset color on previous selection
        int nbRow = ui.kTable->rowCount();
        int nbCol = ui.kTable->columnCount();
        for (int r = 0; r < nbRow; ++r) {
            for (int c = 0; c < nbCol; ++c) {
                QTableWidgetItem *previous = ui.kTable->item(r, c);
                if (previous != nullptr) {
                    QGraphicsItem *val = m_mapItemGraphic.value(previous);
                    if (val != nullptr) {
                        auto *graphicItem = qgraphicsitem_cast<QAbstractGraphicsShapeItem *>(val);
                        if (graphicItem != nullptr) {
                            QColor color = QColor::fromHsv(graphicItem->data(DATA_COLOR_H).toInt(),
                                                           graphicItem->data(DATA_COLOR_S).toInt(),
                                                           graphicItem->data(DATA_COLOR_V).toInt());
                            color.setAlpha(ALPHA);

                            if (graphicItem->data(DATA_MODE).toInt() == 1) {
                                QPen pen = graphicItem->pen();
                                pen.setColor(color);
                                graphicItem->setPen(pen);
                            } else {
                                graphicItem->setBrush(QBrush(color));
                            }
                            graphicItem->setZValue(graphicItem->data(DATA_Z_VALUE).toReal());
                            if (graphicItem->isSelected()) {
                                graphicItem->setSelected(false);
                            }
                        }
                    }
                }
            }
        }

        // Set highlight color on current selection
        QList<QTableWidgetItem *> selected = ui.kTable->selectedItems();
        int nb = selected.count();
        for (int i = 0; i < nb; ++i) {
            QTableWidgetItem *current = selected.at(i);
            if (current != nullptr) {
                QGraphicsItem *val = m_mapItemGraphic.value(current);
                auto *graphicItem = qgraphicsitem_cast<QAbstractGraphicsShapeItem *>(val);
                if (graphicItem != nullptr) {
                    if (graphicItem->data(DATA_MODE).toInt() == 1) {
                        QPen pen = graphicItem->pen();
                        pen.setColor(QApplication::palette().color(QPalette::Highlight));
                        graphicItem->setPen(pen);
                    } else {
                        graphicItem->setBrush(QBrush(QApplication::palette().color(QPalette::Highlight)));
                    }
                    graphicItem->setZValue(15);
                    graphicItem->setSelected(true);
                    graphicItem->ensureVisible();
                }
            }
        }
    }

    Q_EMIT selectionChanged();
}

void SKGTableWithGraph::onDoubleClickGraph()
{
    if (m_scene != nullptr) {
        // Get selection
        QList<QGraphicsItem *> selectedGraphItems = m_scene->selectedItems();
        if (!selectedGraphItems.isEmpty()) {
            Q_EMIT cellDoubleClicked(selectedGraphItems[0]->data(1).toInt(), selectedGraphItems[0]->data(2).toInt());
        }
    }
}

void SKGTableWithGraph::onDoubleClick(int row, int column)
{
    Q_EMIT cellDoubleClicked(row, column);
}

void SKGTableWithGraph::onLinkClicked(const QUrl &url)
{
    QString path = url.toString().remove(QStringLiteral("https://linkclicked/"));
    QStringList items = SKGServices::splitCSVLine(path, ',');
    if (items.count() == 2) {
        Q_EMIT cellDoubleClicked(SKGServices::stringToInt(items[0]), SKGServices::stringToInt(items[1]));
    }
}

void SKGTableWithGraph::onSelectionChangedInGraph()
{
    _SKGTRACEINFUNC(10)
    if (m_scene != nullptr) {
        bool previous = ui.kTable->blockSignals(true);
        ui.kTable->clearSelection();

        // Get selection
        QList<QGraphicsItem *> selectedGraphItems = m_scene->selectedItems();
        int nb = selectedGraphItems.count();
        for (int i = 0; i < nb; ++i) {
            ui.kTable->setCurrentCell(selectedGraphItems.at(i)->data(1).toInt(), selectedGraphItems.at(i)->data(2).toInt(), QItemSelectionModel::Select);
        }
        ui.kTable->blockSignals(previous);

        previous = m_scene->blockSignals(true);
        onSelectionChanged();
        m_scene->blockSignals(previous);
    }
}

void SKGTableWithGraph::redrawGraphDelayed()
{
    m_timerRedraw.start(300);
}

void SKGTableWithGraph::redrawText()
{
    if (!m_textVisible) {
        return;
    }
    SKGTRACEINFUNC(10)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QString html = QStringLiteral(
        "<? xml version = \"1.0\" encoding=\"utf-8\"?>"
        "<!DOCTYPE html PUBLIC \"-// W3C// DTD XHTML 1.0 Strict// EN\" \"https://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">"
        "<html xmlns=\"https://www.w3.org/1999/xhtml\">"
        "<head>"
        "<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />"
        "<meta http-equiv=\"Content-Style-Type\" content=\"text/css\" />"
        "<style type=\"text/css\">"
        "body{background-color: #FFFFFF; font-size : small; display: inline;} a{color: inherit; text-decoration: inherit;} h1{text-decoration: underline; "
        "color: #FF3333;} h2{text-decoration: underline; color: #FF9933;} .table{border: thin solid #000000; border-collapse: collapse; background-color: "
        "#000000;} .tabletitle{background-color: #6495ed; color : #FFFF33; font-weight : bold; font-size : normal} .tabletotal{background-color: "
        "#D0E3FA;font-weight : bold;} tr{background-color: #FFFFFF;padding: 2px;} td{padding: 2px; white-space: nowrap;}"
        "</style>"
        "</head>"
        "<body>"
        "<table class=\"table\"><tr class=\"tabletitle\">");
    // Dump header
    int nbCols = ui.kTable->columnCount();
    for (int i = 0; i < nbCols; ++i) {
        QTableWidgetItem *item = ui.kTable->horizontalHeaderItem(i);
        if (item != nullptr) {
            html += R"(<td align="center" width="1000"><b>)" % item->text() % "</b></td>";
        }
    }
    html += QStringLiteral("</tr>");

    // Dump values
    int nbLines = ui.kTable->rowCount();
    for (int j = 0; j < nbLines; ++j) {
        html += QStringLiteral("<tr") % (m_sumRows.at(j + 1) ? " class=\"tabletotal\"" : "") % u'>';
        for (int i = 0; i < nbCols; ++i) {
            QTableWidgetItem *item = ui.kTable->item(j, i);
            if (item != nullptr) {
                bool red = (item->data(DATA_VALUE).toDouble() < 0);
                html += QStringLiteral("<td align=\"right\">") % (red ? "<font color=\"red\">" : "");
                if ((item->flags() & Qt::ItemIsSelectable) != 0u) {
                    html += "<a href=\"https://linkclicked/" % SKGServices::intToString(j) % "," % SKGServices::intToString(i) % "\">";
                }
                html += item->text();
                if ((item->flags() & Qt::ItemIsSelectable) != 0u) {
                    html += QStringLiteral("</a>");
                }
                html += QString(red ? QStringLiteral("</font>") : QString()) % "</td>";
            } else {
                auto *colorButton = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(j, i));
                if (colorButton != nullptr) {
                    html += "<td><b>" % colorButton->text() % "</b></td>";
                }
            }
        }
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</table>");
    html += QStringLiteral("</body></html>");
    ui.kTextEdit->setHtml(html);
    QApplication::restoreOverrideCursor();
}

void SKGTableWithGraph::redrawGraph()
{
    SKGTRACEINFUNC(10)
    m_mapItemGraphic.clear();
    if (!m_graphVisible) {
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    ui.graphicView->hide();
    ui.kTable->hide();

    // Recreate scene
    if (m_scene != nullptr) {
        SKGTRACEINFUNC(10)
        m_scene->clear();
        delete m_scene;
    }

    m_scene = new SKGGraphicsScene();
    {
        SKGTRACEINFUNC(10)
        m_scene->setBackgroundBrush(m_backgroundColor);

        // Get current selection
        int crow = ui.kTable->currentRow();
        int ccolumn = ui.kTable->currentColumn();

        // Get nb columns and rows
        int nbRows = ui.kTable->rowCount();
        int nbRealRows = nbRows;
        for (int posy = 0; posy < nbRows; ++posy) {
            if (m_sumRows.at(posy + 1)) {
                --nbRealRows;
            }
        }

        int nbColumns = getNbColumns(false);
        int nbRealColumns = nbColumns - m_nbVirtualColumns;

        // Get graphic mode
        GraphType mode = getGraphType();

        // Get in positive
        bool inPositive = false;
        if (mode == STACK || mode == STACKCOLUMNS || mode == HISTOGRAM || mode == POINT || mode == LINE || mode == STACKAREA) {
            m_allPositiveMenu->setEnabled(true);
            inPositive = (m_allPositiveMenu->isChecked());
        } else {
            m_allPositiveMenu->setEnabled(false);
            if (mode == CONCENTRICPIE || mode == PIE || mode == TREEMAP) {
                inPositive = true;
            }
        }

        // Get show origin
        bool showOrigin = true;
        if (mode == HISTOGRAM || mode == POINT || mode == LINE) {
            m_actShowZero->setEnabled(true);
            showOrigin = (m_actShowZero->isChecked());
        } else {
            m_actShowZero->setEnabled(false);
        }

        // Compute y limits
        double minLimit = (showOrigin ? 0 : 9999999);
        double maxLimit = (showOrigin ? 0 : -9999999);
        int nbLevel = 0;
        SKGTRACEL(3) << "mode=" << static_cast<unsigned int>(mode) << Qt::endl;
        SKGTRACEL(3) << "nb rows        =" << nbRows << Qt::endl;
        SKGTRACEL(3) << "nb real rows   =" << nbRealRows << Qt::endl;
        SKGTRACEL(3) << "nb columns     =" << nbColumns << Qt::endl;
        SKGTRACEL(3) << "nb real columns=" << nbRealColumns << Qt::endl;
        SKGTRACEL(3) << "selected row   =" << crow << Qt::endl;
        SKGTRACEL(3) << "selected column=" << ccolumn << Qt::endl;
        if (mode == STACK) {
            // STACK
            for (int posx = 0; posx < nbRows; ++posx) {
                if (!m_sumRows[posx + 1]) {
                    double sumPositive = 0;
                    double sumNegative = 0;
                    for (int posy = 0; posy < nbColumns; ++posy) {
                        QTableWidgetItem *tableItem = ui.kTable->item(posx, posy);
                        if (tableItem != nullptr) {
                            QVariant valQ = tableItem->data(DATA_VALUE);
                            if (valQ.userType() == QMetaType::Double) {
                                double val = valQ.toDouble();
                                if (inPositive || val >= 0) {
                                    sumPositive += qAbs(val);
                                } else {
                                    sumNegative += val;
                                }
                            }
                        }
                    }

                    minLimit = qMin(minLimit, sumNegative);
                    maxLimit = qMax(maxLimit, sumPositive);
                }
            }
        } else if (mode == STACKAREA || mode == STACKCOLUMNS || mode == PIE || mode == CONCENTRICPIE || mode == TREEMAP) {
            // STACKAREA or STACKCOLUMNS or PIE or CONCENTRICPIE
            for (int posy = 0; posy < nbColumns; ++posy) {
                double sumPositive = 0;
                double sumNegative = 0;
                for (int posx = 0; posx < nbRows; ++posx) {
                    if (!m_sumRows[posx + 1]) {
                        QTableWidgetItem *tableItem = ui.kTable->item(posx, posy);
                        if (tableItem != nullptr) {
                            QVariant valQ = tableItem->data(DATA_VALUE);
                            if (valQ.userType() == QMetaType::Double) {
                                double val = valQ.toDouble();
                                if (inPositive || val >= 0) {
                                    sumPositive += qAbs(val);
                                } else {
                                    sumNegative += val;
                                }
                            }
                        }
                    }
                }
                if (mode == TREEMAP) {
                    // TREEMAP
                    minLimit = 0;
                    maxLimit = qAbs(sumNegative) + sumPositive;
                } else {
                    minLimit = qMin(minLimit, sumNegative);
                    maxLimit = qMax(maxLimit, sumPositive);
                }
            }
        } else if (mode == HISTOGRAM || mode == POINT || mode == LINE) {
            // HISTOGRAM or POINTS or LINES
            for (int posx = 0; posx < nbRows; ++posx) {
                if (!m_sumRows[posx + 1]) {
                    for (int posy = 0; posy < nbColumns; ++posy) {
                        QTableWidgetItem *tableItem = ui.kTable->item(posx, posy);
                        if (tableItem != nullptr) {
                            QVariant valQ = tableItem->data(DATA_VALUE);
                            if (valQ.userType() == QMetaType::Double) {
                                double val = valQ.toDouble();
                                if (inPositive) {
                                    maxLimit = qMax(maxLimit, qAbs(val));
                                    minLimit = qMin(minLimit, qAbs(val));
                                } else {
                                    minLimit = qMin(minLimit, val);
                                    maxLimit = qMax(maxLimit, val);
                                }
                            }
                        }
                    }
                }
            }
        } else if (mode == BUBBLE) {
            for (int posx = 0; posx < nbRows; ++posx) {
                if (!m_sumRows[posx + 1]) {
                    for (int posy = 0; posy < nbColumns; ++posy) {
                        QTableWidgetItem *tableItem = ui.kTable->item(posx, posy);
                        if (tableItem != nullptr) {
                            QVariant valQ = tableItem->data(DATA_VALUE);
                            if (valQ.userType() == QMetaType::Double) {
                                double val = valQ.toDouble();
                                maxLimit = qMax(maxLimit, qAbs(val));
                            }
                        }
                    }
                }
            }
        }

        if (mode == CONCENTRICPIE) {
            for (int posx = 0; posx < nbRows; ++posx) {
                auto *btn = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(posx, 0));
                if (btn != nullptr) {
                    QString xname = btn->text();
                    QStringList vals = xname.split(OBJECTSEPARATOR);
                    int nbvals = vals.count();
                    nbLevel = qMax(nbLevel, nbvals - 1);
                }
            }
        }

        // Compute
        double yorigin = 0.0;
        double widthItem = 10;
        double maxX = 0;
        double margin = 0;
        double marginLeft = 0;
        double xstep = 0.0;
        double radius = 0.0;
        int jstep = qMax(computeStepSize(nbColumns, 10), static_cast<double>(1.0));
        double ystep = computeStepSize(maxLimit - minLimit, 10);
        if (mode != BUBBLE && mode != PIE && mode != CONCENTRICPIE && mode != TREEMAP && ystep != 0) {
            double newMinLimit = ystep * qRound(minLimit / ystep);
            minLimit = (minLimit - newMinLimit < -EPSILON ? newMinLimit - ystep : newMinLimit);
            double newMaxLimit = ystep * qRound(maxLimit / ystep);
            maxLimit = (maxLimit - newMaxLimit > EPSILON ? newMaxLimit + ystep : newMaxLimit);
        }

        if ((nbRealRows != 0) && ystep != 0) {
            QRect vSize = ui.graphicView->rect();
            if (mode == STACK) {
                margin = (maxLimit - minLimit) * 0.3;

                if (!showOrigin) {
                    if (minLimit > 0) {
                        yorigin = ystep * (qRound(minLimit / ystep));
                    } else if (maxLimit < 0) {
                        yorigin = ystep * (qRound(maxLimit / ystep));
                    }
                }

                widthItem = (maxLimit - minLimit) / (nbRealRows + 1);
                widthItem = widthItem * (static_cast<double>(vSize.width())) / (static_cast<double>(vSize.height()));

                xstep = widthItem * (nbRealRows + 1);
                marginLeft = widthItem;

                maxX = widthItem * nbRealRows + margin;
            } else if (mode == HISTOGRAM || mode == POINT || mode == LINE || mode == STACKAREA || mode == STACKCOLUMNS) {
                margin = (maxLimit - minLimit) * 0.3;

                if (!showOrigin) {
                    if (minLimit > 0) {
                        yorigin = ystep * (qRound(minLimit / ystep));
                    } else if (maxLimit < 0) {
                        yorigin = ystep * (qRound(maxLimit / ystep));
                    }
                }

                widthItem = (maxLimit - minLimit) / ((nbRealRows + 1) * (nbColumns - 1));
                widthItem = widthItem * (static_cast<double>(vSize.width())) / (static_cast<double>(vSize.height()));

                xstep = widthItem * (nbRealRows + 1);
                marginLeft = qMin(jstep * xstep, widthItem * (nbColumns - 1));

                maxX = xstep * (nbColumns - 1);
                if (mode == POINT || mode == LINE || mode == STACKAREA) {
                    maxX -= xstep;
                }
                //                 radius = qMax(margin / 100.0, qMin(width, qMin(ystep / 4, jstep * xstep / 4)));
                radius = qMax(margin / 200, qMin(widthItem, qMin(ystep / 8, jstep * xstep / 8)));
            } else if (mode == BUBBLE) {
                margin = ystep * nbRealRows * 0.3;

                widthItem = ystep * nbRealRows / (nbRealRows * (nbColumns - 1));
                widthItem = widthItem * (static_cast<double>(vSize.width())) / (static_cast<double>(vSize.height()));

                xstep = widthItem * (nbRealRows + 1);
                marginLeft = jstep * xstep;

                maxX = widthItem * (nbColumns - 1) * (nbRealRows + 1);
            }
        }

        SKGTRACEL(3) << "minLimit=" << minLimit << Qt::endl;
        SKGTRACEL(3) << "maxLimit=" << maxLimit << Qt::endl;
        SKGTRACEL(3) << "ystep=" << ystep << Qt::endl;
        SKGTRACEL(3) << "yorigin=" << yorigin << Qt::endl;
        SKGTRACEL(3) << "width=" << widthItem << Qt::endl;
        SKGTRACEL(3) << "maxX=" << maxX << Qt::endl;
        SKGTRACEL(3) << "margin=" << margin << Qt::endl;
        SKGTRACEL(3) << "xstep=" << xstep << Qt::endl;
        SKGTRACEL(3) << "marginLeft=" << marginLeft << Qt::endl;
        SKGTRACEL(3) << "radius=" << radius << Qt::endl;

        // Initialise pens
        QPen axisPen = QPen(Qt::SolidLine);
        axisPen.setColor(m_axisColor);
        axisPen.setWidthF(0);
        axisPen.setJoinStyle(Qt::RoundJoin);

        QPen gridPen = QPen(Qt::SolidLine);
        gridPen.setColor(m_gridColor);
        gridPen.setWidthF(0);

        QPen dotPen = QPen(Qt::SolidLine); // WARNING: Qt::DotLine is very bad for performances
        dotPen.setWidthF(0);

        QPen outlinePen(m_outlineColor);
        outlinePen.setWidthF(0);

        // Set rect
        int nbColInMode2 = (nbColumns > 2 ? sqrt(nbColumns - 1) + .5 : 1);
        m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
        SKGTRACEL(10) << "Items:" << nbRealRows << "x" << (nbColumns - 1) << "=" << nbRealRows * (nbColumns - 1) << Qt::endl;
        SKGTRACEL(10) << "nbColInMode2=" << nbColInMode2 << Qt::endl;

        // Compute treemap
        QMap<QString, SKGTreeMap> treemap;
        if (mode == TREEMAP) {
            SKGTreeMap treemapobj(QString(), 0, 0, 0, BOX_SIZE, BOX_SIZE);
            for (int j = 1; j < nbColumns; ++j) {
                SKGTreeMap treemapobj2;
                for (int xx = 0; xx < nbRows; ++xx) {
                    auto *colorButton = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(xx, 0));
                    if (colorButton != nullptr) {
                        // Get cell value
                        QTableWidgetItem *tableItem = ui.kTable->item(xx, j);
                        if (tableItem != nullptr) {
                            QVariant valQ = tableItem->data(DATA_VALUE);
                            if (valQ.userType() == QMetaType::Double) {
                                double val = qAbs(valQ.toDouble());
                                QString id = SKGServices::intToString(xx) % QStringLiteral("-") % SKGServices::intToString(j);
                                treemapobj2.addChild(SKGTreeMap(id, val));
                            }
                        }
                    }
                }
                treemapobj.addChild(treemapobj2);
            }

            treemapobj.compute();
            treemap = treemapobj.getAllTilesById();
        }

        // Redraw scene
        double x0 = 0;
        double x1 = 0;
        double ymin = (showOrigin || mode == STACK || mode == STACKCOLUMNS ? 0 : 9999999);
        double ymax = (showOrigin || mode == STACK || mode == STACKCOLUMNS ? 0 : -9999999);
        int posx = 0;
        int nbRowsDrawed = 0;
        QMap<int, double> previousStackedPlus;
        QMap<int, double> previousStackedMoins;
        QMap<double, double> paretoPoints;
        for (int xx = 0; xx < nbRows; ++xx) {
            auto *colorButton = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(xx, 0));
            if (colorButton != nullptr) {
                QString xname = colorButton->text();
                QColor initialColor = colorButton->color();
                double yPlus = 0;
                double yMoins = 0;
                int jprevious = -1;

                ++nbRowsDrawed;
                for (int j = 1; j < nbColumns; ++j) {
                    // Get cell value
                    QTableWidgetItem *tableItem = ui.kTable->item(xx, j);
                    if (tableItem != nullptr) {
                        QVariant valQ = tableItem->data(DATA_VALUE);
                        if (valQ.userType() == QMetaType::Double) {
                            double val = valQ.toDouble();
                            QString valstring = tableItem->text();

                            if (inPositive) {
                                val = qAbs(val);
                            }

                            int color_h;
                            int color_s;
                            int color_v;
                            initialColor.getHsv(&color_h, &color_s, &color_v);
                            color_s = color_s - color_s * 155 / 255 * (nbColumns - 1 - j) / nbColumns;
                            color_v = color_v - color_v * 155 / 255 * (nbColumns - 1 - j) / nbColumns;
                            if (j >= nbRealColumns) {
                                // Yellow
                                color_h = 60;
                                color_s = 255;
                                color_v = 255;
                            }

                            QColor color;
                            if (posx == crow && j == ccolumn) {
                                color = QApplication::palette().color(QPalette::Highlight);
                            } else {
                                color = QColor::fromHsv(color_h, color_s, color_v);
                            }

                            color.setAlpha(ALPHA);
                            QBrush brush(color);

                            QGraphicsItem *graphItem = nullptr;
                            if (mode == STACK) {
                                // STACK
                                if (val >= 0) {
                                    graphItem = m_scene->addRect(widthItem * posx, -yPlus - qAbs(val), widthItem, qAbs(val), outlinePen, brush);
                                    yPlus += qAbs(val);
                                    if (yPlus > ymax) {
                                        ymax = yPlus;
                                    }
                                } else {
                                    graphItem = m_scene->addRect(widthItem * posx, -yMoins, widthItem, -val, outlinePen, brush);
                                    yMoins += val;
                                    if (yMoins < ymin) {
                                        ymin = yMoins;
                                    }
                                }
                                if (graphItem != nullptr) {
                                    graphItem->setZValue(5 + nbColumns - j);
                                }
                            }
                            if (mode == STACKAREA) {
                                // STACKAREA
                                // Empty cells are ignored
                                if (j == 1) {
                                    x0 = widthItem * (j - 1) * (nbRealRows + 1);
                                }
                                if (j == nbColumns - m_nbVirtualColumns - 1) {
                                    x1 = widthItem * (j - 1) * (nbRealRows + 1);
                                }

                                if (!valstring.isEmpty()) {
                                    if (jprevious == -1) {
                                        jprevious = j;
                                    }
                                    QTableWidgetItem *tableItem2 = ui.kTable->item(xx, jprevious);
                                    if (tableItem2 != nullptr) {
                                        QString val2string = tableItem->text();
                                        if (!val2string.isEmpty()) {
                                            double val2 = tableItem2->data(DATA_VALUE).toDouble();
                                            if (j == jprevious) {
                                                val2 = 0;
                                            }
                                            if (inPositive) {
                                                val2 = qAbs(val2);
                                            }

                                            if (val > EPSILON || (qAbs(val) <= EPSILON && val2 >= EPSILON)) {
                                                double xp = widthItem * (jprevious - 1) * (nbRealRows + 1) - (jprevious == j ? widthItem / 20.0 : 0);
                                                double xn = widthItem * (j - 1) * (nbRealRows + 1);
                                                double yp = (previousStackedPlus.contains(jprevious) ? previousStackedPlus[jprevious] : -val2);
                                                double yn = previousStackedPlus[j] - val;

                                                QPolygonF polygon;
                                                polygon << QPointF(xp, yp + val2) << QPointF(xp, yp) << QPointF(xn, yn) << QPointF(xn, previousStackedPlus[j]);
                                                graphItem = m_scene->addPolygon(polygon, outlinePen, brush);
                                                previousStackedPlus[j] = yn;
                                                if (-yn > ymax) {
                                                    ymax = -yn;
                                                }
                                            } else {
                                                double xp = widthItem * (jprevious - 1) * (nbRealRows + 1) - (jprevious == j ? widthItem / 20.0 : 0);
                                                double xn = widthItem * (j - 1) * (nbRealRows + 1);
                                                double yp = (previousStackedMoins.contains(jprevious) ? previousStackedMoins[jprevious] : -val2);
                                                double yn = previousStackedMoins[j] - val;

                                                QPolygonF polygon;
                                                polygon << QPointF(xp, yp + val2) << QPointF(xp, yp) << QPointF(xn, yn) << QPointF(xn, previousStackedMoins[j]);
                                                graphItem = m_scene->addPolygon(polygon, outlinePen, brush);
                                                previousStackedMoins[j] = yn;
                                                if (-yn < ymin) {
                                                    ymin = -yn;
                                                }
                                            }
                                            jprevious = j;
                                        }
                                    }
                                }
                            }
                            if (mode == STACKCOLUMNS) {
                                // STACKCOLUMNS
                                if (val >= 0) {
                                    graphItem = m_scene->addRect(widthItem * (j - 1) * (nbRealRows + 1),
                                                                 -previousStackedPlus[j] - qAbs(val),
                                                                 widthItem * (nbRealRows + 1),
                                                                 qAbs(val),
                                                                 outlinePen,
                                                                 brush);
                                    previousStackedPlus[j] += qAbs(val);
                                    if (previousStackedPlus[j] > ymax) {
                                        ymax = previousStackedPlus[j];
                                    }
                                } else {
                                    graphItem = m_scene->addRect(widthItem * (j - 1) * (nbRealRows + 1),
                                                                 -previousStackedMoins[j],
                                                                 widthItem * (nbRealRows + 1),
                                                                 -val,
                                                                 outlinePen,
                                                                 brush);
                                    previousStackedMoins[j] += val;
                                    if (previousStackedMoins[j] < ymin) {
                                        ymin = previousStackedMoins[j];
                                    }
                                }
                                if (graphItem != nullptr) {
                                    graphItem->setZValue(5 + nbRows - posx);
                                }
                            } else if (mode == HISTOGRAM) {
                                // HISTOGRAM
                                if (j == 1) {
                                    x0 = widthItem * ((j - 1) * (nbRealRows + 1) + posx) + widthItem;
                                }
                                if (j == nbColumns - m_nbVirtualColumns - 1) {
                                    x1 = widthItem * ((j - 1) * (nbRealRows + 1) + posx) + widthItem;
                                }
                                graphItem = m_scene->addRect(widthItem * ((j - 1) * (nbRealRows + 1) + posx) + widthItem / 2.0,
                                                             (val < 0 ? -yorigin : -val),
                                                             widthItem,
                                                             (val < 0 ? yorigin - val : -yorigin + val),
                                                             outlinePen,
                                                             brush);
                                if (val > ymax) {
                                    ymax = val;
                                }
                                if (val < ymin) {
                                    ymin = val;
                                }
                            } else if (mode == POINT) {
                                // POINTS
                                double xmin = widthItem * (j - 1) * (nbRealRows + 1) - radius;
                                if (j == 1) {
                                    x0 = xmin + radius;
                                }
                                if (j == nbColumns - m_nbVirtualColumns - 1) {
                                    x1 = xmin + radius;
                                }
                                graphItem = drawPoint(xmin, -val, radius, posx, brush);
                                if (val > ymax) {
                                    ymax = val;
                                }
                                if (val < ymin) {
                                    ymin = val;
                                }
                            } else if (mode == LINE) {
                                // LINES
                                // Empty cells are ignored
                                if (j == 1) {
                                    x0 = widthItem * (j - 1) * (nbRealRows + 1);
                                }
                                if (j == nbColumns - m_nbVirtualColumns - 1) {
                                    x1 = widthItem * (j - 1) * (nbRealRows + 1);
                                }

                                if (!valstring.isEmpty()) {
                                    if (jprevious == -1) {
                                        jprevious = j;
                                    }

                                    // Create pen
                                    color.setAlpha(255);
                                    QPen pen = QPen(color);
                                    pen.setWidthF(radius >= (1 << 15) ? 0 : radius);
                                    pen.setCapStyle(Qt::RoundCap);
                                    pen.setJoinStyle(Qt::RoundJoin);

                                    QTableWidgetItem *tableItem2 = ui.kTable->item(xx, jprevious);
                                    if (tableItem2 != nullptr) {
                                        QString val2string = tableItem->text();
                                        if (!val2string.isEmpty()) {
                                            double val2 = tableItem2->data(DATA_VALUE).toDouble();
                                            if (inPositive) {
                                                val2 = qAbs(val2);
                                            }

                                            QGraphicsLineItem *line =
                                                m_scene->addLine(widthItem * (jprevious - 1) * (nbRealRows + 1) - (jprevious == j ? widthItem / 20.0 : 0),
                                                                 -val2,
                                                                 widthItem * (j - 1) * (nbRealRows + 1),
                                                                 -val,
                                                                 pen);
                                            line->setZValue(10);
                                            if (isShadowVisible()) {
                                                auto line_shadow = new QGraphicsDropShadowEffect();
                                                line_shadow->setOffset(3);
                                                line->setGraphicsEffect(line_shadow);
                                            }
                                            graphItem = drawPoint(widthItem * (j - 1) * (nbRealRows + 1) - radius * 0.7, -val, radius * 0.7, posx % 5, brush);
                                            if (isShadowVisible()) {
                                                auto line_shadow = new QGraphicsDropShadowEffect();
                                                line_shadow->setOffset(3);
                                                line->setGraphicsEffect(line_shadow);
                                            }
                                            graphItem->setZValue(20);
                                            if (val > ymax) {
                                                ymax = val;
                                            }
                                            if (val < ymin) {
                                                ymin = val;
                                            }
                                            jprevious = j;
                                        }
                                    }
                                }
                            } else if (mode == BUBBLE) {
                                // BUBBLE
                                ymin = 0;
                                ymax = ystep * nbRealRows;

                                radius = sqrt(qAbs(val) / maxLimit) * qMin(ystep, jstep * xstep);
                                double xmin = widthItem * (j - 1) * (nbRealRows + 1) - radius;

                                graphItem = m_scene->addEllipse(xmin, -ystep * posx - radius, 2 * radius, 2 * radius, outlinePen, brush);
                                if (graphItem != nullptr) {
                                    graphItem->setZValue(5 + 5 * (maxLimit - qAbs(val)) / maxLimit);
                                }
                            } else if (mode == PIE || mode == CONCENTRICPIE) {
                                // PIE
                                val = qAbs(val);

                                // Compute absolute sum of the column
                                double previousSum = 0;
                                for (int x2 = 0; x2 < nbRows; ++x2) {
                                    if (!m_sumRows[x2 + 1]) {
                                        QTableWidgetItem *tabItem = ui.kTable->item(x2, j);
                                        if (tabItem != nullptr) {
                                            double absVal = qAbs(tabItem->data(DATA_VALUE).toDouble());
                                            if (x2 < xx) {
                                                previousSum += absVal;
                                            }
                                        }
                                    }
                                }

                                if (maxLimit != 0.0) {
                                    int nbvals = xname.split(OBJECTSEPARATOR).count();
                                    double step = 0;
                                    double p = 0;
                                    if (mode == CONCENTRICPIE) {
                                        step = 100.0 / static_cast<double>(nbLevel + 1);
                                        p = step * (nbLevel + 1 - nbvals);
                                    }

                                    QPainterPath path;
                                    path.moveTo(BOX_SIZE * ((j - 1) % nbColInMode2) + BOX_SIZE / 2.0,
                                                BOX_SIZE * floor((j - 1) / nbColInMode2) + BOX_SIZE / 2.0);
                                    path.arcTo(BOX_SIZE * ((j - 1) % nbColInMode2) + BOX_MARGIN + p / 2.0,
                                               BOX_SIZE * floor((j - 1) / nbColInMode2) + BOX_MARGIN + p / 2.0,
                                               BOX_SIZE - 2 * BOX_MARGIN - p,
                                               BOX_SIZE - 2 * BOX_MARGIN - p,
                                               90.0 - 360.0 * previousSum / maxLimit,
                                               -360.0 * val / maxLimit);
                                    path.closeSubpath();
                                    if (mode == CONCENTRICPIE && nbvals <= nbLevel + 1 && nbvals > 1) {
                                        p = step * (nbLevel + 1 - nbvals + 1);
                                        QPainterPath path2;
                                        path2.addEllipse(BOX_SIZE * ((j - 1) % nbColInMode2) + BOX_MARGIN + p / 2.0,
                                                         BOX_SIZE * floor((j - 1) / nbColInMode2) + BOX_MARGIN + p / 2.0,
                                                         BOX_SIZE - 2 * BOX_MARGIN - p,
                                                         BOX_SIZE - 2 * BOX_MARGIN - p);
                                        path -= path2;
                                    }
                                    graphItem = m_scene->addPath(path, outlinePen, brush);
                                }
                            } else if (mode == TREEMAP) {
                                // TREEMAP
                                QString id = SKGServices::intToString(xx) % QStringLiteral("-") % SKGServices::intToString(j);
                                SKGTreeMap tm = treemap[id];
                                graphItem = m_scene->addRect(tm.getX(), tm.getY(), tm.getW(), tm.getH(), outlinePen, brush);
                            }

                            // Compute pareto points
                            if (mode == POINT || mode == LINE) {
                                paretoPoints[widthItem * (j - 1) * (nbRealRows + 1)] = val;
                            } else if (mode == HISTOGRAM) {
                                paretoPoints[widthItem * ((j - 1) * (nbRealRows + 1) + posx) + widthItem] = val;
                            }

                            if (graphItem != nullptr) {
                                if (graphItem->zValue() == 0) {
                                    graphItem->setZValue(5);
                                }
                                graphItem->setToolTip(tableItem->toolTip());
                                bool isSelect = (isSelectable() && ((tableItem->flags() & Qt::ItemIsEnabled) != 0u));
                                graphItem->setFlag(QGraphicsItem::ItemIsSelectable, isSelect);
                                if (isSelect) {
                                    graphItem->setCursor(Qt::PointingHandCursor);
                                }
                                graphItem->setData(1, xx);
                                graphItem->setData(2, j);
                                graphItem->setData(DATA_COLOR_H, color_h);
                                graphItem->setData(DATA_COLOR_S, color_s);
                                graphItem->setData(DATA_COLOR_V, color_v);
                                graphItem->setData(DATA_Z_VALUE, graphItem->zValue());

                                m_mapItemGraphic[tableItem] = graphItem;
                            }
                        }
                    } else {
                        SKGTRACE << "WARNING: cell " << posx << "," << j << " null" << Qt::endl;
                    }
                }

                ++posx;
            }
        }

        // Draw axis
        double scaleText = 0.0;
        auto nbRowInMode2 = static_cast<int>(((nbColumns - 1) / nbColInMode2));
        if (nbRealRows != 0) {
            if (mode == TREEMAP) {
                // TREEMAP
            } else if (mode == PIE || mode == CONCENTRICPIE) {
                // PIE
                if (nbRowInMode2 * nbColInMode2 < nbColumns - 1) {
                    ++nbRowInMode2;
                }
                for (int i = 0; i <= nbColInMode2; ++i) {
                    QGraphicsLineItem *item = m_scene->addLine(BOX_SIZE * i, 0, BOX_SIZE * i, BOX_SIZE * nbRowInMode2);
                    item->setPen(axisPen);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                }
                for (int i = 0; i <= nbRowInMode2; ++i) {
                    QGraphicsLineItem *item = m_scene->addLine(0, BOX_SIZE * i, BOX_SIZE * nbColInMode2, BOX_SIZE * i);
                    item->setPen(axisPen);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                }

                for (int j = 1; j < nbColumns; ++j) {
                    // Get column name
                    QString yname = ui.kTable->horizontalHeaderItem(j)->text();

                    QGraphicsTextItem *textItem = m_scene->addText(yname);
                    textItem->setDefaultTextColor(m_textColor);
                    textItem->setPos(BOX_SIZE * ((j - 1) % nbColInMode2) + 2, BOX_SIZE * floor((j - 1) / nbColInMode2) + 2);
                    textItem->setScale(.5);
                    textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    textItem->setZValue(20);
                }
            } else {
                // STACK & HISTOGRAMM
                QGraphicsLineItem *item;

                // Compute scale text
                if (mode != BUBBLE) {
                    QGraphicsTextItem *t = m_scene->addText(
                        SKGServices::toCurrencyString(-qMax(qAbs(ymax), qAbs(ymin)), m_primaryUnit.Symbol, m_decimalsVisible ? m_primaryUnit.NbDecimal : 0));
                    QRectF bRect = t->boundingRect();
                    scaleText = 0.8 * qMin(marginLeft / bRect.width(), qMin(marginLeft / bRect.height(), ystep / bRect.height()));
                    m_scene->removeItem(t);
                } else {
                    maxLimit = ystep * nbRealRows;
                }

                if (ystep > 0) {
                    // Draw
                    int i = 1;
                    for (double posy = yorigin + ystep; posy <= maxLimit; posy = posy + ystep) {
                        item = m_scene->addLine(0, -posy, maxX, -posy);
                        item->setPen(gridPen);
                        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                        item->setZValue(1);
                        QGraphicsTextItem *textItem;
                        if (mode == BUBBLE) {
                            auto *cel = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(i, 0));
                            ++i;
                            if (cel == nullptr) {
                                cel = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(i, 0));
                                ++i;
                            }
                            textItem = m_scene->addText(cel != nullptr ? cel->text() : QString());
                        } else {
                            textItem =
                                m_scene->addText(SKGServices::toCurrencyString(posy, m_primaryUnit.Symbol, m_decimalsVisible ? m_primaryUnit.NbDecimal : 0));
                        }

                        QRectF textRect = textItem->boundingRect();
                        if (scaleText == 0) {
                            scaleText = 0.8 * qMin(marginLeft / textRect.width(), qMin(marginLeft / textRect.height(), ystep / textRect.height()));
                        }
                        textItem->setDefaultTextColor(m_textColor);
                        textItem->setScale(scaleText);
                        textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                        textItem->setZValue(20);
                        double delta = scaleText * textRect.height() / 2.0;

                        textItem->setPos(-marginLeft, -posy - delta);
                    }

                    i = 0;
                    for (double posy = yorigin; (posy == yorigin) || posy >= minLimit; posy = posy - ystep) {
                        item = m_scene->addLine(0, -posy, maxX, -posy);
                        item->setPen(gridPen);
                        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                        item->setZValue(1);

                        QGraphicsTextItem *textItem;
                        if (mode == BUBBLE) {
                            auto *cel = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(i, 0));
                            textItem = m_scene->addText(cel != nullptr ? cel->text() : QString());
                        } else {
                            textItem =
                                m_scene->addText(SKGServices::toCurrencyString(posy, m_primaryUnit.Symbol, m_decimalsVisible ? m_primaryUnit.NbDecimal : 0));
                        }

                        QRectF textRect = textItem->boundingRect();
                        if (scaleText == 0) {
                            scaleText = 0.8 * qMin(marginLeft / textRect.width(), qMin(marginLeft / textRect.height(), ystep / textRect.height()));
                        }
                        textItem->setDefaultTextColor(m_textColor);
                        textItem->setScale(scaleText);
                        textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                        textItem->setZValue(20);
                        double delta = scaleText * textRect.height() / 2.0;

                        textItem->setPos(-marginLeft, -posy - delta);
                    }
                }

                if (mode == HISTOGRAM || mode == POINT || mode == LINE || mode == BUBBLE || mode == STACKAREA || mode == STACKCOLUMNS) {
                    // Line y
                    for (int j = 1; j < nbColumns; j += jstep) {
                        QString yname = ui.kTable->horizontalHeaderItem(j)->text();
                        double posx2 = xstep + (j - 2) * xstep;

                        QGraphicsTextItem *textItem = m_scene->addText(yname);

                        QRectF textRect = textItem->boundingRect();
                        if (scaleText == 0) {
                            scaleText = 0.8 * qMin(marginLeft / textRect.width(), qMin(marginLeft / textRect.height(), ystep / textRect.height()));
                        }
                        textItem->setDefaultTextColor(m_textColor);
                        textItem->setScale(scaleText);
                        textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                        textItem->setZValue(20);
                        double delta = scaleText * textRect.height() / 2.0;
                        textItem->setRotation(90);
                        textItem->moveBy(textRect.height() * scaleText, 0);

                        textItem->setPos(posx2 + delta, -yorigin);

                        QGraphicsLineItem *graphicItem = m_scene->addLine(posx2, qMax(-yorigin, -minLimit), posx2, qMin(-yorigin, -maxLimit));
                        graphicItem->setPen(gridPen);
                        graphicItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    }
                }

                // Axis x
                if (yorigin == 0.0) {
                    item = m_scene->addLine(0, -yorigin, maxX, -yorigin, axisPen);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    item->setZValue(2);
                }

                // Rect
                {
                    QGraphicsRectItem *graphicItem =
                        m_scene->addRect(0, qMax(-yorigin, -minLimit), maxX, qMin(-yorigin, -maxLimit) - (qMax(-yorigin, -minLimit)), axisPen);
                    graphicItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    graphicItem->setZValue(2);
                }
            }
        }

        // Draw Average
        bool lineCondition = (mode == HISTOGRAM || mode == POINT || mode == LINE) && (scaleText != 0.0) && nbRowsDrawed == 1;
        int averageCol = getAverageColumnIndex();
        if (m_averageVisible && lineCondition && averageCol != -1) {
            QTableWidgetItem *tableItem = ui.kTable->item(0, averageCol);
            if (tableItem != nullptr) {
                double posy = tableItem->data(DATA_VALUE).toDouble();
                if (inPositive) {
                    posy = qAbs(posy);
                }

                QGraphicsLineItem *item = m_scene->addLine(0, -posy, maxX, -posy);
                dotPen.setColor(m_averageColor);
                item->setPen(dotPen);
                item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                item->setZValue(1);
                item->setToolTip(tableItem->toolTip());

                QGraphicsTextItem *textItem =
                    m_scene->addText(SKGServices::toCurrencyString(posy, m_primaryUnit.Symbol, m_decimalsVisible ? m_primaryUnit.NbDecimal : 0));
                QRectF textRect = textItem->boundingRect();
                double delta = scaleText * textRect.height() / 2.0;
                textItem->setPos(maxX, -posy - delta);
                textItem->setDefaultTextColor(m_averageColor);
                textItem->setScale(scaleText);
                textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                textItem->setZValue(20);
                textItem->setToolTip(tableItem->toolTip());
            }
        }

        // Draw Min & Max limits
        int minCol = getMinColumnIndex();
        if (lineCondition && minCol != -1) {
            // Min
            {
                QTableWidgetItem *tableItem = ui.kTable->item(0, minCol);
                if (tableItem != nullptr) {
                    double posy = tableItem->data(DATA_VALUE).toDouble();
                    if (inPositive) {
                        posy = qAbs(posy);
                    }

                    QGraphicsLineItem *item = m_scene->addLine(0, -posy, maxX, -posy);
                    dotPen.setColor(m_minColor);
                    item->setPen(dotPen);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    item->setZValue(1);
                    item->setToolTip(tableItem->toolTip());

                    QGraphicsTextItem *textItem =
                        m_scene->addText(SKGServices::toCurrencyString(posy, m_primaryUnit.Symbol, m_decimalsVisible ? m_primaryUnit.NbDecimal : 0));
                    QRectF textRect = textItem->boundingRect();
                    double delta = scaleText * textRect.height() / 2.0;
                    textItem->setPos(maxX, -posy - delta);
                    textItem->setDefaultTextColor(m_minColor);
                    textItem->setScale(scaleText);
                    textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    textItem->setZValue(20);
                    textItem->setToolTip(tableItem->toolTip());
                }
            }

            // Max
            {
                QTableWidgetItem *tableItem = ui.kTable->item(0, minCol + 1);
                if (tableItem != nullptr) {
                    double posy = tableItem->data(DATA_VALUE).toDouble();
                    if (inPositive) {
                        posy = qAbs(posy);
                    }

                    QGraphicsLineItem *item = m_scene->addLine(0, -posy, maxX, -posy);
                    dotPen.setColor(m_maxColor);
                    item->setPen(dotPen);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    item->setZValue(1);
                    item->setToolTip(tableItem->toolTip());

                    QGraphicsTextItem *textItem =
                        m_scene->addText(SKGServices::toCurrencyString(posy, m_primaryUnit.Symbol, m_decimalsVisible ? m_primaryUnit.NbDecimal : 0));
                    QRectF textRect = textItem->boundingRect();
                    double delta = scaleText * textRect.height() / 2.0;
                    textItem->setPos(maxX, -posy - delta);
                    textItem->setDefaultTextColor(m_maxColor);
                    textItem->setScale(scaleText);
                    textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    textItem->setZValue(20);
                    textItem->setToolTip(tableItem->toolTip());
                }
            }
        }

        // Draw Linear Regression
        if (m_linearRegressionVisible && lineCondition && m_indexLinearRegression != -1) {
            QTableWidgetItem *tableItem = ui.kTable->item(0, m_indexLinearRegression);
            if (tableItem != nullptr) {
                QString f = tableItem->text();

                QJSEngine myEngine;
                QString f0 = f;
                f0 = f0.remove(QStringLiteral("y="));
                f0 = f0.replace('x', '1');
                f0 = f0.replace(',', u'.'); // Replace comma by a point in case of typo
                if (inPositive) {
                    f0 = "Math.abs(" % f0 % u')';
                }
                double y0 = myEngine.evaluate(f0).toNumber();

                QString f1 = f;
                f1 = f1.remove(QStringLiteral("y="));
                f1 = f1.replace('x', SKGServices::intToString(nbRealColumns - 1));
                f1 = f1.replace(',', u'.'); // Replace comma by a point in case of typo
                if (inPositive) {
                    f1 = "Math.abs(" % f1 % u')';
                }
                double y1 = myEngine.evaluate(f1).toNumber();

                QGraphicsLineItem *item = m_scene->addLine(x0, -y0, x1, -y1);
                dotPen.setColor(m_tendencyColor);
                item->setPen(dotPen);
                item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                item->setZValue(1);
                item->setToolTip(f);
            }
        }

        // Draw pareto
        if (lineCondition && m_paretoVisible && !paretoPoints.isEmpty()) {
            // Compute the sum
            double sum = 0.0;
            QList<double> list = paretoPoints.keys();
            for (auto xp1 : std::as_const(list)) {
                sum += paretoPoints[xp1];
            }

            // Draw the second axis
            for (int i = 0; i <= 100; i = i + 10) {
                double posy = (maxLimit - minLimit) * static_cast<double>(i) / 100.0 + minLimit;

                QGraphicsTextItem *textItem = m_scene->addText(SKGServices::intToString(i) % "%");
                QRectF textRect = textItem->boundingRect();
                textItem->setDefaultTextColor(m_paretoColor);
                textItem->setScale(scaleText);
                textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);
                textItem->setZValue(19);

                double delta = scaleText * textRect.height() / 2.0;
                textItem->setPos(maxX, -posy - delta);
            }

            // Draw the curve
            double x00 = -1;
            double y00 = -1;
            double csum = 0.0;
            for (auto xp2 : std::as_const(list)) {
                csum += paretoPoints[xp2];
                if (x00 != -1) {
                    QGraphicsLineItem *item =
                        m_scene->addLine(x00, -((maxLimit - minLimit) * y00 / sum + minLimit), xp2, -((maxLimit - minLimit) * csum / sum + minLimit));
                    dotPen.setColor(m_paretoColor);
                    item->setPen(dotPen);
                    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                    item->setZValue(1);
                }
                x00 = xp2;
                y00 = csum;
            }
        }
        // Draw legend
        if (m_legendVisible) {
            QPointF legendPosition;
            double maxY;
            if (mode == TREEMAP) {
                scaleText = 0.2;
                margin = BOX_SIZE / 10;
                legendPosition = QPointF(BOX_SIZE + margin, 0);
                maxY = BOX_SIZE * nbRowInMode2;
            } else if (mode == PIE || mode == CONCENTRICPIE) {
                if (nbRowInMode2 * nbColInMode2 < nbColumns - 1) {
                    ++nbRowInMode2;
                }

                scaleText = 0.2;
                margin = BOX_SIZE / 10;
                legendPosition = QPointF(BOX_SIZE * nbColInMode2 + margin, 0);
                maxY = BOX_SIZE * nbRowInMode2;
            } else {
                legendPosition = QPointF(maxX + margin, qMin(-yorigin, -maxLimit) - margin / 6);
                maxY = qMax(-yorigin, -minLimit);
            }
            addLegend(legendPosition, margin / 4, scaleText, maxY);
        }
    }
    {
        SKGTRACEINFUNC(10)
        m_scene->setSceneRect(QRectF());
        ui.graphicView->setScene(m_scene);
        ui.graphicView->show();
        if (m_tableVisible) {
            ui.kTable->show();
        }
        ui.graphicView->initializeZoom();

        // Add selection event on scene
        connect(m_scene, &SKGGraphicsScene::selectionChanged, this, &SKGTableWithGraph::onSelectionChangedInGraph, Qt::QueuedConnection);
        connect(m_scene, &SKGGraphicsScene::doubleClicked, this, &SKGTableWithGraph::onDoubleClickGraph, Qt::QueuedConnection);
    }
    QApplication::restoreOverrideCursor();
}

int SKGTableWithGraph::getNbColumns(bool iWithComputed) const
{
    int nbColumns = ui.kTable->columnCount();
    if (!iWithComputed) {
        if (m_indexMin != -1) {
            nbColumns -= 2;
        }
        if (m_indexAverage != -1) {
            --nbColumns;
        }
        if (m_indexSum != -1) {
            --nbColumns;
        }
        if (m_indexLinearRegression != -1) {
            --nbColumns;
        }
    }
    return nbColumns;
}

int SKGTableWithGraph::getAverageColumnIndex() const
{
    return m_indexAverage;
}

int SKGTableWithGraph::getMinColumnIndex() const
{
    return m_indexMin;
}

double SKGTableWithGraph::computeStepSize(double iRange, double iTargetSteps)
{
    // Calculate an initial guess at step size
    double tempStep = iRange / iTargetSteps;
    // Get the magnitude of the step size
    double mag = floor(log10(tempStep));
    double magPow = pow(static_cast<double>(10.0), mag);
    // Calculate most significant digit of the new step size
    double magMsd = static_cast<int>(tempStep / magPow + .5);
    // promote the MSD to either 1, 2, or 5
    if (magMsd > 5.0) {
        magMsd = 10.0;
    } else if (magMsd > 2.0) {
        magMsd = 5.0;
    } else if (magMsd > 1.0) {
        magMsd = 2.0;
    }
    return magMsd * magPow;
}

void SKGTableWithGraph::addLegend(const QPointF iPosition, double iSize, double iScaleText, double iMaxY)
{
    SKGTRACEINFUNC(10)

    QPen outlinePen(m_outlineColor);
    outlinePen.setWidthF((iScaleText * 4.0 / 500.0) >= (1 << 15) ? 0 : (iScaleText * 4.0 / 500.0));

    if (m_scene != nullptr) {
        GraphType mode = getGraphType();
        int nbRows = ui.kTable->rowCount();
        int nbColumns = getNbColumns(false);
        if (nbColumns > 1) {
            double margin = 1.2;
            double currentYPos = iPosition.y();
            double currentXPos = iPosition.x();
            double currentXMaxSize = 0;
            for (int i = 0; i < nbRows; ++i) {
                auto *btn = qobject_cast<SKGColorButton *>(ui.kTable->cellWidget(i, 0));
                if (btn != nullptr) {
                    // Get title
                    QString title = btn->text();

                    // Build brush
                    QColor color1;
                    QTableWidgetItem *it = ui.kTable->item(i, 1);
                    if (it != nullptr) {
                        QGraphicsItem *graphicItem = m_mapItemGraphic.value(it);
                        if (graphicItem != nullptr) {
                            // Draw box
                            color1 = QColor::fromHsv(graphicItem->data(DATA_COLOR_H).toInt(),
                                                     graphicItem->data(DATA_COLOR_S).toInt(),
                                                     graphicItem->data(DATA_COLOR_V).toInt());
                            color1.setAlpha(ALPHA);
                        }
                    }
                    QColor color2;
                    it = ui.kTable->item(i, nbColumns - 1 - m_nbVirtualColumns);
                    if (it != nullptr) {
                        QGraphicsItem *graphicItem = m_mapItemGraphic.value(it);
                        if (graphicItem != nullptr) {
                            // Draw box
                            color2 = QColor::fromHsv(graphicItem->data(DATA_COLOR_H).toInt(),
                                                     graphicItem->data(DATA_COLOR_S).toInt(),
                                                     graphicItem->data(DATA_COLOR_V).toInt());
                            color2.setAlpha(ALPHA);
                        }
                    }

                    QLinearGradient grandient(currentXPos, currentYPos, currentXPos + 2.0 * iSize, currentYPos);
                    grandient.setColorAt(0, color1);
                    grandient.setColorAt(1, color2);

                    // Draw legend item
                    QGraphicsItem *item = nullptr;
                    if (mode == POINT || mode == LINE) {
                        item = drawPoint(currentXPos, currentYPos + iSize / 2.0, iSize / 2.0, mode == POINT ? i : (i % 5), QBrush(grandient));
                    } else if (mode == BUBBLE) {
                        item = m_scene->addEllipse(currentXPos, currentYPos, iSize, iSize, outlinePen, QBrush(grandient));
                    } else if (mode == PIE || mode == CONCENTRICPIE) {
                        QPainterPath path;
                        path.moveTo(currentXPos + iSize / 2.0, currentYPos + iSize / 2.0);
                        path.arcTo(currentXPos, currentYPos, iSize, iSize, 45, 270);
                        path.closeSubpath();
                        if (mode == CONCENTRICPIE) {
                            QPainterPath path2;
                            double p = iSize / 3.0;
                            path2.addEllipse(currentXPos + p, currentYPos + p, iSize - 2.0 * p, iSize - 2.0 * p);
                            path -= path2;
                        }
                        item = m_scene->addPath(path, outlinePen, QBrush(grandient));
                    } else {
                        item = m_scene->addRect(currentXPos, currentYPos, iSize, iSize, outlinePen, QBrush(grandient));
                    }
                    if (item != nullptr) {
                        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
                        item->setToolTip(title);

                        // Set shadow
                        if (isShadowVisible()) {
                            auto ellipse_shadow = new QGraphicsDropShadowEffect();
                            ellipse_shadow->setOffset(3);
                            item->setGraphicsEffect(ellipse_shadow);
                        }
                    }

                    // Draw text
                    QGraphicsTextItem *textItem = m_scene->addText(title);
                    textItem->setDefaultTextColor(m_textColor);
                    textItem->setScale(iScaleText);
                    textItem->setPos(currentXPos + margin * iSize, currentYPos + iSize / 2.0 - textItem->boundingRect().height() * iScaleText / 2.0);
                    textItem->setFlag(QGraphicsItem::ItemIsSelectable, false);

                    // Compute next position
                    currentYPos += margin * iSize;
                    QRectF textRect = textItem->boundingRect();
                    currentXMaxSize = qMax(currentXMaxSize, static_cast<double>(iScaleText * textRect.width()));
                    if (currentYPos > iMaxY) {
                        currentYPos = iPosition.y();
                        currentXPos += currentXMaxSize + 2 * margin * iSize;
                        currentXMaxSize = 0;
                    }
                }
            }
        }
    }
}

void SKGTableWithGraph::addArrow(const QPointF iPeak, double iSize, double iArrowAngle, double iDegree)
{
    if (m_scene != nullptr) {
        QPolygonF pol;
        double radian = 3.14 * iArrowAngle / 360.0;
        pol << QPointF(0, 0) << QPointF(iSize * cos(radian), iSize * sin(radian)) << QPointF(iSize * cos(radian), -iSize * sin(radian)) << QPointF(0, 0);
        QGraphicsPolygonItem *item = m_scene->addPolygon(pol, QPen(m_axisColor, iSize / 20.0), QBrush(m_axisColor));
        item->setRotation(iDegree);
        item->moveBy(iPeak.x(), iPeak.y());
        item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        item->setZValue(2);
    }
}

QGraphicsItem *SKGTableWithGraph::drawPoint(qreal iX, qreal iY, qreal iRadius, int iMode, const QBrush &iBrush)
{
    QGraphicsItem *graphItem = nullptr;
    int nbMode = 13;
    if (m_scene != nullptr) {
        QPen outlinePen(m_outlineColor);
        outlinePen.setWidthF((iRadius / 10) >= (1 << 15) ? 0.0 : iRadius / 10);

        QPen pen;
        if ((iMode % nbMode) <= 4) {
            pen = QPen(iBrush.color());
            const QGradient *grad = iBrush.gradient();
            if (grad != nullptr) {
                const auto &stops = grad->stops();
                const auto &stop = stops.last();
                pen = QPen(stop.second);
            }
            pen.setWidthF((iRadius / 10) >= (1 << 15) ? 00.0 : iRadius / 10);
        }

        switch (iMode % nbMode) {
        case 0: {
            graphItem = m_scene->addEllipse(iX, iY - iRadius, 2 * iRadius, 2 * iRadius, pen, m_WhiteColor);
            break;
        }
        case 1: {
            graphItem = m_scene->addRect(iX, iY - iRadius, 2 * iRadius, 2 * iRadius, pen, m_WhiteColor);
            break;
        }
        case 2: {
            QPolygonF polygon;
            polygon << QPointF(iX + iRadius, iY + iRadius) << QPointF(iX + 2 * iRadius, iY - iRadius) << QPointF(iX, iY - iRadius)
                    << QPointF(iX + iRadius, iY + iRadius);
            graphItem = m_scene->addPolygon(polygon, pen, m_WhiteColor);
            break;
        }
        case 3: {
            QPolygonF polygon;
            polygon << QPointF(iX, iY) << QPointF(iX + iRadius, iY + iRadius) << QPointF(iX + 2 * iRadius, iY) << QPointF(iX + iRadius, iY - iRadius)
                    << QPointF(iX, iY);
            graphItem = m_scene->addPolygon(polygon, pen, m_WhiteColor);
            break;
        }
        case 4: {
            QPolygonF polygon;
            polygon << QPointF(iX + iRadius, iY - iRadius) << QPointF(iX + 2 * iRadius, iY + iRadius) << QPointF(iX, iY + iRadius)
                    << QPointF(iX + iRadius, iY - iRadius);
            graphItem = m_scene->addPolygon(polygon, pen, m_WhiteColor);
            break;
        }

        case 5: {
            graphItem = m_scene->addEllipse(iX, iY - iRadius, 2 * iRadius, 2 * iRadius, outlinePen, iBrush);
            break;
        }
        case 6: {
            graphItem = m_scene->addRect(iX, iY - iRadius, 2 * iRadius, 2 * iRadius, outlinePen, iBrush);
            break;
        }
        case 7: {
            QPolygonF polygon;
            polygon << QPointF(iX, iY - iRadius) << QPointF(iX + 2 * iRadius, iY + iRadius) << QPointF(iX + 2 * iRadius, iY - iRadius)
                    << QPointF(iX, iY + iRadius);
            graphItem = m_scene->addPolygon(polygon, outlinePen, iBrush);
            break;
        }
        case 8: {
            QPolygonF polygon;
            polygon << QPointF(iX + iRadius, iY + iRadius) << QPointF(iX + 2 * iRadius, iY - iRadius) << QPointF(iX, iY - iRadius)
                    << QPointF(iX + iRadius, iY + iRadius);
            graphItem = m_scene->addPolygon(polygon, outlinePen, iBrush);
            break;
        }
        case 9: {
            QPolygonF polygon;
            polygon << QPointF(iX, iY) << QPointF(iX + iRadius, iY + iRadius) << QPointF(iX + 2 * iRadius, iY) << QPointF(iX + iRadius, iY - iRadius)
                    << QPointF(iX, iY);
            graphItem = m_scene->addPolygon(polygon, outlinePen, iBrush);
            break;
        }
        case 10: {
            QPolygonF polygon;
            polygon << QPointF(iX, iY - iRadius) << QPointF(iX + 2 * iRadius, iY - iRadius) << QPointF(iX, iY + iRadius)
                    << QPointF(iX + 2 * iRadius, iY + iRadius);
            graphItem = m_scene->addPolygon(polygon, outlinePen, iBrush);
            break;
        }
        case 11: {
            QPolygonF polygon;
            polygon << QPointF(iX + iRadius, iY - iRadius) << QPointF(iX + 2 * iRadius, iY + iRadius) << QPointF(iX, iY + iRadius)
                    << QPointF(iX + iRadius, iY - iRadius);
            graphItem = m_scene->addPolygon(polygon, outlinePen, iBrush);
            break;
        }
        case 12:
        default: {
            QPainterPath path;
            path.addEllipse(iX, iY - iRadius, 2 * iRadius, 2 * iRadius);
            path.closeSubpath();

            QPainterPath path2;
            path2.addEllipse(iX + iRadius / 2.0, iY - iRadius / 2.0, iRadius, iRadius);
            path -= path2;
            graphItem = m_scene->addPath(path, outlinePen, iBrush);
            break;
        }
        }
    }
    if ((graphItem != nullptr) && (iMode % nbMode) <= 4) {
        graphItem->setData(DATA_MODE, 1);
    }
    return graphItem;
}

SKGError SKGTableWithGraph::exportInFile(const QString &iFileName)
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    QString lastCodecUsed = QTextCodec::codecForLocale()->name();

    QString extension = QFileInfo(iFileName).suffix().toUpper();
    if (extension == QStringLiteral("CSV")) {
        // Write file
        QSaveFile file(iFileName);
        if (!file.open(QIODevice::WriteOnly)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", iFileName));
        } else {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::encodingForName(lastCodecUsed.toLatin1().constData()).value_or(QStringConverter::Utf8));
            QStringList dump = SKGServices::tableToDump(getTable(), SKGServices::DUMP_CSV);
            int nbl = dump.count();
            for (int i = 0; i < nbl; ++i) {
                out << dump.at(i) << Qt::endl;
            }

            // Close file
            file.commit();
        }
    } else {
        // Write file
        QSaveFile file(iFileName);
        if (!file.open(QIODevice::WriteOnly)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", iFileName));
        } else {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::encodingForName(lastCodecUsed.toLatin1().constData()).value_or(QStringConverter::Utf8));
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

void SKGTableWithGraph::onExport()
{
    _SKGTRACEINFUNC(10)
    SKGError err;
    QString fileName = SKGMainPanel::getSaveFileName(QStringLiteral("kfiledialog:///IMPEXP"), QStringLiteral("text/csv text/plain"), this);
    if (!fileName.isEmpty()) {
        err = exportInFile(fileName);

        SKGMainPanel::displayErrorMessage(err);
        QDesktopServices::openUrl(QUrl(fileName));
    }
}

void SKGTableWithGraph::setGraphType(SKGTableWithGraph::GraphType iType)
{
    if (m_displayMode != nullptr) {
        auto newIndex = m_displayMode->findData(static_cast<int>(iType));
        if (m_displayMode->currentIndex() != newIndex) {
            m_displayMode->setCurrentIndex(newIndex);
            Q_EMIT modified();
        }
    }
}

SKGTableWithGraph::GraphType SKGTableWithGraph::getGraphType() const
{
    GraphType mode = static_cast<GraphType>(m_displayMode->itemData(m_displayMode->currentIndex()).toInt());
    return mode;
}

void SKGTableWithGraph::setSelectable(bool iSelectable)
{
    if (m_selectable != iSelectable) {
        m_selectable = iSelectable;
        Q_EMIT modified();
    }
}

bool SKGTableWithGraph::isSelectable() const
{
    return m_selectable;
}

void SKGTableWithGraph::setShadowVisible(bool iShadow)
{
    if (m_shadow != iShadow) {
        m_shadow = iShadow;
        Q_EMIT modified();
    }
}

bool SKGTableWithGraph::isShadowVisible() const
{
    return m_shadow;
}
