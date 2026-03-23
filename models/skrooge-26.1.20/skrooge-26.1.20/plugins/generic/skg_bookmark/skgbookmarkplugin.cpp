/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin for bookmarks management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbookmarkplugin.h"

#include <qwidget.h>

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <ktoolbarpopupaction.h>

#include "skgbookmark_settings.h"
#include "skgbookmarkplugindockwidget.h"
#include "skgerror.h"
#include "skgmainpanel.h"
#include "skgnodeobject.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include <utility>

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGBookmarkPlugin, "metadata.json")

SKGBookmarkPlugin::SKGBookmarkPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList &iArg)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
    , m_dockWidget(nullptr)
    , m_bookmarkMenu(nullptr)
{
    Q_UNUSED(iArg)
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGBookmarkPlugin::~SKGBookmarkPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
    m_dockWidget = nullptr;
    m_bookmarkMenu = nullptr;
}

bool SKGBookmarkPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_bookmark"), title());
    setXMLFile(QStringLiteral("skg_bookmark.rc"));

    m_dockWidget = new QDockWidget(SKGMainPanel::getMainPanel());
    m_dockWidget->setObjectName(QStringLiteral("skg_bookmark_docwidget"));
    m_dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_dockWidget->setWindowTitle(title());
    m_dockWidget->setWidget(new SKGBookmarkPluginDockWidget(m_dockWidget, m_currentDocument));

    // add action to control hide / display of Bookmarks
    QAction *toggle = m_dockWidget->toggleViewAction();
    QAction *panelAction = actionCollection()->addAction(QStringLiteral("view_bookmarks"));
    registerGlobalAction(QStringLiteral("view_bookmarks"), panelAction);
    panelAction->setCheckable(true);
    panelAction->setChecked(toggle->isChecked());
    panelAction->setText(toggle->text());
    actionCollection()->setDefaultShortcut(panelAction, Qt::SHIFT | Qt::Key_F10);
    connect(panelAction, &QAction::triggered, toggle, &QAction::trigger);
    connect(toggle, &QAction::toggled, panelAction, &QAction::setChecked);

    // Import bookmarks
    QStringList overlaybookmarks;
    overlaybookmarks.push_back(icon());

    auto actImportStdBookmark =
        new QAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlaybookmarks), i18nc("Verb", "Import standard bookmarks"), this);
    connect(actImportStdBookmark, &QAction::triggered, this, &SKGBookmarkPlugin::importStandardBookmarks);
    registerGlobalAction(QStringLiteral("import_standard_bookmarks"), actImportStdBookmark);

    //
    QAction *goHomeAction = KStandardAction::home(this, SLOT(goHome()), actionCollection());
    goHomeAction->setPriority(QAction::LowPriority);
    registerGlobalAction(QStringLiteral("go_home"), goHomeAction);

    // Bookmark
    auto actBookmark =
        new KToolBarPopupAction(SKGServices::fromTheme(QStringLiteral("bookmark-new-list")), i18nc("Verb, action to display bookmarks", "Bookmarks"), this);
    connect(actBookmark, &KToolBarPopupAction::triggered, this, &SKGBookmarkPlugin::onOpenBookmark);
    m_bookmarkMenu = actBookmark->popupMenu();
    if (m_bookmarkMenu != nullptr) {
        m_bookmarkMenu->setProperty("id", 0);
        connect(m_bookmarkMenu, &QMenu::aboutToShow, this, &SKGBookmarkPlugin::onShowBookmarkMenu);
    }
    actBookmark->setData(0);
    actBookmark->setPriority(QAction::LowPriority);

    registerGlobalAction(QStringLiteral("edit_bookmark"), actBookmark);

    return true;
}

QWidget *SKGBookmarkPlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    auto w = new QWidget();
    ui.setupUi(w);
    return w;
}

KConfigSkeleton *SKGBookmarkPlugin::getPreferenceSkeleton()
{
    return skgbookmark_settings::self();
}

