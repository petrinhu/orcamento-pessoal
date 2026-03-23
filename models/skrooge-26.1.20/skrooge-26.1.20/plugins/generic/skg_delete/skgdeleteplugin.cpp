/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin for delete operation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdeleteplugin.h"

#include <qwidget.h>

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgerror.h"
#include "skgmainpanel.h"
#include "skgobjectbase.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGDeletePlugin, "metadata.json")

SKGDeletePlugin::SKGDeletePlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGDeletePlugin::~SKGDeletePlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
}

bool SKGDeletePlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_delete"), title());
    setXMLFile(QStringLiteral("skg_delete.rc"));

    // Menu
    QStringList tmp;
    m_currentDocument->getDistinctValues(QStringLiteral("sqlite_master"), QStringLiteral("name"), QStringLiteral("type='table' AND name!='parameters'"), tmp);
    auto actDelete = new QAction(SKGServices::fromTheme(QStringLiteral("edit-delete")), i18nc("Verb, delete an item", "Delete"), this);
    connect(actDelete, &QAction::triggered, this, &SKGDeletePlugin::onDelete);
    actionCollection()->setDefaultShortcut(actDelete, Qt::Key_Delete);
    registerGlobalAction(QStringLiteral("edit_delete"), actDelete, tmp, 1, -1, 200, true);
    return true;
}

QString SKGDeletePlugin::title() const
{
    return i18nc("Verb, delete an item", "Delete");
}

int SKGDeletePlugin::getOrder() const
{
    return 5;
}

void SKGDeletePlugin::onDelete()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        if (nb > 0) {
            if (selection[0].getRealTable() == QStringLiteral("doctransaction")) {
                err = m_currentDocument->beginTransaction(QStringLiteral("#INTERNAL#"));
                IFOKDO(err,
                       m_currentDocument->executeSqliteOrder(
                           QStringLiteral("CREATE TRIGGER fkdc_doctransaction_doctransaction_i_parent_id "
                                          "BEFORE DELETE ON doctransaction "
                                          "FOR EACH ROW BEGIN "
                                          "    DELETE FROM doctransaction WHERE OLD.t_name!='#INTERNAL#' AND doctransaction.id = OLD.i_parent; "
                                          "END")));
                for (int i = 0; !err && i < nb; ++i) {
                    err = selection.at(i).remove();
                }
                m_currentDocument->executeSqliteOrder(QStringLiteral("DROP TRIGGER IF EXISTS fkdc_doctransaction_doctransaction_i_parent_id"));
                SKGENDTRANSACTION(m_currentDocument, err)
            } else {
                SKGBEGINPROGRESSTRANSACTION(*m_currentDocument, i18nc("Verb, delete an item", "Delete"), err, nb)
                for (int i = 0; !err && i < nb; ++i) {
                    err = selection.at(i).remove();
                    if (err && err.getReturnCode() == ERR_FORCEABLE) {
                        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
                        int rc = KMessageBox::questionTwoActions(SKGMainPanel::getMainPanel(),
                                                                 err.getFullMessage() % '\n' % i18nc("Question", "Do you want to force the deletion ?"),
                                                                 i18nc("Question", "Do you want to force the deletion ?"),
                                                                 KStandardGuiItem::del(),
                                                                 KStandardGuiItem::cancel(),
                                                                 QStringLiteral("forcedelete"));
                        QApplication::restoreOverrideCursor();
                        if (rc == KMessageBox::PrimaryAction) {
                            err = selection.at(i).remove(true, true);
                        }
                    }
                    IFOKDO(err, m_currentDocument->stepForward(i + 1))
                }
            }
        }

        //
        KMessageBox::enableMessage(QStringLiteral("forcedelete"));

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Objects deleted.")))
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

#include <skgdeleteplugin.moc>
