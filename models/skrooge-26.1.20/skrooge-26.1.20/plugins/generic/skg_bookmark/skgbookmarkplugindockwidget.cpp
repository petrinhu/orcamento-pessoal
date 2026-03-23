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
#include "skgbookmarkplugindockwidget.h"

#include <kicondialog.h>

#include <qevent.h>
#include <qheaderview.h>
#include <qmenu.h>

#include "skgbookmark_settings.h"
#include "skgdocument.h"
#include "skginterfaceplugin.h"
#include "skgmainpanel.h"
#include "skgnodeobject.h"
#include "skgobjectmodelbase.h"
#include "skgsortfilterproxymodel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include <utility>

bool SKGBookmarkPluginDockWidget::m_middleClick = false;

SKGBookmarkPluginDockWidget::SKGBookmarkPluginDockWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGWidget(iParent, iDocument)
    , m_mainMenu(nullptr)
    , m_actDelete(nullptr)
    , m_actRename(nullptr)
    , m_actChangeIcon(nullptr)
    , m_actAddBookmark(nullptr)
    , m_actAddBookmarks(nullptr)
    , m_actAddBookmarkGroup(nullptr)
    , m_actSetAutostart(nullptr)
    , m_actUnsetAutostart(nullptr)
    , m_modelview(nullptr)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    QPalette newPalette = QApplication::palette();
    newPalette.setColor(QPalette::Base, Qt::transparent);
    ui.kBookmarksList->setPalette(newPalette);
    ui.kBookmarksList->setRootIsDecorated(false);
    ui.kBookmarksList->setAnimated(false); // Not compatible with double click
    ui.kBookmarksList->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui.kTools->setIcon(SKGServices::fromTheme(QStringLiteral("configure")));

    ui.kBookmarksList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.kBookmarksList, &SKGTreeView::customContextMenuRequested, this, &SKGBookmarkPluginDockWidget::showMenu);

    // Add model
    m_modelview =
        new SKGObjectModelBase(getDocument(), QStringLiteral("v_node"), QStringLiteral("1=1 ORDER BY f_sortorder, t_name"), this, QStringLiteral("rd_node_id"));
    auto modelproxy = new SKGSortFilterProxyModel(this);
    modelproxy->setSourceModel(m_modelview);
    ui.kBookmarksList->setModel(modelproxy);

    connect(ui.kFilterEdit, &QLineEdit::textChanged, this, [modelproxy](const QString &itext) {
        modelproxy->setFilterKeyColumn(-1);
        modelproxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        modelproxy->setFilterFixedString(itext);
    });

    connect(ui.kBookmarksList, &SKGTreeView::selectionChangedDelayed, this, &SKGBookmarkPluginDockWidget::refresh);
    connect(ui.kBookmarksList, &SKGTreeView::pressed, this, &SKGBookmarkPluginDockWidget::onBeforeOpenBookmark);
    connect(ui.kBookmarksList, &SKGTreeView::pressed, this, &SKGBookmarkPluginDockWidget::onOpenBookmark);
    connect(ui.kBookmarksList, &SKGTreeView::doubleClicked, this, &SKGBookmarkPluginDockWidget::onOpenBookmarkFolder);
    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::currentPageChanged, this, &SKGBookmarkPluginDockWidget::onPageChanged);

    connect(m_modelview, &SKGObjectModelBase::beforeReset, ui.kBookmarksList, &SKGTreeView::saveSelection);
    connect(m_modelview, &SKGObjectModelBase::afterReset, ui.kBookmarksList, &SKGTreeView::resetSelection);

    ui.kBookmarksList->setTextResizable(false);

    // Refresh action states
    refresh();
}

SKGBookmarkPluginDockWidget::~SKGBookmarkPluginDockWidget()
{
    SKGTRACEINFUNC(1)
    m_mainMenu = nullptr;
    m_actDelete = nullptr;
    m_actRename = nullptr;
    m_actChangeIcon = nullptr;
    m_actAddBookmark = nullptr;
    m_actAddBookmarks = nullptr;
    m_actAddBookmarkGroup = nullptr;
    m_actSetAutostart = nullptr;
    m_actUnsetAutostart = nullptr;
    m_modelview = nullptr;
}