void SKGBookmarkPlugin::refresh()
{
    SKGTRACEINFUNC(10)
    if (m_dockWidget != nullptr) {
        auto *p = qobject_cast<SKGBookmarkPluginDockWidget *>(m_dockWidget->widget());
        if (p != nullptr) {
            p->refresh();
        }
    }

    if (m_currentDocument != nullptr) {
        // Automatic bookmarks creation
        if (m_currentDocument->getMainDatabase() != nullptr) {
            QString doc_id = m_currentDocument->getUniqueIdentifier();
            if (m_docUniqueIdentifier != doc_id) {
                m_docUniqueIdentifier = std::move(doc_id);

                bool exist = false;
                SKGError err = m_currentDocument->existObjects(QStringLiteral("node"), QString(), exist);
                if (!err && !exist) {
                    importStandardBookmarks();

                    // The file is considered has not modified
                    m_currentDocument->setFileNotModified();
                }

                // Automatic open of autostart bookmark
                if (!err && !(QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
                    goHome();
                }
            }
        }
    }
}

QString SKGBookmarkPlugin::title() const
{
    return i18nc("Noun, a bookmark as in a webbrowser bookmark", "Bookmarks");
}

QString SKGBookmarkPlugin::icon() const
{
    return QStringLiteral("bookmarks");
}

QStringList SKGBookmarkPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… some bookmarks can be opened automatically when the application is launched.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… bookmarks can be reorganized by drag & drop.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… a double click on a folder of bookmarks will open all the bookmarks it contains.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can <a href=\"skg://import_standard_bookmarks\">import standard bookmarks</a>.</p>"));
    return output;
}

int SKGBookmarkPlugin::getOrder() const
{
    return 3;
}

QDockWidget *SKGBookmarkPlugin::getDockWidget()
{
    return m_dockWidget;
}

