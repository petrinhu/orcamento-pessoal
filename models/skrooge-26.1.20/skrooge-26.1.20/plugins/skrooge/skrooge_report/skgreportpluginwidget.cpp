/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin to generate report.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgreportpluginwidget.h"

#include <klocalizedstring.h>

#include <qgraphicsscene.h>
#include <qlistwidget.h>
#include <qmenu.h>
#include <qwidgetaction.h>

#include "skgbankincludes.h"
#include "skgcolorbutton.h"
#include "skgmainpanel.h"
#include "skgreport_settings.h"
#include "skgreportplugin.h"
#include "skgtraces.h"
#include <utility>

/**
 * The size of the forecast
 */
static const int FORECASTMAX = 100;

SKGReportPluginWidget::SKGReportPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument, bool iMinimmumMode)
    : SKGTabPage(iParent, iDocument)
    , m_openReportAction(nullptr)
    , m_openAction(nullptr)
    , m_mode(0)
    , m_nbLevelLines(0)
    , m_nbLevelColumns(0)
    , m_refreshNeeded(true)
{
    SKGTRACEINFUNC(10)
    if (iDocument == nullptr) {
        return;
    }

    m_timer.setSingleShot(true);
    connect(
        &m_timer,
        &QTimer::timeout,
        this,
        [this]() {
            this->dataModified();
        },
        Qt::QueuedConnection);

    ui.setupUi(this);

    ui.kCorrectedBy->setDocument(iDocument);
    ui.kCorrectedBy->setCurrentIndex(0);
    ui.kCorrectedBy->setWhereClauseCondition(QStringLiteral("t_type='I'"));

    ui.kCorrectedByMode->addItem(QStringLiteral("x"));
    ui.kCorrectedByMode->addItem(QStringLiteral("/"));

    ui.kLineLabel->setText(QString());

    ui.kLineUp->setIcon(SKGServices::fromTheme(QStringLiteral("format-indent-more")));
    ui.kLineDown->setIcon(SKGServices::fromTheme(QStringLiteral("format-indent-less")));
    ui.kColUp->setIcon(SKGServices::fromTheme(QStringLiteral("format-indent-more")));
    ui.kColDown->setIcon(SKGServices::fromTheme(QStringLiteral("format-indent-less")));

    ui.kLineAdd->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-right")));
    ui.kLineRemove->setIcon(SKGServices::fromTheme(QStringLiteral("edit-delete")));

    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-line")),
                      i18nc("Display graph values as the sum of transactions amount", "Sum of transactions"),
                      0);
    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-line-stacked")),
                      i18nc("Display graph values as the cumulated sum of transactions amount", "Cumulated sum of transactions"),
                      1);
    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-bar-percentage")), i18nc("Display graph values in base 100", "Base 100"), 7);
    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-bar-percentage")),
                      i18nc("Display graph values in base 100", "Cumulated sum in base 100"),
                      8);
    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-area")), i18nc("Display graph values in percentage", "Percent of columns"), 2);
    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-area")),
                      i18nc("Display graph values in percentage", "Absolute percent of columns"),
                      5);
    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-line")), i18nc("Display graph values in percentage", "Percent of lines"), 4);
    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("office-chart-line")), i18nc("Display graph values in percentage", "Absolute percent of lines"), 6);
    ui.kMode->addItem(SKGServices::fromTheme(QStringLiteral("irc-operator")),
                      i18nc("Display graph values as a number of transactions (or transaction)", "Count number of transactions"),
                      3);

    ui.kForecastCmb->addItem(i18nc("Noun", "None"), 0);
    ui.kForecastCmb->addItem(SKGServices::fromTheme(QStringLiteral("download-later")), i18nc("Noun", "Moving average"), 2);
    ui.kForecastCmb->addItem(SKGServices::fromTheme(QStringLiteral("download-later")), i18nc("Noun", "Weighted moving average"), 3);
    // TODO(Stephane MANKOWSKI): ui.kForecastCmb->addItem(SKGServices::fromTheme("applications-education-mathematics"), i18nc("Noun", "Interest rate") , 5);
    ui.kForecastCmb->addItem(SKGServices::fromTheme(QStringLiteral("view-calendar-day")), i18nc("Noun", "Schedule"), 1);
    ui.kForecastCmb->addItem(SKGServices::fromTheme(QStringLiteral("view-calendar-whatsnext")), i18nc("Noun", "Budget"), 4);

    ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("configure")),
                                  i18n("Setup Report"),
                                  i18n("Display the edit panel for report"),
                                  ui.setupWidget);
    connect(ui.kWidgetSelector, &SKGWidgetSelector::selectedModeChanged, this, &SKGReportPluginWidget::onBtnModeClicked);

    // Set colors
    setSettings();
    if (SKGMainPanel::getMainPanel() != nullptr) {
        connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::settingsChanged, this, &SKGReportPluginWidget::setSettings);
    }

    // Build contextual menu
    QMenu *tableMenu = ui.kTableWithGraph->getTableContextualMenu();
    if (tableMenu != nullptr) {
        tableMenu->addSeparator();
        m_openAction = tableMenu->addAction(SKGServices::fromTheme(QStringLiteral("quickopen")), i18nc("Verb", "Open…"));
        if (m_openAction != nullptr) {
            m_openAction->setEnabled(false);
        }
        QStringList overlayopen;
        overlayopen.push_back(QStringLiteral("quickopen"));
        m_openReportAction = tableMenu->addAction(SKGServices::fromTheme(QStringLiteral("view-statistics"), overlayopen), i18nc("Verb", "Open report…"));
        if (m_openReportAction != nullptr) {
            m_openReportAction->setEnabled(false);
        }
    }

    QMenu *graphMenu = ui.kTableWithGraph->getGraphContextualMenu();
    SKGComboBox *secondColumns = nullptr;
    if (iMinimmumMode && graphMenu != nullptr) {
        graphMenu->addSeparator();
        QList<QCheckBox *> list;
        list << ui.kIncomes << ui.kExpenses << ui.kGrouped << ui.kTransfers << ui.kTracked;
        for (auto actref : std::as_const(list)) {
            auto act = graphMenu->addAction(actref->text());
            if (act != nullptr) {
                act->setCheckable(true);
                act->setChecked(actref->isChecked());
                connect(act, &QAction::triggered, actref, &QCheckBox::setChecked);
                connect(actref, &QCheckBox::toggled, act, &QAction::setChecked);
            }
        }
        graphMenu->addSeparator();
        auto act = new QWidgetAction(this);

        secondColumns = new SKGComboBox(this);

        connect(ui.kColumns,
                static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
                secondColumns,
                &SKGComboBox::setCurrentText);
        connect(secondColumns,
                static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
                ui.kColumns,
                &SKGComboBox::setCurrentText);

        act->setDefaultWidget(secondColumns);
        graphMenu->addAction(act);

        graphMenu->addSeparator();
        graphMenu->addAction(m_openAction);
        graphMenu->addAction(m_openReportAction);
    }

    connect(m_openAction, &QAction::triggered, this, &SKGReportPluginWidget::onOpen);
    connect(m_openReportAction, &QAction::triggered, this, &SKGReportPluginWidget::onOpenReport);

    // Init comboboxes
    m_attsForColumns << QStringLiteral("d_date") << QStringLiteral("d_DATEWEEK") << QStringLiteral("d_DATEMONTH") << QStringLiteral("d_DATEQUARTER")
                     << QStringLiteral("d_DATESEMESTER") << QStringLiteral("d_DATEYEAR");
    m_attsForLines << QStringLiteral("#NOTHING#") << QStringLiteral("t_BANK") << QStringLiteral("t_ACCOUNTTYPE") << QStringLiteral("t_ACCOUNT")
                   << QStringLiteral("t_TOACCOUNT") << QStringLiteral("t_REALCATEGORY") << QStringLiteral("t_PAYEE") << QStringLiteral("t_TYPEEXPENSENLS")
                   << QStringLiteral("t_mode") << QStringLiteral("t_status") << QStringLiteral("t_UNITTYPE") << QStringLiteral("t_UNIT")
                   << QStringLiteral("t_REALREFUND") << QStringLiteral("t_TRANSFER");

    // Adding properties of transactions and sub transactions
    QStringList properties;
    iDocument->getDistinctValues(QStringLiteral("parameters"),
                                 QStringLiteral("'p_'||t_name"),
                                 QStringLiteral("(t_uuid_parent like '%-operation' OR t_uuid_parent like '%-suboperation')  AND t_name NOT LIKE 'SKG_%'"),
                                 properties);
    int nb = properties.count();
    m_attsForLines.reserve(m_attsForLines.count() + nb);
    for (int i = 0; i < nb; ++i) {
        m_attsForLines.push_back(properties.at(i));
    }

    // Adding properties of categories
    iDocument->getDistinctValues(QStringLiteral("parameters"),
                                 '\'' + iDocument->getDisplay(QStringLiteral("t_category")) + ".p_'||t_name",
                                 QStringLiteral("t_uuid_parent like '%-category' AND t_name NOT LIKE 'SKG_%'"),
                                 properties);
    nb = properties.count();
    m_attsForLines.reserve(m_attsForLines.count() + nb);
    for (int i = 0; i < nb; ++i) {
        m_attsForLines.push_back(properties.at(i));
    }

    // Adding properties of accounts
    iDocument->getDistinctValues(QStringLiteral("parameters"),
                                 '\'' + iDocument->getDisplay(QStringLiteral("t_account")) + ".p_'||t_name",
                                 QStringLiteral("t_uuid_parent like '%-account' AND t_name NOT LIKE 'SKG_%'"),
                                 properties);
    nb = properties.count();
    m_attsForLines.reserve(m_attsForLines.count() + nb);
    for (int i = 0; i < nb; ++i) {
        m_attsForLines.push_back(properties.at(i));
    }

    // Adding properties of payee
    iDocument->getDistinctValues(QStringLiteral("parameters"),
                                 '\'' + iDocument->getDisplay(QStringLiteral("t_payee")) + ".p_'||t_name",
                                 QStringLiteral("t_uuid_parent like '%-payee' AND t_name NOT LIKE 'SKG_%'"),
                                 properties);
    nb = properties.count();
    m_attsForLines.reserve(m_attsForLines.count() + nb);
    for (int i = 0; i < nb; ++i) {
        m_attsForLines.push_back(properties.at(i));
    }

    // Adding properties of unit
    iDocument->getDistinctValues(QStringLiteral("parameters"),
                                 '\'' + iDocument->getDisplay(QStringLiteral("t_unit")) + ".p_'||t_name",
                                 QStringLiteral("t_uuid_parent like '%-unit' AND t_name NOT LIKE 'SKG_%'"),
                                 properties);
    nb = properties.count();
    m_attsForLines.reserve(m_attsForLines.count() + nb);
    for (int i = 0; i < nb; ++i) {
        m_attsForLines.push_back(properties.at(i));
    }

    if (!iMinimmumMode) {
        m_attsForColumns += m_attsForLines;
    }

    for (const auto &att : std::as_const(m_attsForColumns)) {
        ui.kColumns->addItem(iDocument->getIcon(att), iDocument->getDisplay(att));
        if (secondColumns) {
            secondColumns->addItem(iDocument->getIcon(att), iDocument->getDisplay(att));
        }
    }

    for (const auto &att : std::as_const(m_attsForLines)) {
        ui.kLines->addItem(iDocument->getIcon(att), iDocument->getDisplay(att));
    }

    ui.kColumns->setCurrentIndex(2);

    connect(ui.kTableWithGraph, &SKGTableWithGraph::selectionChanged, this, &SKGReportPluginWidget::onSelectionChanged);

    // Refresh
    connect(ui.kPeriod, &SKGPeriodEdit::changed, this, &SKGReportPluginWidget::refresh, Qt::QueuedConnection);

    connect(ui.kColumns,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGReportPluginWidget::refresh,
            Qt::QueuedConnection);
    connect(ui.kLines,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGReportPluginWidget::refresh,
            Qt::QueuedConnection);
    connect(ui.kMode,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGReportPluginWidget::refresh,
            Qt::QueuedConnection);
    connect(ui.kIncomes, &QCheckBox::checkStateChanged, this, &SKGReportPluginWidget::refresh, Qt::QueuedConnection);
    connect(ui.kExpenses, &QCheckBox::checkStateChanged, this, &SKGReportPluginWidget::refresh, Qt::QueuedConnection);
    connect(ui.kTransfers, &QCheckBox::checkStateChanged, this, &SKGReportPluginWidget::refresh, Qt::QueuedConnection);
    connect(ui.kGrouped, &QCheckBox::checkStateChanged, this, &SKGReportPluginWidget::refresh, Qt::QueuedConnection);
    connect(ui.kTracked, &QCheckBox::checkStateChanged, this, &SKGReportPluginWidget::refresh, Qt::QueuedConnection);
    connect(ui.kForecastCmb,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGReportPluginWidget::refresh,
            Qt::QueuedConnection);
    connect(ui.kForecastValue, static_cast<void (QSlider::*)(int)>(&QSlider::valueChanged), this, &SKGReportPluginWidget::refresh, Qt::QueuedConnection);
    connect(ui.kCorrectedBy,
            static_cast<void (SKGUnitComboBox::*)(int)>(&SKGUnitComboBox::currentIndexChanged),
            this,
            &SKGReportPluginWidget::refresh,
            Qt::QueuedConnection);
    connect(ui.kCorrectedByMode,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGReportPluginWidget::refresh,
            Qt::QueuedConnection);

    connect(getDocument(), &SKGDocument::tableModified, this, &SKGReportPluginWidget::dataModified);
    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::currentPageChanged, this, &SKGReportPluginWidget::pageChanged, Qt::QueuedConnection);
    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::pageClosed, this, &SKGReportPluginWidget::pageChanged, Qt::QueuedConnection);
    connect(ui.kOtherFilters, &QListWidget::itemChanged, this, &SKGReportPluginWidget::pageChanged, Qt::QueuedConnection);
    connect(ui.kOtherFilters, &QListWidget::itemChanged, this, &SKGReportPluginWidget::refresh, Qt::QueuedConnection);

    connect(ui.kTableWithGraph, &SKGTableWithGraph::cellDoubleClicked, this, &SKGReportPluginWidget::onDoubleClick);
    connect(ui.kLineRemove, &QToolButton::clicked, this, &SKGReportPluginWidget::onRemoveLine);
    connect(ui.kLineAdd, &QToolButton::clicked, this, &SKGReportPluginWidget::onAddLine);
    connect(ui.kLineDown, &QToolButton::clicked, this, &SKGReportPluginWidget::onOneLevelLess);
    connect(ui.kLineUp, &QToolButton::clicked, this, &SKGReportPluginWidget::onOneLevelMore);
    connect(ui.kColDown, &QToolButton::clicked, this, &SKGReportPluginWidget::onOneLevelLess);
    connect(ui.kColUp, &QToolButton::clicked, this, &SKGReportPluginWidget::onOneLevelMore);
    connect(ui.kGrouped, &QCheckBox::toggled, ui.kTransfers, &QCheckBox::setEnabled);

    if (iMinimmumMode) {
        ui.kTableWithGraph->getShowWidget()->setState(QStringLiteral("\"graph\""));
        ui.kWidgetSelector->setSelectedMode(-1);
    } else {
        ui.kWidgetSelector->setSelectedMode(0);
    }
    onBtnModeClicked(ui.kWidgetSelector->getSelectedMode());
}

