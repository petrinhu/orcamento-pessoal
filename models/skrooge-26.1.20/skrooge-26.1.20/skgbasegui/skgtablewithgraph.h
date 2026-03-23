/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTABLEWITHGRAPH_H
#define SKGTABLEWITHGRAPH_H
/** @file
 * A table with graph with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qwidget.h>

#include "skgbasegui_export.h"
#include "skgcombobox.h"
#include "skgservices.h"
#include "ui_skgtablewithgraph.h"

class SKGGraphicsScene;
class QMenu;
class QTimer;
class QWidgetAction;
class QGraphicsItem;

/**
 * This file is a table with graph with more features
 */
class SKGBASEGUI_EXPORT SKGTableWithGraph : public QWidget
{
    Q_OBJECT

public:
    /**
     * Graph type
     */
    enum GraphType {
        STACK,
        HISTOGRAM,
        PIE,
        CONCENTRICPIE,
        POINT,
        LINE,
        STACKAREA,
        BUBBLE,
        STACKCOLUMNS,
        TREEMAP
    };
    /**
     * Graph type
     */
    Q_ENUM(GraphType)

    /**
     * Additional information to display
     */
    enum DisplayAdditional {
        NONE = 0x0,
        SUM = 0x1,
        AVERAGE = 0x2,
        LIMITS = 0x4,
        ALL = 255
    };
    /**
     * Additional information to display
     */
    Q_ENUM(DisplayAdditional)

    /**
     * Additional information to display
     */
    Q_DECLARE_FLAGS(DisplayAdditionalFlag, DisplayAdditional)

    /**
     * Graph Type widget visibility
     */
    Q_PROPERTY(bool graphTypeSelectorVisible READ isGraphTypeSelectorVisible WRITE setGraphTypeSelectorVisible NOTIFY modified)
    /**
     * Items selectable or not
     */
    Q_PROPERTY(bool selectable READ isSelectable WRITE setSelectable NOTIFY modified)
    /**
     * Items with shadow or not
     */
    Q_PROPERTY(bool shadow READ isShadowVisible WRITE setShadowVisible NOTIFY modified)
    /**
     * Graph Type
     */
    Q_PROPERTY(GraphType graphType READ getGraphType WRITE setGraphType NOTIFY modified)
    /**
     * Default Constructor
     */
    SKGTableWithGraph();

