/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin to generate categories.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgcategoriespluginwidget.h"

#include <klocalizedstring.h>
#include <qaction.h>

#include <qdom.h>
#include <qevent.h>
#include <qgraphicsscene.h>
#include <qheaderview.h>
#include <qwidget.h>

#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgsortfilterproxymodel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGCategoriesPluginWidget::SKGCategoriesPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument)
    : SKGTabPage(iParent, iDocument)
{
    SKGTRACEINFUNC(10)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    // Define action
    QStringList overlaycategories;
    overlaycategories.push_back(QStringLiteral("applications-all-symbolic"));
    if (SKGMainPanel::getMainPanel() != nullptr) {
        auto addCategoryAction = new QAction(SKGServices::fromTheme(QStringLiteral("list-add"), overlaycategories), i18nc("Verb", "Add category"), this);
        connect(addCategoryAction, &QAction::triggered, this, &SKGCategoriesPluginWidget::onAddCategory);
        SKGMainPanel::getMainPanel()
            ->registerGlobalAction(QStringLiteral("edit_add_category"), addCategoryAction, true, QStringList() << QStringLiteral("category"), 1, -1, 450);
    }

    // Set show widget
    ui.kCategoriesView->getShowWidget()->addGroupedItem(QStringLiteral("all"), i18n("All"), QString(), QString(), QString(), Qt::META | Qt::Key_A);
    ui.kCategoriesView->getShowWidget()->addGroupedItem(QStringLiteral("opened"),
                                                        i18n("Opened"),
                                                        QStringLiteral("vcs-normal"),
                                                        QStringLiteral("t_HASOPENEDCHILD='Y'"),
                                                        QString(),
                                                        Qt::META | Qt::Key_O);
    ui.kCategoriesView->getShowWidget()->addGroupedItem(QStringLiteral("closed"),
                                                        i18n("Closed"),
                                                        QStringLiteral("vcs-conflicting"),
                                                        QStringLiteral("t_HASCLOSEDCHILD='Y'"),
                                                        QString(),
                                                        Qt::META | Qt::Key_C);
    ui.kCategoriesView->getShowWidget()->addGroupedItem(QStringLiteral("income"),
                                                        i18n("Income"),
                                                        QStringLiteral("list-add"),
                                                        QStringLiteral("f_REALCURRENTAMOUNT>=0"),
                                                        QString(),
                                                        Qt::META | Qt::Key_Plus);
    ui.kCategoriesView->getShowWidget()->addGroupedItem(QStringLiteral("expenditure"),
                                                        i18n("Expenditure"),
                                                        QStringLiteral("list-remove"),
                                                        QStringLiteral("f_REALCURRENTAMOUNT<=0"),
                                                        QString(),
                                                        Qt::META | Qt::Key_Minus);
    ui.kCategoriesView->getShowWidget()->addGroupedItem(QStringLiteral("highlighted"),
                                                        i18n("Highlighted"),
                                                        QStringLiteral("bookmarks"),
                                                        QStringLiteral("t_HASBOOKMARKEDCHILD<>'N'"),
                                                        QString(),
                                                        Qt::META | Qt::Key_H);
    ui.kCategoriesView->getShowWidget()->setDefaultState(QStringLiteral("all"));

    ui.kAdd->setIcon(SKGServices::fromTheme(QStringLiteral("list-add"), overlaycategories));
    ui.kAdd->setToolTip(i18nc("An help", "Add a sub category to the selected one"));
    ui.kAdd->setMaximumWidth(ui.kAdd->height());

    ui.kNameLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_name"))));

    ui.kModifyCategoryButton->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kDeleteUnusedButton->setIcon(SKGServices::fromTheme(QStringLiteral("edit-delete")));

    ui.kCategoriesView->setModel(new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()),
                                                    QStringLiteral("v_category_display"),
                                                    QStringLiteral("1=0"),
                                                    this,
                                                    QStringLiteral("rd_category_id"),
                                                    false));
    ui.kCategoriesView->getView()->setRootIsDecorated(true);
    ui.kCategoriesView->getView()->setAnimated(false); // Not compatible with double click
    ui.kCategoriesView->getView()->resizeColumnToContents(0);
    ui.kCategoriesView->getView()->header()->setStretchLastSection(false);

    connect(getDocument(), &SKGDocument::tableModified, this, &SKGCategoriesPluginWidget::dataModified, Qt::QueuedConnection);
    connect(ui.kCategoriesView->getView(), &SKGTreeView::clickEmptyArea, this, &SKGCategoriesPluginWidget::cleanEditor);
    connect(ui.kCategoriesView->getView(),
            &SKGTreeView::doubleClicked,
            SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open")).data(),
            &QAction::trigger);
    connect(ui.kCategoriesView->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
        this->onSelectionChanged();
    });

    connect(ui.kModifyCategoryButton, &QPushButton::clicked, this, &SKGCategoriesPluginWidget::onUpdateCategory);
    connect(ui.kNameInput1, &QLineEdit::textChanged, this, &SKGCategoriesPluginWidget::onEditorModified);
    connect(ui.kNameInput2, &QLineEdit::textChanged, this, &SKGCategoriesPluginWidget::onEditorModified);
    connect(ui.kDeleteUnusedButton, &QPushButton::clicked, this, &SKGCategoriesPluginWidget::onDeleteUnused);
    connect(ui.kAdd, &QPushButton::clicked, this, &SKGCategoriesPluginWidget::onAddCategory);

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);

    dataModified(QString(), 0);
}

