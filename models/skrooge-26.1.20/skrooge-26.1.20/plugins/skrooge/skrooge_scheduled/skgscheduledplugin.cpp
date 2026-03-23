/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to manage scheduled transactions
 *
 * @author Stephane MANKOWSKI
 */
#include "skgscheduledplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>

#include <qstandardpaths.h>

#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgmenuitem.h"
#include "skgrecurrentoperationobject.h"
#include "skgscheduled_settings.h"
#include "skgscheduledboardwidget.h"
#include "skgscheduledpluginwidget.h"
#include "skgsuboperationobject.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGScheduledPlugin, "metadata.json")

SKGScheduledPlugin::SKGScheduledPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
    , m_counterAdvice(0)
    , m_assignScheduleMenu(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGScheduledPlugin::~SKGScheduledPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGScheduledPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skrooge_scheduled"), title());
    setXMLFile(QStringLiteral("skrooge_scheduled.rc"));

    QStringList listOperation;
    listOperation << QStringLiteral("operation");

    // Create yours actions here
    auto actScheduleOperation = new QAction(SKGServices::fromTheme(icon()), i18nc("Verb, create a scheduled transaction", "Schedule"), this);
    connect(actScheduleOperation, &QAction::triggered, this, &SKGScheduledPlugin::onScheduleOperation);
    actionCollection()->setDefaultShortcut(actScheduleOperation, Qt::CTRL | Qt::Key_I);
    registerGlobalAction(QStringLiteral("schedule_operation"), actScheduleOperation, QStringList() << QStringLiteral("operation"), 1, -1, 410);

    auto actSkipScheduledOperation =
        new QAction(SKGServices::fromTheme(QStringLiteral("nextuntranslated")), i18nc("Verb, skip scheduled transactions", "Skip"), this);
    connect(actSkipScheduledOperation, &QAction::triggered, this, &SKGScheduledPlugin::onSkipScheduledOperations);
    registerGlobalAction(QStringLiteral("skip_scheduled_operations"),
                         actSkipScheduledOperation,
                         QStringList() << QStringLiteral("recurrentoperation"),
                         1,
                         -1,
                         410);

    auto actAssignScheduleAction =
        new QAction(SKGServices::fromTheme(QStringLiteral("edit-guides")), i18nc("Verb, action to assign a schedule", "Assign schedule"), this);
    m_assignScheduleMenu = new QMenu();
    actAssignScheduleAction->setMenu(m_assignScheduleMenu);
    connect(m_assignScheduleMenu, &QMenu::aboutToShow, this, &SKGScheduledPlugin::onShowAssignScheduleMenu);
    actAssignScheduleAction->setData(1);
    registerGlobalAction(QStringLiteral("edit_assign_schedule"), actAssignScheduleAction, listOperation, 1, -1, 412);

    return true;
}

