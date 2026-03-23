/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin for undo and redo operation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgundoredoplugin.h"

#include <qwidget.h>

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>
#include <ktoolbarpopupaction.h>

#include "skgerror.h"
#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgundoredo_settings.h"
#include "skgundoredoplugindockwidget.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGUndoRedoPlugin, "metadata.json")

SKGUndoRedoPlugin::SKGUndoRedoPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_undoSaveAction(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_undoMenu(nullptr)
    , m_redoMenu(nullptr)
    , m_currentDocument(nullptr)
    , m_dockWidget(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGUndoRedoPlugin::~SKGUndoRedoPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
    m_dockWidget = nullptr;
    m_undoSaveAction = nullptr;
    m_undoAction = nullptr;
    m_redoAction = nullptr;
    m_undoMenu = nullptr;
    m_redoMenu = nullptr;
}

bool SKGUndoRedoPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_undoredo"), title());
    setXMLFile(QStringLiteral("skg_undoredo.rc"));

    // Menu
    m_undoSaveAction =
        new QAction(SKGServices::fromTheme(QStringLiteral("document-revert")), i18nc("Verb, action to cancel previous action", "Revert document"), this);
    connect(m_undoSaveAction, &QAction::triggered, this, &SKGUndoRedoPlugin::onUndoSave);
    actionCollection()->setDefaultShortcut(m_undoSaveAction, Qt::CTRL | Qt::ALT | Qt::Key_Z);
    registerGlobalAction(QStringLiteral("edit_undolastsave"), m_undoSaveAction);

    m_undoAction = new KToolBarPopupAction(SKGServices::fromTheme(QStringLiteral("edit-undo")), i18nc("Verb, action to cancel previous action", "Undo"), this);
    connect(m_undoAction, &KToolBarPopupAction::triggered, this, &SKGUndoRedoPlugin::onUndo);
    actionCollection()->setDefaultShortcut(m_undoAction, Qt::CTRL | Qt::Key_Z);
    m_undoAction->setPriority(QAction::LowPriority);
    m_undoMenu = m_undoAction->popupMenu();
    connect(m_undoMenu, &QMenu::aboutToShow, this, &SKGUndoRedoPlugin::onShowUndoMenu);
    m_undoAction->setData(1);
    registerGlobalAction(QStringLiteral("edit_undo"), m_undoAction);

    m_redoAction =
        new KToolBarPopupAction(SKGServices::fromTheme(QStringLiteral("edit-redo")), i18nc("Verb, action to redo previous cancelled action", "Redo"), this);
    connect(m_redoAction, &KToolBarPopupAction::triggered, this, &SKGUndoRedoPlugin::onRedo);
    actionCollection()->setDefaultShortcut(m_redoAction, Qt::CTRL | Qt::SHIFT | Qt::Key_Z);
    m_redoAction->setPriority(QAction::LowPriority);
    m_redoMenu = m_redoAction->popupMenu();
    connect(m_redoMenu, &QMenu::aboutToShow, this, &SKGUndoRedoPlugin::onShowRedoMenu);
    m_redoAction->setData(1);
    registerGlobalAction(QStringLiteral("edit_redo"), m_redoAction);

    // Menu
    auto act =
        new QAction(SKGServices::fromTheme(QStringLiteral("edit-clear-history")), i18nc("Verb, action to cancel previous action", "Clear history"), this);
    connect(act, &QAction::triggered, this, &SKGUndoRedoPlugin::onClearHistory);
    registerGlobalAction(QStringLiteral("edit_clear_history"), act);

    // Dock creation
    m_dockWidget = new QDockWidget(SKGMainPanel::getMainPanel());
    m_dockWidget->setObjectName(QStringLiteral("skg_undoredo_docwidget"));
    m_dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_dockWidget->setWindowTitle(title());

    // add action to control hide / display of history
    QAction *toggle = m_dockWidget->toggleViewAction();
    QAction *panelAction = actionCollection()->addAction(QStringLiteral("view_transactions"));
    registerGlobalAction(QStringLiteral("view_transactions"), panelAction);
    panelAction->setCheckable(true);
    panelAction->setChecked(toggle->isChecked());
    panelAction->setText(toggle->text());
    actionCollection()->setDefaultShortcut(panelAction, Qt::SHIFT | Qt::Key_F11);
    connect(panelAction, &QAction::triggered, toggle, &QAction::trigger);
    connect(toggle, &QAction::toggled, panelAction, &QAction::setChecked);

    return true;
}