SKGCategoriesPluginWidget::~SKGCategoriesPluginWidget()
{
    SKGTRACEINFUNC(10)
}

bool SKGCategoriesPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
        if (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && iObject == this) {
            if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u && ui.kAdd->isEnabled()) {
                ui.kAdd->click();
            } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0u && ui.kModifyCategoryButton->isEnabled()) {
                ui.kModifyCategoryButton->click();
            }
        }
    }

    return SKGTabPage::eventFilter(iObject, iEvent);
}

void SKGCategoriesPluginWidget::dataModified(const QString &iTableName, int iIdTransaction, bool iLightTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    if (!iLightTransaction) {
        if (iTableName == QStringLiteral("category") || iTableName.isEmpty()) {
            // Set completions
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kNameInput1,
                                                getDocument(),
                                                QStringLiteral("category"),
                                                QStringLiteral("t_name"),
                                                QString(),
                                                true);

            onSelectionChanged();
        }
    }
}

void SKGCategoriesPluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)
    // Clean widgets
    int nb = ui.kLayout->count();
    for (int i = 0; i < nb; ++i) {
        QLayoutItem *item = ui.kLayout->itemAt(0);
        if (item != nullptr) {
            ui.kLayout->removeItem(item);
            delete item->widget();
            delete item;
        }
    }

    ui.kNameInput2->setText(QString());

    int nbSelect = getNbSelectedObjects();
    if (nbSelect == 1) {
        SKGCategoryObject obj(getFirstSelectedObject());
        ui.kNameInput1->setText(obj.getName());

        SKGCategoryObject parentCat;
        obj.getParentCategory(parentCat);
        QString parentName = parentCat.getFullName();

        QStringList items = SKGServices::splitCSVLine(parentName, OBJECTSEPARATOR.trimmed().at(0));
        int nbItems = items.count();
        QString fullname;
        for (int i = 0; i < nbItems; ++i) {
            auto btn = new QPushButton(ui.kFrame);
            btn->setFlat(true);
            btn->setText(items.at(i).trimmed());
            if (!fullname.isEmpty()) {
                fullname += OBJECTSEPARATOR;
            }
            fullname += items.at(i).trimmed();
            btn->setProperty("FULLNAME", fullname);
            connect(btn, &QPushButton::clicked, this, &SKGCategoriesPluginWidget::changeSelection);
            ui.kLayout->addWidget(btn);

            auto lbl = new QLabel(ui.kFrame);
            lbl->setText(OBJECTSEPARATOR);
            ui.kLayout->addWidget(lbl);
        }
    } else if (nbSelect > 1) {
        ui.kNameInput1->setText(NOUPDATE);
    }

    onEditorModified();
    Q_EMIT selectionChanged();
}

void SKGCategoriesPluginWidget::changeSelection()
{
    QString fullname = sender()->property("FULLNAME").toString();
    SKGObjectBase::SKGListSKGObjectBase list;
    getDocument()->getObjects(QStringLiteral("v_category"), "t_fullname='" % SKGServices::stringToSqlString(fullname) % u'\'', list);

    if (!list.isEmpty()) {
        ui.kCategoriesView->getView()->selectObject(list.at(0).getUniqueID());
        onSelectionChanged();
    }
}

QString SKGCategoriesPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    // Memorize table settings
    root.setAttribute(QStringLiteral("view"), ui.kCategoriesView->getState());
    return doc.toString();
}

void SKGCategoriesPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    ui.kCategoriesView->setFilter(SKGServices::fromTheme(root.attribute(QStringLiteral("title_icon"))),
                                  root.attribute(QStringLiteral("title")),
                                  root.attribute(QStringLiteral("whereClause")));
    ui.kCategoriesView->setState(root.attribute(QStringLiteral("view")));
}

QString SKGCategoriesPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGCATEGORIES_DEFAULT_PARAMETERS");
}

QWidget *SKGCategoriesPluginWidget::mainWidget()
{
    return ui.kCategoriesView->getView();
}

void SKGCategoriesPluginWidget::onEditorModified()
{
    _SKGTRACEINFUNC(10)
    int nb = getNbSelectedObjects();
    ui.kNameInput1->setVisible(nb >= 1);
    ui.kCatSeparator->setVisible(nb >= 1);
    ui.kModifyCategoryButton->setEnabled(!ui.kNameInput1->text().isEmpty() && nb >= 1);
    ui.kAdd->setEnabled(!ui.kNameInput2->text().isEmpty());
}

void SKGCategoriesPluginWidget::onAddCategory()
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    SKGCategoryObject cat;
    QString name = ui.kNameInput2->text();
    if (name.isEmpty()) {
        name = i18nc("Noun, default name for a new category", "New category");
    }
    {
        // Get Selection
        SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

        int nb = selection.count();
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Category creation '%1'", name), err)

        if (nb == 1) {
            SKGCategoryObject parentCat(selection[0]);
            name = parentCat.getFullName() % OBJECTSEPARATOR % name;
        }
        IFOKDO(err, SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), name, cat, false, true))

        // Send message
        IFOKDO(err,
               cat.getDocument()->sendMessage(i18nc("An information message", "The category '%1' has been created", cat.getDisplayName()), SKGDocument::Hidden))
    }

    // status bar
    IFOK(err)
    {
        ui.kCategoriesView->getView()->selectObject(cat.getUniqueID());
        err = SKGError(0, i18nc("Successful message after an user action", "Category '%1' created", name));
    }
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Category creation failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGCategoriesPluginWidget::onUpdateCategory()
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    // Get Selection
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

    int nb = selection.count();
    {
        QString name = ui.kNameInput1->text();
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Category update"), err, nb)
        if (nb > 1 && name != NOUPDATE && !name.startsWith(QStringLiteral("="))) {
            getDocument()->sendMessage(i18nc("Information message", "You tried to modify all names of selected categories. Categories have been merged."));

            // Do the merge
            SKGCategoryObject catObj1(selection[0]);
            for (int i = 1; !err && i < nb; ++i) {
                SKGCategoryObject catObj(selection.at(i));

                // Send message
                IFOKDO(err,
                       catObj.getDocument()->sendMessage(i18nc("An information message",
                                                               "The category '%1' has been merged with category '%2'",
                                                               catObj1.getDisplayName(),
                                                               catObj.getDisplayName()),
                                                         SKGDocument::Hidden))

                IFOKDO(err, catObj1.merge(catObj))

                IFOKDO(err, getDocument()->stepForward(i))
            }

            // Change selection for the rest of the transaction
            selection.clear();
            selection.push_back(catObj1);
            nb = 1;
        }

        for (int i = 0; !err && i < nb; ++i) {
            // Modification of object
            SKGCategoryObject cat(selection.at(i));
            err = cat.setName(name);
            IFOKDO(err, cat.save())

            // Send message
            IFOKDO(err,
                   cat.getDocument()->sendMessage(i18nc("An information message", "The category '%1' has been updated", cat.getDisplayName()),
                                                  SKGDocument::Hidden))
        }
    }
    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Category updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Category update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kCategoriesView->getView()->setFocus();
}

void SKGCategoriesPluginWidget::onDeleteUnused()
{
    QAction *act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("clean_delete_unused_categories"));
    if (act != nullptr) {
        act->trigger();
    }
}

void SKGCategoriesPluginWidget::cleanEditor()
{
    if (getNbSelectedObjects() == 0) {
        ui.kNameInput1->setText(QString());
        ui.kNameInput2->setText(QString());
    }
}
void SKGCategoriesPluginWidget::activateEditor()
{
    ui.kNameInput1->setFocus();
}

bool SKGCategoriesPluginWidget::isEditor()
{
    return true;
}
