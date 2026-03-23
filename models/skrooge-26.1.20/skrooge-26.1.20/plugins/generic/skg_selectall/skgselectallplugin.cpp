/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin to select all
 *
 * @author Stephane MANKOWSKI
 */
#include "skgselectallplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include <qabstractitemview.h>

#include "skgmainpanel.h"
#include "skgtraces.h"
#include "skgtreeview.h"
/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGSelectAllPlugin, "metadata.json")

SKGSelectAllPlugin::SKGSelectAllPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
    , m_selectionMessage(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)

    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::selectionChanged, this, &SKGSelectAllPlugin::onSelectionChanged);
}

SKGSelectAllPlugin::~SKGSelectAllPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
    m_selectionMessage = nullptr;
}

bool SKGSelectAllPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_selectall"), title());
    setXMLFile(QStringLiteral("skg_selectall.rc"));

    // Menu
    registerGlobalAction(QStringLiteral("edit_select_all"),
                         actionCollection()->addAction(KStandardAction::SelectAll, QStringLiteral("edit_select_all"), this, SLOT(onSelectAll())),
                         QStringList(),
                         0);
    return true;
}

QString SKGSelectAllPlugin::title() const
{
    return i18nc("Select all objects in a list", "Select all");
}

QString SKGSelectAllPlugin::icon() const
{
    return QStringLiteral("edit-select-all");
}

QString SKGSelectAllPlugin::toolTip() const
{
    return i18nc("Select all objects in a list", "Select all");
}

int SKGSelectAllPlugin::getOrder() const
{
    return 6;
}

QStringList SKGSelectAllPlugin::tips() const
{
    QStringList output;
    return output;
}

void SKGSelectAllPlugin::onSelectionChanged()
{
    SKGTRACEINFUNC(10)
    if (SKGMainPanel::getMainPanel() != nullptr) {
        if (m_selectionMessage == nullptr) {
            // Add statusbar
            m_selectionMessage = new QLabel(SKGMainPanel::getMainPanel());
            SKGMainPanel::getMainPanel()->statusBar()->insertPermanentWidget(1, m_selectionMessage);
        }
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        if (nb != 0) {
            double sum = 0.0;
            double max = -std::numeric_limits<double>::max();
            double min = std::numeric_limits<double>::max();
            qlonglong nbdec = -1;
            int count = 0;
            for (int i = 0; i < nb; ++i) {
                const SKGObjectBase &obj(selection.at(i));
                QString val = obj.getAttribute(QStringLiteral("f_REALCURRENTAMOUNT"));
                if (val.isEmpty()) {
                    val = obj.getAttribute(QStringLiteral("f_CURRENTAMOUNT"));
                }
                if (!val.isEmpty()) {
                    double vald = SKGServices::stringToDouble(val);
                    sum += vald;
                    max = qMax(max, vald);
                    min = qMin(min, vald);
                    count++;
                }
                auto dec = obj.getAttribute(QStringLiteral("i_NBDEC"));
                if (!dec.isEmpty()) {
                    nbdec = qMax(nbdec, SKGServices::stringToInt(dec));
                }
            }
            if (count > 1) {
                m_selectionMessage->setText(i18n("Selection: %1 lines for %2 (min: %3, average: %4, max: %5)",
                                                 nb,
                                                 m_currentDocument->formatPrimaryMoney(sum, nbdec),
                                                 m_currentDocument->formatPrimaryMoney(min, nbdec),
                                                 m_currentDocument->formatPrimaryMoney(sum / count, nbdec),
                                                 m_currentDocument->formatPrimaryMoney(max, nbdec)));
            } else if (count > 0) {
                m_selectionMessage->setText(i18n("Selection: %1 line for %2", nb, m_currentDocument->formatPrimaryMoney(sum, nbdec)));
            } else {
                m_selectionMessage->setText(i18np("Selection: %1 line", "Selection: %1 lines", nb));
            }
        } else {
            m_selectionMessage->clear();
        }
    }
}

void SKGSelectAllPlugin::onSelectAll()
{
    SKGTRACEINFUNC(10)
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGTabPage *page = SKGMainPanel::getMainPanel()->currentPage();
        if (page != nullptr) {
            auto *view = qobject_cast<QAbstractItemView *>(page->mainWidget());
            if (view != nullptr) {
                view->selectAll();
            }
        }
    }
}

#include <skgselectallplugin.moc>