void SKGScheduledPlugin::onShowAssignScheduleMenu()
{
    if ((m_assignScheduleMenu != nullptr) && (m_currentBankDocument != nullptr)) {
        // Clean Menu
        QMenu *m = m_assignScheduleMenu;
        m->clear();

        // Search templates
        SKGStringListList listTmp;
        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT t_displayname, id, i_nb_times FROM v_recurrentoperation_displayname ORDER BY i_nb_times DESC, t_displayname"),
            listTmp);

        // Get assigned schedules of selected transactions
        QSet<int> recurrentIds;
        if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
            SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
            int nb = selection.count();
            for (int i = 0; i < nb; ++i) {
                SKGOperationObject operationObj(selection.at(i));
                if (operationObj.isTemplate()) {
                    QAction *act = m->addAction(
                        i18nc("A message that is shown in assign schedule menu for a template operation", "Cannot assign a schedule to a template"));
                    if (act != nullptr) {
                        act->setEnabled(false);
                    }
                    return;
                }
                recurrentIds.insert(operationObj.getRecurrentOperation());
            }
        }

        const auto fontDisabledScheduleColor =
            KSharedConfig::openConfig()->group(QStringLiteral("skrooge_scheduled")).readEntry("fontFutureColor", QColor(Qt::gray));
        const auto addAction = [this, recurrentIds, fontDisabledScheduleColor](QMenu *menu, const QString &text, const QString &recurrentId, bool isActive) {
            auto act = new QWidgetAction(menu);
            auto menuItem = new SKGMenuitem;
            menuItem->setText(text);
            menuItem->setIcon(SKGServices::fromTheme(QStringLiteral("edit-guides")));
            if (!isActive) {
                menuItem->setColor(fontDisabledScheduleColor);
            }
            menuItem->setIsBold(recurrentIds.contains(SKGServices::stringToInt(recurrentId)));
            act->setDefaultWidget(menuItem);
            act->setData(recurrentId);
            connect(act, &QAction::triggered, this, &SKGScheduledPlugin::onAssignScheduleMenu);
            menu->addAction(act);
        };

        // Add the "unassign" item
        addAction(m, i18nc("Noun, item to unassign a schedule", "None"), QStringLiteral("0"), true);

        // Build menus
        bool hasActive = false;
        bool hasInactive = false;
        int count = 0;
        int nb = listTmp.count();
        for (int i = 1; i < nb; ++i) {
            // Add more sub menu
            if (count == 8) {
                m = m->addMenu(i18nc("More items in a menu", "More"));
                count = 0;
            }
            count++;

            const bool isScheduleActive = SKGServices::stringToInt(listTmp.at(i).at(2)) > 0;
            if (isScheduleActive && !hasActive) {
                m->addSection(i18nc("Adjective, a header for active schedule items that should follow", "active"));
                hasActive = true;
            }
            if (!isScheduleActive && !hasInactive) {
                m->addSection(i18nc("Adjective, a header for non-active schedule items that should follow", "inactive"));
                hasInactive = true;
            }

            addAction(m, listTmp.at(i).at(0), listTmp.at(i).at(1), isScheduleActive);
        }
    }
}

void SKGScheduledPlugin::onAssignScheduleMenu()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        // Get template
        const auto id = SKGServices::stringToInt(act->data().toString());

        // Get Selection
        if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
            SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
            int nb = selection.count();
            {
                SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Assign schedule"), err, nb)
                for (int i = 0; !err && i < nb; ++i) {
                    SKGOperationObject operationObj(selection.at(i));
                    IFOKDO(err, operationObj.setRecurrentOperation(id))

                    IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
                }
            }

            // status bar
            IFOK(err)
            {
                err = SKGError(0, i18nc("Successful message after an user action", "Schedule assigned."));
            }
            else
            {
                err.addError(ERR_FAIL, i18nc("Error message", "Schedule assignment failed"));
            }
        }
        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}
void SKGScheduledPlugin::refresh()
{
    SKGTRACEINFUNC(10)
    // Automatic insert
    if ((m_currentBankDocument != nullptr) && m_currentBankDocument->getMainDatabase() != nullptr) {
        QString doc_id = m_currentBankDocument->getUniqueIdentifier();
        if (m_docUniqueIdentifier != doc_id && m_currentBankDocument->getParameter(QStringLiteral("SKG_DB_BANK_VERSION")) >= QStringLiteral("0.5")) {
            m_docUniqueIdentifier = std::move(doc_id);

            SKGError err;
            // Read Setting
            bool check_on_open = skgscheduled_settings::check_on_open();

            if (check_on_open) {
                SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Insert recurrent transactions"), err)
                int nbi = 0;
                err = SKGRecurrentOperationObject::process(m_currentBankDocument, nbi);
            }
            // Display error
            SKGMainPanel::displayErrorMessage(err);
        }
    }
}

int SKGScheduledPlugin::getNbDashboardWidgets()
{
    return 1;
}

QString SKGScheduledPlugin::getDashboardWidgetTitle(int iIndex)
{
    Q_UNUSED(iIndex)
    return i18nc("Noun, the title of a section", "Scheduled transactions");
}

