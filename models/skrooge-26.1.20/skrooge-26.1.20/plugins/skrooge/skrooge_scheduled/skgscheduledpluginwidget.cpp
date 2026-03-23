/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to manage scheduled transactions.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgscheduledpluginwidget.h"

#include <qlineedit.h>

#include <qdom.h>
#include <qevent.h>
#include <qheaderview.h>

#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgoperationobject.h"
#include "skgrecurrentoperationobject.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGScheduledPluginWidget::SKGScheduledPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument)
    : SKGTabPage(iParent, iDocument)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    // Add options to adapter
    ui.kAdapter->addItem(i18n("None"), static_cast<int>(SKGRecurrentOperationObject::NONE));
    ui.kAdapter->addItem(i18n("First open day of the month"), static_cast<int>(SKGRecurrentOperationObject::FIRST_OPEN_DAY_OF_MONTH));
    ui.kAdapter->addItem(i18n("Previous open day"), static_cast<int>(SKGRecurrentOperationObject::PREVIOUS_OPEN_DAY));
    ui.kAdapter->addItem(i18n("Closest open day"), static_cast<int>(SKGRecurrentOperationObject::CLOSEST_OPEN_DAY));
    ui.kAdapter->addItem(i18n("Next open day"), static_cast<int>(SKGRecurrentOperationObject::NEXT_OPEN_DAY));
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        ui.kAdapter->addItem(i18nc("The parameter is a day of the week (monday to sunday)", "Previous %1", QLocale().dayName(day)),
                             SKGRecurrentOperationObject::PREVIOUS_MONDAY + (day - 1));
    }
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        ui.kAdapter->addItem(i18nc("The parameter is a day of the week (monday to sunday)", "Closest %1", QLocale().dayName(day)),
                             SKGRecurrentOperationObject::CLOSEST_MONDAY + (day - 1));
    }
    for (int day = Qt::Monday; day <= Qt::Sunday; ++day) {
        ui.kAdapter->addItem(i18nc("The parameter is a day of the week (monday to sunday)", "Next %1", QLocale().dayName(day)),
                             SKGRecurrentOperationObject::NEXT_MONDAY + (day - 1));
    }
    ui.kAdapter->setCurrentIndex(0);

    // Define action
    if (SKGMainPanel::getMainPanel() != nullptr) {
        auto actJumpToOperation = new QAction(SKGServices::fromTheme(QStringLiteral("quickopen")), ui.kJumpBtn->text(), this);
        connect(actJumpToOperation, &QAction::triggered, this, &SKGScheduledPluginWidget::onJumpToTheOperation);
        SKGMainPanel::getMainPanel()->registerGlobalAction(QStringLiteral("jump_to_scheduled_operation"),
                                                           actJumpToOperation,
                                                           true,
                                                           QStringList() << QStringLiteral("recurrentoperation"),
                                                           1,
                                                           -1,
                                                           160);
    }

    // Set show widget
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("all"), i18n("All"), QString(), QString(), QString(), Qt::META | Qt::Key_A);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("ongoing"),
                                              i18n("Ongoing"),
                                              QStringLiteral("vcs-normal"),
                                              QStringLiteral("t_times='N' OR i_nb_times>0"),
                                              QString(),
                                              Qt::META | Qt::Key_O);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("finished"),
                                              i18n("Complete"),
                                              QStringLiteral("vcs-conflicting"),
                                              QStringLiteral("t_times='Y' AND i_nb_times=0"),
                                              QString(),
                                              Qt::META | Qt::Key_C);
    ui.kView->getShowWidget()->setDefaultState(QStringLiteral("all"));

    ui.kView->setModel(
        new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()), QStringLiteral("v_recurrentoperation_display"), QString(), this, QString(), false));

    connect(ui.kView->getView(), &SKGTreeView::doubleClicked, SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open")).data(), &QAction::trigger);
    connect(ui.kView->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
        this->onSelectionChanged();
    });

    // Add Standard KDE Icons to buttons to Transactions
    ui.kModifyBtn->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kProcessBtn->setIcon(SKGServices::fromTheme(QStringLiteral("system-run")));
    connect(ui.kProcessBtn, &QToolButton::clicked, this, &SKGScheduledPluginWidget::onProcess);

    auto processImmediatelyAction = new QAction(SKGServices::fromTheme(QStringLiteral("system-run")), i18nc("User action", "Process immediately"), this);
    connect(processImmediatelyAction, &QAction::triggered, this, &SKGScheduledPluginWidget::onProcessImmediately);
    auto processMenu = new QMenu(this);
    processMenu->addAction(processImmediatelyAction);
    ui.kProcessBtn->setMenu(processMenu);

    ui.kJumpBtn->setIcon(SKGServices::fromTheme(QStringLiteral("quickopen")));

    ui.kTitle->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-information")), KTitleWidget::ImageLeft);
    bool exist = false;
    getDocument()->existObjects(QStringLiteral("recurrentoperation"), QString(), exist);
    ui.kTitle->setVisible(!exist);

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);

    connect(ui.kRemindMe, &QCheckBox::toggled, ui.kRemindMeVal, &QSpinBox::setEnabled);
    connect(ui.kRemindMe, &QCheckBox::toggled, ui.label_3, &QSpinBox::setEnabled);

    connect(ui.kNbTimes, &QCheckBox::toggled, ui.kNbTimesVal, &QSpinBox::setEnabled);
    connect(ui.kNbTimes, &QCheckBox::toggled, ui.kLastOccurenceDate, &QSpinBox::setEnabled);

    connect(ui.kAutoWrite, &QCheckBox::toggled, ui.kAutoWriteVal, &QSpinBox::setEnabled);
    connect(ui.kAutoWrite, &QCheckBox::toggled, ui.label_4, &QSpinBox::setEnabled);

    connect(ui.kModifyBtn, &QPushButton::clicked, this, &SKGScheduledPluginWidget::onUpdate);
    connect(ui.kJumpBtn, &QPushButton::clicked, this, &SKGScheduledPluginWidget::onJumpToTheOperation);

    connect(ui.kOnceEveryUnit,
            static_cast<void (KComboBox::*)(const QString &)>(&KComboBox::currentTextChanged),
            this,
            &SKGScheduledPluginWidget::onNbOccurrenceChanged);
    connect(ui.kAdapter,
            static_cast<void (KComboBox::*)(const QString &)>(&KComboBox::currentTextChanged),
            this,
            &SKGScheduledPluginWidget::onNbOccurrenceChanged);
    connect(ui.kLastOccurenceDate, &SKGDateEdit::dateChanged, this, &SKGScheduledPluginWidget::onNbOccurrenceChanged);
    connect(ui.kNbTimesVal, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SKGScheduledPluginWidget::onNbOccurrenceChanged);
    connect(ui.kOnceEveryVal, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SKGScheduledPluginWidget::onNbOccurrenceChanged);
}

