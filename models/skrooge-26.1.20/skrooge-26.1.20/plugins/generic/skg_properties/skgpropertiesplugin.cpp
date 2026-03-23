/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin to manage properties on objects
 *
 * @author Stephane MANKOWSKI
 */
#include "skgpropertiesplugin.h"

#include <qdir.h>
#include <qdockwidget.h>

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgmainpanel.h"
#include "skgpropertiesplugindockwidget.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGPropertiesPlugin, "metadata.json")

SKGPropertiesPlugin::SKGPropertiesPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
    , m_dockWidget(nullptr)
    , m_dockContent(nullptr)
    , m_addPropertyMenu(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)

    // Get list of bills
    m_billsProcess.setStandardOutputFile(QDir::tempPath() % "/skg_bills.csv");
    m_billsProcess.start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << QStringLiteral("boobill bills  -q -f csv -v"));

    connect(&m_billsProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SKGPropertiesPlugin::onBillsRetreived);
    connect(&m_billsProcess, static_cast<void (QProcess::*)(QProcess::ProcessError)>(&QProcess::errorOccurred), this, &SKGPropertiesPlugin::onBillsRetreived);
}

SKGPropertiesPlugin::~SKGPropertiesPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
    m_dockWidget = nullptr;
    m_dockContent = nullptr;
    m_addPropertyMenu = nullptr;

    if (m_billsProcess.state() == QProcess::Running) {
        m_billsProcess.kill();
    }
    if (m_billsProcess.state() == QProcess::Running) {
        m_billsProcess.kill();
        m_billsProcess.waitForFinished(-1);
    }
}

void SKGPropertiesPlugin::onBillsRetreived()
{
    QFile file(QDir::tempPath() % "/skg_bills.csv");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.readLine(); // To avoid header id;date;format;label;idparent;price;currency;deadline;startdate;finishdate
        while (!stream.atEnd()) {
            // Read line
            QString line = stream.readLine().trimmed();

            m_bills.push_back(line);
        }

        // close file
        file.close();
    }
    file.remove();
}

bool SKGPropertiesPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_properties"), title());
    setXMLFile(QStringLiteral("skg_properties.rc"));

    m_dockContent = new SKGPropertiesPluginDockWidget(SKGMainPanel::getMainPanel(), m_currentDocument);
    if (m_dockContent != nullptr) {
        connect(m_dockContent, &SKGPropertiesPluginDockWidget::selectionChanged, SKGMainPanel::getMainPanel(), &SKGMainPanel::refresh);
        m_dockWidget = new QDockWidget(SKGMainPanel::getMainPanel());
        if (m_dockWidget != nullptr) {
            m_dockWidget->setObjectName(QStringLiteral("skg_properties_docwidget"));
            m_dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
            m_dockWidget->setWindowTitle(title());
            m_dockWidget->setWidget(m_dockContent);

            // add action to control hide / display of Bookmarks
            QAction *toggle = m_dockWidget->toggleViewAction();
            QAction *panelAction = actionCollection()->addAction(QStringLiteral("view_properties"));
            registerGlobalAction(QStringLiteral("view_properties"), panelAction);
            panelAction->setCheckable(true);
            panelAction->setChecked(toggle->isChecked());
            panelAction->setText(toggle->text());
            actionCollection()->setDefaultShortcut(panelAction, Qt::SHIFT | Qt::Key_F12);
            connect(panelAction, &QAction::triggered, toggle, &QAction::trigger);
            connect(toggle, &QAction::toggled, panelAction, &QAction::setChecked);
        }
    }

    // Menu
    auto actAddProperty = new QAction(SKGServices::fromTheme(icon()), i18nc("Allows user to add a user defined property on an object", "Add property"), this);
    m_addPropertyMenu = new QMenu();
    actAddProperty->setMenu(m_addPropertyMenu);
    connect(m_addPropertyMenu, &QMenu::aboutToShow, this, &SKGPropertiesPlugin::onShowAddPropertyMenu);
    registerGlobalAction(QStringLiteral("add_property"),
                         actAddProperty,
                         QStringList() << QStringLiteral("query:type='table' AND name NOT LIKE 'doctransaction%'"),
                         1,
                         -1,
                         450);
    return true;
}