SKGReportPluginWidget::~SKGReportPluginWidget()
{
    SKGTRACEINFUNC(10)
    m_openAction = nullptr;
    m_openReportAction = nullptr;
}

QString SKGReportPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("columns"), m_attsForColumns.value(ui.kColumns->currentIndex()));

    QString lineString;
    int nb = m_attsForLinesAdded.count();
    for (int i = 0; i < nb; ++i) {
        lineString += m_attsForLinesAdded.at(i);
        if (i < nb - 1) {
            lineString += OBJECTSEPARATOR;
        }
    }

    root.setAttribute(QStringLiteral("lines"), lineString);
    root.setAttribute(QStringLiteral("lines2"), m_attsForLines.value(ui.kLines->currentIndex()));
    root.setAttribute(QStringLiteral("mode"), SKGServices::intToString(ui.kMode->itemData(ui.kMode->currentIndex()).toInt()));
    root.setAttribute(QStringLiteral("periodDef"), ui.kPeriod->getState());
    root.setAttribute(QStringLiteral("incomes"), ui.kIncomes->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("expenses"), ui.kExpenses->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("transfers"), ui.kTransfers->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("grouped"), ui.kGrouped->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("tracked"), ui.kTracked->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("currentPage"), SKGServices::intToString(ui.kWidgetSelector->getSelectedMode()));
    root.setAttribute(QStringLiteral("tableAndGraphState"), ui.kTableWithGraph->getState());
    root.setAttribute(QStringLiteral("nbLevelLines"), SKGServices::intToString(m_nbLevelLines));
    root.setAttribute(QStringLiteral("nbLevelColumns"), SKGServices::intToString(m_nbLevelColumns));
    root.setAttribute(QStringLiteral("forecast"), SKGServices::intToString(ui.kForecastCmb->itemData(ui.kForecastCmb->currentIndex()).toInt()));
    root.setAttribute(QStringLiteral("forecastValue"), SKGServices::intToString(ui.kForecastValue->value()));
    root.setAttribute(QStringLiteral("zoomPosition"), SKGServices::intToString(zoomPosition()));
    root.setAttribute(QStringLiteral("correctedby"), ui.kCorrectedBy->currentText());
    root.setAttribute(QStringLiteral("correctedbymode"), ui.kCorrectedByMode->currentText());

    // Addition of other filter
    QString wc;
    QString title = root.attribute(QStringLiteral("title"));
    QString title_icon;
    nb = ui.kOtherFilters->count();
    for (int i = 0; i < nb; ++i) {
        QListWidgetItem *item = ui.kOtherFilters->item(i);
        if (item->checkState() == Qt::Checked) {
            if (!wc.isEmpty()) {
                wc = u'(' % wc % ") AND (" % item->data(1000).toString() % u')';
            } else {
                wc = item->data(1000).toString();
            }
            title_icon = item->data(1001).toString();

            title += item->text();
        }
    }

    if (!wc.isEmpty()) {
        root.setAttribute(QStringLiteral("title"), title);
        root.setAttribute(QStringLiteral("operationWhereClause"), wc);
        root.setAttribute(QStringLiteral("title_icon"), title_icon);
    }

    return doc.toString();
}

void SKGReportPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    m_timer.stop();

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString columns = root.attribute(QStringLiteral("columns"));
    QString lines = root.attribute(QStringLiteral("lines"));
    QString lines2 = root.attribute(QStringLiteral("lines2"));
    QString mode = root.attribute(QStringLiteral("mode"));
    QString incomes = root.attribute(QStringLiteral("incomes"));
    QString expenses = root.attribute(QStringLiteral("expenses"));
    QString transfers = root.attribute(QStringLiteral("transfers"));
    QString grouped = root.attribute(QStringLiteral("grouped"));
    QString tracked = root.attribute(QStringLiteral("tracked"));
    QString currentPage = root.attribute(QStringLiteral("currentPage"));
    QString forecast = root.attribute(QStringLiteral("forecast"));
    QString forecastValue = root.attribute(QStringLiteral("forecastValue"));
    QString tableAndGraphState = root.attribute(QStringLiteral("tableAndGraphState"));
    QString title = root.attribute(QStringLiteral("title"));
    QString title_icon = root.attribute(QStringLiteral("title_icon"));
    QString wc = root.attribute(QStringLiteral("operationWhereClause"));
    QString nbLevelLinesString = root.attribute(QStringLiteral("nbLevelLines"));
    QString nbLevelColumnsString = root.attribute(QStringLiteral("nbLevelColumns"));
    QString zoomPositionString = root.attribute(QStringLiteral("zoomPosition"));
    QString correctedby = root.attribute(QStringLiteral("correctedby"));
    QString correctedbymode = root.attribute(QStringLiteral("correctedbymode"));
    QString periodDef = root.attribute(QStringLiteral("periodDef"));

    // Default values
    if (nbLevelLinesString.isEmpty()) {
        nbLevelLinesString = '0';
    }
    if (nbLevelColumnsString.isEmpty()) {
        nbLevelColumnsString = '0';
    }
    if (columns.isEmpty()) {
        columns = m_attsForColumns.at(2);
    }
    if (lines2.isEmpty()) {
        lines2 = m_attsForLines.at(0);
    }
    if (mode.isEmpty()) {
        mode = '0';
    }
    if (incomes.isEmpty()) {
        incomes = 'Y';
    }
    if (expenses.isEmpty()) {
        expenses = 'Y';
    }
    if (transfers.isEmpty()) {
        transfers = 'N';
    }
    if (grouped.isEmpty()) {
        grouped = 'Y';
    }
    if (tracked.isEmpty()) {
        tracked = 'Y';
    }
    if (currentPage.isEmpty()) {
        currentPage = '0';
    }
    if (forecast.isEmpty()) {
        forecast = '0';
    }
    if (forecastValue.isEmpty()) {
        forecastValue = '0';
    }

    m_nbLevelLines = SKGServices::stringToInt(nbLevelLinesString);
    m_nbLevelColumns = SKGServices::stringToInt(nbLevelColumnsString);
    ui.kColumns->setCurrentIndex(m_attsForColumns.indexOf(columns));
    ui.kLines->setCurrentIndex(m_attsForLines.indexOf(lines2));
    m_attsForLinesAdded.clear();
    if (!lines.isEmpty()) {
        m_attsForLinesAdded = lines.split(OBJECTSEPARATOR);
    }
    ui.kMode->setCurrentIndex(ui.kMode->findData(SKGServices::stringToInt(mode)));
    if (periodDef.isEmpty()) {
        QString period = root.attribute(QStringLiteral("period"));
        QString interval = root.attribute(QStringLiteral("interval"));
        QString nb_interval = root.attribute(QStringLiteral("nb_intervals"));
        QString timeline = root.attribute(QStringLiteral("timeline"));
        QString date_begin = root.attribute(QStringLiteral("date_begin"));
        QString date_end = root.attribute(QStringLiteral("date_end"));
        if (period.isEmpty()) {
            period = '1';
        }
        if (interval.isEmpty()) {
            interval = '2';
        }
        if (nb_interval.isEmpty()) {
            nb_interval = '1';
        }
        if (timeline.isEmpty()) {
            timeline = '1';
        }

        QDomDocument doc2(QStringLiteral("SKGML"));
        QDomElement root2 = doc2.createElement(QStringLiteral("parameters"));
        doc2.appendChild(root2);

        root2.setAttribute(QStringLiteral("period"), period);
        if (period == QStringLiteral("4")) {
            root2.setAttribute(QStringLiteral("date_begin"), date_begin);
            root2.setAttribute(QStringLiteral("date_end"), date_end);
        }
        root2.setAttribute(QStringLiteral("interval"), interval);
        root2.setAttribute(QStringLiteral("nb_intervals"), nb_interval);
        root2.setAttribute(QStringLiteral("timeline"), timeline);

        periodDef = doc2.toString();
    }
    ui.kPeriod->setState(periodDef);

    ui.kIncomes->setChecked(incomes != QStringLiteral("N"));
    ui.kExpenses->setChecked(expenses != QStringLiteral("N"));
    ui.kTransfers->setChecked(transfers != QStringLiteral("N"));
    ui.kGrouped->setChecked(grouped != QStringLiteral("N"));
    ui.kTracked->setChecked(tracked != QStringLiteral("N"));
    int currentPageInt = SKGServices::stringToInt(currentPage);
    ui.kWidgetSelector->setSelectedMode(currentPageInt);
    ui.kLine->setVisible(currentPageInt >= -1);
    ui.kForecastCmb->setCurrentIndex(ui.kForecastCmb->findData(SKGServices::stringToInt(forecast)));
    ui.kForecastValue->setValue(SKGServices::stringToInt(forecastValue));
    if (!zoomPositionString.isEmpty()) {
        setZoomPosition(SKGServices::stringToInt(zoomPositionString));
    }
    if (!correctedby.isEmpty()) {
        ui.kCorrectedBy->setCurrentIndex(ui.kCorrectedBy->findText(correctedby));
    }
    if (!correctedbymode.isEmpty()) {
        ui.kCorrectedByMode->setCurrentIndex(ui.kCorrectedByMode->findText(correctedbymode));
    }

    refresh();

    ui.kTableWithGraph->setState(tableAndGraphState);

    // Remove previous other filters
    ui.kOtherFilters->clear();

    if (!title.isEmpty() && !wc.isEmpty()) {
        // Add new filter
        auto item = new QListWidgetItem(SKGServices::fromTheme(title_icon), title);
        item->setCheckState(Qt::Checked);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setData(1000, wc);
        item->setData(1001, title_icon);
        ui.kOtherFilters->addItem(item);
    }

    m_previousParametersUsed = QString();
}

