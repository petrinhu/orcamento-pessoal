/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to track transactions.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgtrackerpluginwidget.h"

#include <qdom.h>
#include <qevent.h>

#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgtraces.h"
#include "skgtrackerobject.h"
#include "skgtransactionmng.h"

SKGTrackerPluginWidget::SKGTrackerPluginWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGTabPage(iParent, iDocument)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    // Set show widget
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("all"), i18n("All"), QString(), QString(), QString(), Qt::META | Qt::Key_A);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("opened"),
                                              i18n("Opened"),
                                              QStringLiteral("vcs-normal"),
                                              QStringLiteral("t_close='N'"),
                                              QString(),
                                              Qt::META | Qt::Key_O);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("closed"),
                                              i18n("Closed"),
                                              QStringLiteral("vcs-conflicting"),
                                              QStringLiteral("t_close='Y'"),
                                              QString(),
                                              Qt::META | Qt::Key_C);
    ui.kView->getShowWidget()->setDefaultState(QStringLiteral("opened"));

    ui.kNameLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_name"))));
    ui.kCommentLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_comment"))));

    ui.kAddButton->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    ui.kModifyButton->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));

    ui.kView->setModel(
        new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()), QStringLiteral("v_refund_display"), QStringLiteral("1=0"), this, QString(), false));

    ui.kView->getView()->resizeColumnToContents(0);

    connect(getDocument(), &SKGDocument::tableModified, this, &SKGTrackerPluginWidget::dataModified, Qt::QueuedConnection);
    connect(ui.kView->getView(), &SKGTreeView::clickEmptyArea, this, &SKGTrackerPluginWidget::cleanEditor);
    connect(ui.kView->getView(), &SKGTreeView::doubleClicked, SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open")).data(), &QAction::trigger);
    connect(ui.kView->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
        this->onSelectionChanged();
    });

    connect(ui.kAddButton, &QPushButton::clicked, this, &SKGTrackerPluginWidget::onAddTracker);
    connect(ui.kModifyButton, &QPushButton::clicked, this, &SKGTrackerPluginWidget::onModifyTracker);
    connect(ui.kNameInput, &QLineEdit::textChanged, this, &SKGTrackerPluginWidget::onEditorModified);

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);

    dataModified(QString(), 0);
}

SKGTrackerPluginWidget::~SKGTrackerPluginWidget() = default;

bool SKGTrackerPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
        if (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && iObject == this) {
            if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u && ui.kAddButton->isEnabled()) {
                ui.kAddButton->click();
            } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0u && ui.kModifyButton->isEnabled()) {
                ui.kModifyButton->click();
            }
        }
    }

    return SKGTabPage::eventFilter(iObject, iEvent);
}

void SKGTrackerPluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)

    int nbSelect = ui.kView->getView()->getNbSelectedObjects();
    if (nbSelect == 1) {
        SKGTrackerObject obj(ui.kView->getView()->getFirstSelectedObject());

        ui.kNameInput->setText(obj.getName());
        ui.kCommentEdit->setText(obj.getComment());
    } else if (nbSelect > 1) {
        ui.kNameInput->setText(NOUPDATE);
        ui.kCommentEdit->setText(NOUPDATE);
    }

    onEditorModified();
    Q_EMIT selectionChanged();
}

QString SKGTrackerPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);
    root.setAttribute(QStringLiteral("view"), ui.kView->getState());
    return doc.toString();
}

void SKGTrackerPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();
    ui.kView->setState(root.attribute(QStringLiteral("view")));
}

QString SKGTrackerPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGREFUND_DEFAULT_PARAMETERS");
}

QWidget *SKGTrackerPluginWidget::mainWidget()
{
    return ui.kView->getView();
}

void SKGTrackerPluginWidget::onEditorModified()
{
    _SKGTRACEINFUNC(10)
    int nb = ui.kView->getView()->getNbSelectedObjects();
    ui.kModifyButton->setEnabled(!ui.kNameInput->text().isEmpty() && nb >= 1);
    ui.kAddButton->setEnabled(!ui.kNameInput->text().isEmpty() && !ui.kNameInput->text().startsWith(u'='));
}

void SKGTrackerPluginWidget::dataModified(const QString &iTableName, int iIdTransaction, bool iLightTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    if (!iLightTransaction) {
        if (iTableName == QStringLiteral("refund") || iTableName.isEmpty()) {
            // Set completions
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kNameInput,
                                                getDocument(),
                                                QStringLiteral("refund"),
                                                QStringLiteral("t_name"),
                                                QString(),
                                                true);
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kCommentEdit,
                                                getDocument(),
                                                QStringLiteral("refund"),
                                                QStringLiteral("t_comment"),
                                                QString(),
                                                true);
        }
    }
}

void SKGTrackerPluginWidget::onAddTracker()
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)

    QString name = ui.kNameInput->text();
    SKGTrackerObject tracker;
    {
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Tracker creation '%1'", name), err)

        err = SKGTrackerObject::createTracker(qobject_cast<SKGDocumentBank *>(getDocument()), name, tracker);
        IFOKDO(err, tracker.setComment(ui.kCommentEdit->text()))
        IFOKDO(err, tracker.save())

        // Send message
        IFOKDO(err,
               tracker.getDocument()->sendMessage(i18nc("An information to the user", "The tracker '%1' have been added", tracker.getDisplayName()),
                                                  SKGDocument::Hidden))
    }

    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("Successful message after an user action", "Tracker '%1' created", name));
        ui.kView->getView()->selectObject(tracker.getUniqueID());
    }
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Tracker creation failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGTrackerPluginWidget::onModifyTracker()
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    // Get Selection
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

    int nb = selection.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Tracker update"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            // Modification of object
            SKGTrackerObject tracker(selection.at(i));
            err = tracker.setName(ui.kNameInput->text());
            IFOKDO(err, tracker.setComment(ui.kCommentEdit->text()))
            IFOKDO(err, tracker.save())

            // Send message
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The tracker '%1' has been updated", tracker.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }
    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Tracker updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Tracker update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kView->getView()->setFocus();
}

void SKGTrackerPluginWidget::cleanEditor()
{
    if (getNbSelectedObjects() == 0) {
        ui.kNameInput->setText(QString());
        ui.kCommentEdit->setText(QString());
    }
}

void SKGTrackerPluginWidget::activateEditor()
{
    ui.kNameInput->setFocus();
}

bool SKGTrackerPluginWidget::isEditor()
{
    return true;
}