SKGBoardWidget *SKGScheduledPlugin::getDashboardWidget(int iIndex)
{
    Q_UNUSED(iIndex)
    return new SKGScheduledBoardWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

SKGTabPage *SKGScheduledPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGScheduledPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QWidget *SKGScheduledPlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    auto w = new QWidget();
    ui.setupUi(w);

    ui.kcfg_convert_to_from_template->hide();
    clearConvertToFromTemplateSetting();

    connect(skgscheduled_settings::self(), &skgscheduled_settings::configChanged, ui.kcfg_convert_to_from_template, [this]() {
        ui.kcfg_convert_to_from_template->setChecked(false);
        ui.kcfg_convert_to_from_template->hide();
    });
    connect(ui.kcfg_create_template, &QCheckBox::toggled, this, [this](bool isChecked) {
        ui.kcfg_convert_to_from_template->setChecked(false);
        const bool isChanged = (isChecked != skgscheduled_settings::create_template());
        ui.kcfg_convert_to_from_template->setVisible(isChanged);
    });
    connect(ui.kcfg_remind_me, &QCheckBox::toggled, ui.kcfg_remind_me_days, &QSpinBox::setEnabled);
    connect(ui.kcfg_remind_me, &QCheckBox::toggled, ui.label_3, &QSpinBox::setEnabled);
    connect(ui.kcfg_nb_times, &QCheckBox::toggled, ui.kcfg_nb_times_val, &QSpinBox::setEnabled);
    connect(ui.kcfg_auto_write, &QCheckBox::toggled, ui.kcfg_auto_write_days, &QSpinBox::setEnabled);
    connect(ui.kcfg_auto_write, &QCheckBox::toggled, ui.label_4, &QSpinBox::setEnabled);
    return w;
}

KConfigSkeleton *SKGScheduledPlugin::getPreferenceSkeleton()
{
    return skgscheduled_settings::self();
}

QString SKGScheduledPlugin::title() const
{
    return i18nc("Noun", "Scheduled transactions");
}

QString SKGScheduledPlugin::icon() const
{
    return QStringLiteral("view-calendar-day");
}

QString SKGScheduledPlugin::toolTip() const
{
    return i18nc("Noun", "Transactions scheduled management");
}

int SKGScheduledPlugin::getOrder() const
{
    return 20;
}

QStringList SKGScheduledPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… you can <a href=\"skg://skrooge_scheduled_plugin\">schedule</a> transactions or templates.</p>"));
    return output;
}

bool SKGScheduledPlugin::isInPagesChooser() const
{
    return true;
}

SKGError SKGScheduledPlugin::savePreferences() const
{
    SKGError err;
    if (m_currentBankDocument != nullptr) {
        // Read Setting
        if (skgscheduled_settings::convert_to_from_template()) {
            const auto newTemplateMode = skgscheduled_settings::create_template();
            SKGObjectBase::SKGListSKGObjectBase recurrents;
            err = m_currentBankDocument->getObjects(QStringLiteral("v_recurrentoperation"),
                                                    QStringLiteral("(select count(1) from operation where operation.id=rd_operation_id and t_template='%1')=1")
                                                        .arg(newTemplateMode ? QStringLiteral("N") : QStringLiteral("Y")),
                                                    recurrents);
            int nb = recurrents.count();
            if (nb != 0) {
                SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Conversion schedule"), err, nb)
                for (int i = 0; !err && i < nb; ++i) {
                    // Converting an existing schedule
                    SKGRecurrentOperationObject recOp(recurrents.at(i));
                    IFOKDO(err, recOp.setTemplate(newTemplateMode))
                    IFOKDO(err, recOp.save(true, false))
                    IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
                }
                IFOK(err) m_currentBankDocument->sendMessage(i18nc("An information message", "All scheduled transactions have been converted in template"));
            }

            clearConvertToFromTemplateSetting();
        }
    }
    return err;
}

void SKGScheduledPlugin::clearConvertToFromTemplateSetting() const
{
    // This setting should be enabled only manually and we reset it after conversion is done
    skgscheduled_settings::setConvert_to_from_template(false);
    skgscheduled_settings::self()->save();
}

