/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin for debug.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdebugpluginwidget.h"

#include <qdom.h>
#include <qjsengine.h>

#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

const auto EXECUTE = QStringLiteral("EXECUTE");
const auto BENCHMARK = QStringLiteral("BENCHMARK");
const auto EXECUTE_MULTI = QStringLiteral("EXECUTE_MULTI");
const auto EXPLAIN = QStringLiteral("EXPLAIN");
const auto EXPLAIN_PLAN = QStringLiteral("EXPLAIN_PLAN");
const auto RUN_SCRIPT = QStringLiteral("RUN_SCRIPT");

SKGDebugPluginWidget::SKGDebugPluginWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGTabPage(iParent, iDocument)
{
    SKGTRACEINFUNC(10)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    // Set icons
    ui.kSQLPushButton->setIcon(SKGServices::fromTheme(QStringLiteral("system-run")));
    ui.kSQLTransactionPushButton->setIcon(SKGServices::fromTheme(QStringLiteral("system-run")));
    ui.kRefreshViewsAndIndexes->setIcon(SKGServices::fromTheme(QStringLiteral("view-refresh")));

    // Fill combo box
    ui.kExplainCmb->addItem(SKGServices::fromTheme(QStringLiteral("system-run")), i18nc("Execute an SQL query", "Execute"), EXECUTE);
    ui.kExplainCmb->addItem(SKGServices::fromTheme(QStringLiteral("system-run")), i18nc("Execute an SQL query (Benchmark)", "Execute benchmark"), BENCHMARK);
    ui.kExplainCmb->addItem(SKGServices::fromTheme(QStringLiteral("system-run")),
                            i18nc("Execute an SQL queries (one per line)", "Execute multi queries"),
                            EXECUTE_MULTI);
    ui.kExplainCmb->addItem(SKGServices::fromTheme(QStringLiteral("help-hint")), i18nc("Explain an SQL query", "Explain"), EXPLAIN);
    ui.kExplainCmb->addItem(SKGServices::fromTheme(QStringLiteral("games-hint")), i18nc("Explain the SQL query plan", "Explain query plan"), EXPLAIN_PLAN);
    ui.kExplainCmb->addItem(SKGServices::fromTheme(QStringLiteral("media-playback-start")),
                            i18nc("Execute script", "Execute script [%1]", "javascript"),
                            RUN_SCRIPT);
    ui.kInput->setVisible(false);

    // Set level trace
    ui.kTraceLevel->setValue(SKGTraces::SKGLevelTrace);

    // Set profiling mode
    ui.kEnableProfilingChk->setCheckState(SKGTraces::SKGPerfo ? Qt::Checked : Qt::Unchecked);

    // Init debug page
    QStringList tables;
    ui.kSQLInput->addItem(QStringLiteral("SELECT * FROM sqlite_master;"));
    iDocument->getDistinctValues(QStringLiteral("sqlite_master"), QStringLiteral("name"), QStringLiteral("type in ('table', 'view')"), tables);
    int nb = tables.count();
    for (int i = 0; i < nb; ++i) {
        ui.kSQLInput->addItem("SELECT * FROM " % tables.at(i) % u';');
    }
    ui.kSQLInput->addItem(QStringLiteral("ANALYZE;"));
    ui.kSQLInput->addItem(QStringLiteral("PRAGMA integrity_check;"));
    for (int i = 0; i < nb; ++i) {
        ui.kSQLInput->addItem("PRAGMA table_info(" % tables.at(i) % ");");
        ui.kSQLInput->addItem("PRAGMA index_list(" % tables.at(i) % ");");
    }

    iDocument->getDistinctValues(QStringLiteral("sqlite_master"), QStringLiteral("name"), QStringLiteral("type='index'"), tables);
    nb = tables.count();
    for (int i = 0; i < nb; ++i) {
        ui.kSQLInput->addItem("PRAGMA index_info(" % tables.at(i) % ");");
    }
    connect(ui.kTraceLevel, &QSlider::valueChanged, this, &SKGDebugPluginWidget::onTraceLevelModified);
    connect(ui.kEnableProfilingChk, &QCheckBox::checkStateChanged, this, &SKGDebugPluginWidget::onProfilingModeChanged);
    connect(ui.kExplainCmb, static_cast<void (SKGComboBox::*)(int)>(&SKGComboBox::currentIndexChanged), this, &SKGDebugPluginWidget::onModeChanged);
    connect(ui.kSQLPushButton, &QPushButton::clicked, this, &SKGDebugPluginWidget::onExecuteSqlOrder);
    connect(ui.kSQLTransactionPushButton, &QPushButton::clicked, this, &SKGDebugPluginWidget::onExecuteSqlOrderInTransaction);
    connect(ui.kRefreshViewsAndIndexes, &QPushButton::clicked, this, &SKGDebugPluginWidget::onRefreshViewsAndIndexes);
}

