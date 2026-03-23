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
#include "skgpayeepluginwidget.h"

#include <qaction.h>

#include <qdom.h>
#include <qevent.h>

#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgpayeeobject.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGPayeePluginWidget::SKGPayeePluginWidget(QWidget *iParent, SKGDocument *iDocument)
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
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("highlighted"),
                                              i18n("Highlighted only"),
                                              QStringLiteral("bookmarks"),
                                              QStringLiteral("t_bookmarked='Y'"),
                                              QString(),
                                              Qt::META | Qt::Key_H);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("income"),
                                              i18n("Income"),
                                              QStringLiteral("list-add"),
                                              QStringLiteral("f_CURRENTAMOUNT>=0"),
                                              QString(),
                                              Qt::META | Qt::Key_Plus);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("expenditure"),
                                              i18n("Expenditure"),
                                              QStringLiteral("list-remove"),
                                              QStringLiteral("f_CURRENTAMOUNT<=0"),
                                              QString(),
                                              Qt::META | Qt::Key_Minus);
    ui.kView->getShowWidget()->setDefaultState(QStringLiteral("all"));

    ui.kView->getShowWidget()->setDefaultState(QStringLiteral("all"));

    ui.kNameLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_name"))));
    ui.kAddressLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_address"))));
    ui.kCategoryLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_CATEGORY"))));

    ui.kAddButton->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    ui.kModifyButton->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kDeleteUnusedButton->setIcon(SKGServices::fromTheme(QStringLiteral("edit-delete")));

    ui.kView->setModel(
        new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()), QStringLiteral("v_payee_display"), QStringLiteral("1=0"), this, QString(), false));
    ui.kView->getView()->resizeColumnToContents(0);

    connect(getDocument(), &SKGDocument::tableModified, this, &SKGPayeePluginWidget::dataModified, Qt::QueuedConnection);
    connect(ui.kView->getView(), &SKGTreeView::clickEmptyArea, this, &SKGPayeePluginWidget::cleanEditor);
    connect(ui.kView->getView(), &SKGTreeView::doubleClicked, SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open")).data(), &QAction::trigger);
    connect(ui.kView->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
        this->onSelectionChanged();
    });

    connect(ui.kAddButton, &QPushButton::clicked, this, &SKGPayeePluginWidget::onAddPayee);
    connect(ui.kModifyButton, &QPushButton::clicked, this, &SKGPayeePluginWidget::onModifyPayee);
    connect(ui.kNameInput, &QLineEdit::textChanged, this, &SKGPayeePluginWidget::onEditorModified);
    connect(ui.kDeleteUnusedButton, &QPushButton::clicked, this, &SKGPayeePluginWidget::onDeleteUnused);

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);

    dataModified(QString(), 0);
}

SKGPayeePluginWidget::~SKGPayeePluginWidget()
{
    SKGTRACEINFUNC(1)
}

bool SKGPayeePluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
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

void SKGPayeePluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)

    int nbSelect = ui.kView->getView()->getNbSelectedObjects();
    if (nbSelect == 1) {
        SKGPayeeObject obj(ui.kView->getView()->getFirstSelectedObject());

        ui.kNameInput->setText(obj.getName());
        ui.kAddressEdit->setText(obj.getAddress());
        ui.kCategoryEdit->setText(obj.getAttribute(QStringLiteral("t_CATEGORY")));
    } else if (nbSelect > 1) {
        ui.kNameInput->setText(NOUPDATE);
        ui.kAddressEdit->setText(NOUPDATE);
        ui.kCategoryEdit->setText(NOUPDATE);
    }

    onEditorModified();
    Q_EMIT selectionChanged();
}

QString SKGPayeePluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);
    root.setAttribute(QStringLiteral("view"), ui.kView->getState());
    return doc.toString();
}

void SKGPayeePluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    ui.kView->setFilter(SKGServices::fromTheme(root.attribute(QStringLiteral("title_icon"))),
                        root.attribute(QStringLiteral("title")),
                        root.attribute(QStringLiteral("whereClause")));
    ui.kView->setState(root.attribute(QStringLiteral("view")));
}

QString SKGPayeePluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGPAYEE_DEFAULT_PARAMETERS");
}

QWidget *SKGPayeePluginWidget::mainWidget()
{
    return ui.kView->getView();
}

void SKGPayeePluginWidget::onEditorModified()
{
    _SKGTRACEINFUNC(10)
    int nb = getNbSelectedObjects();
    ui.kModifyButton->setEnabled(!ui.kNameInput->text().isEmpty() && nb >= 1);
    ui.kAddButton->setEnabled(!ui.kNameInput->text().isEmpty() && !ui.kNameInput->text().startsWith(u'='));
}