QWidget *SKGBookmarkPluginDockWidget::mainWidget()
{
    return ui.kBookmarksList;
}

void SKGBookmarkPluginDockWidget::initMenu()
{
    if (m_mainMenu == nullptr) {
        // Build contextual menu
        m_mainMenu = new QMenu(ui.kBookmarksList);
        QAction *actExpandAll = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("format-indent-more")), i18nc("Noun, user action", "Expand all"));
        connect(actExpandAll, &QAction::triggered, ui.kBookmarksList, &SKGTreeView::expandAll);

        QAction *actCollapseAll =
            m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("format-indent-less")), i18nc("Noun, user action", "Collapse all"));
        connect(actCollapseAll, &QAction::triggered, ui.kBookmarksList, &SKGTreeView::collapseAll);
        ui.kBookmarksList->insertAction(nullptr, actCollapseAll);

        m_mainMenu->addSeparator();

        m_actAddBookmark = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("list-add")), i18nc("Verb", "Bookmark current page"));
        connect(m_actAddBookmark, &QAction::triggered, this, &SKGBookmarkPluginDockWidget::onAddBookmark);

        m_actAddBookmarks = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("list-add")), i18nc("Verb", "Bookmark all pages"));
        connect(m_actAddBookmarks, &QAction::triggered, this, &SKGBookmarkPluginDockWidget::onAddBookmarks);

        m_actAddBookmarkGroup = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("folder-new")), i18nc("Verb", "Add bookmark group"));
        connect(m_actAddBookmarkGroup, &QAction::triggered, this, &SKGBookmarkPluginDockWidget::onAddBookmarkGroup);

        m_mainMenu->addSeparator();

        m_actDelete = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("edit-delete")), i18nc("Verb, delete an item", "Delete"));
        connect(m_actDelete, &QAction::triggered, this, &SKGBookmarkPluginDockWidget::onRemoveBookmark);

        m_mainMenu->addSeparator();

        m_actSetAutostart = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("media-playback-start")),
                                                  i18nc("Verb, automatically load when the application is started", "Autostart"));
        connect(m_actSetAutostart, &QAction::triggered, this, &SKGBookmarkPluginDockWidget::onSetAutostart);

        m_actUnsetAutostart = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("media-playback-stop")), i18nc("Verb", "Remove Autostart"));
        connect(m_actUnsetAutostart, &QAction::triggered, this, &SKGBookmarkPluginDockWidget::onUnsetAutostart);

        m_actRename = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("edit-rename")), i18nc("Verb, change the name of an item", "Rename"));
        m_actRename->setShortcut(Qt::Key_F2);
        connect(m_actRename, &QAction::triggered, this, &SKGBookmarkPluginDockWidget::onRenameBookmark);

        m_actChangeIcon =
            m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("edit-image-face-add")), i18nc("Verb, change the icon of an item", "Change icon…"));
        connect(m_actChangeIcon, &QAction::triggered, this, &SKGBookmarkPluginDockWidget::onChangeIconBookmark);

        m_mainMenu->addSeparator();
        m_mainMenu->addAction(SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("tab_overwritebookmark")));

        ui.kTools->setMenu(m_mainMenu);
    }
}

void SKGBookmarkPluginDockWidget::showMenu(const QPoint iPos)
{
    initMenu();
    m_mainMenu->popup(ui.kBookmarksList->mapToGlobal(iPos));
}

void SKGBookmarkPluginDockWidget::onSetAutostart()
{
    SKGTRACEINFUNC(10)
    setAutostart(QStringLiteral("Y"));
}

void SKGBookmarkPluginDockWidget::onUnsetAutostart()
{
    SKGTRACEINFUNC(10)
    setAutostart(QStringLiteral("N"));
}