SKGError SKGScheduledPlugin::scheduleOperation(const SKGOperationObject &iOperation, SKGRecurrentOperationObject &oRecurrent) const
{
    SKGError err;
    SKGOperationObject operationObjDuplicate = iOperation;
    bool isTemplate = operationObjDuplicate.isTemplate();

    SKGOperationObject operationObjOrig;
    if (!isTemplate && skgscheduled_settings::create_template()) {
        // The selected transaction is not a template and settings is set to create one
        operationObjOrig = operationObjDuplicate;
        IFOKDO(err, operationObjOrig.duplicate(operationObjDuplicate, operationObjOrig.getDate(), true))
        IFOK(err) m_currentBankDocument->sendMessage(i18nc("An information message", "A template has been created"), SKGDocument::Positive);
    }

    SKGRecurrentOperationObject recOp;
    err = operationObjDuplicate.addRecurrentOperation(recOp);
    IFOKDO(err, recOp.warnEnabled(skgscheduled_settings::remind_me()))
    IFOKDO(err, recOp.setWarnDays(skgscheduled_settings::remind_me_days()))
    IFOKDO(err, recOp.autoWriteEnabled(skgscheduled_settings::auto_write()))
    IFOKDO(err, recOp.setAutoWriteDays(skgscheduled_settings::auto_write_days()))
    IFOKDO(err, recOp.timeLimit(skgscheduled_settings::nb_times()))
    IFOKDO(err, recOp.setTimeLimit(skgscheduled_settings::nb_times_val()))
    IFOKDO(err, recOp.setPeriodIncrement(skgscheduled_settings::once_every()))
    IFOKDO(err, recOp.setPeriodUnit(static_cast<SKGRecurrentOperationObject::PeriodUnit>(SKGServices::stringToInt(skgscheduled_settings::once_every_unit()))))
    if (!err && !isTemplate) {
        err = recOp.setDate(recOp.getNextDate());
    }
    IFOKDO(err, recOp.save())
    if (!isTemplate && skgscheduled_settings::create_template()) {
        IFOKDO(err, recOp.load())
        IFOKDO(err, operationObjOrig.setAttribute(QStringLiteral("r_recurrentoperation_id"), SKGServices::intToString(recOp.getID())))
        IFOKDO(err, operationObjOrig.save())
    }

    oRecurrent = recOp;

    return err;
}

void SKGScheduledPlugin::onScheduleOperation()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Get Selection
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        if ((nb != 0) && (m_currentBankDocument != nullptr)) {
            QStringList list;
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Transaction schedule"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject operationObj(selection.at(i));
                SKGRecurrentOperationObject rop;
                err = scheduleOperation(operationObj, rop);

                // Send message
                IFOKDO(err,
                       m_currentBankDocument->sendMessage(
                           i18nc("An information to the user", "The transaction '%1' has been scheduled", operationObj.getDisplayName()),
                           SKGDocument::Hidden))

                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
                list.push_back(operationObj.getUniqueID());
            }

            IFOK(err)
            {
                // Open the scheduled transaction
                SKGMainPanel::getMainPanel()->openPage("skg://skrooge_scheduled_plugin/?selection="
                                                       % SKGServices::encodeForUrl(SKGServices::stringsToCsv(list)));
            }
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Transaction scheduled.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Transaction schedule failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGScheduledPlugin::onSkipScheduledOperations()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Get Selection
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGObjectBase::SKGListSKGObjectBase selection;
        auto selectionString = sender()->property("selection").toString();
        if (!selectionString.isEmpty()) {
            selection.append(
                SKGRecurrentOperationObject(m_currentBankDocument, SKGServices::stringToInt(SKGServices::splitCSVLine(selectionString, u'-', false).at(0))));
        } else {
            selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        }
        int nb = selection.count();
        if ((nb != 0) && (m_currentBankDocument != nullptr)) {
            QStringList list;
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Skip scheduled transactions"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGRecurrentOperationObject rop(m_currentBankDocument, selection.at(i).getID());
                err = rop.setDate(rop.getNextDate());
                if (!err && rop.hasTimeLimit()) {
                    err = rop.setTimeLimit(rop.getTimeLimit() - 1);
                }
                IFOKDO(err, rop.save())

                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
                list.push_back(rop.getUniqueID());
            }
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Scheduled transactions skipped.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Skip of scheduled transaction failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