SKGDebugPluginWidget::~SKGDebugPluginWidget(){SKGTRACEINFUNC(10)}

QString SKGDebugPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("explain"), ui.kExplainCmb->currentIndex());
    root.setAttribute(QStringLiteral("enableProfiling"), ui.kEnableProfilingChk->checkState() == Qt::Checked ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("levelTraces"), ui.kTraceLevel->value());
    root.setAttribute(QStringLiteral("sqlOrder"), ui.kSQLInput->currentText());

    return doc.toString();
}

void SKGDebugPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString explain = root.attribute(QStringLiteral("explain"));
    QString enableProfiling = root.attribute(QStringLiteral("enableProfiling"));
    QString levelTraces = root.attribute(QStringLiteral("levelTraces"));
    QString sqlOrder = root.attribute(QStringLiteral("sqlOrder"));
    QString sqlResult = root.attribute(QStringLiteral("sqlResult"));

    if (!explain.isEmpty()) {
        ui.kExplainCmb->setCurrentIndex(SKGServices::stringToInt(explain == QStringLiteral("Y") ? QStringLiteral("1") : std::move(explain)));
    }
    if (!enableProfiling.isEmpty()) {
        ui.kEnableProfilingChk->setCheckState(enableProfiling == QStringLiteral("Y") ? Qt::Checked : Qt::Unchecked);
    }
    if (!levelTraces.isEmpty()) {
        ui.kTraceLevel->setValue(SKGServices::stringToInt(levelTraces));
    }
    ui.kSQLInput->setText(sqlOrder);
    ui.kSQLResult->setPlainText(sqlResult);
}

void SKGDebugPluginWidget::onExecuteSqlOrderInTransaction()
{
    onExecuteSqlOrder(true);
}

SKGError SKGDebugPluginWidget::executeSqlOrders(const QStringList &iSQL, QString &oOutput)
{
    SKGError err;
    int nb = iSQL.count();
    for (int i = 0; i < nb; ++i) {
        auto sql = iSQL[i].trimmed();
        if (!sql.isEmpty()) {
            oOutput += sql + '\n';

            QString oResult;
            double time = SKGServices::getMicroTime();
            err = getDocument()->dumpSelectSqliteOrder(sql, oResult);
            time = SKGServices::getMicroTime() - time;

            oOutput += oResult;
            oOutput += i18nc("Display the execution time needed by an SQL query", "\nExecution time: %1 ms", SKGServices::doubleToString(time));
            oOutput += QStringLiteral("\n\n");
        }
    }

    IFKO(err)
    {
        oOutput += err.getFullMessageWithHistorical();
    }
    return err;
}