SKGScheduledPluginWidget::~SKGScheduledPluginWidget()
{
    SKGTRACEINFUNC(1)
}

bool SKGScheduledPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
        if (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && iObject == this) {
            if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0u && ui.kModifyBtn->isEnabled()) {
                ui.kModifyBtn->click();
            }
        }
    }

    return SKGTabPage::eventFilter(iObject, iEvent);
}

QString SKGScheduledPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);
    root.setAttribute(QStringLiteral("view"), ui.kView->getState());
    return doc.toString();
}

void SKGScheduledPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();
    ui.kView->setState(root.attribute(QStringLiteral("view")));

    QString selection = root.attribute(QStringLiteral("selection"));

    if (!selection.isEmpty()) {
        QStringList uuids = SKGServices::splitCSVLine(selection);
        ui.kView->getView()->selectObjects(uuids, true); // FIXME // TODO(Stephane MANKOWSKI)
        onSelectionChanged();
    }
}

QString SKGScheduledPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGSCHEDULED_DEFAULT_PARAMETERS");
}

QWidget *SKGScheduledPluginWidget::mainWidget()
{
    return ui.kView->getView();
}

void SKGScheduledPluginWidget::onJumpToTheOperation()
{
    // Get selection
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();
    if (!selection.isEmpty()) {
        // Build where clause and title
        QString wc = QStringLiteral("id IN (");
        QString title = i18nc("Noun, a list of items", "Transactions of the schedule");
        int nb = selection.count();
        for (int i = 0; i < nb; ++i) {
            SKGRecurrentOperationObject recOp(selection.at(i));

            SKGOperationObject op;
            recOp.getParentOperation(op);

            wc += SKGServices::intToString(op.getID());
            if (i < nb - 1) {
                wc += ',';
            }
        }
        wc += ')';

        // Call transaction plugin
        SKGMainPanel::getMainPanel()->openPage(
            "skg://skrooge_operation_plugin/?template=Y&title_icon=view-calendar&operationTable=v_operation_display_all&title="
            % SKGServices::encodeForUrl(title) % "&operationWhereClause=" % SKGServices::encodeForUrl(wc));
    }
}

void SKGScheduledPluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)

    int nb = getNbSelectedObjects();
    ui.kModifyBtn->setEnabled(nb != 0);
    ui.kProcessBtn->setEnabled(nb != 0);
    ui.kJumpBtn->setEnabled(nb > 0);

    if (nb == 1) {
        SKGRecurrentOperationObject recOp(ui.kView->getView()->getFirstSelectedObject());

        const auto isTemplate = recOp.isTemplate();
        ui.kIsTemplate->setCheckState(isTemplate ? Qt::Checked : Qt::Unchecked);
        if (isTemplate) {
            SKGError err;
            SKGObjectBase::SKGListSKGObjectBase transactions;
            err = recOp.getRecurredOperations(transactions);
            IFOK(err) ui.kIsTemplate->setEnabled(!transactions.isEmpty());
            IFOK(err)
            ui.kIsTemplate->setToolTip(transactions.isEmpty() ? i18nc("Information message", "A non-template schedule requires at least one transaction")
                                                              : i18nc("Information message", "Convert to a non-template schedule"));
        } else {
            ui.kIsTemplate->setEnabled(true);
            ui.kIsTemplate->setToolTip(i18nc("Information message", "Convert to a template schedule"));
        }

        ui.kFirstOccurenceDate->setDate(recOp.getDate());
        ui.kOnceEveryVal->setValue(recOp.getPeriodIncrement());
        ui.kOnceEveryUnit->setCurrentIndex(static_cast<int>(recOp.getPeriodUnit()));
        ui.kAdapter->setCurrentIndex(ui.kAdapter->findData(static_cast<int>(recOp.getAdapter())));

        ui.kRemindMeVal->setValue(recOp.getWarnDays());
        ui.kRemindMe->setCheckState(recOp.isWarnEnabled() ? Qt::Checked : Qt::Unchecked);

        ui.kAutoWriteVal->setValue(recOp.getAutoWriteDays());
        ui.kAutoWrite->setCheckState(recOp.isAutoWriteEnabled() ? Qt::Checked : Qt::Unchecked);

        ui.kNbTimesVal->setValue(recOp.getTimeLimit());
        ui.kNbTimes->setCheckState(recOp.hasTimeLimit() ? Qt::Checked : Qt::Unchecked);
    } else if (nb > 1) {
        ui.kFirstOccurenceDate->setEditText(NOUPDATE);
    }

    Q_EMIT selectionChanged();
}