SKGAdviceList SKGScheduledPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    output.reserve(20);

    // Recurrent transaction with the last inserted transaction having a different amount
    if (!iIgnoredAdvice.contains(QStringLiteral("skgscheduledplugin_notuptodate"))) {
        SKGStringListList result;
        m_currentBankDocument->executeSelectSqliteOrder(
            "SELECT r.id, r.rd_operation_id, r.f_CURRENTAMOUNT, r2.f_CURRENTAMOUNT FROM v_recurrentoperation_display r INNER JOIN (SELECT MAX(d_date), "
            "f_CURRENTAMOUNT, r_recurrentoperation_id FROM v_operation_display GROUP BY r_recurrentoperation_id) r2 WHERE r2.r_recurrentoperation_id=r.id AND "
            "ABS(r.f_CURRENTAMOUNT-r2.f_CURRENTAMOUNT)>"
                % SKGServices::doubleToString(EPSILON),
            result);
        int nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            int idRecu = SKGServices::stringToInt(line.at(0));
            const QString &idOper = line.at(1);
            const QString &amountLastOperation = line.at(3);

            SKGRecurrentOperationObject recu(m_currentBankDocument, idRecu);
            QString name = recu.getDisplayName();

            SKGAdvice ad;
            ad.setUUID("skgscheduledplugin_notuptodate|" % idOper % u';' % amountLastOperation);
            ad.setPriority(4);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Scheduled transaction '%1' not uptodate", name));
            ad.setLongMessage(i18nc("Advice on making the best (long)",
                                    "The scheduled transaction '%1' does not have the amount of the last inserted transaction (%2)",
                                    name,
                                    amountLastOperation));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Update the next scheduled transaction amount (%1)", amountLastOperation);
                a.IconName = QStringLiteral("system-run");
                a.IsRecommended = true;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Recurrent transaction with the last inserted transaction having a different date
    if (!iIgnoredAdvice.contains(QStringLiteral("skgscheduledplugin_newdate"))) {
        SKGStringListList result;
        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT r.id, r.d_date, "
                           "date(r2.d_date, '+'||((CASE r.t_period_unit WHEN 'W' THEN 7  ELSE 1 END)*r.i_period_increment)||' '||(CASE r.t_period_unit WHEN "
                           "'M' THEN 'month' WHEN 'Y' THEN 'year' ELSE 'day' END)) "
                           "FROM v_recurrentoperation_display r "
                           "INNER JOIN (SELECT MAX(d_date) as d_date, r_recurrentoperation_id FROM v_operation_display GROUP BY r_recurrentoperation_id) r2 "
                           "WHERE r2.r_recurrentoperation_id=r.id AND r.d_PREVIOUS!=r2.d_date"),
            result);
        int nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            int idRecu = SKGServices::stringToInt(line.at(0));
            const QString &currentDate = line.at(1);
            const QString &newDate = line.at(2);
            if (SKGServices::stringToDate(newDate) > QDate::currentDate() && SKGServices::stringToDate(newDate) != SKGServices::stringToDate(currentDate)) {
                SKGRecurrentOperationObject recu(m_currentBankDocument, idRecu);
                QString name = recu.getDisplayName();

                SKGAdvice ad;
                ad.setUUID("skgscheduledplugin_newdate|" % line.at(0) % u';' % newDate);
                ad.setPriority(4);
                ad.setShortMessage(i18nc("Advice on making the best (short)", "Scheduled transaction '%1' not uptodate", name));
                ad.setLongMessage(i18nc("Advice on making the best (long)",
                                        "The scheduled transaction '%1' does not have the date aligned with the last inserted transaction (%2)",
                                        name,
                                        currentDate));
                autoCorrections.resize(0);
                {
                    SKGAdvice::SKGAdviceAction a;
                    a.Title = i18nc("Advice on making the best (action)", "Update the next scheduled transaction date (%1)", newDate);
                    a.IconName = QStringLiteral("system-run");
                    a.IsRecommended = true;
                    autoCorrections.push_back(a);
                }
                ad.setAutoCorrections(autoCorrections);
                output.push_back(ad);
            }
        }
    }

    // Possible recurrent transactions
    if (!iIgnoredAdvice.contains(QStringLiteral("skgscheduledplugin_possibleschedule"))) {
        SKGStringListList result;

        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT op1.id, op1.t_displayname FROM v_operation_displayname op1 WHERE op1.id||'#'||op1.f_QUANTITY IN (SELECT "
                           "op1.id||'#'||op2.f_QUANTITY FROM operation op1, v_operation_tmp1 op2 "
                           "WHERE op1.rd_account_id=op2.rd_account_id AND op1.r_payee_id=op2.r_payee_id AND op1.rc_unit_id=op2.rc_unit_id "
                           "AND op1.r_recurrentoperation_id=0 AND op1.d_date<>'0000-00-00' AND op1.d_date=date(op2.d_date, '+1 month') "
                           "AND op1.d_date>(date('now', 'localtime','-2 month')) AND op2.d_date>(date('now', 'localtime','-3 month'))) "
                           "AND op1.t_TRANSFER='N' "
                           "AND NOT EXISTS (SELECT 1 FROM recurrentoperation ro, v_operation_tmp1 rop  WHERE ro.rd_operation_id=rop.id AND "
                           "ro.i_period_increment=1 AND ro.t_period_unit='M' AND op1.rd_account_id=rop.rd_account_id AND op1.r_payee_id=rop.r_payee_id AND "
                           "op1.f_QUANTITY=rop.f_QUANTITY AND op1.rc_unit_id=rop.rc_unit_id)"),
            result);
        int nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        autoCorrections.reserve(nb);
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            const QString &id = line.at(0);
            const QString &name = line.at(1);

            SKGAdvice ad;
            ad.setUUID("skgscheduledplugin_possibleschedule|" % id);
            ad.setPriority(4);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Possible schedule '%1'", name));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "The transaction '%1' seems to be regularly created and could be scheduled", name));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Monthly schedule the transaction '%1'", name);
                a.IconName = icon();
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }
    m_counterAdvice++;
    return output;
}