void SKGPropertiesPlugin::onAddProperty()
{
    SKGTRACEINFUNC(10)
    SKGError err;
    // Scope for the transaction
    auto *act = qobject_cast<QAction *>(sender());
    if ((act != nullptr) && (m_currentDocument != nullptr)) {
        // Get parameters
        QStringList list = act->data().toStringList();
        const QString &name = list.at(0);
        const QString &value = list.at(1);

        // Create properties
        IFOK(err)
        {
            SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
            int nb = selection.count();
            SKGBEGINPROGRESSTRANSACTION(*m_currentDocument, i18nc("Create a user defined property", "Property creation"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                err = selection.at(i).setProperty(name, value);
                IFOKDO(err, m_currentDocument->stepForward(i + 1))
            }
        }
    }

    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("The user defined property was successfully created", "Property created"));
    }
    SKGMainPanel::displayErrorMessage(err);
}

void SKGPropertiesPlugin::onDownloadAndAddBills()
{
    SKGTRACEINFUNC(10)
    SKGError err;
    // Scope for the transaction
    auto *act = qobject_cast<QAction *>(sender());
    if ((act != nullptr) && (m_currentDocument != nullptr)) {
        // Get parameters
        QStringList list = act->data().toStringList();
        const QString &id = list.at(0);
        QString fileName = QDir::tempPath() % u'/' % list.at(3) % u'.' % list.at(2);

        // Create properties
        IFOK(err)
        {
            SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
            int nb = selection.count();
            SKGBEGINPROGRESSTRANSACTION(*m_currentDocument, i18nc("Create a user defined property", "Property creation"), err, 2 * nb)
            for (int i = 0; !err && i < nb; ++i) {
                // Download the files
                QFile::remove(fileName);
                QString cmd = "boobill download " % id % " \"" % fileName % u'"';
                QProcess p;
                p.start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << cmd);
                if (!p.waitForFinished(60000) || p.exitCode() != 0) {
                    err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "The following command line failed with code %2:\n'%1'", cmd, p.exitCode()));
                } else {
                    IFOKDO(err, m_currentDocument->stepForward(2 * i))

                    IFOKDO(err, selection.at(i).setProperty(i18nc("Noun", "Bill"), id, fileName))
                    QStringList importedBills = SKGServices::splitCSVLine(m_currentDocument->getParameter(QStringLiteral("SKG_IMPORTED_BILLS")));
                    importedBills.push_back(id);
                    IFOKDO(err, m_currentDocument->setParameter(QStringLiteral("SKG_IMPORTED_BILLS"), SKGServices::stringsToCsv(importedBills)))
                    IFOKDO(err, m_currentDocument->stepForward(2 * i + 1))

                    QFile::remove(fileName);
                }
            }
        }
    }

    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("The user defined property was successfully created", "Property created"));
    }
    SKGMainPanel::displayErrorMessage(err);
}

