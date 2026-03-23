/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin for monthly report.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgmonthlypluginwidget.h"

#include <kaboutdata.h>
#include <kzip.h>

#include <KNSWidgets/Dialog>
#include <qdesktopservices.h>
#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qmenu.h>
#include <qtextstream.h>
#ifdef SKG_WEBENGINE
#include <qwebenginepage.h>
#endif
#include <qdiriterator.h>
#include <qstandardpaths.h>
#include <qvalidator.h>

#include "skgmainpanel.h"
#include "skgreport.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGMonthlyPluginWidget::SKGMonthlyPluginWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGTabPage(iParent, iDocument)
    , m_upload(nullptr)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);
    ui.kMonth->setMode(SKGSimplePeriodEdit::PREVIOUS_PERIODS | SKGSimplePeriodEdit::PREVIOUS_MONTHS);
    ui.kDeleteTemplate->hide();

    ui.kRefresh->setIcon(SKGServices::fromTheme(QStringLiteral("view-refresh")));
    ui.kGetNewHotStuff->setIcon(SKGServices::fromTheme(QStringLiteral("get-hot-new-stuff")));
    ui.kDeleteTemplate->setIcon(SKGServices::fromTheme(QStringLiteral("edit-delete")));

    auto newValidator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^[\\w\\s]+$")), this);
    ui.kTemplate->setValidator(newValidator);

    connect(getDocument(), &SKGDocument::tableModified, this, &SKGMonthlyPluginWidget::dataModified, Qt::QueuedConnection);
    connect(ui.kMonth,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGMonthlyPluginWidget::onPeriodChanged,
            Qt::QueuedConnection);

    QStringList overlays;
    overlays.push_back(QStringLiteral("list-add"));
    m_upload = new QAction(SKGServices::fromTheme(QStringLiteral("get-hot-new-stuff"), overlays), i18n("Upload"), this);
    connect(m_upload, &QAction::triggered, this, &SKGMonthlyPluginWidget::onPutNewHotStuff);

    auto menu = new QMenu(this);
    menu->addAction(m_upload);
    ui.kGetNewHotStuff->setMenu(menu);

    connect(ui.kDeleteTemplate, &QToolButton::clicked, this, &SKGMonthlyPluginWidget::onDeleteTemplate);
    connect(ui.kTemplate, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::returnPressed), this, &SKGMonthlyPluginWidget::onAddTemplate);
    connect(ui.kTemplate, &SKGComboBox::editTextChanged, this, &SKGMonthlyPluginWidget::onTemplateChanged);
    connect(ui.kGetNewHotStuff, &QToolButton::clicked, this, &SKGMonthlyPluginWidget::onGetNewHotStuff);
    connect(ui.kRefresh, &QToolButton::clicked, this, &SKGMonthlyPluginWidget::onPeriodChanged);

    // Refresh
    fillTemplateList();
    dataModified(QString(), 0);
}

SKGMonthlyPluginWidget::~SKGMonthlyPluginWidget(){SKGTRACEINFUNC(1)}

QString SKGMonthlyPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    // Get state
    root.setAttribute(QStringLiteral("month"), ui.kMonth->text());
    root.setAttribute(QStringLiteral("template"), ui.kTemplate->text());
    root.setAttribute(QStringLiteral("web"), ui.kWebView->getState());

    return doc.toString();
}

void SKGMonthlyPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    // Set state
    QString webS = root.attribute(QStringLiteral("web"));
    if (!webS.isEmpty()) {
        ui.kWebView->setState(webS);
    }

    QString templat = root.attribute(QStringLiteral("template"));
    if (!templat.isEmpty()) {
        bool p = ui.kTemplate->blockSignals(true);
        ui.kTemplate->setText(templat);
        ui.kTemplate->blockSignals(p);
        onTemplateChanged();
    }

    QString month = root.attribute(QStringLiteral("month"));
    if (!month.isEmpty()) {
        ui.kMonth->setText(month);
    }
    onPeriodChanged();
}

QString SKGMonthlyPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGMONTHLY_DEFAULT_PARAMETERS");
}

QWidget *SKGMonthlyPluginWidget::mainWidget()
{
    return ui.kWebView;
}

