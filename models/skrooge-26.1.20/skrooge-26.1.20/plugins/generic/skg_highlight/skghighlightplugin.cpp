/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin to highlight objects
 *
 * @author Stephane MANKOWSKI
 */
#include "skghighlightplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgmainpanel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGHighlightPlugin, "metadata.json")

SKGHighlightPlugin::SKGHighlightPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGHighlightPlugin::~SKGHighlightPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
}

bool SKGHighlightPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_highlight"), title());
    setXMLFile(QStringLiteral("skg_highlight.rc"));

    // ------------
    auto actSwitchHighLight = new QAction(SKGServices::fromTheme(QStringLiteral("bookmarks")), i18nc("Verb", "Switch highlight"), this);
    connect(actSwitchHighLight, &QAction::triggered, this, &SKGHighlightPlugin::onSwitchHighLight);
    actionCollection()->setDefaultShortcut(actSwitchHighLight, Qt::CTRL | Qt::Key_H);
    registerGlobalAction(QStringLiteral("edit_switch_highlight"),
                         actSwitchHighLight,
                         QStringList() << QStringLiteral("query:type='table' AND sql LIKE '%t_bookmarked%'"),
                         1,
                         -1,
                         301);

    // ------------
    auto actSwitchClose = new QAction(SKGServices::fromTheme(QStringLiteral("dialog-close")), i18nc("Verb", "Switch close"), this);
    connect(actSwitchClose, &QAction::triggered, this, &SKGHighlightPlugin::onSwitchClose);
    registerGlobalAction(QStringLiteral("edit_switch_close"),
                         actSwitchClose,
                         QStringList() << QStringLiteral("query:type='table' AND sql LIKE '%t_close%'"),
                         1,
                         -1,
                         301);

    // Create yours actions here
    return true;
}

QString SKGHighlightPlugin::title() const
{
    return toolTip();
}

QString SKGHighlightPlugin::icon() const
{
    return QStringLiteral("bookmarks");
}

QString SKGHighlightPlugin::toolTip() const
{
    return i18nc("The tool tip", "Highlight");
}

int SKGHighlightPlugin::getOrder() const
{
    return 6;
}

void SKGHighlightPlugin::onSwitchClose()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        {
            SKGBEGINPROGRESSTRANSACTION(*m_currentDocument, i18nc("Noun, name of the user action", "Close"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGObjectBase obj(selection.at(i));
                IFOKDO(err,
                       obj.setAttribute(QStringLiteral("t_close"),
                                        obj.getAttribute(QStringLiteral("t_close")) == QStringLiteral("Y") ? QStringLiteral("N") : QStringLiteral("Y")))
                IFOKDO(err, obj.save())

                // Send message
                IFOKDO(err,
                       m_currentDocument->sendMessage(i18nc("An information to the user", "The close status of '%1' has been changed", obj.getDisplayName()),
                                                      SKGDocument::Hidden))

                IFOKDO(err, m_currentDocument->stepForward(i + 1))
            }
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Closed.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Closure failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGHighlightPlugin::onSwitchHighLight()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        {
            SKGBEGINPROGRESSTRANSACTION(*m_currentDocument, i18nc("Noun, name of the user action", "Highlight"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGObjectBase obj(selection.at(i));
                IFOKDO(err,
                       obj.setAttribute(QStringLiteral("t_bookmarked"),
                                        obj.getAttribute(QStringLiteral("t_bookmarked")) == QStringLiteral("Y") ? QStringLiteral("N") : QStringLiteral("Y")))
                IFOKDO(err, obj.save())

                // Send message
                IFOKDO(
                    err,
                    m_currentDocument->sendMessage(i18nc("An information to the user", "The highlight status of '%1' has been changed", obj.getDisplayName()),
                                                   SKGDocument::Hidden))

                IFOKDO(err, m_currentDocument->stepForward(i + 1))
            }
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Highlighted.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Highlight failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

#include <skghighlightplugin.moc>