    /**
     * Constructor
     * @param iParent the parent
     */
    explicit SKGTableWithGraph(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGTableWithGraph() override;

    /**
     * Get the current state
     * MUST BE OVERWRITTEN
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    QString getState();

    /**
     * Returns the table.
     * @return table
     */
    QTableWidget *table() const;

    /**
     * Returns the graph.
     * @return graph
     */
    SKGGraphicsView *graph() const;

    /**
     * Returns the text report.
     * @return text report
     */
    SKGWebView *textReport() const;

    /**
     * Get the mode for the additional display
     * @return the mode
     */
    SKGTableWithGraph::DisplayAdditionalFlag getAdditionalDisplayMode() const;

    /**
     * Get the table content
     * @return the table content
     */
    SKGStringListList getTable();

    /**
     * Get a pointer on the contextual menu of the table
     * @return contextual menu
     */
    QMenu *getTableContextualMenu() const;

    /**
     * Get a pointer on the contextual menu of the graph
     * @return contextual menu
     */
    QMenu *getGraphContextualMenu() const;

    /**
     * Get the visibility of the graph type selector zone
     * @return the visibility
     */
    bool isGraphTypeSelectorVisible() const;

    /**
     * Get the selectability of items
     * @return the selectability
     */
    bool isSelectable() const;

    /**
     * Get the shadows visibility
     * @return the visibility
     */
    bool isShadowVisible() const;

    /**
     * Get the graph type
     *  @return the type of graph
     */
    SKGTableWithGraph::GraphType getGraphType() const;

    /**
     * Get the number of columns
     * @param iWithComputed with compute columns (average, sum, forecast, …) or not
     * @return the number of columns
     */
    int getNbColumns(bool iWithComputed = false) const;

    /**
     * @brief Get show widget
     *
     * @return SKGShow*
     **/
    SKGShow *getShowWidget() const;

    /**
     * To know if the table is visible
     * @return the visibility
     */
    bool isTableVisible() const;

    /**
     * To know if the graph is visible
     * @return the visibility
     */
    bool isGraphVisible() const;

    /**
     * To know if the text report is visible
     * @return the visibility
     */
    bool isTextReportVisible() const;

public Q_SLOTS:

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    void setState(const QString &iState);

    /**
     * Set Data
     * @param iData the data
     * @param iPrimaryUnit the primary unit
     * @param iSecondaryUnit the secondary unit
     * @param iAdditionalInformation show sum and average columns
     * @param iNbVirtualColumn number of columns
     */
    void setData(const SKGStringListList &iData,
                 const SKGServices::SKGUnitInfo &iPrimaryUnit,
                 const SKGServices::SKGUnitInfo &iSecondaryUnit,
                 SKGTableWithGraph::DisplayAdditionalFlag iAdditionalInformation = SKGTableWithGraph::ALL,
                 int iNbVirtualColumn = 0);

    /**
     * Set the visibility of the graph type selector zone
     * @param iVisible the visibility
     */
    void setGraphTypeSelectorVisible(bool iVisible);

    /**
     * Enable / disable the selectability of items
     * @param iSelectable the selectability
     */
    void setSelectable(bool iSelectable);

    /**
     * Enable / disable the shadows
     * @param iShadow the shadows
     */
    void setShadowVisible(bool iShadow);

    /**
     * Set the graph type
     * @param iType the type of graph
     */
    void setGraphType(SKGTableWithGraph::GraphType iType);

    /**
     * Set tool bar visibility
     * @param iVisibility the visibility
     */
    void setFilterVisibility(bool iVisibility) const;

    /**
     * Set the axis color
     * @param iColor the color
     */
    void setAxisColor(const QColor &iColor = Qt::gray);

    /**
     * Set the grid color
     * @param iColor the color
     */
    void setGridColor(const QColor &iColor = Qt::lightGray);

    /**
     * Set the min color
     * @param iColor the color
     */
    void setMinColor(const QColor &iColor = Qt::red);

    /**
     * Set the max color
     * @param iColor the color
     */
    void setMaxColor(const QColor &iColor = Qt::green);

    /**
     * Set the pareto color
     * @param iColor the color
     */
    void setParetoColor(const QColor &iColor = Qt::darkRed);

    /**
     * Set the average color
     * @param iColor the color
     */
    void setAverageColor(const QColor &iColor = Qt::blue);

    /**
     * Set the tendency color
     * @param iColor the color
     */
    void setTendencyColor(const QColor &iColor = Qt::darkYellow);

    /**
     * Set the outline color
     * @param iColor the color
     */
    void setOutlineColor(const QColor &iColor = Qt::black);

    /**
     * Set the background color
     * @param iColor the color
     */
    void setBackgroundColor(const QColor &iColor = Qt::white);

    /**
     * Set the text color
     * @param iColor the color
     */
    void setTextColor(const QColor &iColor = Qt::black);

    /**
     * Set antialiasing
     * @param iAntialiasing enabled or disabled
     */
    void setAntialiasing(bool iAntialiasing = true);

    /**
     * Redraw the graph after some milliseconds
     */
    void redrawGraphDelayed();

    /**
     * Switch the limits visibility
     * @return the new visibility
     */
    bool switchLimitsVisibility();

    /**
     * Switch the average visibility
     * @return the new visibility
     */
    bool switchAverageVisibility();

    /**
     * Switch the linear regression visibility
     * @return the new visibility
     */
    bool switchLinearRegressionVisibility();

    /**
     * Switch the pareto curve visibility
     * @return the new visibility
     */
    bool switchParetoVisibility();

    /**
     * Switch the legend visibility
     * @return the new visibility
     */
    bool switchLegendVisibility();

    /**
     * Switch the origin visibility
     * @return the new visibility
     */
    bool swithOriginVisibility();

    /**
     * Switch the decimals visibility
     * @return the new visibility
     */
    bool swithDecimalsVisibility();

    /**
     * Reset the colors
     */
    void resetColors();

    /**
     * Export to a file
     * @param iFileName the file name
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError exportInFile(const QString &iFileName);

Q_SIGNALS:
    /**
     * Emitted when a cell is double clicked
     * @param row row of the cell
     * @param column column of the cell
     */
    void cellDoubleClicked(int row, int column);

    /**
     * Selection changed
     */
    void selectionChanged();

    /**
     * The object is modified
     */
    void modified();

private Q_SLOTS:
    void onExport();
    void onSelectionChanged();
    void onSelectionChangedInGraph();
    void onDoubleClick(int row, int column);
    void onDoubleClickGraph();
    void onLinkClicked(const QUrl &url);
    void onFilterModified();
    void onDisplayModeChanged();
    void onChangeColor();
    void onHorizontalScrollBarChanged(int /*iValue*/);
    void refresh();
    void redrawText();
    void redrawGraph();
    void showMenu(QPoint iPos);

private:
    Q_DISABLE_COPY(SKGTableWithGraph)

    double computeStepSize(double iRange, double iTargetSteps);
    void addArrow(QPointF iPeak, double iSize, double iArrowAngle = 45, double iDegree = 90);
    void addLegend(QPointF iPosition, double iSize, double iScaleText, double iMaxY);
    QGraphicsItem *drawPoint(qreal iX, qreal iY, qreal iRadius, int iMode, const QBrush &iBrush);
    int getAverageColumnIndex() const;
    int getMinColumnIndex() const;

    QStringList getSumItems(const QString &iString) const;
    void addSums(SKGStringListList &ioTable, int &iNblines);

    Ui::skgtablewithgraph_base ui{};
    SKGGraphicsScene *m_scene;

    SKGStringListList m_data;
    QList<bool> m_sumRows;
    SKGServices::SKGUnitInfo m_primaryUnit;
    SKGServices::SKGUnitInfo m_secondaryUnit;
    DisplayAdditionalFlag m_additionalInformation;
    int m_nbVirtualColumns;
    bool m_selectable;
    bool m_toolBarVisible;
    bool m_graphTypeVisible;
    bool m_limitVisible;
    bool m_averageVisible;
    bool m_linearRegressionVisible;
    bool m_paretoVisible;
    bool m_legendVisible;
    bool m_graphVisible;
    bool m_tableVisible;
    bool m_textVisible;
    bool m_zeroVisible;
    bool m_decimalsVisible;
    bool m_shadow;

    QMenu *m_mainMenu;
    QTimer m_timer;
    QTimer m_timerRedraw;
    QAction *m_actShowLimits;
    QAction *m_actShowAverage;
    QAction *m_actShowLinearRegression;
    QAction *m_actShowPareto;
    QAction *m_actShowLegend;
    QAction *m_actShowZero;
    QAction *m_actShowDecimal;
    QAction *m_allPositiveMenu;
    QWidgetAction *m_displayModeWidget{};

    int m_indexSum;
    int m_indexAverage;
    int m_indexMin;
    int m_indexLinearRegression;
    QMap<QString, QColor> m_mapTitleColor;
    QMap<QTableWidgetItem *, QGraphicsItem *> m_mapItemGraphic{};

    QColor m_axisColor;
    QColor m_backgroundColor;
    QColor m_textColor;
    QColor m_gridColor;
    QColor m_minColor;
    QColor m_maxColor;
    QColor m_paretoColor;
    QColor m_averageColor;
    QColor m_tendencyColor;
    QColor m_outlineColor;
    QBrush m_NegativeColor;
    QBrush m_WhiteColor;

    SKGComboBox *m_displayMode{};

    Qt::SortOrder m_sortOrder;
    int m_sortColumn;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SKGTableWithGraph::DisplayAdditionalFlag)

#endif // SKGTABLEWITHGRAPH_H