QString SKGReportPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGREPORT_DEFAULT_PARAMETERS");
}

QWidget *SKGReportPluginWidget::zoomableWidget()
{
    return ui.kTableWithGraph->table();
}

QList<QWidget *> SKGReportPluginWidget::printableWidgets()
{
    QList<QWidget *> output;
    if (ui.kTableWithGraph->isTableVisible()) {
        output.push_back(ui.kTableWithGraph->table());
    }
    if (ui.kTableWithGraph->isGraphVisible()) {
        output.push_back(ui.kTableWithGraph->graph()->graphicsView());
    }
    if (ui.kTableWithGraph->isTextReportVisible()) {
        output.push_back(ui.kTableWithGraph->textReport());
    }
    return output;
}

void SKGReportPluginWidget::getWhereClauseAndTitleForItem(int row, int column, QString &oWc, QString &oTitle)
{
    // Build where clause and title
    oTitle.clear();

    // Addition of other filter
    int nb = ui.kOtherFilters->count();
    for (int i = 0; i < nb; ++i) {
        QListWidgetItem *item = ui.kOtherFilters->item(i);
        if (item->checkState() == Qt::Checked) {
            oTitle += item->text() % u'.';
        }
    }

    // Condition on line attribute
    oTitle += i18nc("Noun, a list of items", "Sub transactions");
    auto *btn = qobject_cast<SKGColorButton *>(ui.kTableWithGraph->table()->cellWidget(row, 0));

    QString attLine;
    QStringList listAtt = m_attsForLinesAdded;
    if (listAtt.isEmpty() || ui.kLines->currentIndex() > 0) {
        listAtt.push_back(m_attsForLines.at(ui.kLines->currentIndex()));
    }
    nb = listAtt.count();
    for (int i = 0; i < nb; ++i) {
        QString att = listAtt.at(i);
        if (att == QStringLiteral("#NOTHING#")) {
            att = QString();
        }
        if (att.startsWith(QStringLiteral("p_")) || att.contains(QStringLiteral(".p_"))) {
            att = getWhereClauseForProperty(att);
        }
        if (att.isEmpty()) {
            att = '\'' % i18nc("Noun", "All") % u'\'';
        }

        if (!attLine.isEmpty()) {
            attLine += "||'" % OBJECTSEPARATOR % "'||";
        }
        attLine += att;
    }

    oWc = attLine;
    QString lineVal = (btn != nullptr ? btn->text() : ui.kTableWithGraph->table()->item(row, 0)->data(1).toString());
    QString title = ui.kTableWithGraph->table()->horizontalHeaderItem(0)->text();
    bool avoidAnd = false;
    if (lineVal.isEmpty() && row == ui.kTableWithGraph->table()->rowCount() - 1) {
        // This is the last sum
        oWc = QString();
        if (!attLine.isEmpty()) {
            oWc = attLine % " NOT LIKE '%#NULL#%'";
            avoidAnd = true;
        }
    } else {
        // This is not the last sum
        if (lineVal.isEmpty()) {
            oWc += " IS NULL OR " % attLine;
        }
        oWc +=
            " = '" % SKGServices::stringToSqlString(lineVal) % "' OR " % attLine % " like '" % SKGServices::stringToSqlString(lineVal) % OBJECTSEPARATOR % "%'";
        oWc = u'(' % oWc % u')';
        oTitle += i18nc("Noun", " with ");
        oTitle += i18nc("Noun", "'%1' with '%2'", title, lineVal);
    }

    // Condition on column attribute
    int nbCol = ui.kTableWithGraph->getNbColumns();
    QString att = m_attsForColumns[ui.kColumns->currentIndex()];
    if (att == QStringLiteral("#NOTHING#")) {
        att = QString();
    }
    if (att.startsWith(QStringLiteral("p_")) || att.contains(QStringLiteral(".p_"))) {
        att = getWhereClauseForProperty(att);
    }
    if (!att.isEmpty()) {
        if (column != 0 && column < nbCol) {
            if (!oWc.isEmpty()) {
                oWc += QStringLiteral(" AND ");
                if (!avoidAnd) {
                    oTitle += i18nc("Noun", " and ");
                } else {
                    oTitle += i18nc("Noun", " with ");
                }
            } else {
                oTitle += i18nc("Noun", " with ");
            }

            oWc += u'(' % att;
            QString val = ui.kTableWithGraph->table()->horizontalHeaderItem(column)->text();
            if (val.isEmpty()) {
                oWc += " IS NULL OR " % att % "=''";
                oTitle += i18nc("Noun", "'%1' are empty", ui.kColumns->currentText());
            } else {
                oWc += " = '" % SKGServices::stringToSqlString(val) % "' OR " % att % " like '" % SKGServices::stringToSqlString(val) % OBJECTSEPARATOR % "%'";
                oTitle += i18nc("Noun", "'%1' with '%2'", ui.kColumns->currentText(), val);
            }
            oWc += ')';
        } else {
            oWc = u'(' % oWc % ") AND " % att % " NOT LIKE '%#NULL#%'";
        }
    }

    // Condition on other attribute
    if (!oWc.isEmpty()) {
        oWc += QStringLiteral(" AND ");
        oTitle += i18nc("Noun", " and ");
    }
    oWc += getConsolidatedWhereClause();

    QString during = ui.kPeriod->text();

    QStringList types;
    if (ui.kIncomes->isChecked()) {
        types.push_back(i18nc("Noun", "incomes"));
    }
    if (ui.kExpenses->isChecked()) {
        types.push_back(i18nc("Noun", "expenses"));
    }
    if (ui.kTransfers->isChecked()) {
        types.push_back(i18nc("Noun", "transfers"));
    } else if (ui.kGrouped->isChecked()) {
        types.push_back(i18nc("Noun", "grouped"));
    }
    if (ui.kTracked->isChecked()) {
        types.push_back(i18nc("Noun", "tracked"));
    }

    oTitle += i18nc("Noun", "during '%1' for '%2'", during, types.join(QStringLiteral(" ")));
}

void SKGReportPluginWidget::getWhereClauseAndTitleForSelection(QString &oWc, QString &oTitle)
{
    oWc.clear();
    oTitle.clear();

    QList<QTableWidgetItem *> selection = ui.kTableWithGraph->table()->selectedItems();
    int nb = selection.count();
    if (nb != 0) {
        for (int i = 0; i < nb; ++i) {
            QString wc2;
            QString title2;
            getWhereClauseAndTitleForItem(selection.at(i)->row(), selection.at(i)->column(), wc2, title2);
            if (!wc2.isEmpty()) {
                if (!oWc.isEmpty()) {
                    oWc = u'(' % oWc % ") OR (" % wc2 % u')';
                } else {
                    oWc = wc2;
                }
            }
            if (!title2.isEmpty()) {
                if (!oTitle.isEmpty()) {
                    oTitle = i18n("(%1) or (%2)", oTitle, title2);
                } else {
                    oTitle = std::move(title2);
                }
            }
        }
    }
}

void SKGReportPluginWidget::onDoubleClick(int row, int column)
{
    _SKGTRACEINFUNC(10)

    QString wc;
    QString title;
    getWhereClauseAndTitleForItem(row, column, wc, title);
    SKGMainPanel::getMainPanel()->openPage(
        "skg://skrooge_operation_plugin/SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/"
        "?currentPage=-1&title_icon=view-statistics&operationTable=v_suboperation_consolidated&operationWhereClause="
        % SKGServices::encodeForUrl(wc) % "&title=" % SKGServices::encodeForUrl(title));
}