void SKGUndoRedoPlugin::refresh()
{
    SKGTRACEINFUNC(10)

    if (m_dockWidget->widget() == nullptr) {
        auto w = new SKGUndoRedoPluginDockWidget(SKGMainPanel::getMainPanel(), m_currentDocument);
        connect(w, &SKGUndoRedoPluginDockWidget::selectionChanged, SKGMainPanel::getMainPanel(), &SKGMainPanel::refresh);
        m_dockWidget->setWidget(w);
    }

    if (m_currentDocument != nullptr) {
        bool undoPossible = (m_currentDocument->getNbTransaction(SKGDocument::UNDO) > 0);
        if (m_undoSaveAction != nullptr) {
            m_undoSaveAction->setEnabled(undoPossible);
        }
        if (m_undoAction != nullptr) {
            m_undoAction->setEnabled(undoPossible);
        }
        if (m_redoAction != nullptr) {
            m_redoAction->setEnabled(m_currentDocument->getNbTransaction(SKGDocument::REDO) > 0);
        }

        // Refresh undo redo
        QString name;
        m_currentDocument->getTransactionToProcess(SKGDocument::UNDO, &name);
        QString message = i18nc("Verb", "Undo transaction '%1'.", name);
        if (name.isEmpty()) {
            message = QString();
        }
        if (m_undoAction != nullptr) {
            m_undoAction->setStatusTip(message);
        }

        m_currentDocument->getTransactionToProcess(SKGDocument::REDO, &name);
        message = i18nc("Verb", "Redo transaction '%1'.", name);
        if (name.isEmpty()) {
            message = QString();
        }
        if (m_redoAction != nullptr) {
            m_redoAction->setStatusTip(message);
        }
    }
}

void SKGUndoRedoPlugin::initPreferences()
{
    // Read Setting
    if (m_currentDocument != nullptr) {
        KSharedConfigPtr config = KSharedConfig::openConfig();
        KConfigGroup pref = config->group(QStringLiteral("skg_undoredo"));
        pref.writeEntry("maxNumberOfUndo", SKGServices::stringToInt(m_currentDocument->getParameter(QStringLiteral("SKG_UNDO_MAX_DEPTH"))));
        pref.writeEntry("cleanHistoryOnSave", (m_currentDocument->getParameter(QStringLiteral("SKG_UNDO_CLEAN_AFTER_SAVE")) == QStringLiteral("Y")));

        skgundoredo_settings::self()->read();
    }
}

QWidget *SKGUndoRedoPlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    // Create widget
    auto w = new QWidget();
    ui.setupUi(w);
    return w;
}

KConfigSkeleton *SKGUndoRedoPlugin::getPreferenceSkeleton()
{
    return skgundoredo_settings::self();
}

SKGError SKGUndoRedoPlugin::savePreferences() const
{
    SKGError err;
    if (m_currentDocument != nullptr) {
        // Read Setting
        QString max = SKGServices::intToString(skgundoredo_settings::maxNumberOfUndo());
        QString clean = (skgundoredo_settings::cleanHistoryOnSave() ? QStringLiteral("Y") : QStringLiteral("N"));

        // Save setting in document
        if (max != m_currentDocument->getParameter(QStringLiteral("SKG_UNDO_MAX_DEPTH"))) {
            err = m_currentDocument->setParameter(QStringLiteral("SKG_UNDO_MAX_DEPTH"), max);
        }
        if (clean != m_currentDocument->getParameter(QStringLiteral("SKG_UNDO_CLEAN_AFTER_SAVE"))) {
            err = m_currentDocument->setParameter(QStringLiteral("SKG_UNDO_CLEAN_AFTER_SAVE"), clean);
        }
    }
    return err;
}

QString SKGUndoRedoPlugin::title() const
{
    return i18nc("Noun", "History");
}

QString SKGUndoRedoPlugin::icon() const
{
    return QStringLiteral("edit-undo");
}

QString SKGUndoRedoPlugin::toolTip() const
{
    return i18nc("Noun", "History");
}

QStringList SKGUndoRedoPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… you can undo and redo your modifications.</p>"));
    output.push_back(
        i18nc("Description of a tips",
              "<p>… you can modify the maximum size of the undo/redo stack in the <a href=\"skg://tab_configure?page=Undo redo plugin\">settings</a>.</p>"));
    return output;
}