void SKGMonthlyPluginWidget::fillTemplateList()
{
    disconnect(ui.kTemplate,
               static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
               this,
               &SKGMonthlyPluginWidget::onPeriodChanged);
    // Get previous selected item
    QString current = ui.kTemplate->text();

    // Fill
    ui.kTemplate->clear();
    const auto dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                KAboutData::applicationData().componentName() % "/html",
                                                QStandardPaths::LocateDirectory);
    for (const auto &dir : dirs) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.txt"));
        while (it.hasNext()) {
            QString file = it.next();

            QFileInfo f(file);
            QString file2 = f.completeBaseName();
            if (!ui.kTemplate->contains(file2) && file2 != QStringLiteral("main")) {
                ui.kTemplate->addItem(file2, file);
            }
        }
    }

    // Set previous selected itemData
    if (!current.isEmpty() && ui.kTemplate->contains(current)) {
        ui.kTemplate->setCurrentItem(current);
    }
    connect(ui.kTemplate,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGMonthlyPluginWidget::onPeriodChanged,
            Qt::QueuedConnection);
}

void SKGMonthlyPluginWidget::onAddTemplate()
{
    QString templat = ui.kTemplate->text().trimmed();
    QString templateDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/' + KAboutData::applicationData().componentName();
    QString templatFileName = templateDir % "/html/" % templat % ".txt";
    QStringList templates;
    const auto dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                KAboutData::applicationData().componentName() % "/html",
                                                QStandardPaths::LocateDirectory);
    for (const auto &dir : dirs) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.txt"));
        while (it.hasNext()) {
            templates.append(it.next());
        }
    }

    if (!templat.isEmpty() && (!templates.contains(templatFileName) || QFileInfo(templatFileName).isWritable())) {
        SKGError err;
        if (!templates.contains(templatFileName)) {
            // Create the new template
            QString source = QStandardPaths::locate(QStandardPaths::GenericDataLocation, KAboutData::applicationData().componentName() % "/html/tutorial.txt");
            QDir(templateDir).mkpath(QStringLiteral("html"));
            if (SKGServices::upload(QUrl::fromLocalFile(source), QUrl::fromLocalFile(templatFileName))) {
                err.setReturnCode(ERR_FAIL).setMessage(i18nc("An error message", "Impossible to copy file from '%1' to '%2'", source, templatFileName));
            } else {
                fillTemplateList();
            }
        }

        // Open the created or already existing file
        QDesktopServices::openUrl(QUrl::fromLocalFile(templatFileName));

        onTemplateChanged();

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGMonthlyPluginWidget::onDeleteTemplate()
{
    QString templat = ui.kTemplate->text().trimmed();
    QString templatFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/'
        + KAboutData::applicationData().componentName() % "/html/" % templat % ".txt";
    if (!templat.isEmpty()) {
        // This is a new source
        SKGError err;

        // Delete the file
        QFile file(templatFileName);
        if (!file.remove()) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Deletion of '%1' failed", templatFileName));
        }

        IFOK(err) ui.kTemplate->removeItem(ui.kTemplate->findText(templat));

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGMonthlyPluginWidget::onTemplateChanged()
{
    QString templat = ui.kTemplate->text().trimmed();
    QString templatFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/'
        + KAboutData::applicationData().componentName() % "/html/" % templat % ".txt";
    bool local = !templat.isEmpty() && QFileInfo(templatFileName).isWritable();
    ui.kDeleteTemplate->setVisible(local);
    m_upload->setEnabled(local);
}

void SKGMonthlyPluginWidget::dataModified(const QString &iTableName, int iIdTransaction)
{
    SKGTRACEINFUNC(1)
    Q_UNUSED(iIdTransaction)

    QSqlDatabase *db = getDocument()->getMainDatabase();
    setEnabled(db != nullptr);
    // TODO(Stephane MANKOWSKI): v_operation_display must be generic
    if (db != nullptr && (iTableName == QStringLiteral("v_operation_display") || iTableName.isEmpty())) {
        // Fill combo

        QDate date = QDate::currentDate();
        QStringList list;
        // TODO(Stephane MANKOWSKI): v_operation_display must be generic
        getDocument()->getDistinctValues(QStringLiteral("v_operation_display"),
                                         QStringLiteral("MIN(d_DATEMONTH)"),
                                         QStringLiteral("d_date<=CURRENT_DATE"),
                                         list);
        if (!list.isEmpty()) {
            if (!list[0].isEmpty()) {
                date = SKGServices::periodToDate(list[0]);
            }
        }

        ui.kMonth->setFirstDate(date);
        ui.kRefresh->setEnabled(!list.isEmpty());
    }
}

QString SKGMonthlyPluginWidget::getPeriod()
{
    return ui.kMonth->period();
}

void SKGMonthlyPluginWidget::onPeriodChanged()
{
    SKGTRACEINFUNC(1)
    QString month = getPeriod();
    if (!month.isEmpty()) {
        // Display report
        QString htmlReport = getDocument()->getParameter("SKG_MONTHLY_REPORT_" % month);
        if (htmlReport.isEmpty() || sender() == ui.kRefresh || sender() == ui.kTemplate) {
            SKGError err;
            SKGBEGINLIGHTTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Compute monthly report for '%1'", month), err)
            htmlReport = getReport();
            err = getDocument()->setParameter("SKG_MONTHLY_REPORT_" % month, htmlReport);
        }

        // Display html report
#ifdef SKG_WEBENGINE
        ui.kWebView->page()->setHtml(htmlReport, QUrl(QStringLiteral("file://")));
#endif
#if !defined(SKG_WEBENGINE)
        ui.kWebView->setHtml(htmlReport);
#endif
    }
}

void SKGMonthlyPluginWidget::onGetNewHotStuff()
{
    QPointer<KNSWidgets::Dialog> dialog = new KNSWidgets::Dialog(KAboutData::applicationData().componentName() % "_monthly.knsrc", this);
    dialog->exec();

    fillTemplateList();
}

void SKGMonthlyPluginWidget::onPutNewHotStuff()
{
    QString templat = ui.kTemplate->text().trimmed();

    // Create zip file
    QString templatFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/'
        + KAboutData::applicationData().componentName() % "/html/" % templat % ".txt";
    QString templatHFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/'
        + KAboutData::applicationData().componentName() % "/html/" % templat % ".html";
    QString zipFileName = QDir::tempPath() % "/" % templat % ".zip";
    KZip zip(zipFileName);
    if (zip.open(QIODevice::WriteOnly)) {
        zip.addLocalFile(templatFileName, templat % ".txt");
        if (QFile(templatHFileName).exists()) {
            zip.addLocalFile(templatHFileName, templat % ".html");
        }
        zip.close();

        // Create screen shots
        QString preview1 = QDir::tempPath() % "/" % templat % "_preview1.png";
        QString preview2 = QDir::tempPath() % "/" % templat % "_preview2.png";
        QString preview3 = QDir::tempPath() % "/" % templat % "_preview3.png";
#ifdef SKG_WEBENGINE
        // TODO(SMI): QWebEngine
#endif
#if !defined(SKG_WEBENGINE)
        // TODO(SMI): QTextBrowser
#endif
        // Open dialog
        SKGMainPanel::getMainPanel()->displayMessage(
            i18nc("Upload message",
                  "The package is ready. You can find it here %1 and 3 screen capture there %2, %3 , %4. You can now upload it manually.",
                  zipFileName,
                  preview1,
                  preview2,
                  preview3));
    }
}

QString SKGMonthlyPluginWidget::getReport()
{
    QString html;
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Get period
    if (!getPeriod().isEmpty()) {
        SKGReport *rep = getDocument()->getReport();
        if (rep != nullptr) {
            rep->setPeriod(getPeriod());

            // Enrich with tips of the day
            rep->setTipsOfDay(SKGMainPanel::getMainPanel()->getTipsOfDay());

            err = SKGReport::getReportFromTemplate(rep, ui.kTemplate->itemData(ui.kTemplate->currentIndex()).toString(), html);

            delete rep;
        }
    }
    QApplication::restoreOverrideCursor();

    // status bar
    IFKO(err) html += err.getFullMessageWithHistorical();
    return html;
}