void SKGBookmarkPlugin::importStandardBookmarks()
{
    SKGTRACEINFUNC(10)
    SKGError err;
    SKGBEGINTRANSACTION(*m_currentDocument, i18nc("Noun, name of the user action", "Import standard bookmarks"), err)

    QStringList bks;
    bks << i18nc("Noun, bookmark name", "Dashboard") % "|N|dashboard-show|\"Dashboard plugin\";\"Dashboard\";\"<!DOCTYPE SKGML><parameters zoomPosition=\"\"0\"\"> <ITEM-1 zoom=\"\"0\"\" index=\"\"0\"\" name=\"\"Skrooge bank plugin\"\" state=\"\"&lt;!DOCTYPE SKGML>&#xa;&lt;parameters menuOther=&quot;Y&quot; menuCurrent=&quot;Y&quot; menuFavorite=&quot;N&quot; menuAssets=&quot;Y&quot; menuInvestment=&quot;Y&quot; menuCreditCard=&quot;Y&quot;/>&#xa;\"\"/> <ITEM-2 zoom=\"\"0\"\" index=\"\"0\"\" name=\"\"Skrooge operation plugin\"\" state=\"\"&lt;!DOCTYPE SKGML>"
        "&#xa;&lt;parameters menuTransfert=&quot;Y&quot;/>&#xa;\"\"/> <ITEM-3 zoom=\"\"0\"\" index=\"\"1\"\" name=\"\"Skrooge operation plugin\"\" state=\"\"\"\"/> <ITEM-4 zoom=\"\"0\"\" index=\"\"0\"\" name=\"\"Skrooge scheduled plugin\"\" state=\"\"\"\"/> <ITEM-5 zoom=\"\"0\"\" index=\"\"0\"\" name=\"\"Skrooge search plugin\"\" state=\"\"&lt;!DOCTYPE SKGML>"
        "&#xa;&lt;parameters menuFavorite=&quot;N&quot;/>&#xa;\"\"/> <ITEM-6 zoom=\"\"0\"\" index=\"\"0\"\" name=\"\"Skrooge unit plugin\"\" state=\"\"&lt;!DOCTYPE SKGML>&#xa;&lt;parameters menuShares=&quot;Y&quot; menuSharesOwnedOnly=&quot;N&quot; menuIndexes=&quot;Y&quot;/>&#xa;\"\"/> <ITEM-7 zoom=\"\"0\"\" index=\"\"0\"\" name=\"\"Skrooge calculator plugin\"\" state=\"\"\"\"/></parameters>\""
        << i18nc("Noun, bookmark name", "Report > Income vs Expenditure on 12 last months") % "|N|view-statistics|\"Skrooge report plugin\";\"Rapport\";\"<!DOCTYPE SKGML><parameters  expenses=\"\"Y\"\" tableAndGraphState=\"\"&lt;!DOCTYPE SKGML>"
        "&#xa;&lt;parameters graphMode=&quot;1&quot; graphicViewState=&quot;&amp;lt;!DOCTYPE SKGML>&amp;#xa;&amp;lt;parameters isToolBarVisible=&amp;quot;Y&amp;quot; antialiasing=&amp;quot;Y&amp;quot;/>&amp;#xa;&quot; splitterState=&quot;000000ff00000000000000020000025b0000032901000000060100000001&quot; bottomToolBarVisible=&quot;Y&quot; filter=&quot;&quot; legendVisible=&quot;N&quot; allPositive=&quot;Y&quot; linearRegressionVisible=&quot;Y&quot; sortColumn=&quot;0&quot; limitVisible=&quot;Y&quot; web=&quot;&amp;lt;!DOCTYPE SKGML>"
        "&amp;#xa;&amp;lt;parameters zoomFactor=&amp;quot;0&amp;quot;/>&amp;#xa;&quot; sortOrder=&quot;0&quot;/>&#xa;\"\" mode=\"\"0\"\" period=\"\"3\"\" incomes=\"\"Y\"\" interval=\"\"2\"\" lines=\"\"t_TYPEEXPENSENLS\"\" lines2=\"\"#NOTHING#\"\" nbLevelLines=\"\"0\"\" nbLevelColumns=\"\"0\"\" forecast=\"\"0\"\" columns=\"\"d_DATEMONTH\"\" nb_intervals=\"\"12\"\" forecastValue=\"\"20\"\" transfers=\"\"N\"\" currentPage=\"\"0\"\"/>\""
        << i18nc("Noun, bookmark name", "Report > Pie categories in 12 last months") % "|N|view-statistics|\"Skrooge report plugin\";\"Rapport\";\"<!DOCTYPE SKGML><parameters expenses=\"\"Y\"\" tableAndGraphState=\"\"&lt;!DOCTYPE SKGML>&#xa;&lt;parameters graphMode=&quot;2&quot; graphicViewState=&quot;&amp;lt;!DOCTYPE SKGML>"
        "&amp;#xa;&amp;lt;parameters isToolBarVisible=&amp;quot;Y&amp;quot; antialiasing=&amp;quot;Y&amp;quot;/>&amp;#xa;&quot; splitterState=&quot;000000ff00000000000000020000025b0000032901000000060100000001&quot; bottomToolBarVisible=&quot;Y&quot; filter=&quot;&quot; legendVisible=&quot;N&quot; allPositive=&quot;Y&quot; linearRegressionVisible=&quot;Y&quot; sortColumn=&quot;-1&quot; limitVisible=&quot;Y&quot; web=&quot;&amp;lt;!DOCTYPE SKGML>"
        "&amp;#xa;&amp;lt;parameters zoomFactor=&amp;quot;0&amp;quot;/>&amp;#xa;&quot; sortOrder=&quot;0&quot;/>&#xa;\"\" mode=\"\"0\"\" period=\"\"3\"\" incomes=\"\"Y\"\" interval=\"\"2\"\" lines=\"\"t_REALCATEGORY\"\" lines2=\"\"#NOTHING#\"\" nbLevelLines=\"\"0\"\" nbLevelColumns=\"\"0\"\" forecast=\"\"0\"\" columns=\"\"d_DATEMONTH\"\" nb_intervals=\"\"12\"\" forecastValue=\"\"20\"\" transfers=\"\"N\"\" currentPage=\"\"0\"\"/>\""
        << i18nc("Noun, bookmark name", "Report > History") % "|N|view-statistics|\"Skrooge report plugin\";\"Rapport\";\"<!DOCTYPE SKGML><parameters expenses=\"\"Y\"\" tableAndGraphState=\"\"&lt;!DOCTYPE SKGML>"
        "&#xa;&lt;parameters graphMode=&quot;1&quot; graphicViewState=&quot;&amp;lt;!DOCTYPE SKGML>&amp;#xa;&amp;lt;parameters isToolBarVisible=&amp;quot;Y&amp;quot; antialiasing=&amp;quot;Y&amp;quot;/>&amp;#xa;&quot; splitterState=&quot;000000ff0000000000000002000002b50000032601000000060100000001&quot; bottomToolBarVisible=&quot;Y&quot; filter=&quot;&quot; legendVisible=&quot;N&quot; allPositive=&quot;N&quot; linearRegressionVisible=&quot;Y&quot; sortColumn=&quot;-1&quot; limitVisible=&quot;Y&quot; web=&quot;&amp;lt;!DOCTYPE SKGML>"
        "&amp;#xa;&amp;lt;parameters zoomFactor=&amp;quot;0&amp;quot;/>&amp;#xa;&quot; sortOrder=&quot;0&quot;/>&#xa;\"\" mode=\"\"1\"\" period=\"\"0\"\" incomes=\"\"Y\"\" interval=\"\"2\"\" lines=\"\"#NOTHING#\"\" lines2=\"\"#NOTHING#\"\" nbLevelLines=\"\"0\"\" nbLevelColumns=\"\"0\"\" forecast=\"\"0\"\" columns=\"\"d_DATEMONTH\"\" nb_intervals=\"\"1\"\" forecastValue=\"\"20\"\" transfers=\"\"Y\"\" currentPage=\"\"0\"\"/>\"";

    for (const auto &bk : std::as_const(bks)) {
        QStringList line = SKGServices::splitCSVLine(bk, '|', false);
        if (line.count() == 4) {
            SKGNodeObject node;
            err = SKGNodeObject::createPathNode(m_currentDocument, line.at(0), node);
            IFOKDO(err, node.setAutoStart(line.at(1) == QStringLiteral("Y")))
            IFOKDO(err, node.setIcon(line.at(2)))
            IFOKDO(err, node.setData(line.at(3)))
            IFOKDO(err, node.save())
        }
    }

    // status
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Standard bookmarks imported.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Import standard bookmarks failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBookmarkPlugin::goHome()
{
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGMainPanel::getMainPanel()->closeAllPages(false); // Home do not close pinned pages
    }

    SKGObjectBase::SKGListSKGObjectBase oNodeList;
    if (m_currentDocument != nullptr) {
        m_currentDocument->getObjects(QStringLiteral("v_node"), QStringLiteral("t_autostart='Y' ORDER BY f_sortorder, t_name"), oNodeList);
    }
    int nbAutoStartedBookmarks = oNodeList.count();
    for (int i = 0; i < nbAutoStartedBookmarks; ++i) {
        SKGNodeObject autostarted_bookmark(oNodeList.at(i));
        autostarted_bookmark.load();
        SKGTRACEIN(10, "autostarting bookmark : " % autostarted_bookmark.getName())
        SKGBookmarkPluginDockWidget::openBookmark(autostarted_bookmark, i > 0, true);
    }
}

