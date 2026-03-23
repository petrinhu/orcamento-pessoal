/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin to manage properties on objects.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgpropertiesplugindockwidget.h"

#include <kmessagebox.h>

#include <qdesktopservices.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qlineedit.h>

#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgnamedobject.h"
#include "skgobjectmodelbase.h"
#include "skgpropertyobject.h"
#include "skgservices.h"
#include "skgsortfilterproxymodel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGPropertiesPluginDockWidget::SKGPropertiesPluginDockWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGWidget(iParent, iDocument)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    ui.kPicture->hide();

    ui.kAdd->setMaximumWidth(ui.kAdd->height());
    ui.kRemove->setMaximumWidth(ui.kRemove->height());
    ui.kSelectFile->setMaximumWidth(ui.kSelectFile->height());

    ui.kAdd->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    ui.kRename->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kRemove->setIcon(SKGServices::fromTheme(QStringLiteral("list-remove")));
    ui.kSelectFile->setIcon(SKGServices::fromTheme(QStringLiteral("document-open")));
    ui.kOpenBtn->setIcon(SKGServices::fromTheme(QStringLiteral("quickopen")));

    ui.kAttribute->lineEdit()->setPlaceholderText(i18n("Name"));
    ui.kValue->lineEdit()->setPlaceholderText(i18n("Value"));

    ui.kForCmb->addItem(i18n("Selection"));
    ui.kForCmb->addItem(i18n("All"));

    // Add model
    auto modelview =
        new SKGObjectModelBase(getDocument(), QStringLiteral("parameters"), QStringLiteral("1=1 ORDER BY t_uuid_parent, t_name"), this, QString(), false);
    auto modelproxy = new SKGSortFilterProxyModel(this);
    modelproxy->setSourceModel(modelview);
    ui.kView->setModel(modelproxy);

    connect(ui.kFilterEdit, &QLineEdit::textChanged, this, [modelproxy](const QString &itext) {
        modelproxy->setFilterKeyColumn(-1);
        modelproxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        modelproxy->setFilterFixedString(itext);
    });

    auto actOpenPropertyFileAction = new QAction(QStringLiteral("internal action to open property file"), this);
    connect(actOpenPropertyFileAction, &QAction::triggered, this, &SKGPropertiesPluginDockWidget::onOpenPropertyFileByUrl);
    SKGMainPanel::getMainPanel()->registerGlobalAction(QStringLiteral("open_property_file"), actOpenPropertyFileAction);

    ui.kView->setDefaultSaveParameters(getDocument(), QStringLiteral("SKG_DEFAULT_PROPERTIES"));
    connect(modelview, &SKGObjectModelBase::beforeReset, ui.kView, &SKGTreeView::saveSelection);
    connect(modelview, &SKGObjectModelBase::afterReset, ui.kView, &SKGTreeView::resetSelection);
    connect(ui.kView, &SKGTableView::selectionChangedDelayed, this, &SKGPropertiesPluginDockWidget::onSelectionChanged);
    connect(ui.kForCmb, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged), this, &SKGPropertiesPluginDockWidget::refresh);
    connect(ui.kAdd, &QPushButton::clicked, this, &SKGPropertiesPluginDockWidget::onAddProperty);
    connect(ui.kRemove, &QPushButton::clicked, this, &SKGPropertiesPluginDockWidget::onRemoveProperty);
    connect(ui.kSelectFile, &QPushButton::clicked, this, &SKGPropertiesPluginDockWidget::onSelectFile);
    connect(ui.kOpenBtn, &QPushButton::clicked, this, &SKGPropertiesPluginDockWidget::onOpenFile);
    connect(ui.kView, &SKGTableView::clickEmptyArea, this, &SKGPropertiesPluginDockWidget::cleanEditor);
    connect(ui.kRename, &QPushButton::clicked, this, &SKGPropertiesPluginDockWidget::onRenameProperty);
    ui.kView->setTextResizable(false);
}

SKGPropertiesPluginDockWidget::~SKGPropertiesPluginDockWidget()
{
    SKGTRACEINFUNC(1)
}