int SKGUndoRedoPlugin::getOrder() const
{
    return 4;
}

SKGAdviceList SKGUndoRedoPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    if (!iIgnoredAdvice.contains(QStringLiteral("skgundoredoplugin_too_big"))) {
        // Get nb transaction
        int nbObject = m_currentDocument->getNbTransaction();
        int priority = qMin(10, nbObject / 50);
        if (priority > 0) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgundoredoplugin_too_big"));
            ad.setPriority(priority);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "History is too large"));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "You can improve performances by reducing your history size in settings."));
            QStringList autoCorrections;
            autoCorrections.push_back(QStringLiteral("skg://edit_clear_history"));
            autoCorrections.push_back(i18nc("Advice on making the best (action)", "Open settings panel"));
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    return output;
}

SKGError SKGUndoRedoPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    SKGError err;
    if ((m_currentDocument != nullptr) && iAdviceIdentifier == QStringLiteral("skgundoredoplugin_too_big")) {
        SKGMainPanel::getMainPanel()->optionsPreferences(this->objectName());
        return err;
    }
    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

void SKGUndoRedoPlugin::onClearHistory()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((m_currentDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        err = m_currentDocument->removeAllTransactions();
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
}

void SKGUndoRedoPlugin::onUndoSave()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((m_currentDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        err = m_currentDocument->undoRedoTransaction(SKGDocument::UNDOLASTSAVE);
        QApplication::restoreOverrideCursor();

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Undo successfully done.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Undo failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGUndoRedoPlugin::onUndo()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((m_currentDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        int pos = qobject_cast<QAction *>(sender())->data().toInt();
        for (int i = 1; !err && i <= pos; ++i) {
            err = m_currentDocument->undoRedoTransaction(SKGDocument::UNDO);
        }
        QApplication::restoreOverrideCursor();

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Undo successfully done.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Undo failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGUndoRedoPlugin::onShowUndoMenu()
{
    if ((m_undoMenu != nullptr) && (m_currentDocument != nullptr)) {
        m_undoMenu->clear();
        SKGStringListList listTmp;
        m_currentDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT t_name, t_savestep FROM doctransaction WHERE t_mode='U' ORDER BY d_date DESC LIMIT 7"),
            listTmp);
        int nb = listTmp.count();
        for (int i = 1; i < nb; ++i) {
            QAction *act = m_undoMenu->addAction(listTmp.at(i).at(1) == QStringLiteral("Y") ? SKGServices::fromTheme(QStringLiteral("document-revert"))
                                                                                            : SKGServices::fromTheme(QStringLiteral("edit-undo")),
                                                 listTmp.at(i).at(0));
            if (act != nullptr) {
                act->setData(i);
                connect(act, &QAction::triggered, this, &SKGUndoRedoPlugin::onUndo);
            }
        }
    }
}

void SKGUndoRedoPlugin::onRedo()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((m_currentDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        int pos = qobject_cast<QAction *>(sender())->data().toInt();
        for (int i = 1; !err && i <= pos; ++i) {
            err = m_currentDocument->undoRedoTransaction(SKGDocument::REDO);
        }
        QApplication::restoreOverrideCursor();

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Redo successfully done.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Redo failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGUndoRedoPlugin::onShowRedoMenu()
{
    if ((m_redoMenu != nullptr) && (m_currentDocument != nullptr)) {
        m_redoMenu->clear();
        SKGStringListList listTmp;
        m_currentDocument->executeSelectSqliteOrder(QStringLiteral("SELECT t_name FROM doctransaction WHERE t_mode='R' ORDER BY d_date ASC LIMIT 7"), listTmp);
        int nb = listTmp.count();
        for (int i = 1; i < nb; ++i) {
            QAction *act = m_redoMenu->addAction(SKGServices::fromTheme(QStringLiteral("edit-redo")), listTmp.at(i).at(0));
            if (act != nullptr) {
                act->setData(i);
                connect(act, &QAction::triggered, this, &SKGUndoRedoPlugin::onRedo);
            }
        }
    }
}

QDockWidget *SKGUndoRedoPlugin::getDockWidget()
{
    return m_dockWidget;
}

#include <skgundoredoplugin.moc>
