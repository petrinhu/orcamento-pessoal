/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin for undoredo management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgundoredoplugindockwidget.h"

#include <qheaderview.h>

#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgobjectmodelbase.h"
#include "skgtraces.h"

SKGUndoRedoPluginDockWidget::SKGUndoRedoPluginDockWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGWidget(iParent, iDocument)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    QPalette newPalette = QApplication::palette();
    newPalette.setColor(QPalette::Base, Qt::transparent);
    ui.kTransactionList->setPalette(newPalette);

    auto modelview = new SKGObjectModelBase(getDocument(), QStringLiteral("doctransaction"), QStringLiteral("1=1 ORDER BY d_date DESC, id DESC"), this);
    ui.kTransactionList->setModel(modelview);
    ui.kTransactionList->header()->hide();

    QAction *act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("edit_clear_history"));
    if (act != nullptr) {
        ui.kClearHistoryBtn->setIcon(act->icon());
        connect(ui.kClearHistoryBtn, &QPushButton::clicked, act, &QAction::trigger);
    }

    ui.kTransactionList->setDefaultSaveParameters(getDocument(), QStringLiteral("SKG_DEFAULT_UNDOREDO"));

    connect(ui.kTransactionList, &SKGTableView::doubleClicked, this, &SKGUndoRedoPluginDockWidget::onUndoRedo);
    connect(ui.kTransactionList, &SKGTableView::selectionChangedDelayed, this, &SKGUndoRedoPluginDockWidget::selectionChanged);
    connect(getDocument(),
            &SKGDocument::transactionSuccessfullyEnded,
            ui.kTransactionList,
            &SKGTableView::resizeColumnsToContentsDelayed,
            Qt::QueuedConnection);

    ui.kTransactionList->setTextResizable(false);
}

SKGUndoRedoPluginDockWidget::~SKGUndoRedoPluginDockWidget(){SKGTRACEINFUNC(1)}

QWidget *SKGUndoRedoPluginDockWidget::mainWidget()
{
    return ui.kTransactionList;
}

void SKGUndoRedoPluginDockWidget::onClearHistory()
{
    SKGTRACEINFUNC(1)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    SKGError err = getDocument()->removeAllTransactions();
    QApplication::restoreOverrideCursor();

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Clear history successfully done.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Clear history failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGUndoRedoPluginDockWidget::onUndoRedo(const QModelIndex &index)
{
    SKGTRACEINFUNC(1)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Get Selection
    SKGError err;
    SKGDocument::UndoRedoMode mode = SKGDocument::UNDO;
    auto *model = qobject_cast<SKGObjectModelBase *>(ui.kTransactionList->model());
    if (model != nullptr) {
        SKGObjectBase obj = model->getObject(index);
        int id = obj.getID();
        int lastExecuted = -1;
        mode = (obj.getAttribute(QStringLiteral("t_mode")) == QStringLiteral("U") ? SKGDocument::UNDO : SKGDocument::REDO);
        do {
            lastExecuted = getDocument()->getTransactionToProcess(mode);
            err = getDocument()->undoRedoTransaction(mode);
        } while (!err && lastExecuted != id);
    }
    QApplication::restoreOverrideCursor();

    // status bar
    IFOKDO(err,
           SKGError(0,
                    mode == SKGDocument::UNDO ? i18nc("Message for successful user action", "Undo successfully done.")
                                              : i18nc("Message for successful user action", "Redo successfully done.")))
    else
    {
        err.addError(ERR_FAIL, mode == SKGDocument::UNDO ? i18nc("Error message", "Undo failed") : i18nc("Error message", "Redo failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}