void SKGBookmarkPlugin::onOpenBookmark()
{
    auto *callerAction = qobject_cast<QAction *>(this->sender());
    if (callerAction != nullptr) {
        SKGNodeObject node(m_currentDocument, callerAction->data().toInt());
        SKGBookmarkPluginDockWidget::openBookmark(node,
                                                  (QGuiApplication::mouseButtons() & Qt::MiddleButton) != 0u
                                                      || (QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u);
    }
}

void SKGBookmarkPlugin::onAddBookmark()
{
    SKGTRACEINFUNC(1)
    SKGError err;

    // Get current page
    SKGNodeObject node;
    {
        // Get current selection
        SKGNodeObject parentNode;
        auto *callerAction = qobject_cast<QAction *>(this->sender());
        if (callerAction != nullptr) {
            parentNode = SKGNodeObject(m_currentDocument, callerAction->data().toInt());
        }

        // Create bookmark
        err = SKGBookmarkPluginDockWidget::createNodeFromPage(SKGMainPanel::getMainPanel()->currentPage(), parentNode, node);
    }
    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("Successful message after an user action", "Bookmark created"));
    }
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBookmarkPlugin::onShowBookmarkMenu()
{
    auto *callerMenu = qobject_cast<QMenu *>(this->sender());
    if ((callerMenu != nullptr) && (m_currentDocument != nullptr)) {
        // Remove previous menu
        callerMenu->clear();

        // Build query
        QString wc = QStringLiteral("rd_node_id=0 OR rd_node_id IS NULL OR rd_node_id=''");
        int idParent = callerMenu->property("id").toInt();
        if (idParent != 0) {
            wc = "rd_node_id=" % SKGServices::intToString(idParent);
        }

        // Build new menu
        SKGObjectBase::SKGListSKGObjectBase listNode;
        m_currentDocument->getObjects(QStringLiteral("v_node"), wc % " ORDER BY f_sortorder, t_name", listNode);
        int nb = listNode.count();
        for (int i = 0; i < nb; ++i) {
            SKGNodeObject node(listNode.at(i));
            if (node.isFolder()) {
                // This is a folder
                auto menu = new QMenu(callerMenu);
                if (menu != nullptr) {
                    callerMenu->addMenu(menu);
                    menu->setTitle(node.getName());
                    menu->setIcon(node.getIcon());
                    menu->setProperty("id", node.getID());
                    connect(menu, &QMenu::aboutToShow, this, &SKGBookmarkPlugin::onShowBookmarkMenu);
                }
            } else {
                // This is a bookmark
                auto act = new QAction(callerMenu);
                if (act != nullptr) {
                    callerMenu->addAction(act);
                    act->setText(node.getName());
                    act->setIcon(node.getIcon());
                    act->setData(node.getID());
                    connect(act, &QAction::triggered, this, &SKGBookmarkPlugin::onOpenBookmark);
                }
            }
        }

        // Add separator
        auto sep = new QAction(this);
        sep->setSeparator(true);
        callerMenu->addAction(sep);

        // This common actions
        {
            // Open all
            auto act = new QAction(callerMenu);
            if (act != nullptr) {
                callerMenu->addAction(act);
                act->setText(i18nc("Action", "Open all"));
                act->setIcon(SKGServices::fromTheme(QStringLiteral("quickopen")));
                act->setData(idParent);
                connect(act, &QAction::triggered, this, &SKGBookmarkPlugin::onOpenBookmark);
            }
            if (SKGMainPanel::getMainPanel()->currentPageIndex() >= 0) {
                // Bookmark current here
                auto act2 = new QAction(callerMenu);
                if (act2 != nullptr) {
                    callerMenu->addAction(act2);
                    act2->setText(i18nc("Action", "Bookmark current page here"));
                    act2->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
                    act2->setData(idParent);

                    connect(act2, &QAction::triggered, this, &SKGBookmarkPlugin::onAddBookmark);
                }
            }
        }
    }
}

#include <skgbookmarkplugin.moc>