void SKGPropertiesPlugin::onShowAddPropertyMenu()
{
    if ((m_addPropertyMenu != nullptr) && (m_currentDocument != nullptr)) {
        m_addPropertyMenu->clear();

        // Get selection
        SKGObjectBase::SKGListSKGObjectBase sels = SKGMainPanel::getMainPanel()->getSelectedObjects();
        if (!sels.isEmpty()) {
            // Get the table of the selection
            QString table = sels.at(0).getRealTable();

            // Get list of more used properties for this table
            SKGStringListList listTmp;
            m_currentDocument->executeSelectSqliteOrder(
                "SELECT t_name, t_value FROM (SELECT t_name, t_value, COUNT(1) AS nb FROM parameters WHERE (t_uuid_parent like '%-" % table
                    % "' OR t_uuid_parent like '%-sub" % table
                    % "') AND t_name NOT LIKE 'SKG_%' AND b_blob IS NULL GROUP BY t_name, t_value) ORDER BY nb DESC LIMIT 7",
                listTmp);

            // Create actions
            int nb = listTmp.count();
            QIcon iconp = SKGServices::fromTheme(icon());
            if (nb > 1) {
                for (int i = 1; i < nb; ++i) {
                    // Should the string below be translated ??? It contains no word…
                    QAction *act =
                        m_addPropertyMenu->addAction(iconp, i18nc("Add a property (attribute=value)", "Add %1=%2", listTmp.at(i).at(0), listTmp.at(i).at(1)));
                    if (act != nullptr) {
                        act->setData(listTmp.at(i));
                        connect(act, &QAction::triggered, this, &SKGPropertiesPlugin::onAddProperty);
                    }
                }
            } else {
                QAction *act = m_addPropertyMenu->addAction(iconp, i18nc("Help", "No property found. You must create a property from the dock first."));
                act->setEnabled(false);
            }

            // Check if the sub process is still running
            if (m_billsProcess.state() == QProcess::Running) {
                // Create separator
                {
                    QAction *act = m_addPropertyMenu->addAction(QString());
                    act->setSeparator(true);
                }

                // Add download on going
                QAction *act = m_addPropertyMenu->addAction(i18nc("Message", "Download list of available bills on going…"));
                if (act != nullptr) {
                    act->setEnabled(false);
                }

            } else {
                // Check if some bills can be downloaded
                int nb2 = m_bills.count();
                if (nb2 != 0) {
                    // Create separator
                    {
                        QAction *act = m_addPropertyMenu->addAction(QString());
                        act->setSeparator(true);
                    }

                    // Create action
                    QStringList importedBills = SKGServices::splitCSVLine(m_currentDocument->getParameter(QStringLiteral("SKG_IMPORTED_BILLS")));

                    QMenu *menuMore = nullptr;
                    QIcon icond = SKGServices::fromTheme(icon(), QStringList() << QStringLiteral("download"));
                    QSet<QString> backendDone;
                    for (int j = 1; j < nb2; ++j) {
                        // id;date;format;label;idparent;price;currency;deadline;startdate;finishdate
                        QStringList fields = SKGServices::splitCSVLine(m_bills.at(j));
                        if (fields.count() > 3 && !importedBills.contains(fields.at(0))) {
                            QStringList ids = SKGServices::splitCSVLine(fields.at(0), '@');
                            if (ids.count() == 2) {
                                const QString &backend = ids.at(1);

                                // Selection of the menu where the item must be added
                                QMenu *menu;
                                if (!backendDone.contains(backend)) {
                                    // This item must be added in root menu
                                    menu = m_addPropertyMenu;
                                    backendDone.insert(backend);
                                } else {
                                    // This item must be added in "More…" menu
                                    if (menuMore == nullptr) {
                                        menuMore = new QMenu(i18nc("Noun", "More…"), m_addPropertyMenu);
                                    }
                                    menu = menuMore;
                                }

                                // Should the string below be translated ??? It contains no word…
                                QAction *act = menu->addAction(
                                    icond,
                                    i18nc("Add a property (attribute=value)", "Download and add %1 (%2)", fields[3] % u'.' % fields[2], fields[0]));
                                if (act != nullptr) {
                                    act->setToolTip(fields[0]);
                                    act->setData(fields);
                                    connect(act, &QAction::triggered, this, &SKGPropertiesPlugin::onDownloadAndAddBills);
                                }
                            }
                        }
                    }

                    // Add "More…" menu
                    if (menuMore != nullptr) {
                        m_addPropertyMenu->addMenu(menuMore);
                    }
                }
            }
        }
    }
}

void SKGPropertiesPlugin::refresh()
{
    SKGTRACEINFUNC(10)
    if (m_dockContent != nullptr) {
        m_dockContent->refresh();
    }
}

QDockWidget *SKGPropertiesPlugin::getDockWidget()
{
    return m_dockWidget;
}

QString SKGPropertiesPlugin::title() const
{
    return i18nc("Noun, an item's properties", "Properties");
}

QString SKGPropertiesPlugin::icon() const
{
    return QStringLiteral("tag");
}

int SKGPropertiesPlugin::getOrder() const
{
    return 6;
}

QStringList SKGPropertiesPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tip", "<p>… you can manage properties on all objects.</p>"));
    output.push_back(i18nc("Description of a tip", "<p>… you can add files or Internet links as property.</p>"));
    output.push_back(i18nc("Description of a tip", "<p>… you can automatically download and add bills as properties by using %1.</p>", "weboob"));
    return output;
}

#include <skgpropertiesplugin.moc>