void SKGBookmarkPluginDockWidget::setAutostart(const QString &value)
{
    SKGTRACEINFUNC(10)
    SKGObjectBase::SKGListSKGObjectBase selectedBookmarks = getSelectedObjects();

    SKGError err;

    // foreach selected bookmark, set the t_autostart attribute to 'y' or 'n'
    {
        int nb = selectedBookmarks.count();
        SKGBEGINPROGRESSTRANSACTION(*getDocument(),
                                    value == QStringLiteral("Y") ? i18nc("Noun, name of the user action", "Autostart bookmarks")
                                                                 : i18nc("Noun, name of the user action", "Do not Autostart bookmarks"),
                                    err,
                                    nb)
        for (int i = 0; !err && i < nb; ++i) {
            SKGNodeObject bookmark(selectedBookmarks.at(i));
            err = bookmark.setAttribute(QStringLiteral("t_autostart"), value);
            IFOKDO(err, bookmark.save())

            // Send message
            IFOKDO(err,
                   bookmark.getDocument()->sendMessage(
                       i18nc("An information message", "The Autostart status of bookmark '%1' has been changed", bookmark.getDisplayName()),
                       SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err,
           SKGError(0,
                    value == QStringLiteral("Y") ? i18nc("Successful message after an user action", "Autostart bookmarks")
                                                 : i18nc("Successful message after an user action", "Do not Autostart bookmarks")))
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBookmarkPluginDockWidget::openBookmark(const SKGNodeObject &iNode, bool iFirstInNewPage, bool iPin)
{
    SKGTRACEINFUNC(1)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QVector<SKGNodeObject> nodes;
    nodes.reserve(20);
    nodes.push_back(iNode);

    int setForcusOn = 0;
    int nbTab = SKGMainPanel::getMainPanel()->countPages();
    if (nbTab != 0) {
        setForcusOn = nbTab - 1;

        SKGTabPage *cPage = SKGMainPanel::getMainPanel()->currentPage();
        if (m_middleClick || ((cPage != nullptr) && cPage->isPin())) {
            setForcusOn = nbTab;
        } else {
            setForcusOn = SKGMainPanel::getMainPanel()->currentPageIndex();
        }
    }

    int tabNumberForNextOpen = (m_middleClick || iFirstInNewPage ? -1 : SKGMainPanel::getMainPanel()->currentPageIndex());
    for (int i = 0; i < nodes.count(); ++i) { // WARNING: This list is modified dynamically
        SKGNodeObject selectedNode = nodes.at(i);
        QStringList data = SKGServices::splitCSVLine(selectedNode.getData());
        if (data.count() > 2) {
            // This bookmark has information ==> we open it
            SKGTabPage *page = SKGMainPanel::getMainPanel()->openPage(SKGMainPanel::getMainPanel()->getPluginByName(data[0]),
                                                                      tabNumberForNextOpen,
                                                                      data[2],
                                                                      selectedNode.getName(),
                                                                      SKGServices::intToString(selectedNode.getID()),
                                                                      i == nodes.count() - 1);
            if (page != nullptr) {
                if (skgbookmark_settings::pinhomebookmarks()) {
                    page->setPin(iPin);
                }
                SKGTabWidget *tab = SKGMainPanel::getMainPanel()->getTabWidget();
                if (tab != nullptr) {
                    tab->setTabIcon(tab->indexOf(page), selectedNode.getIcon());
                }
                selectedNode.opened = true;
                tabNumberForNextOpen = -1;
            }
        } else {
            // This bookmark is a folder ==> we open children by recursion
            SKGObjectBase::SKGListSKGObjectBase children;
            selectedNode.getNodes(children);
            for (const auto &item : std::as_const(children)) {
                nodes.push_back(SKGNodeObject(item));
            }
        }
    }
    QApplication::restoreOverrideCursor();

    // Set focus on first page
    SKGMainPanel::getMainPanel()->setCurrentPage(setForcusOn);
}

void SKGBookmarkPluginDockWidget::onBeforeOpenBookmark()
{
    m_middleClick = (QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u || (QApplication::mouseButtons() & Qt::MiddleButton) != 0u;
}

void SKGBookmarkPluginDockWidget::onOpenBookmark(const QModelIndex &index)
{
    SKGTRACEINFUNC(1)
    if (!(QApplication::mouseButtons() & Qt::RightButton)) {
        auto *proxyModel = qobject_cast<QSortFilterProxyModel *>(ui.kBookmarksList->model());
        auto *model = qobject_cast<SKGObjectModelBase *>(proxyModel->sourceModel());
        if (model != nullptr) {
            SKGNodeObject node(model->getObject(proxyModel->mapToSource(index)));
            if (!node.isFolder()) {
                openBookmark(node);
            }
        }
    }
}

void SKGBookmarkPluginDockWidget::onOpenBookmarkFolder(const QModelIndex &index)
{
    SKGTRACEINFUNC(1)
    if (!(QApplication::mouseButtons() & Qt::RightButton)) {
        auto *proxyModel = qobject_cast<QSortFilterProxyModel *>(ui.kBookmarksList->model());
        auto *model = qobject_cast<SKGObjectModelBase *>(proxyModel->sourceModel());
        if (model != nullptr) {
            SKGNodeObject node(model->getObject(proxyModel->mapToSource(index)));
            if (node.isFolder()) {
                openBookmark(node);
            }
        }
    }
}

void SKGBookmarkPluginDockWidget::onAddBookmarkGroup()
{
    SKGTRACEINFUNC(1)
    SKGError err;
    SKGNodeObject node;
    {
        // Get current selection name
        QString name;
        SKGObjectBase::SKGListSKGObjectBase bookmarks = getSelectedObjects();
        if (!bookmarks.isEmpty()) {
            SKGNodeObject parentNode(bookmarks.at(0));
            if (!parentNode.isFolder()) {
                // This is not a folder, we have to take the parent
                SKGNodeObject parentNodeTmp;
                parentNode.getParentNode(parentNodeTmp);
                parentNode = parentNodeTmp;
            }
            name = parentNode.getFullName();
        }

        // Add current name
        if (!name.isEmpty()) {
            name += OBJECTSEPARATOR;
        }
        name += i18nc("Default name for bookmark", "New bookmark");

        // Create bookmark folder
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Bookmark folder creation '%1'", name), err)
        IFOKDO(err, SKGNodeObject::createPathNode(getDocument(), name, node, true))

        // Send message
        IFOKDO(err,
               node.getDocument()->sendMessage(i18nc("An information message", "The bookmark folder '%1' has been added", node.getDisplayName()),
                                               SKGDocument::Hidden))
    }

    // status bar
    IFOK(err)
    {
        ui.kBookmarksList->selectObject(node.getUniqueID());
        err = SKGError(0, i18nc("Successful message after an user action", "Bookmark group created"));
    }
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBookmarkPluginDockWidget::onAddBookmarks()
{
    SKGTRACEINFUNC(1)
    SKGError err;
    SKGNodeObject rootNode;
    {
        // Get current selection name
        QString name;
        SKGObjectBase::SKGListSKGObjectBase bookmarks = getSelectedObjects();
        if (!bookmarks.isEmpty()) {
            SKGNodeObject parentNode(bookmarks.at(0));
            if (!parentNode.isFolder()) {
                // This is not a folder, we have to take the parent
                SKGNodeObject parentNodeTmp;
                parentNode.getParentNode(parentNodeTmp);
                parentNode = parentNodeTmp;
            }
            name = parentNode.getFullName();
        }

        // Add current name
        if (!name.isEmpty()) {
            name += OBJECTSEPARATOR;
        }
        name += i18nc("Default name for bookmark", "New bookmark");

        // Create bookmark folder
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Bookmarks creation"), err)
        err = SKGNodeObject::createPathNode(getDocument(), name, rootNode, true);
        int nb = SKGMainPanel::getMainPanel()->countPages();
        for (int i = 0; !err && i < nb; ++i) {
            SKGNodeObject node;
            err = createNodeFromPage(SKGMainPanel::getMainPanel()->page(i), rootNode, node);

            // Send message
            IFOKDO(err,
                   node.getDocument()->sendMessage(i18nc("An information message", "The bookmark '%1' has been added", node.getDisplayName()),
                                                   SKGDocument::Hidden))
        }
    }

    // status bar
    IFOK(err)
    {
        ui.kBookmarksList->selectObject(rootNode.getUniqueID());
        err = SKGError(0, i18nc("Successful message after an user action", "Bookmarks created"));
    }
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBookmarkPluginDockWidget::onAddBookmark()
{
    SKGTRACEINFUNC(1)
    SKGError err;

    // Get current page
    SKGNodeObject node;
    {
        // Get current selection
        SKGNodeObject parentNode(getDocument(), 0);
        SKGObjectBase::SKGListSKGObjectBase bookmarks = getSelectedObjects();
        if (!bookmarks.isEmpty()) {
            parentNode = bookmarks.at(0);
        }

        // Create bookmark
        err = createNodeFromPage(SKGMainPanel::getMainPanel()->currentPage(), parentNode, node);
    }
    // status bar
    IFOK(err)
    {
        ui.kBookmarksList->selectObject(node.getUniqueID());
        err = SKGError(0, i18nc("Successful message after an user action", "Bookmark created"));
    }
    SKGMainPanel::displayErrorMessage(err);
}

SKGError SKGBookmarkPluginDockWidget::createNodeFromPage(SKGTabPage *iPage, const SKGNodeObject &iParentNode, SKGNodeObject &oCreatedNode)
{
    SKGTRACEINFUNC(1)
    SKGError err;

    // Get current page
    oCreatedNode = SKGNodeObject();
    if (iPage != nullptr) {
        // Get current selection name
        QString name;
        SKGNodeObject parentNode = iParentNode;
        if (!parentNode.isFolder()) {
            // This is not a folder, we have to take the parent
            SKGNodeObject parentNodeTmp;
            parentNode.getParentNode(parentNodeTmp);
            parentNode = parentNodeTmp;
        }
        name = parentNode.getFullName();

        // Add current name
        if (!name.isEmpty()) {
            name += OBJECTSEPARATOR;
        }

        QString defaultName = iPage->objectName();
        QString defaultIcon;

        // Get title and icon from tab (in case of changed in bookmark)
        auto tab = SKGMainPanel::getMainPanel()->getTabWidget();
        if (tab) {
            auto index = tab->indexOf(iPage);
            if (index != -1) {
                defaultName = tab->tabText(index);
                defaultIcon = tab->tabIcon(index).name();
            }
        }
        name += defaultName.replace(QStringLiteral("&"), QString());

        // Create bookmark
        SKGBEGINTRANSACTION(*iParentNode.getDocument(), i18nc("Noun, name of the user action", "Bookmark creation '%1'", name), err)
        err = SKGNodeObject::createPathNode(iParentNode.getDocument(), name, oCreatedNode, true);
        IFOK(err)
        {
            QString value = SKGServices::stringToCsv(iPage->objectName()) % u';' % SKGServices::stringToCsv(defaultName) % u';'
                % SKGServices::stringToCsv(iPage->getState());

            err = oCreatedNode.setData(value);
            IFOKDO(err, oCreatedNode.setIcon(defaultIcon))
            IFOKDO(err, oCreatedNode.save())

            // Send message
            IFOKDO(err,
                   oCreatedNode.getDocument()->sendMessage(i18nc("An information message", "The bookmark '%1' has been added", oCreatedNode.getDisplayName()),
                                                           SKGDocument::Hidden))
        }
    }
    return err;
}

void SKGBookmarkPluginDockWidget::onRenameBookmark()
{
    SKGTRACEINFUNC(1)

    QItemSelectionModel *selModel = ui.kBookmarksList->selectionModel();
    auto *proxyModel = qobject_cast<QSortFilterProxyModel *>(ui.kBookmarksList->model());
    if ((proxyModel != nullptr) && (selModel != nullptr)) {
        auto *model = qobject_cast<SKGObjectModelBase *>(proxyModel->sourceModel());
        if (model != nullptr) {
            QModelIndexList indexes = selModel->selectedRows();
            if (indexes.count() == 1) {
                ui.kBookmarksList->edit(indexes.at(0));
            }
        }
    }
}

void SKGBookmarkPluginDockWidget::onChangeIconBookmark()
{
    SKGTRACEINFUNC(1)

    SKGObjectBase::SKGListSKGObjectBase bookmarks = getSelectedObjects();
    if (bookmarks.count() == 1) {
        SKGNodeObject node(bookmarks.at(0));
        KIconDialog diag(this);
        QString icon = diag.getIcon(KIconLoader::NoGroup);
        if (!icon.isEmpty()) {
            SKGError err;
            {
                SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Bookmark icon change"), err)
                err = node.setIcon(icon);
                IFOKDO(err, node.save())

                // Send message
                IFOKDO(err,
                       node.getDocument()->sendMessage(i18nc("An information message", "The icon of the bookmark '%1' has been changed", node.getDisplayName()),
                                                       SKGDocument::Hidden))
            }
            // status bar
            IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Bookmark icon changed")))
            SKGMainPanel::displayErrorMessage(err);
        }
    }
}

void SKGBookmarkPluginDockWidget::onRemoveBookmark()
{
    SKGTRACEINFUNC(1)
    SKGError err;
    {
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Bookmark delete"), err)
        // Get current selection
        SKGObjectBase::SKGListSKGObjectBase bookmarks = getSelectedObjects();
        int nb = bookmarks.count();
        for (int i = 0; i < nb && !err; ++i) {
            SKGNodeObject node(bookmarks.at(i));
            err = node.remove();
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Bookmark deleted")))
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBookmarkPluginDockWidget::onBookmarkEditorChanged()
{
    SKGTRACEINFUNC(10)

    // Enable bookmark buttons
    int nbSelectedObjects = getNbSelectedObjects();
    bool testAdd = getDocument()->getMainDatabase() != nullptr && nbSelectedObjects <= 1;
    if (m_actAddBookmarkGroup != nullptr) {
        m_actAddBookmarkGroup->setEnabled(testAdd);
    }
    if (m_actAddBookmark != nullptr) {
        m_actAddBookmark->setEnabled(testAdd && SKGMainPanel::getMainPanel()->currentPageIndex() >= 0);
    }
    if (m_actAddBookmarks != nullptr) {
        m_actAddBookmarks->setEnabled(testAdd && SKGMainPanel::getMainPanel()->currentPageIndex() >= 0);
    }
    if (m_actDelete != nullptr) {
        m_actDelete->setEnabled(nbSelectedObjects > 0);
    }
    if (m_actRename != nullptr) {
        m_actRename->setEnabled(nbSelectedObjects == 1);
    }
    if (m_actChangeIcon != nullptr) {
        m_actChangeIcon->setEnabled(nbSelectedObjects == 1);
    }
}

void SKGBookmarkPluginDockWidget::refresh()
{
    SKGTRACEINFUNC(10)
    // Bookmarks
    SKGObjectBase::SKGListSKGObjectBase bookmarks = getSelectedObjects();
    int nbSelectedObjects = bookmarks.count();
    if (nbSelectedObjects == 1) {
        SKGNodeObject node(bookmarks.at(0));
        if (m_actSetAutostart != nullptr) {
            m_actSetAutostart->setEnabled(node.getAttribute(QStringLiteral("t_autostart")) != QStringLiteral("Y"));
        }
        if (m_actUnsetAutostart != nullptr) {
            m_actUnsetAutostart->setEnabled(node.getAttribute(QStringLiteral("t_autostart")) == QStringLiteral("Y"));
        }
    } else {
        if (m_actSetAutostart != nullptr) {
            m_actSetAutostart->setEnabled(nbSelectedObjects > 1);
        }
        if (m_actUnsetAutostart != nullptr) {
            m_actUnsetAutostart->setEnabled(nbSelectedObjects > 1);
        }
    }

    onBookmarkEditorChanged();
}

void SKGBookmarkPluginDockWidget::onPageChanged()
{
    // Set current selection of dock
    QString id;
    SKGTabPage *cPage = SKGMainPanel::getMainPanel()->currentPage();
    if (cPage != nullptr) {
        id = cPage->getBookmarkID();
    }
    ui.kBookmarksList->selectObject(id % "-node");
}

void SKGBookmarkPluginDockWidget::resizeEvent(QResizeEvent *iEvent)
{
    if (iEvent != nullptr) {
        QSize newSize = iEvent->size();

        // Compute icon size
        int s = qMax(qMin(newSize.width() / 5, 64), 16);
        ui.kBookmarksList->setIconSize(QSize(s, s));
    }

    QWidget::resizeEvent(iEvent);
}