SKGError SKGScheduledPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    SKGError err;
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgscheduledplugin_notuptodate|"))) {
        // Get parameters
        QString parameters = iAdviceIdentifier.right(iAdviceIdentifier.length() - 31);
        int pos = parameters.indexOf(';');
        int idOper = SKGServices::stringToInt(parameters.left(pos));
        double amount = SKGServices::stringToDouble(parameters.right(parameters.length() - 1 - pos));

        // Update the operation
        {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Update scheduled transaction"), err)
            SKGOperationObject op(m_currentBankDocument, idOper);
            SKGObjectBase::SKGListSKGObjectBase subOps;
            IFOKDO(err, op.getSubOperations(subOps))

            if (subOps.count() == 1) {
                // Change the quantity of the sub operation
                SKGSubOperationObject so1(subOps.at(0));
                IFOKDO(err, so1.setQuantity(amount))
                IFOKDO(err, so1.save())
            } else if (subOps.count() >= 1) {
                // Add a split
                SKGSubOperationObject so1;
                IFOKDO(err, op.addSubOperation(so1))
                IFOKDO(err, so1.setQuantity(amount - op.getCurrentAmount()))
                IFOKDO(err, so1.save())
            }

            // Send message
            IFOKDO(err,
                   op.getDocument()->sendMessage(
                       i18nc("An information to the user", "The amount of the scheduled transaction of '%1' have been updated", op.getDisplayName()),
                       SKGDocument::Hidden))
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Scheduled transaction updated.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Update failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);

        return err;
    }
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgscheduledplugin_newdate|"))) {
        // Get parameters
        QString parameters = iAdviceIdentifier.right(iAdviceIdentifier.length() - 27);
        int pos = parameters.indexOf(';');
        int id = SKGServices::stringToInt(parameters.left(pos));
        QString newDate = parameters.right(parameters.length() - 1 - pos);

        // Update the operation
        {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Update scheduled transaction"), err)
            SKGRecurrentOperationObject rop(m_currentBankDocument, id);
            IFOKDO(err, rop.setDate(SKGServices::stringToDate(newDate)))
            IFOKDO(err, rop.save())

            // Send message
            IFOKDO(err,
                   rop.getDocument()->sendMessage(
                       i18nc("An information to the user", "The date of the scheduled transaction of '%1' have been updated", rop.getDisplayName()),
                       SKGDocument::Hidden))
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Scheduled transaction updated.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Update failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);

        return err;
    }
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgscheduledplugin_possibleschedule|"))) {
        // Get parameters
        int idOper = SKGServices::stringToInt(iAdviceIdentifier.right(iAdviceIdentifier.length() - 36));

        // Update the operation
        {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Scheduled transaction"), err)
            SKGOperationObject op(m_currentBankDocument, idOper);
            SKGRecurrentOperationObject rop;
            err = scheduleOperation(op, rop);
            IFOKDO(err, rop.setPeriodUnit(SKGRecurrentOperationObject::MONTH))
            IFOKDO(err, rop.setPeriodIncrement(1))
            IFOKDO(err, rop.setDate(op.getDate()))
            IFOKDO(err, rop.setDate(rop.getNextDate()))
            IFOKDO(err, rop.save())

            // Send message
            IFOKDO(
                err,
                rop.getDocument()->sendMessage(i18nc("An information to the user", "The scheduled transaction of '%1' have been added", rop.getDisplayName()),
                                               SKGDocument::Hidden))

            m_counterAdvice = 0; // To force the update
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Transaction scheduled.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Schedule failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);

        return err;
    }

    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

#include <skgscheduledplugin.moc>