void SKGReportPluginWidget::onBtnModeClicked(int mode)
{
    ui.kTableWithGraph->setFilterVisibility(mode == 0);
}

void SKGReportPluginWidget::onOpen()
{
    QString wc;
    QString title;
    getWhereClauseAndTitleForSelection(wc, title);
    SKGMainPanel::getMainPanel()->openPage(
        "skg://skrooge_operation_plugin/SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/"
        "?currentPage=-1&title_icon=view-statistics&operationTable=v_suboperation_consolidated&operationWhereClause="
        % SKGServices::encodeForUrl(wc) % "&title=" % SKGServices::encodeForUrl(title));
}

void SKGReportPluginWidget::onOpenReport()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    QString wc;
    QString title;
    getWhereClauseAndTitleForSelection(wc, title);
    if (!wc.isEmpty()) {
        // Call report plugin
        QDomDocument doc(QStringLiteral("SKGML"));
        doc.setContent(getState());
        QDomElement root = doc.documentElement();
        root.setAttribute(QStringLiteral("operationWhereClause"), wc);
        root.setAttribute(QStringLiteral("title"), title);
        root.setAttribute(QStringLiteral("title_icon"), QStringLiteral("view-statistics"));
        QString currentPage = root.attribute(QStringLiteral("currentPage"));
        if (SKGServices::stringToInt(currentPage) < -1) {
            root.setAttribute(QStringLiteral("currentPage"), QStringLiteral("-1"));
        }
        SKGMainPanel::getMainPanel()->openPage(SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge report plugin")), -1, doc.toString());
    }
}

void SKGReportPluginWidget::onOneLevelMore()
{
    _SKGTRACEINFUNC(10)
    if (sender() == ui.kLineUp) {
        ++m_nbLevelLines;
    } else {
        ++m_nbLevelColumns;
    }
    refresh();
}

void SKGReportPluginWidget::onOneLevelLess()
{
    _SKGTRACEINFUNC(10)
    if (sender() == ui.kLineDown) {
        --m_nbLevelLines;
    } else {
        --m_nbLevelColumns;
    }
    refresh();
}

void SKGReportPluginWidget::onAddLine()
{
    _SKGTRACEINFUNC(10)
    m_attsForLinesAdded.push_back(m_attsForLines.value(ui.kLines->currentIndex()));
    ui.kLines->setCurrentIndex(0);
    refresh();
}

void SKGReportPluginWidget::onRemoveLine()
{
    _SKGTRACEINFUNC(10)
    if (!m_attsForLinesAdded.isEmpty()) {
        m_attsForLinesAdded.pop_back();
    }
    refresh();
}

QString SKGReportPluginWidget::getConsolidatedWhereClause(QString *oWhereClausForPreviousData, QString *oWhereClausForForecastData)
{
    // Build where clause
    int forecastmode = ui.kForecastCmb->itemData(ui.kForecastCmb->currentIndex()).toInt();
    QString wc = ui.kPeriod->getWhereClause(forecastmode != 1, oWhereClausForPreviousData, oWhereClausForForecastData);

    wc = "((" % wc % ") OR d_date='0000') AND d_date!='0000-00-00'";
    if (oWhereClausForPreviousData != nullptr) {
        *oWhereClausForPreviousData = "((" % *oWhereClausForPreviousData % ") OR d_date='0000-00-00')";
    }

    QString operationTypes;
    if (ui.kIncomes->isChecked() && !ui.kExpenses->isChecked()) {
        operationTypes = QStringLiteral("t_TYPEEXPENSE='+'");
    } else if (ui.kExpenses->isChecked() && !ui.kIncomes->isChecked()) {
        operationTypes = QStringLiteral("t_TYPEEXPENSE='-'");
    }
    if (!operationTypes.isEmpty()) {
        QString condition = " AND " % operationTypes;
        wc += condition;
        if (oWhereClausForPreviousData != nullptr) {
            *oWhereClausForPreviousData += condition;
        }
    }

    if (!ui.kGrouped->isChecked()) {
        QString condition = QStringLiteral(" AND i_group_id=0");
        wc += condition;
        if (oWhereClausForPreviousData != nullptr) {
            *oWhereClausForPreviousData += condition;
        }
    } else {
        if (!ui.kTransfers->isChecked()) {
            QString condition = QStringLiteral(" AND t_TRANSFER='N'");
            wc += condition;
            if (oWhereClausForPreviousData != nullptr) {
                *oWhereClausForPreviousData += condition;
            }
        }
    }

    if (!ui.kTracked->isChecked()) {
        QString condition = QStringLiteral(" AND r_refund_id=0");
        wc += condition;
        if (oWhereClausForPreviousData != nullptr) {
            *oWhereClausForPreviousData += condition;
        }
    }

    // Addition of other filters
    int nb = ui.kOtherFilters->count();
    for (int i = 0; i < nb; ++i) {
        QListWidgetItem *item = ui.kOtherFilters->item(i);
        if (item->checkState() == Qt::Checked) {
            QString condition = " AND (" % item->data(1000).toString() % ")";
            wc += condition;
            if (oWhereClausForPreviousData != nullptr) {
                *oWhereClausForPreviousData += condition;
            }
        }
    }

    return wc;
}

void SKGReportPluginWidget::onSelectionChanged()
{
    if (m_openReportAction != nullptr) {
        m_openReportAction->setEnabled(!ui.kTableWithGraph->table()->selectedItems().isEmpty());
    }
    if (m_openAction != nullptr) {
        m_openAction->setEnabled(!ui.kTableWithGraph->table()->selectedItems().isEmpty());
    }
}

void SKGReportPluginWidget::refresh()
{
    int p = ui.kPeriod->mode();

    bool dateCol = (m_attsForColumns.value(ui.kColumns->currentIndex()).startsWith(QStringLiteral("d_")));
    if (!dateCol) {
        ui.kForecastCmb->setCurrentIndex(0);
    }
    ui.kForecastCmb->setEnabled(dateCol);
    ui.kForecastValue->setEnabled(ui.kForecastCmb->currentIndex() > 0);
    ui.kLineRemove->setEnabled(!m_attsForLinesAdded.isEmpty());

    int mode = ui.kMode->itemData(ui.kMode->currentIndex()).toInt();
    ui.kCorrectedBy->setEnabled(mode != 3);
    ui.kCorrectedByMode->setEnabled(mode != 3);

    // Check income & expense
    if (!ui.kIncomes->isChecked() && !ui.kExpenses->isChecked()) {
        if (sender() == ui.kIncomes) {
            ui.kExpenses->setChecked(true);
        } else {
            ui.kIncomes->setChecked(true);
        }
    }

    bool timeline = (p == 5);
    ui.kForecastFrm->setEnabled(!timeline);
    if (timeline) {
        ui.kForecastCmb->setCurrentIndex(0);
    }

    m_timer.start(300);
}