void SKGPayeePluginWidget::dataModified(const QString &iTableName, int iIdTransaction, bool iLightTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    if (!iLightTransaction) {
        if (iTableName == QStringLiteral("payee") || iTableName.isEmpty()) {
            // Set completions
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kNameInput,
                                                getDocument(),
                                                QStringLiteral("payee"),
                                                QStringLiteral("t_name"),
                                                QString(),
                                                true);
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAddressEdit,
                                                getDocument(),
                                                QStringLiteral("payee"),
                                                QStringLiteral("t_address"),
                                                QString(),
                                                true);
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kCategoryEdit,
                                                getDocument(),
                                                QStringLiteral("category"),
                                                QStringLiteral("t_fullname"),
                                                QString());
        }
    }
}

void SKGPayeePluginWidget::onAddPayee()
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)

    QString name = ui.kNameInput->text();
    SKGPayeeObject payee;
    {
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Payee creation '%1'", name), err)

        IFOKDO(err, SKGPayeeObject::createPayee(qobject_cast<SKGDocumentBank *>(getDocument()), name, payee))
        IFOKDO(err, payee.setAddress(ui.kAddressEdit->text()))
        SKGCategoryObject cat;
        QString catName = ui.kCategoryEdit->text().trimmed();
        if (!err && catName != NOUPDATE) {
            err = SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), catName, cat, true);
        }
        IFOKDO(err, payee.setCategory(cat))

        IFOKDO(err, payee.save())

        // Send message
        IFOKDO(err,
               payee.getDocument()->sendMessage(i18nc("An information message", "The payee '%1' has been added", payee.getDisplayName()), SKGDocument::Hidden))
    }
    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("Successful message after an user action", "Payee '%1' created", name));
        ui.kView->getView()->selectObject(payee.getUniqueID());
    }
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Payee creation failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGPayeePluginWidget::onModifyPayee()
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)

    // Get Selection
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

    int nb = selection.count();

    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Payee update"), err, nb)
        auto name = ui.kNameInput->text();
        if (name != NOUPDATE && !name.startsWith(QStringLiteral("="))) {
            // Is this name already existing?
            bool messageSent = false;
            SKGPayeeObject p(getDocument());
            p.setName(name);
            IFOK(p.load())
            {
                if (selection.indexOf(p) == -1) {
                    // We will have to merge with the existing payee
                    selection.insert(0, p);
                    nb++;

                    getDocument()->sendMessage(
                        i18nc("Information message", "You tried to modify names of selected payees to an existing payee. Payees have been merged."));
                    messageSent = true;
                }
            }

            // Is it a massive modification of payees to merge them ?
            if (nb > 1) {
                if (!messageSent) {
                    getDocument()->sendMessage(i18nc("Information message", "You tried to modify all names of selected payees. Payees have been merged."));
                }

                // Do the merge
                SKGPayeeObject payeeObj1(selection[0]);
                for (int i = 1; !err && i < nb; ++i) {
                    SKGPayeeObject payeeObj(selection.at(i));

                    // Send message
                    IFOKDO(err,
                           payeeObj.getDocument()->sendMessage(i18nc("An information message",
                                                                     "The payee '%1' has been merged with payee '%2'",
                                                                     payeeObj1.getDisplayName(),
                                                                     payeeObj.getDisplayName()),
                                                               SKGDocument::Hidden))

                    err = payeeObj1.merge(payeeObj);
                }

                // Change selection for the rest of the transaction
                selection.clear();
                selection.push_back(payeeObj1);
                nb = 1;
            }
        }

        for (int i = 0; !err && i < nb; ++i) {
            // Modification of object
            SKGPayeeObject payee(selection.at(i));
            err = payee.setName(name);
            QString address = ui.kAddressEdit->text();
            if (address != NOUPDATE) {
                IFOKDO(err, payee.setAddress(address))
            }
            SKGCategoryObject cat;
            QString catName = ui.kCategoryEdit->text().trimmed();
            if (!err && catName != NOUPDATE) {
                err = SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), catName, cat, true);
                IFOKDO(err, payee.setCategory(cat))
            }
            IFOKDO(err, payee.save())

            // Send message
            IFOKDO(err,
                   payee.getDocument()->sendMessage(i18nc("An information message", "The payee '%1' has been updated", payee.getDisplayName()),
                                                    SKGDocument::Hidden))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Payee updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Payee update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kView->getView()->setFocus();
}

void SKGPayeePluginWidget::cleanEditor()
{
    if (getNbSelectedObjects() == 0) {
        ui.kNameInput->setText(QString());
        ui.kAddressEdit->setText(QString());
    }
}

void SKGPayeePluginWidget::activateEditor()
{
    ui.kNameInput->setFocus();
}

bool SKGPayeePluginWidget::isEditor()
{
    return true;
}

void SKGPayeePluginWidget::onDeleteUnused()
{
    QAction *act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("clean_delete_unused_payees"));
    if (act != nullptr) {
        act->trigger();
    }
}