void SKGPropertiesPluginDockWidget::refresh()
{
    SKGTRACEINFUNC(1)

    // Change filter
    auto *proxyModel = qobject_cast<QSortFilterProxyModel *>(ui.kView->model());
    auto *model = qobject_cast<SKGObjectModelBase *>(proxyModel->sourceModel());
    if (model != nullptr) {
        QString filter;
        if (ui.kForCmb->currentIndex() == 1) {
            filter = QStringLiteral("t_uuid_parent!='document' AND t_name NOT LIKE 'SKG_%'");
            ui.kAdd->setEnabled(false);
            ui.kSelectFile->setEnabled(false);
            ui.kRemove->setEnabled(false);
            ui.kAttribute->setEnabled(false);
            ui.kValue->setEnabled(false);
        } else if (ui.kForCmb->currentIndex() == 0) {
            filter = QStringLiteral("t_uuid_parent IN (");
            SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
            int nb = selection.count();
            if (nb != 0) {
                ui.kAdd->setEnabled(true);
                ui.kSelectFile->setEnabled(true);
                ui.kRemove->setEnabled(false);
                ui.kAttribute->setEnabled(true);
                ui.kValue->setEnabled(true);

                QString tableName;
                for (int i = 0; i < nb; ++i) {
                    if (i > 0) {
                        filter += ',';
                    } else {
                        tableName = selection.at(i).getRealTable();
                    }
                    filter += '\'' % selection.at(i).getUniqueID() % u'\'';
                }

                // Fill combo box
                QString t = std::move(tableName);
                if (t.startsWith(QStringLiteral("sub"))) {
                    t = t.right(t.length() - 3);
                }
                SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAttribute,
                                                    getDocument(),
                                                    QStringLiteral("parameters"),
                                                    QStringLiteral("t_name"),
                                                    "(t_uuid_parent like '%-" % t % "' OR t_uuid_parent like '%-sub" % t % "') AND t_name NOT LIKE 'SKG_%'");
                SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kValue,
                                                    getDocument(),
                                                    QStringLiteral("parameters"),
                                                    QStringLiteral("t_value"),
                                                    "(t_uuid_parent like '%-" % t % "' OR t_uuid_parent like '%-sub" % t % "') AND t_name NOT LIKE 'SKG_%'");
            } else {
                filter += QStringLiteral("'XXX'"); // Always false
                ui.kAdd->setEnabled(false);
                ui.kSelectFile->setEnabled(false);
                ui.kRemove->setEnabled(false);
                ui.kAttribute->setEnabled(false);
                ui.kValue->setEnabled(false);
            }
            filter += QStringLiteral(") AND t_name NOT LIKE 'SKG_%'");
        }
        filter += QStringLiteral(" ORDER BY t_uuid_parent, t_name");
        ui.kView->saveSelection();

        model->setFilter(filter);
        model->refresh();

        ui.kView->resetSelection();
    }

    ui.kView->setState(QString());
    if (ui.kView->isAutoResized()) {
        ui.kView->resizeColumnsToContentsDelayed();
    }

    onSelectionChanged();
}

void SKGPropertiesPluginDockWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)
    int nbSelected = getNbSelectedObjects();
    ui.kPicture->hide();
    ui.kOpenBtn->hide();
    ui.kRemove->setEnabled(nbSelected > 0);
    if (nbSelected > 0) {
        SKGObjectBase::SKGListSKGObjectBase objs = getSelectedObjects();
        SKGPropertyObject obj(objs.at(0));
        ui.kAttribute->setText(obj.getAttribute(QStringLiteral("t_name")));
        ui.kValue->setText(obj.getAttribute(QStringLiteral("t_value")));

        if (nbSelected == 1) {
            QUrl u = obj.getUrl(true);

            ui.kOpenBtn->show();
            if (u.scheme() == QStringLiteral("file")) {
                ui.kPicture->show();
                ui.kPicture->showPreview(u);
            }
        }
    }

    if (ui.kView->isAutoResized()) {
        ui.kView->resizeColumnsToContentsDelayed();
    }
}

void SKGPropertiesPluginDockWidget::openPropertyFile(const SKGPropertyObject &iProp)
{
    SKGTRACEINFUNC(10)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QUrl url = iProp.getUrl(true);

    if (!url.scheme().isEmpty() && !QDesktopServices::openUrl(url)) {
        QString fileNameToSave = SKGMainPanel::getSaveFileName(QStringLiteral("kfiledialog:///IMPEXP"), QString(), SKGMainPanel::getMainPanel());
        if (!fileNameToSave.isEmpty()) {
            QFile(url.toLocalFile()).copy(fileNameToSave);
        }
    }
    QApplication::restoreOverrideCursor();
}

void SKGPropertiesPluginDockWidget::onOpenPropertyFileByUrl()
{
    SKGTRACEINFUNC(10)
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        SKGPropertyObject obj(getDocument(), SKGServices::stringToInt(act->property("id").toString()));
        openPropertyFile(obj);
    }
}