void SKGReportPluginWidget::pageChanged()
{
    if (m_refreshNeeded) {
        m_timer.start(300);
    }

    // Refresh other filter
    auto *rep = qobject_cast<SKGReportPlugin *>(SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge report plugin")));
    if (rep != nullptr) {
        // Remove unused filters
        int nbf = ui.kOtherFilters->count();
        for (int i = nbf - 1; i >= 0; --i) {
            QListWidgetItem *item = ui.kOtherFilters->item(i);
            if (item->checkState() == Qt::Unchecked) {
                QListWidgetItem *item = ui.kOtherFilters->takeItem(i);
                delete item;
            }
        }

        // Add pages filters
        int nb = SKGMainPanel::getMainPanel()->countPages();
        for (int i = 0; i < nb; ++i) {
            SKGTabPage *page = SKGMainPanel::getMainPanel()->page(i);
            SKGObjectBase::SKGListSKGObjectBase selection = page->getSelectedObjects();

            QString title;
            QString wc;
            rep->getTitleAndWhereClause(selection, title, wc);
            if (!title.isEmpty()) {
                // Check if already existing
                bool existing = false;
                int nbf = ui.kOtherFilters->count();
                for (int j = 0; !existing && j < nbf; ++j) {
                    QListWidgetItem *item2 = ui.kOtherFilters->item(j);
                    if (item2->data(1000).toString() == wc) {
                        existing = true;
                    }
                }

                // Add it if not existing
                if (!existing) {
                    QString icon = SKGMainPanel::getMainPanel()->getPluginByName(page->objectName())->icon();
                    auto item = new QListWidgetItem(SKGServices::fromTheme(icon), title);
                    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
                    item->setCheckState(Qt::Unchecked);
                    item->setData(1000, wc);
                    item->setData(1001, icon);
                    ui.kOtherFilters->addItem(item);
                }
            }
        }
    }
}

void SKGReportPluginWidget::dataModified(const QString &iTableName, int iIdTransaction)

{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    // Refresh panel
    QSqlDatabase *db = getDocument()->getMainDatabase();
    setEnabled(db != nullptr);
    // Check if needed
    if (db != nullptr && (iTableName == QStringLiteral("v_suboperation_consolidated") || iTableName.isEmpty())) {
        SKGError err;
        auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
        if (doc != nullptr) {
            // Check if update is needed
            SKGTabPage *page = SKGTabPage::parentTabPage(this->parentWidget());
            SKGTabPage *cpage = SKGMainPanel::getMainPanel()->currentPage();
            if (page != nullptr && page != cpage) {
                m_refreshNeeded = true;
                return;
            }

            QString ParametersUsed = getState() % u';' % SKGServices::intToString(doc->getTransactionToProcess(SKGDocument::UNDO));
            if (ParametersUsed == m_previousParametersUsed) {
                SKGTRACEL(10) << "Same parameters. Refresh ignored" << Qt::endl;
                return;
            }

            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            m_refreshNeeded = false;
            m_previousParametersUsed = std::move(ParametersUsed);

            // Fill line and create title
            QString lineString;
            QString title;
            int nb = m_attsForLinesAdded.count();
            for (int i = 0; i < nb; ++i) {
                lineString += doc->getDisplay(m_attsForLinesAdded.at(i));
                if (i < nb - 1) {
                    lineString += OBJECTSEPARATOR;
                }
            }
            if (ui.kLines->currentIndex() > 0 && (nb != 0)) {
                lineString += OBJECTSEPARATOR;
            }
            title = lineString;
            if (ui.kLines->currentIndex() > 0 || nb == 0) {
                title += ui.kLines->text();
            }
            ui.kLineLabel->setVisible(nb > 0);
            ui.kLineLabel->setText(lineString);
            ui.kLineLabel->setToolTip(lineString);

            // Get parameters
            int col = ui.kColumns->currentIndex();
            int line = ui.kLines->currentIndex();

            if (col >= 0 && line >= 0) {
                int mode = ui.kMode->itemData(ui.kMode->currentIndex()).toInt();

                // Mode history ?
                bool modeHistory = (mode == 1 || mode == 8);

                // Get condition (must be done before forecast on scheduled transactions)
                int nbVirtualColumn = 0;
                QString conditionPrevious;
                QString conditionForecast;
                QString condition = getConsolidatedWhereClause(&conditionPrevious, &conditionForecast);
                int forecastMode = ui.kForecastCmb->itemData(ui.kForecastCmb->currentIndex()).toInt();

                // Compute forecast on scheduled transactions
                bool transactionToRollback = false;
                if (forecastMode == 1 || forecastMode == 4) {
                    // Create transactions
                    QDate lastDate =
                        QDate::currentDate().addDays(ui.kForecastValue->value() * skgreport_settings::maxForecasts() * 365.0 / ui.kForecastValue->maximum());
                    if (forecastMode == 1) {
                        // Create scheduled transactions
                        doc->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Create scheduled transactions"));
                        transactionToRollback = true;
                        int nbInserted = 0;
                        SKGRecurrentOperationObject::process(doc, nbInserted, true, lastDate);
                    } else {
                        // Create budgeted transactions
                        SKGObjectBase::SKGListSKGObjectBase budgets;
                        IFOKDO(err,
                               doc->getObjects(QStringLiteral("v_budget"),
                                               "t_PERIOD>STRFTIME('%Y-%m', date('now', 'localtime')) AND "
                                               "t_PERIOD<=STRFTIME('%Y-%m', '"
                                                   % SKGServices::dateToSqlString(lastDate) % "')",
                                               budgets));
                        int nbBudgets = budgets.count();
                        if (nbBudgets != 0) {
                            // Get most active account
                            SKGStringListList list;
                            err = doc->executeSelectSqliteOrder(
                                QStringLiteral("SELECT  rd_account_id, r_category_id FROM v_suboperation_consolidated WHERE d_date>date('now', 'localtime', "
                                               "'-3 month') group by r_category_id, rd_account_id order by count(rd_account_id) DESC"),
                                list);
                            int nbmap = list.count();
                            if (!err && nbmap >= 2) {
                                // Build map
                                int default_account = 0;
                                QMap<int, int> category_account;
                                for (int i = 1; i < nbmap; ++i) {
                                    if (i == 1) {
                                        default_account = SKGServices::stringToInt(list.at(i).at(0));
                                    }
                                    category_account[SKGServices::stringToInt(list.at(i).at(1))] = SKGServices::stringToInt(list.at(i).at(0));
                                }

                                // Get unit
                                SKGUnitObject unit(doc);
                                IFOKDO(err, unit.setName(doc->getPrimaryUnit().Name))
                                IFOKDO(err, unit.setSymbol(doc->getPrimaryUnit().Symbol))
                                IFOKDO(err, unit.load())

                                doc->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Create budgeted transactions"));
                                transactionToRollback = true;
                                for (int i = 0; !err && i < nbBudgets; ++i) {
                                    SKGBudgetObject budget(budgets.at(i));

                                    SKGCategoryObject cat;
                                    budget.getCategory(cat);

                                    // Get better account for the category
                                    SKGAccountObject account(doc, category_account.contains(cat.getID()) ? category_account[cat.getID()] : default_account);
                                    account.load();

                                    SKGOperationObject op;
                                    IFOKDO(err, account.addOperation(op))
                                    IFOKDO(err, op.setDate(QDate(budget.getYear(), qMax(budget.getMonth(), 1), 1)))
                                    IFOKDO(err, op.setUnit(unit))
                                    IFOKDO(err, op.save())

                                    SKGSubOperationObject sop;
                                    IFOKDO(err, op.addSubOperation(sop))
                                    IFOKDO(err, sop.setCategory(cat))
                                    IFOKDO(err, sop.setQuantity(budget.getBudgetedAmount()))
                                    IFOKDO(err, sop.save())
                                }
                            }
                        }
                    }
                    ui.kForecastValue->setToolTip(SKGServices::dateToSqlString(lastDate));
                } else {
                    ui.kForecastValue->setToolTip(QString());
                    conditionForecast = QStringLiteral("1=0");
                }

                // Execute sql order
                SKGStringListList table;
                QString tableName = QStringLiteral("v_suboperation_consolidated");
                QString attCol = m_attsForColumns[col];
                if (attCol == QStringLiteral("#NOTHING#")) {
                    attCol = QString();
                }
                if (attCol.startsWith(QStringLiteral("p_")) || attCol.contains(QStringLiteral(".p_"))) {
                    attCol = getWhereClauseForProperty(attCol);
                }

                QStringList listAtt = m_attsForLinesAdded;
                if (listAtt.isEmpty() || line > 0) {
                    listAtt.push_back(m_attsForLines.at(line));
                }
                QString attLine;
                int nb2 = listAtt.count();
                for (int i = 0; i < nb2; ++i) {
                    QString att = listAtt.at(i);
                    if (att == QStringLiteral("#NOTHING#")) {
                        att = QString();
                    }
                    if (att.startsWith(QStringLiteral("p_")) || att.contains(QStringLiteral(".p_"))) {
                        att = getWhereClauseForProperty(att);
                    }
                    if (att.isEmpty()) {
                        att = '\'' % i18nc("Noun", "All") % u'\'';
                    }

                    if (!attLine.isEmpty()) {
                        attLine += "||'" % OBJECTSEPARATOR % "'||";
                    }
                    attLine += att;
                }

                if ((modeHistory || (forecastMode != 0)) && !attCol.isEmpty()) {
                    tableName = QStringLiteral("(SELECT ");
                    if (!modeHistory) {
                        tableName += QStringLiteral("d_date, ");
                        tableName += attCol % "||(CASE WHEN " % conditionForecast % " THEN '999' ELSE '' END) AS " % m_attsForColumns[col]
                            % ",* FROM v_suboperation_consolidated) as v_suboperation_consolidated";
                    } else {
                        if (attCol != QStringLiteral("d_date")) {
                            tableName += "(CASE WHEN (" % conditionPrevious % ") AND f_REALQUANTITY<>0 THEN '0000' ELSE d_date END) AS d_date, ";
                        }
                        tableName += "(CASE WHEN (" % conditionPrevious % ") AND f_REALQUANTITY<>0 THEN '0000' ELSE " % attCol % "||(CASE WHEN "
                            % conditionForecast % " THEN '999' ELSE '' END) END) AS " % m_attsForColumns[col]
                            % ",* FROM v_suboperation_consolidated) as v_suboperation_consolidated";
                    }
                }

                // Remove #NULL# columns and lines
                if (!attLine.isEmpty()) {
                    condition = u'(' % condition % ") AND " % attLine % " NOT LIKE '%#NULL#%'";
                }
                if (!attCol.isEmpty()) {
                    condition = u'(' % condition % ") AND " % attCol % " NOT LIKE '%#NULL#%'";
                }

                // Limit size of the result
                condition = u'(' % condition
                    % ") AND (d_date>(date('now', 'localtime', '-50 year')) OR d_date='0000') AND d_date<(date('now', 'localtime', '+50 year'))";

                QString coef;
                if (ui.kCorrectedBy->currentIndex() > 0 && mode != 3) {
                    SKGUnitObject unit = ui.kCorrectedBy->getUnit();
                    QString id = SKGServices::intToString(unit.getID());
                    if (ui.kCorrectedByMode->currentIndex() == 0) {
                        coef = '*';
                    } else {
                        coef = '/';
                    }
                    coef = coef % "COALESCE((SELECT u1.f_quantity FROM unitvalue u1 where u1.rd_unit_id=" % id
                        % " and u1.d_date=(SELECT MAX(u2.d_date) FROM unitvalue u2 where u2.rd_unit_id=" % id
                        % " and u2.d_date<=v_suboperation_consolidated.d_date)), (SELECT u1.f_quantity FROM unitvalue u1 where u1.rd_unit_id=" % id
                        % " and u1.d_date=(SELECT MIN(u2.d_date) FROM unitvalue u2 where u2.rd_unit_id=" % id % ")),0)";
                }

                IFOKDO(err,
                       doc->getConsolidatedView(tableName,
                                                attCol,
                                                attLine,
                                                "f_REALCURRENTAMOUNT" % coef,
                                                (mode == 3 ? QStringLiteral("COUNT") : QStringLiteral("TOTAL")),
                                                condition,
                                                table));
                int nbLines = table.count();
                IFSKGTRACEL(10)
                {
                    QStringList dump = SKGServices::tableToDump(table, SKGServices::DUMP_TEXT);
                    int nbl = dump.count();
                    for (int i = 0; i < nbl; ++i) {
                        SKGTRACE << dump.at(i) << Qt::endl;
                    }
                }

                // Rollback create transactions
                if (transactionToRollback) {
                    doc->endTransaction(false);
                }

                if (forecastMode == 1 || forecastMode == 4) {
                    // Compute nb date in futur
                    if (nbLines != 0) {
                        QStringList line1 = table.at(0);
                        int nbCol = line1.count();
                        for (int i = 0; i < nbCol; ++i) {
                            QString newTitle = line1.at(i);
                            if (newTitle.endsWith(QStringLiteral("999"))) {
                                newTitle = newTitle.left(newTitle.size() - 3);
                                line1.replace(i, newTitle);
                                ++nbVirtualColumn;
                            }
                        }
                        table.replace(0, line1);
                    }
                }

                IFOK(err)
                {
                    // Update title
                    if (nbLines != 0) {
                        // Change title
                        QStringList line1 = table.at(0);
                        if (!line1.isEmpty()) {
                            line1.replace(0, title);
                            table.replace(0, line1);
                        }
                    }

                    // Compute forecast on average
                    if ((nbLines != 0) && (forecastMode == 2 || forecastMode == 3)) {
                        QStringList newLine = table.at(0);
                        int nbVals = newLine.count() - 1;
                        int percent = ui.kForecastValue->value();
                        if (nbVals >= 3 && percent > 0) {
                            // Compute nb value to add
                            nbVirtualColumn = percent * nbVals / FORECASTMAX;

                            // Build header
                            newLine.reserve(newLine.count() + nbVirtualColumn);
                            for (int i = 0; i < nbVirtualColumn; ++i) {
                                newLine.push_back(i18nc("Noun", "N %1", (i + 1)));
                            }
                            table.replace(0, newLine);

                            // Build values
                            for (int j = 1; j < nbLines; ++j) {
                                QStringList newLineTmp = table.at(j);
                                newLineTmp.reserve(newLineTmp.count() + nbVirtualColumn);
                                for (int i = 0; i < nbVirtualColumn; ++i) {
                                    // Moving average
                                    double nv = 0;
                                    double weight = 0;
                                    for (int k = 0; k < nbVirtualColumn; ++k) {
                                        if (forecastMode == 2) {
                                            // Simple mode
                                            nv += SKGServices::stringToDouble(newLineTmp.at(nbVals + i - k));
                                            ++weight;
                                        } else {
                                            // Weighted mode
                                            nv += (nbVirtualColumn - k) * SKGServices::stringToDouble(newLineTmp.at(nbVals + i - k));
                                            weight += nbVirtualColumn - k;
                                        }
                                    }
                                    newLineTmp.push_back(SKGServices::doubleToString(nv / weight));
                                }
                                table.replace(j, newLineTmp);
                            }
                        }
                    }

                    // Create grouped by lines table
                    int nbLevelLineMax = 0;
                    {
                        SKGStringListList groupedByLineTable = table;
                        {
                            int nbCols = -1;
                            QString previousLineName = QStringLiteral("###");
                            if (!groupedByLineTable.isEmpty()) {
                                nbCols = groupedByLineTable.at(0).count();
                            }
                            for (int i = 1; (nbCols != 0) && i < groupedByLineTable.count(); ++i) { // Dynamically modified
                                QStringList line2 = groupedByLineTable.at(i);
                                QString val = line2.at(0);

                                // Rebuild val for the number of level
                                QStringList vals = val.split(OBJECTSEPARATOR);
                                int nbvals = vals.count();
                                if (nbvals > m_nbLevelLines + 1) {
                                    // Rebuild val
                                    val = QString();
                                    for (int k = 0; k <= m_nbLevelLines; ++k) {
                                        val += vals[k];
                                        if (k != m_nbLevelLines) {
                                            val += OBJECTSEPARATOR;
                                        }
                                    }
                                }
                                nbLevelLineMax = qMax(nbLevelLineMax, nbvals - 1);

                                if (val == previousLineName) {
                                    // Current line is merged with previous one
                                    QStringList newLine;
                                    newLine.reserve(nbCols);
                                    newLine.push_back(val);
                                    for (int k = 1; k < nbCols; ++k) {
                                        const QString &valstring1 = line2.at(k);
                                        QString valstring2 = groupedByLineTable.at(i - 1).at(k);

                                        if (!valstring1.isEmpty() || !valstring2.isEmpty()) {
                                            double sum2 = 0;
                                            if (!valstring1.isEmpty()) {
                                                sum2 += SKGServices::stringToDouble(valstring1);
                                            }
                                            if (!valstring2.isEmpty()) {
                                                sum2 += SKGServices::stringToDouble(valstring2);
                                            }
                                            newLine.push_back(SKGServices::doubleToString(sum2));
                                        } else {
                                            newLine.push_back(QString());
                                        }
                                    }

                                    groupedByLineTable.replace(i - 1, newLine);

                                    // Remove current line
                                    groupedByLineTable.removeAt(i);
                                    --i;
                                } else {
                                    // Current line is just modified
                                    QStringList newLine;
                                    newLine.reserve(nbCols);
                                    newLine.push_back(val);
                                    for (int k = 1; k < nbCols; ++k) {
                                        newLine.push_back(line2.at(k));
                                    }
                                    groupedByLineTable.replace(i, newLine);

                                    previousLineName = std::move(val);
                                }
                            }
                        }
                        table = std::move(groupedByLineTable);
                    }

                    // Create grouped by columns table
                    int nbLevelColMax = 0;
                    {
                        SKGStringListList groupedByColTable = table;
                        int nbLines2 = groupedByColTable.count();
                        if (nbLines2 != 0) {
                            QString previousColumnName = QStringLiteral("###");
                            for (int i = 1; (nbLines2 != 0) && i < groupedByColTable.at(0).count(); ++i) { // Dynamically modified
                                QString val = groupedByColTable.at(0).at(i);

                                // Rebuild val for the number of level
                                QStringList vals = val.split(OBJECTSEPARATOR);
                                int nbvals = vals.count();
                                if (nbvals > m_nbLevelColumns + 1) {
                                    // Rebuild val
                                    val = QString();
                                    for (int k = 0; k <= m_nbLevelColumns; ++k) {
                                        val += vals[k];
                                        if (k != m_nbLevelColumns) {
                                            val += OBJECTSEPARATOR;
                                        }
                                    }
                                }
                                nbLevelColMax = qMax(nbLevelColMax, nbvals - 1);

                                if (val == previousColumnName) {
                                    // Current column is merged with previous one
                                    QStringList newLine = groupedByColTable.at(0);
                                    newLine.removeAt(i);
                                    groupedByColTable.replace(0, newLine);

                                    for (int k = 1; k < nbLines2; ++k) {
                                        newLine = groupedByColTable.at(k);
                                        QString valstring1 = newLine.at(i);
                                        QString valstring2 = newLine.at(i - 1);

                                        if (!valstring1.isEmpty() || !valstring2.isEmpty()) {
                                            double sum2 = 0;
                                            if (!valstring1.isEmpty()) {
                                                sum2 += SKGServices::stringToDouble(valstring1);
                                            }
                                            if (!valstring2.isEmpty()) {
                                                sum2 += SKGServices::stringToDouble(valstring2);
                                            }
                                            newLine.replace(i - 1, SKGServices::doubleToString(sum2));
                                        } else {
                                            newLine.removeAt(i - 1);
                                        }
                                        newLine.removeAt(i);
                                        groupedByColTable.replace(k, newLine);
                                    }

                                    // Remove current line
                                    --i;
                                } else {
                                    // Current column is just modified
                                    QStringList newLine = groupedByColTable.at(0);
                                    newLine.replace(i, val);
                                    groupedByColTable.replace(0, newLine);

                                    previousColumnName = std::move(val);
                                }
                            }
                        }
                        table = std::move(groupedByColTable);
                    }

                    // Initialize parameters
                    SKGServices::SKGUnitInfo primaryUnit = doc->getPrimaryUnit();
                    SKGServices::SKGUnitInfo secondaryUnit = doc->getSecondaryUnit();

                    SKGTableWithGraph::DisplayAdditionalFlag dmode = SKGTableWithGraph::NONE;
                    if (m_attsForColumns.at(col).startsWith(QStringLiteral("d_"))) {
                        dmode |= SKGTableWithGraph::AVERAGE | SKGTableWithGraph::LIMITS;
                    }

                    // Create history (must be done after groups)
                    if (modeHistory) {
                        table = SKGServices::getHistorizedTable(table);
                    } else {
                        // Add SUM if not in HISTORY mode
                        dmode |= SKGTableWithGraph::SUM;
                    }

                    // Mode percent
                    bool modePercent = (mode == 2 || mode == 4 || mode == 5 || mode == 6);
                    if (modePercent) {
                        primaryUnit.Symbol = '%';
                        SKGServices::SKGUnitInfo empty;
                        empty.Value = 0.0;
                        empty.NbDecimal = 2;
                        secondaryUnit = empty;

                        table = SKGServices::getPercentTable(table, (mode == 2 || mode == 5), (mode == 5 || mode == 6));
                    }

                    // Mode base 100
                    if (mode == 7 || mode == 8) {
                        table = SKGServices::getBase100Table(table);
                    }

                    // Mode Count
                    if (mode == 3) {
                        primaryUnit.Symbol = ' ';
                        SKGServices::SKGUnitInfo empty;
                        empty.Value = 0.0;
                        empty.NbDecimal = 2;
                        secondaryUnit = empty;
                    }

                    // Set data
                    ui.kTableWithGraph->setData(table, primaryUnit, secondaryUnit, dmode, nbVirtualColumn);

                    // Enable/Disable buttons
                    m_nbLevelLines = qMin(nbLevelLineMax, m_nbLevelLines);
                    ui.kLineDown->setEnabled(m_nbLevelLines > 0);
                    ui.kLineUp->setEnabled(m_nbLevelLines < nbLevelLineMax);

                    m_nbLevelColumns = qMin(nbLevelColMax, m_nbLevelColumns);
                    ui.kColDown->setEnabled(m_nbLevelColumns > 0);
                    ui.kColUp->setEnabled(m_nbLevelColumns < nbLevelColMax);
                }
            }
            QApplication::restoreOverrideCursor();
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGReportPluginWidget::setSettings()
{
    ui.kTableWithGraph->setAxisColor(skgreport_settings::axisColor());
    ui.kTableWithGraph->setGridColor(skgreport_settings::gridColor());
    ui.kTableWithGraph->setMinColor(skgreport_settings::minColor());
    ui.kTableWithGraph->setMaxColor(skgreport_settings::maxColor());
    ui.kTableWithGraph->setParetoColor(skgreport_settings::paretoColor());
    ui.kTableWithGraph->setAverageColor(skgreport_settings::averageColor());
    ui.kTableWithGraph->setTendencyColor(skgreport_settings::tendencyColor());
    ui.kTableWithGraph->setBackgroundColor(skgreport_settings::backgroundColor());
    ui.kTableWithGraph->setTextColor(skgreport_settings::textColor());
    ui.kTableWithGraph->setOutlineColor(skgreport_settings::outlineColor());

    ui.kTableWithGraph->setAntialiasing(skgreport_settings::antialiasing());

    ui.kTableWithGraph->redrawGraphDelayed();
}

QString SKGReportPluginWidget::getWhereClauseForProperty(const QString &iProperty) const
{
    QString propertyName = iProperty.right(iProperty.length() - 2);
    QString check = getDocument()->getDisplay(QStringLiteral("t_category"));
    if (iProperty.startsWith(check)) {
        propertyName = iProperty.right(iProperty.length() - check.length() - 1 - 2);
        return "(SELECT t_value FROM parameters WHERE t_uuid_parent=v_suboperation_consolidated.i_IDCATEGORY||'-category' AND t_name='" % propertyName % "')";
    }
    check = getDocument()->getDisplay(QStringLiteral("t_account"));
    if (iProperty.startsWith(check)) {
        propertyName = iProperty.right(iProperty.length() - check.length() - 1 - 2);
        return "(SELECT t_value FROM parameters WHERE t_uuid_parent=v_suboperation_consolidated.rd_account_id||'-account' AND t_name='" % propertyName % "')";
    }
    check = getDocument()->getDisplay(QStringLiteral("t_payee"));
    if (iProperty.startsWith(check)) {
        propertyName = iProperty.right(iProperty.length() - check.length() - 1 - 2);
        return "(SELECT t_value FROM parameters WHERE t_uuid_parent=v_suboperation_consolidated.r_payee_id||'-payee' AND t_name='" % propertyName % "')";
    }
    check = getDocument()->getDisplay(QStringLiteral("t_unit"));
    if (iProperty.startsWith(check)) {
        propertyName = iProperty.right(iProperty.length() - check.length() - 1 - 2);
        return "(SELECT t_value FROM parameters WHERE t_uuid_parent=v_suboperation_consolidated.rc_unit_id||'-unit' AND t_name='" % propertyName % "')";
    }
    return "IFNULL((SELECT t_value FROM parameters WHERE t_uuid_parent=v_suboperation_consolidated.id||'-suboperation' AND t_name='" % propertyName
        % "'), IFNULL((SELECT t_value FROM parameters WHERE t_uuid_parent=v_suboperation_consolidated.i_OPID||'-operation' AND t_name='" % propertyName
        % "'), '#NULL#'))";
}