void SKGDebugPluginWidget::onExecuteSqlOrder(bool iInTransaction)
{
    SKGTRACEINFUNC(10)
    SKGError err;
    auto exp = ui.kExplainCmb->currentData().toString();
    if (exp == RUN_SCRIPT) {
        // Script execution
        ui.kSQLResult->clear();
        QJSEngine myEngine;
        // skgresult.setText(skgdocument.getUniqueIdentifier())
        // skgerror=skgdocument.sendMessage(QStringLiteral("Hello"))
        // skgerror=skgdocument.sendMessage(QStringLiteral("Hello"))
        // skgmainpanel.closeAllOtherPages(skgmainpanel.currentPage())
        auto t = myEngine.globalObject();
        t.setProperty(QStringLiteral("skgresult"), myEngine.newQObject(ui.kSQLResult));
        t.setProperty(QStringLiteral("skgdocument"), myEngine.newQObject(getDocument()));
        // t.setProperty(QStringLiteral("skgerror"), myEngine.newQObject(&err));
        t.setProperty(QStringLiteral("skgmainpanel"), myEngine.newQObject(SKGMainPanel::getMainPanel()));

        // Finally execute the scripting code.
        myEngine.evaluate(ui.kInput->toPlainText());
    } else if (exp == EXECUTE_MULTI) {
        // SQL multi lines
        auto sqls = ui.kInput->toPlainText().split('\n');
        QString oResultGlobal;
        if (iInTransaction) {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Display an SQL command from the debug plugin", "SQL command from debug plugin"), err)
            IFOKDO(err, err = SKGDebugPluginWidget::executeSqlOrders(sqls, oResultGlobal))
        } else {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            err = SKGDebugPluginWidget::executeSqlOrders(sqls, oResultGlobal);
            QApplication::restoreOverrideCursor();
        }

        IFKO(err)
        {
            oResultGlobal += err.getFullMessageWithHistorical();
        }
        ui.kSQLResult->setPlainText(oResultGlobal);
    } else {
        // SQL execution
        QString text = ui.kSQLInput->currentText();
        if (exp == EXPLAIN) {
            text = "EXPLAIN " % text;
        } else if (exp == EXPLAIN_PLAN) {
            text = "EXPLAIN QUERY PLAN " % text;
        }
        QString oResult;
        double time = SKGServices::getMicroTime();
        int max_nb = exp == BENCHMARK ? 1000 : 1; // If benchmark, execute 10 times
        int max_time = 10000; // 10 seconds max
        int nb_run = 0;
        for (int i = 0; i < max_nb; ++i) {
            if (iInTransaction) {
                SKGBEGINTRANSACTION(*getDocument(), i18nc("Display an SQL command from the debug plugin", "SQL command from debug plugin"), err)
                IFOKDO(err, getDocument()->dumpSelectSqliteOrder(text, oResult))
            } else {
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                err = getDocument()->dumpSelectSqliteOrder(text, oResult);
                QApplication::restoreOverrideCursor();
            }
            nb_run++;
            if (SKGServices::getMicroTime() - time > max_time) {
                break; // Stop if we reach the max time
            }
        }
        time = (SKGServices::getMicroTime() - time) / nb_run;

        oResult +=
            i18nc("Display the execution time needed by an SQL query",
                  "\nExecution time: %1 ms %2",
                  SKGServices::doubleToString(time),
                  nb_run > 1 ? "(" % i18nc("Display the average execution time of an SQL query", "Average for %1 runs", nb_run) % ")" : QStringLiteral(""));

        IFOK(err)
        {
            ui.kSQLResult->setPlainText(oResult);
        }
        else
        {
            ui.kSQLResult->setPlainText(err.getFullMessageWithHistorical());
        }
    }
}

void SKGDebugPluginWidget::onTraceLevelModified()
{
    SKGTRACEINFUNC(10)
    SKGTraces::SKGLevelTrace = ui.kTraceLevel->value();
}

void SKGDebugPluginWidget::onModeChanged()
{
    SKGTRACEINFUNC(10)
    auto exp = ui.kExplainCmb->currentData().toString();
    ui.kInput->setVisible(exp == RUN_SCRIPT || exp == EXECUTE_MULTI);
    ui.kSQLInput->setVisible(!ui.kInput->isVisible());
}

void SKGDebugPluginWidget::onProfilingModeChanged()
{
    SKGTRACEINFUNC(10)
    SKGTraces::SKGPerfo = (ui.kEnableProfilingChk->checkState() == Qt::Checked);
}

void SKGDebugPluginWidget::onRefreshViewsAndIndexes()
{
    SKGTRACEINFUNC(10)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    SKGError err;
    err = getDocument()->refreshViewsIndexesAndTriggers();
    IFKO(err)
    {
        ui.kSQLResult->setPlainText(err.getFullMessageWithHistorical());
    }
    QApplication::restoreOverrideCursor();
}