void SKGPropertiesPluginDockWidget::onOpenFile()
{
    SKGTRACEINFUNC(10)
    int nbSelected = getNbSelectedObjects();
    if (nbSelected == 1) {
        SKGObjectBase::SKGListSKGObjectBase objs = getSelectedObjects();
        SKGPropertyObject obj(objs.at(0));
        openPropertyFile(obj);
    }

    if (ui.kView->isAutoResized()) {
        ui.kView->resizeColumnsToContentsDelayed();
    }
}

void SKGPropertiesPluginDockWidget::onAddProperty()
{
    SKGTRACEINFUNC(10)
    SKGError err;
    QStringList listUUID;
    // Scope for the transaction
    {
        // Get parameters
        QString name = ui.kAttribute->text();
        QString value = ui.kValue->text();
        QVariant blob;
        QFile file(value);
        if (file.exists()) {
            int mode = KMessageBox::questionTwoActionsCancel(this,
                                                             i18nc("Question", "Do you want copy or link the file?"),
                                                             QString(),
                                                             KGuiItem(i18nc("Question", "Copy"), QStringLiteral("edit-copy")),
                                                             KGuiItem(i18nc("Question", "Link"), QStringLiteral("edit-link")));
            if (mode == KMessageBox::Cancel) {
                return;
            }
            if (mode == KMessageBox::PrimaryAction) {
                // Value is a file name ==> blob
                if (Q_UNLIKELY(!file.open(QIODevice::ReadOnly))) {
                    err = SKGError(ERR_INVALIDARG, i18nc("Error message: could not open the requested file", "Open file '%1' failed", value));
                } else {
                    QByteArray blob_bytes = file.readAll();
                    if (blob_bytes.isEmpty()) {
                        err = SKGError(ERR_INVALIDARG, i18nc("Error message: could not open the requested file", "Open file '%1' failed", value));
                    } else {
                        blob = blob_bytes;
                        value = QFileInfo(value).fileName();
                    }

                    // close file
                    file.close();
                }
            }
        }

        // Create properties
        IFOK(err)
        {
            SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
            int nb = selection.count();
            SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Create a user defined property", "Property creation"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGPropertyObject prop;
                err = selection.at(i).setProperty(name, value, blob, &prop);

                IFOK(err)
                {
                    listUUID.push_back(prop.getUniqueID());
                    err = getDocument()->stepForward(i + 1);
                }
            }
        }
    }

    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("The user defined property was successfully created", "Property created"));
        ui.kView->selectObjects(listUUID, true);
    }
    SKGMainPanel::displayErrorMessage(err);
}

void SKGPropertiesPluginDockWidget::onRenameProperty()
{
    SKGTRACEINFUNC(10)
    SKGError err;
    QStringList listUUID;
    // Scope for the transaction
    {
        // Rename properties
        IFOK(err)
        {
            SKGObjectBase::SKGListSKGObjectBase selection = ui.kView->getSelectedObjects();
            int nb = selection.count();
            SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Create a user defined property", "Rename property"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                const SKGObjectBase &prop(selection.at(i));
                IFOKDO(err,
                       getDocument()->executeSqliteOrder("UPDATE parameters SET t_name='" % SKGServices::stringToSqlString(ui.kAttribute->text())
                                                         % "' WHERE id=" % SKGServices::intToString(prop.getID())))
                IFOK(err)
                {
                    listUUID.push_back(prop.getUniqueID());
                    err = getDocument()->stepForward(i + 1);
                }
            }
        }
    }

    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("The user property was successfully renamed", "Property renamed"));
        ui.kView->selectObjects(listUUID, true);
    }
    SKGMainPanel::displayErrorMessage(err);
}

void SKGPropertiesPluginDockWidget::onSelectFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, i18nc("Open panel caption", "Select a file"));
    ui.kValue->setText(fileName);
}

void SKGPropertiesPluginDockWidget::onRemoveProperty()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();
        int nb = selection.count();
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Verb, delete an item", "Delete"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            err = selection.at(i).remove();
            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("The user defined property was successfully deleted", "Properties deleted.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message: Could not delete an item", "Delete failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGPropertiesPluginDockWidget::cleanEditor()
{
    if (getNbSelectedObjects() == 0) {
        ui.kAttribute->setText(QString());
        ui.kValue->setText(QString());
    }
}

QWidget *SKGPropertiesPluginDockWidget::mainWidget()
{
    return ui.kView;
}