void SKGScheduledPluginWidget::onNbOccurrenceChanged()
{
    QDate firstDate = ui.kFirstOccurenceDate->date();
    auto punit = static_cast<SKGRecurrentOperationObject::PeriodUnit>(ui.kOnceEveryUnit->currentIndex());
    int p = ui.kOnceEveryVal->value();

    if (ui.kLastOccurenceDate == this->sender()) {
        // End date has been modified.
        // We must set the number of occurrence
        QDate lastDate = ui.kLastOccurenceDate->date();
        if (lastDate <= firstDate) {
            ui.kLastOccurenceDate->setDate(firstDate);
            ui.kNbTimesVal->setValue(1);
        } else {
            int nbd = firstDate.daysTo(lastDate);
            if (punit == SKGRecurrentOperationObject::DAY) {
                nbd = nbd / p;
            } else if (punit == SKGRecurrentOperationObject::WEEK) {
                nbd = nbd / p / 7;
            } else if (punit == SKGRecurrentOperationObject::MONTH) {
                nbd = (lastDate.day() >= firstDate.day() ? 0 : -1) + (lastDate.year() - firstDate.year()) * 12 + (lastDate.month() - firstDate.month());
            } else if (punit == SKGRecurrentOperationObject::YEAR) {
                nbd = nbd / (365 * p);
            }

            bool previous = ui.kNbTimesVal->blockSignals(true);
            ui.kNbTimesVal->setValue(nbd + 1);
            ui.kNbTimesVal->blockSignals(previous);
        }

    } else {
        // We must compute the date
        p *= (ui.kNbTimesVal->value() - 1);
        if (punit == SKGRecurrentOperationObject::DAY) {
            firstDate = firstDate.addDays(p);
        } else if (punit == SKGRecurrentOperationObject::WEEK) {
            firstDate = firstDate.addDays(7 * p);
        } else if (punit == SKGRecurrentOperationObject::MONTH) {
            firstDate = firstDate.addMonths(p);
        } else if (punit == SKGRecurrentOperationObject::YEAR) {
            firstDate = firstDate.addYears(p);
        }

        bool previous = ui.kLastOccurenceDate->blockSignals(true);
        ui.kLastOccurenceDate->setDate(firstDate);
        ui.kLastOccurenceDate->blockSignals(previous);
    }
}

void SKGScheduledPluginWidget::onUpdate()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        // Get Selection
        SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

        int nb = selection.count();
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Recurrent transaction update"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            // Get the real object, not the object from the view
            SKGRecurrentOperationObject recOp = SKGRecurrentOperationObject(selection.at(i).getDocument(), selection.at(i).getID());

            // Convert to/from template if needed
            const auto isTemplate = recOp.isTemplate();
            if (isTemplate && ui.kIsTemplate->checkState() == Qt::Unchecked) {
                err = recOp.setTemplate(false);
            } else if (!isTemplate && ui.kIsTemplate->checkState() == Qt::Checked) {
                err = recOp.setTemplate(true);
            }

            // Update it
            if (ui.kFirstOccurenceDate->currentText() != NOUPDATE) {
                IFOKDO(err, recOp.setDate(ui.kFirstOccurenceDate->date()))
            }
            IFOKDO(err, recOp.setPeriodIncrement(ui.kOnceEveryVal->value()))
            IFOKDO(err, recOp.setPeriodUnit(static_cast<SKGRecurrentOperationObject::PeriodUnit>(ui.kOnceEveryUnit->currentIndex())))
            IFOKDO(err, recOp.setAdapter(static_cast<SKGRecurrentOperationObject::Adapter>((ui.kAdapter->currentData()).toInt())))
            IFOKDO(err, recOp.setWarnDays(ui.kRemindMeVal->value()))
            IFOKDO(err, recOp.warnEnabled(ui.kRemindMe->checkState() == Qt::Checked))
            IFOKDO(err, recOp.setAutoWriteDays(ui.kAutoWriteVal->value()))
            IFOKDO(err, recOp.autoWriteEnabled(ui.kAutoWrite->checkState() == Qt::Checked))
            IFOKDO(err, recOp.setTimeLimit(ui.kNbTimesVal->value()))
            IFOKDO(err, recOp.timeLimit(ui.kNbTimes->checkState() == Qt::Checked))
            IFOKDO(err, recOp.save())

            // Send message
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The recurrent transaction '%1' has been updated", recOp.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }
    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Recurrent transaction updated.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kView->getView()->setFocus();
}

void SKGScheduledPluginWidget::onProcessImmediately()
{
    onProcess(true);
}

void SKGScheduledPluginWidget::onProcess(bool iImmediately)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        // Get Selection
        SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

        int nb = selection.count();
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Insert recurrent transactions"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            // Get the real object, not the object from the view
            SKGRecurrentOperationObject recOp = SKGRecurrentOperationObject(selection.at(i).getDocument(), selection.at(i).getID());

            // Process it
            int nbi = 0;
            err = recOp.process(nbi, true, (iImmediately ? recOp.getDate() : QDate::currentDate()));
            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }
    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Recurrent transaction inserted.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Insertion failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}
