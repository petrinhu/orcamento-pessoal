/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes skgmainpanel.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgmainpanel.h"

#include <KNotification>
#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kcombobox.h>
#include <kconfigdialog.h>
#include <kencodingfiledialog.h>
#include <kformat.h>
#include <kmessagebox.h>
#include <kmessagewidget.h>
#include <knotifyconfigwidget.h>
#include <kpluginfactory.h>
#include <kpluginmetadata.h>
#include <kselectaction.h>
#include <kstandardaction.h>
#include <qwindow.h>
#ifdef HAS_STATUSNOTIFIER
#include <kstatusnotifieritem.h>
#endif
#include <kstringhandler.h>
#include <ktoggleaction.h>
#include <ktoolbar.h>
#include <ktoolbarpopupaction.h>
#include <kxmlguifactory.h>

#ifdef HAS_ACTIVITIES
#include <PlasmaActivities/ResourceInstance>
#endif

#ifdef SKG_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#endif

#include <QRandomGenerator>
#include <QtCore5Compat/QTextCodec>
#include <qaction.h>
#include <qapplication.h>
#include <qcollator.h>
#include <qcompleter.h>
#include <qdesktopservices.h>
#include <qdir.h>
#include <qdom.h>
#include <qevent.h>
#include <qfiledialog.h>
#include <qheaderview.h>
#include <qlineedit.h>
#include <qmenu.h>
#include <qprocess.h>
#include <qprogressbar.h>
#include <qpushbutton.h>
#include <qscrollarea.h>
#include <qsplashscreen.h>
#include <qtabbar.h>
#include <qtabwidget.h>
#include <qurlquery.h>
#include <qvariant.h>
#include <utility>

#include <algorithm> // std::sort

#include "skgdefine.h"
#include "skginterfaceplugin.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include "skgtreeview.h"
#include "skgwebview.h"
#include "skgzoomselector.h"

struct doublePointer {
    void *p1;
    void *p2;
};

struct historyPage {
    SKGTabPage::SKGPageHistoryItem current;
    SKGTabPage::SKGPageHistoryItemList next;
    SKGTabPage::SKGPageHistoryItemList previous;
};

struct actionDetails {
    QPointer<QAction> action;
    QStringList tables;
    int min{};
    int max{};
    int ranking{};
    bool focus{};
};
class SKGMainPanelPrivate
{
public:
    Ui::skgmainpanel_base ui{};
    Ui::skgmainpanel_pref uipref{};

    SKGTabWidget *m_tabWidget;
    QSplashScreen *m_splashScreen;

    SKGDocument *m_currentDocument;

    QList<SKGInterfacePlugin *> m_pluginsList;
    QMap<QString, actionDetails> m_registeredGlogalAction;
    QList<historyPage> m_historyClosedPages;

    QMenu *m_contextMenu;
    QAction *m_actHideContextItem;
    QAction *m_actShowAllContextItems;
    QAction *m_actDelete;
    QAction *m_closePageAction;
    QAction *m_closeAllOtherPagesAction;
    QAction *m_switchPinState;
    QAction *m_saveDefaultStateAction;
    QAction *m_resetDefaultStateAction;
    QAction *m_overwriteBookmarkStateAction;
    QAction *m_configureAction;
    QAction *m_enableEditorAction;
    QAction *m_fullScreenAction;
    QAction *m_actLock;
    QAction *m_actUnLock;
    QAction *m_reopenLastClosed;
#ifdef HAS_STATUSNOTIFIER
    KStatusNotifierItem *m_kSystemTrayIcon;
#endif
    QLabel *m_kNormalMessage;
    SKGZoomSelector *m_zoomSelector;

    KToolBarPopupAction *m_previousAction;
    KToolBarPopupAction *m_nextAction;
    KToolBarPopupAction *m_buttonMenuAction;
    KToggleAction *m_showMenuBarAction;

    QMenu *m_previousMenu;
    QMenu *m_nextMenu;
    QMenu *m_buttonMenu;
    QWidget *m_mainWidget;
    QVBoxLayout *m_mainLayout{};

    doublePointer m_progressObjects{};
    bool m_middleClick;
    bool m_saveOnClose;
    QString m_fileName;
    SKGWidget *m_widgetHavingSelection;
    QStringList m_tipsOfTheDay;

#ifdef HAS_ACTIVITIES
    KActivities::ResourceInstance *m_activityResourceInstance;
#endif

    static bool m_currentActionCanceled;
    static SKGMainPanel *m_mainPanel;

    SKGMainPanelPrivate()
    {
        m_actHideContextItem = nullptr;
        m_actShowAllContextItems = nullptr;
        m_actDelete = nullptr;
        m_closePageAction = nullptr;
        m_closeAllOtherPagesAction = nullptr;
        m_switchPinState = nullptr;
        m_saveDefaultStateAction = nullptr;
        m_resetDefaultStateAction = nullptr;
        m_overwriteBookmarkStateAction = nullptr;
        m_configureAction = nullptr;
        m_enableEditorAction = nullptr;
        m_fullScreenAction = nullptr;
        m_actLock = nullptr;
        m_actUnLock = nullptr;
#ifdef HAS_STATUSNOTIFIER
        m_kSystemTrayIcon = nullptr;
#endif
        m_kNormalMessage = nullptr;
        m_zoomSelector = nullptr;
        m_previousAction = nullptr;
        m_nextAction = nullptr;
        m_buttonMenuAction = nullptr;
        m_mainWidget = nullptr;
        m_mainLayout = nullptr;
        m_middleClick = false;
        m_saveOnClose = false;
        m_widgetHavingSelection = nullptr;
        m_tabWidget = nullptr;
        m_splashScreen = nullptr;
        m_currentDocument = nullptr;
        m_contextMenu = nullptr;
        m_reopenLastClosed = nullptr;
        m_showMenuBarAction = nullptr;
        m_previousMenu = nullptr;
        m_nextMenu = nullptr;
        m_buttonMenu = nullptr;
        m_progressObjects.p1 = nullptr;
        m_progressObjects.p2 = nullptr;
#ifdef HAS_ACTIVITIES
        m_activityResourceInstance = nullptr;
#endif
    }

    static int progressBarCallBack(int iPos, qint64 iTime, const QString &iName, void *iProgressBar)
    {
        Q_UNUSED(iTime)
        QProgressBar *progressBar = nullptr;
        QPushButton *button = nullptr;
        auto *pointers = static_cast<doublePointer *>(iProgressBar);
        if (pointers != nullptr) {
            progressBar = static_cast<QProgressBar *>(pointers->p1);
            button = static_cast<QPushButton *>(pointers->p2);
        }

        bool visible = (iPos > 0 && iPos <= 100);
        if (progressBar != nullptr) {
            QString commonFormat =
                i18nc("Progress bar percent; %p is the value, % is the sign. Use the appropriate percent sign and positioning for your locale.", "%p%");
            /*if (iPos > 0) {
             *           qint64 estimatedTime = 100 * iTime / iPos;
             *           qint64 remainingTime = estimatedTime - iTime;
             *           commonFormat = commonFormat % " - " % i18nc("To print a remaining time (in seconde) / estimated time (in second)", "%1s / %2s",
             *                          SKGServices::intToString(remainingTime / 1000),
             *                          SKGServices::intToString(estimatedTime / 1000));
            }*/
            progressBar->setFormat(iName.isEmpty() ? commonFormat : commonFormat % '\n' % iName);
            progressBar->setValue(iPos);
            progressBar->setVisible(visible);
            if (iPos == 100) {
                QTimer::singleShot(300, Qt::CoarseTimer, progressBar, &QProgressBar::hide);
            }
            progressBar->setToolTip(iName);
#ifdef SKG_DBUS
            QDBusMessage message =
                QDBusMessage::createSignal(QStringLiteral("/org/") + KAboutData::applicationData().componentName() + QStringLiteral("/UnityLauncher"),
                                           QStringLiteral("com.canonical.Unity.LauncherEntry"),
                                           QStringLiteral("Update"));
            message << "application://org.kde.skrooge.desktop";

            QVariantMap setProperty;
            if (iPos == 100) {
                setProperty.insert(QStringLiteral("progress"), 0.0);
                setProperty.insert(QStringLiteral("progress-visible"), false);
            } else {
                setProperty.insert(QStringLiteral("progress"), static_cast<double>(iPos / 100.0));
                setProperty.insert(QStringLiteral("progress-visible"), true);
            }
            message << setProperty;
            QDBusConnection::sessionBus().send(message);
#endif
        }
        if (button != nullptr) {
            button->setVisible(visible);
            if (iPos == 100) {
                QTimer::singleShot(300, Qt::CoarseTimer, button, &QPushButton::hide);
            }
        }

        SKGMainPanelPrivate::m_currentActionCanceled = false;
        if (iPos != 0 && iPos != 100) {
            qApp->processEvents(QEventLoop::AllEvents, 500);
        }
        return (SKGMainPanelPrivate::m_currentActionCanceled ? 1 : 0);
    }

    static bool adviceLessThan(const SKGAdvice &s1, const SKGAdvice &s2)
    {
        if (s1.getPriority() == s2.getPriority()) {
            return (s1.getShortMessage() > s2.getShortMessage());
        }
        return (s1.getPriority() > s2.getPriority());
    }

    static void setAttribute(QDomElement &iRoot, const QString &iPath, const QString &iValue)
    {
        int pos = iPath.indexOf(u'.');
        if (pos == -1) {
            iRoot.setAttribute(iPath, iValue);
        } else {
            QString newElementName = iPath.left(pos);
            QString newAttribue = iPath.right(iPath.size() - pos - 1);

            QDomDocument doc(QStringLiteral("SKGML"));
            doc.setContent(iRoot.attribute(newElementName));
            QDomElement root = doc.documentElement();
            if (root.isNull()) {
                root = doc.createElement(QStringLiteral("parameters"));
                doc.appendChild(root);
            }
            setAttribute(root, newAttribue, iValue);

            iRoot.setAttribute(newElementName, doc.toString());
        }
    }

    void refreshTabPosition()
    {
        m_tabWidget->setTabPosition(static_cast<QTabWidget::TabPosition>(skgbasegui_settings::main_tabs_position()));
    }

    void rebuildSystemTray()
    {
#ifdef HAS_STATUSNOTIFIER
        if (skgbasegui_settings::icon_in_system_tray()) {
            if (m_kSystemTrayIcon == nullptr) {
                m_kSystemTrayIcon = new KStatusNotifierItem(m_mainPanel);
                m_kSystemTrayIcon->setStandardActionsEnabled(true);
                m_kSystemTrayIcon->setAssociatedWindow((QWindow *)qApp->activeWindow());
                m_kSystemTrayIcon->setIconByPixmap(QApplication::windowIcon());
            }
        } else {
            if (m_kSystemTrayIcon != nullptr) {
                delete m_kSystemTrayIcon;
                m_kSystemTrayIcon = nullptr;
            }
        }
#endif
    }
};

bool SKGMainPanelPrivate::m_currentActionCanceled = false;
SKGMainPanel *SKGMainPanelPrivate::m_mainPanel = nullptr;

SKGMainPanel::SKGMainPanel(QSplashScreen *iSplashScreen, SKGDocument *iDocument)
    : d(new SKGMainPanelPrivate)
{
    SKGTRACEINFUNC(1)
    d->m_tabWidget = new SKGTabWidget(this);
    d->m_splashScreen = iSplashScreen;
    d->m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg"), KAboutData::applicationData().displayName());
    setObjectName(qApp->applicationDisplayName());

    // Set main panel
    SKGMainPanelPrivate::m_mainPanel = this;
    auto w = new QScrollArea(this);
    w->setFrameShape(QFrame::NoFrame);
    w->setFocusPolicy(Qt::NoFocus);
    d->m_mainLayout = new QVBoxLayout(w);
    d->m_mainLayout->setSpacing(0);
    d->m_mainLayout->setContentsMargins(0, 0, 0, 0);
    d->m_mainLayout->addWidget(d->m_tabWidget);
    d->ui.setupUi(this);

    // setMainWidget(new QLabel("hello"));

    // Initialize settings
    KSharedConfigPtr config = KSharedConfig::openConfig();
    KConfigGroup prefskg = config->group(QStringLiteral("Main Panel"));
    int option = prefskg.readEntry("update_modified_contexts", 100);
    if (option == 100) {
        // First call, we set default values
        prefskg.writeEntry("update_modified_contexts", 2, KConfigBase::Normal);
        // NEVER: set following setting
        KMessageBox::saveDontShowAgainTwoActions(QStringLiteral("updateContextOnClose"), KMessageBox::SecondaryAction);
        SKGTRACEL(1) << "update_modified_contexts set to NEVER" << Qt::endl;
        SKGTRACEL(1) << "updateContextOnClose set to No" << Qt::endl;
    }

    option = prefskg.readEntry("update_modified_bookmarks", 100);
    if (option == 100) {
        // First call, we set default values
        prefskg.writeEntry("update_modified_bookmarks", 2, KConfigBase::Normal);
        // NEVER: set following setting
        KMessageBox::saveDontShowAgainTwoActions(QStringLiteral("updateBookmarkOnClose"), KMessageBox::SecondaryAction);
        SKGTRACEL(1) << "update_modified_bookmarks set to NEVER" << Qt::endl;
        SKGTRACEL(1) << "updateBookmarkOnClose set to No" << Qt::endl;
    }

    const auto plugins = KPluginMetaData::findPlugins(QStringLiteral("skg_gui"));

    // Load plugins
    int nb = plugins.count();
    SKGTRACEL(1) << nb << " plugins found" << Qt::endl;
    if (d->m_splashScreen != nullptr) {
        d->m_splashScreen->showMessage(i18nc("Splash screen message", "Loading plugins…"), Qt::AlignLeft, QColor(221, 130, 8)); // krazy:exclude=qmethods
    }
    SKGError err;
    QStringList listAuthors;
    QStringList listTasks;
    QStringList listEmails;
    QStringList listOscs;
    for (int i = 0; i < nb; ++i) {
        const auto &plugin = plugins.at(i);
        const auto name = plugin.name();
        const auto filename = plugin.fileName();
        QString id = plugin.value(QStringLiteral("X-Krunner-ID"));
        QString msg = i18nc("Splash screen message", "Loading plugin %1/%2: %3…", i + 1, nb, name);
        SKGTRACEL(1) << msg << " from " << filename << Qt::endl;
        if (d->m_splashScreen != nullptr) {
            d->m_splashScreen->showMessage(msg, Qt::AlignLeft, QColor(221, 130, 8)); // krazy:exclude=qmethods
        }

        const auto factory = KPluginFactory::loadFactory(plugin);
        const auto pluginInterface = factory.plugin ? factory.plugin->create<SKGInterfacePlugin>(this) : nullptr;
        if (pluginInterface != nullptr) {
            if (pluginInterface->isEnabled() && pluginInterface->setupActions(getDocument())) {
                // Add plugin in about
                QStringList listOfAuthors;
                QStringList listOfEmails;
                QStringList listOfPlugins;
                QString author = plugin.authors().at(0).name();
                QString email = plugin.authors().at(0).emailAddress();
                if (!author.isEmpty()) {
                    listOfAuthors.push_back(author);
                    listOfEmails.push_back(email);
                    listOfPlugins.push_back(name);
                }

                const auto subPlugins = pluginInterface->subPlugins();
                auto subPluginsNotFound = subPlugins;
                for (const QString &subPlugin : subPlugins) {
                    const auto subOffers = SKGServices::findDataPlugins(subPlugin);
                    int nbSubOffers = subOffers.count();
                    for (int j = 0; j < nbSubOffers; ++j) {
                        const auto &subService = subOffers.at(j);
                        subPluginsNotFound.removeAll(subPlugin);
                        auto autors = subService.authors();
                        if (autors.length()) {
                            auto author2 = subService.authors().at(0).name();
                            auto email2 = subService.authors().at(0).emailAddress();
                            if (!author2.isEmpty()) {
                                listOfAuthors.push_back(author2);
                                listOfEmails.push_back(email2);
                                listOfPlugins.push_back(subService.name());
                            }
                        }
                    }
                }

                for (const QString &subPlugin : subPluginsNotFound) {
                    const auto subOffers = KPluginMetaData::findPlugins(subPlugin);
                    int nbSubOffers = subOffers.count();
                    for (int j = 0; j < nbSubOffers; ++j) {
                        const auto subService = subOffers.at(j);
                        QString author2 = subService.value(QStringLiteral("X-KDE-PluginInfo-Author"), QString());
                        QString email2 = subService.value(QStringLiteral("X-KDE-PluginInfo-Email"), QString());
                        if (!author2.isEmpty()) {
                            listOfAuthors.push_back(author2);
                            listOfEmails.push_back(email2);
                            listOfPlugins.push_back(subService.name());
                        }
                    }
                }

                int nbAuthors = listOfAuthors.count();
                for (int j = 0; j < nbAuthors; ++j) {
                    QString authorId;
                    QString author = listOfAuthors.at(j);
                    QStringList authors = SKGServices::splitCSVLine(author, ',');
                    if (authors.count() == 2) {
                        author = authors.at(0);
                        authorId = authors.at(1);
                    }
                    int pos2 = listAuthors.indexOf(author);
                    if (pos2 == -1) {
                        listAuthors.push_back(author);
                        listTasks.push_back(i18n("Developer of plugin '%1'", listOfPlugins.at(j)));
                        listEmails.push_back(listOfEmails.at(j));
                        listOscs.push_back(authorId);
                    } else {
                        listTasks[pos2] += i18n(",\n '%1'", listOfPlugins.at(j));
                    }
                }

                // Store plugin
                int nbplugin = d->m_pluginsList.count();
                int pos3 = nbplugin;
                for (int j = nbplugin - 1; j >= 0; --j) {
                    if (pluginInterface->getOrder() < d->m_pluginsList.at(j)->getOrder()) {
                        pos3 = j;
                    }
                }

                d->m_pluginsList.insert(pos3, pluginInterface);
                pluginInterface->setObjectName(id);

                // Add tips
                d->m_tipsOfTheDay += pluginInterface->tips();
            } else {
                SKGTRACEL(1) << "Disabled" << Qt::endl;
            }
        } else {
            SKGTRACEL(1) << "Loading error: " << factory.errorString << Qt::endl;
        }
    }

    // Add credits
    KAboutData about = KAboutData::applicationData();
    nb = listAuthors.count();
    for (int i = 0; i < nb; ++i) {
        about.addCredit(listAuthors.at(i), listTasks.at(i), listEmails.at(i), QString(), listOscs.at(i));
    }
    KAboutData::setApplicationData(about);

    // accept dnd
    setAcceptDrops(true);

    // tell the KXmlGuiWindow that this is indeed the main widget
    setCentralWidget(w);

    d->refreshTabPosition();
    d->m_tabWidget->setMovable(true);
    d->m_tabWidget->setTabsClosable(true);
    d->m_tabWidget->setUsesScrollButtons(true);
    d->m_tabWidget->tabBar()->setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    d->m_tabWidget->setDocumentMode(true);
    d->m_tabWidget->show();

    // System tray
    d->rebuildSystemTray();

    // then, setup our actions
    setupActions();

    displayErrorMessage(err);

    // Add a status bar
    statusBar()->show();

    QPalette palette2 = QApplication::palette();
    palette2.setColor(QPalette::Base, Qt::transparent);
    d->ui.kContextList->setPalette(palette2);

    QAction *toggle = d->ui.kDockContext->toggleViewAction();
    QAction *panelAction = actionCollection()->addAction(QStringLiteral("view_context"));
    registerGlobalAction(QStringLiteral("view_context"), panelAction);
    panelAction->setCheckable(true);
    panelAction->setChecked(toggle->isChecked());
    panelAction->setText(toggle->text());
    actionCollection()->setDefaultShortcut(panelAction, Qt::SHIFT | Qt::Key_F9);
    connect(panelAction, &QAction::triggered, toggle, &QAction::trigger);
    connect(toggle, &QAction::toggled, panelAction, &QAction::setChecked);

    QAction *toggle2 = d->ui.kDockMessages->toggleViewAction();
    QAction *panelAction2 = actionCollection()->addAction(QStringLiteral("view_messages"));
    registerGlobalAction(QStringLiteral("view_messages"), panelAction2);
    panelAction2->setCheckable(true);
    panelAction2->setChecked(toggle2->isChecked());
    panelAction2->setText(toggle2->text());
    actionCollection()->setDefaultShortcut(panelAction2, Qt::SHIFT | Qt::Key_F8);
    connect(panelAction2, &QAction::triggered, toggle2, &QAction::trigger);
    connect(toggle2, &QAction::toggled, panelAction2, &QAction::setChecked);

    auto contextMenu = new KSelectAction(SKGServices::fromTheme(QStringLiteral("tab-new")), i18nc("Noun", "Pages"), this);
    registerGlobalAction(QStringLiteral("view_contextmenu"), contextMenu);

    // Add plugin in client in right order
    QList<QListWidgetItem *> pageNotVisible;
    KConfigGroup prefContextVisibility = config->group(QStringLiteral("Context Visibility"));
    int shortCutIndex = 0;
    QString shortCutPrefix = QStringLiteral("Ctrl+");
    int nbplugin = d->m_pluginsList.count();
    QList<QAction *> contextActionList;
    for (int j = 0; j < nbplugin; ++j) {
        SKGInterfacePlugin *pluginInterface = d->m_pluginsList.at(j);
        if (pluginInterface != nullptr) {
            // Creation of the item
            QString title = pluginInterface->title();
            SKGTRACEL(1) << "Add plugin (" << pluginInterface->getOrder() << ") : " << title << Qt::endl;
            if (!title.isEmpty()) {
                // Add menu item with shortcuts
                if (pluginInterface->isInPagesChooser()) {
                    bool visible = prefContextVisibility.readEntry(pluginInterface->objectName(), true);
                    QIcon icon = SKGServices::fromTheme(pluginInterface->icon());

                    auto contextItem = new QListWidgetItem(icon, title);
                    contextItem->setStatusTip(pluginInterface->statusTip());
                    contextItem->setToolTip(pluginInterface->toolTip());
                    contextItem->setData(12, j); // context item ==> plugin
                    int page2 = d->ui.kContextList->count();
                    pluginInterface->setProperty("contextItem", page2); // plugin ==> context item

                    d->ui.kContextList->addItem(contextItem);

                    QAction *newAction = contextMenu->addAction(icon, title);
                    if (newAction != nullptr) {
                        newAction->setCheckable(false);
                        newAction->setData(page2);
                        contextItem->setData(15, QVariant::fromValue(static_cast<void *>(newAction))); // context item ==> action
                        if (!shortCutPrefix.isEmpty()) {
                            ++shortCutIndex;
                            if (shortCutIndex == 10) {
                                shortCutIndex = 0;
                                if (shortCutPrefix == QStringLiteral("Ctrl+")) {
                                    shortCutPrefix += QStringLiteral("Alt+");
                                } else {
                                    shortCutPrefix = QString();
                                }
                            }
                            if (!shortCutPrefix.isEmpty()) {
                                actionCollection()->setDefaultShortcut(newAction, QString(shortCutPrefix % SKGServices::intToString(shortCutIndex)));
                            }
                        }
                        connect(newAction, &QAction::triggered, this, &SKGMainPanel::onOpenContext);

                        contextActionList.append(newAction);

                        if (!visible) {
                            pageNotVisible.push_back(contextItem);
                        }

                        // Register action
                        QString id = "page_" % pluginInterface->objectName();
                        registerGlobalAction(id, newAction);
                        registerGlobalAction(id, newAction);
                    }
                }

                // Create dock if needed
                QDockWidget *dockWidget = pluginInterface->getDockWidget();
                if (dockWidget != nullptr) {
                    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
                    tabifyDockWidget(d->ui.kDockContext, dockWidget);
                    tabifyDockWidget(dockWidget, d->ui.kDockContext);
                }
            }
        }
    }

    // Lock docs if needed
    KConfigGroup pref = getMainConfigGroup();
    if (pref.readEntry("docks_locked", false)) {
        onLockDocks();
    }

    if (!pref.readEntry("first_launch", false)) {
        this->setWindowState(windowState() | Qt::WindowMaximized);
        pref.writeEntry("first_launch", true);
    }

    // a call to KXmlGuiWindow::setupGUI() populates the GUI
    // with actions, using KXMLGUI.
    // It also applies the saved mainwindow settings, if any, and ask the
    // mainwindow to automatically save settings if changed: window size,
    // toolbar position, icon size, etc.
    setupGUI(Default, QStringLiteral("skgmainpanel.rc"));
    plugActionList(QStringLiteral("context_actionlist"), contextActionList);

    for (int j = 0; j < nbplugin; ++j) {
        SKGInterfacePlugin *pluginInterface = d->m_pluginsList.at(j);
        if (pluginInterface != nullptr) {
            QString title = pluginInterface->title();
            SKGTRACEL(1) << "Add plugin client (" << pluginInterface->getOrder() << ") : " << title << Qt::endl;
            guiFactory()->addClient(pluginInterface);
        }
    }

    // Hide items in context
    nb = pageNotVisible.count();
    for (int i = 0; i < nb; ++i) {
        setContextVisibility(pageNotVisible.at(i), false);
    }

    // Set status bar
    d->m_kNormalMessage = new QLabel(this);
    d->m_kNormalMessage->setSizePolicy(QSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred));

    auto kProgressBar = new QProgressBar(this);
    kProgressBar->setObjectName(QStringLiteral("kProgressBar"));
    kProgressBar->setValue(0);
    kProgressBar->setMinimumSize(100, 20);
    QSizePolicy sizePolicyProgessBar(QSizePolicy::Fixed, QSizePolicy::Expanding);
    kProgressBar->setSizePolicy(sizePolicyProgessBar);
    kProgressBar->setToolTip(i18nc("Widget description", "Progress of the current action"));
    QFont f = kProgressBar->font();
    f.setPointSize(6.0);
    kProgressBar->setFont(f);
    kProgressBar->setVisible(false);
    kProgressBar->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    kProgressBar->setRange(0, 100);

    d->m_zoomSelector = new SKGZoomSelector(this);
    d->m_zoomSelector->setSizePolicy(sizePolicyProgessBar);
    connect(d->m_zoomSelector, &SKGZoomSelector::changed, this, &SKGMainPanel::onZoomChanged);

    statusBar()->addWidget(d->m_kNormalMessage, 1);
    statusBar()->addPermanentWidget(kProgressBar);

    // Set Cancel button
    auto kCancelButton = new QPushButton();
    kCancelButton->setObjectName(QStringLiteral("kCancelButton"));
    kCancelButton->setIcon(SKGServices::fromTheme(QStringLiteral("media-playback-stop")));
    kCancelButton->setToolTip(i18nc("Widget description", "Cancel the current action"));
    kCancelButton->setStatusTip(i18nc("Widget description", "Cancel the current action"));
    kCancelButton->setVisible(false);
    connect(kCancelButton, &QPushButton::clicked, this, &SKGMainPanel::onCancelCurrentAction);

    //
    d->ui.kClearMessageBtn->setIcon(SKGServices::fromTheme(QStringLiteral("edit-clear")));
    connect(d->ui.kClearMessageBtn, &QPushButton::clicked, this, &SKGMainPanel::onClearMessages);

    // Add special button in toolbar
    KToolBar *tb = toolBar();
    if (tb != nullptr) {
        auto label = new QLabel(this);
        QSizePolicy sizePolicyLabel(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicyLabel.setHorizontalStretch(0);
        sizePolicyLabel.setVerticalStretch(0);
        sizePolicyLabel.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicyLabel);
        tb->addWidget(label);
        tb->addAction(d->m_buttonMenuAction);
    }

    statusBar()->addPermanentWidget(kCancelButton);
    statusBar()->addPermanentWidget(d->m_zoomSelector);

    // Set progress bar call back
    if (getDocument() != nullptr) {
        d->m_progressObjects.p1 = static_cast<void *>(kProgressBar);
        d->m_progressObjects.p2 = static_cast<void *>(kCancelButton);
        getDocument()->setProgressCallback(&SKGMainPanelPrivate::progressBarCallBack, static_cast<void *>(&d->m_progressObjects));
    }

    // Connection
    if (getDocument() != nullptr) {
        connect(getDocument(), &SKGDocument::transactionSuccessfullyEnded, this, &SKGMainPanel::refresh, Qt::QueuedConnection);
        connect(getDocument(), &SKGDocument::transactionSuccessfullyEnded, this, &SKGMainPanel::notify, Qt::QueuedConnection);
    }
    connect(d->ui.kContextList, &QListWidget::itemPressed, this, &SKGMainPanel::onBeforeOpenContext);
    connect(d->ui.kContextList, &QListWidget::itemPressed, this, &SKGMainPanel::onOpenContext);
    connect(d->m_tabWidget->tabBar(), &QTabBar::tabCloseRequested, this, &SKGMainPanel::closePageByIndex);
    connect(d->m_tabWidget->tabBar(), &QTabBar::tabBarClicked, this, [this](int index) {
        if ((QApplication::mouseButtons() & Qt::MiddleButton) != 0u) {
            closePageByIndex(index);
        }
    });
    connect(d->m_tabWidget->tabBar(), &QTabBar::tabBarDoubleClicked, this, &SKGMainPanel::addTab);
    connect(d->m_tabWidget->tabBar(), &QTabBar::currentChanged, this, &SKGMainPanel::currentPageChanged);
    connect(this, &SKGMainPanel::currentPageChanged, this, &SKGMainPanel::refresh);
    connect(this, &SKGMainPanel::currentPageChanged, this, &SKGMainPanel::selectionChanged);

    // Refresh
    refresh();

    d->ui.kContextList->installEventFilter(this);

    // Build contextual menu
    d->ui.kContextList->setContextMenuPolicy(Qt::CustomContextMenu);

    d->m_contextMenu = new QMenu(d->ui.kContextList);
    d->m_actHideContextItem = d->m_contextMenu->addAction(SKGServices::fromTheme(QStringLiteral("layer-visible-off")), i18nc("Verb", "Hide"));
    connect(d->m_actHideContextItem, &QAction::triggered, this, &SKGMainPanel::onHideContextItem);

    d->m_actShowAllContextItems = d->m_contextMenu->addAction(SKGServices::fromTheme(QStringLiteral("layer-visible-on")), i18nc("Verb", "Show all"));
    connect(d->m_actShowAllContextItems, &QAction::triggered, this, &SKGMainPanel::onShowAllContextItems);

    d->m_contextMenu->addSeparator();
    d->m_contextMenu->addAction(getGlobalAction(QStringLiteral("tab_savedefaultstate")));
    d->m_contextMenu->addAction(getGlobalAction(QStringLiteral("tab_resetdefaultstate")));

    connect(d->ui.kContextList, &QListWidget::customContextMenuRequested, this, &SKGMainPanel::showMenu);

#ifdef HAS_ACTIVITIES
    // Initialize kactivities resource instance
    d->m_activityResourceInstance = new KActivities::ResourceInstance(window()->winId());
    d->m_activityResourceInstance->setParent(this);
#endif

    // Show menu bar and status bar
    menuBar()->show();
    statusBar()->show();

    notify(); // Due to sendMessage not in a transaction
    d->m_splashScreen = nullptr;
}

SKGMainPanel::~SKGMainPanel()
{
    SKGTRACEINFUNC(1)
    qInstallMessageHandler(nullptr);
    SKGMainPanelPrivate::m_mainPanel = nullptr;
    disconnect(getDocument(), nullptr, this, nullptr);

    // close plugins
    int nb = d->m_pluginsList.count();
    for (int i = 0; i < nb; ++i) {
        auto plugin = getPluginByIndex(i);
        plugin->close();
    }

    if (getDocument() != nullptr) {
        getDocument()->close();
    }
    delete d;
}

void SKGMainPanel::showMenu(const QPoint iPos)
{
    if (d->m_contextMenu != nullptr) {
        d->m_contextMenu->popup(d->ui.kContextList->mapToGlobal(iPos));
    }
}

void SKGMainPanel::setContextVisibility(QListWidgetItem *iItem, bool iVisibility)
{
    if (iItem != nullptr) {
        // Hide item in context
        iItem->setHidden(!iVisibility);

        // Hide corresponding action
        QAction *act = static_cast<QAction *>(iItem->data(15).value<void *>());
        if (act != nullptr) {
            act->setVisible(iVisibility);
        }

        // Save state in settings
        SKGInterfacePlugin *plugin = getPluginByIndex(iItem->data(12).toInt());
        if (plugin != nullptr) {
            KSharedConfigPtr config = KSharedConfig::openConfig();
            KConfigGroup pref = config->group(QStringLiteral("Context Visibility"));
            pref.writeEntry(plugin->objectName(), iVisibility);
        }
    }
}

void SKGMainPanel::setContextVisibility(int iPage, bool iVisibility)
{
    setContextVisibility(d->ui.kContextList->item(iPage), iVisibility);
}

void SKGMainPanel::onHideContextItem()
{
    setContextVisibility(d->ui.kContextList->currentRow(), false);
}

void SKGMainPanel::onShowAllContextItems()
{
    int nb = d->ui.kContextList->count();
    for (int i = 0; i < nb; ++i) {
        setContextVisibility(i, true);
    }
}

SKGDocument *SKGMainPanel::getDocument() const
{
    return d->m_currentDocument;
}

QStringList SKGMainPanel::processArguments(const QStringList &iArgument)
{
    QStringList output = iArgument;
    for (auto plugin : std::as_const(d->m_pluginsList)) {
        if (plugin != nullptr) {
            output = plugin->processArguments(output);
        }
    }
    return output;
}

void SKGMainPanel::registerGlobalAction(const QString &iIdentifier,
                                        QAction *iAction,
                                        bool iAddInCollection,
                                        const QStringList &iListOfTable,
                                        int iMinSelection,
                                        int iMaxSelection,
                                        int iRanking,
                                        bool iSelectionMustHaveFocus)
{
    if (iAction == nullptr) {
        SKGTRACE << "WARNING: registerGlobalAction(" << iIdentifier << ",nullptr)" << Qt::endl;
    } else {
        if (d->m_registeredGlogalAction.contains(iIdentifier) && d->m_registeredGlogalAction.value(iIdentifier).action != iAction) {
            SKGTRACEL(1) << "WARNING: Another action already registered for identifier " << iIdentifier << Qt::endl;
        }
        QStringList keys = d->m_registeredGlogalAction.keys();
        for (const auto &id : std::as_const(keys)) {
            QPointer<QAction> act = d->m_registeredGlogalAction.value(id).action;
            if ((act != nullptr) && iIdentifier != id && act != iAction && !act->shortcut().isEmpty() && act->shortcut() == iAction->shortcut()) {
                SKGTRACE << "WARNING: The actions [" << iAction->text() << " (" << iIdentifier << ")] and [" << act->text() << " (" << id
                         << ")] has same shortcut [" << iAction->shortcut().toString() << "]" << Qt::endl;
            }
        }
        actionDetails actDetails;
        actDetails.action = iAction;
        actDetails.tables = iListOfTable;
        actDetails.min = iMinSelection;
        actDetails.max = iMaxSelection;
        actDetails.focus = iSelectionMustHaveFocus;
        actDetails.ranking = (iRanking == -1 ? 10 * (d->m_registeredGlogalAction.count() + 1) : iRanking);
        d->m_registeredGlogalAction[iIdentifier] = std::move(actDetails);

        // This connect has not been migrated on new connect mechanism to avoid crash when leaving the application
        connect(iAction, SIGNAL(destroyed(QObject *)), this, SLOT(unRegisterGlobalAction(QObject *))); // clazy:exclude=old-style-connect
        if (iAddInCollection) {
            QKeySequence shortCut = iAction->shortcut();
            if (!shortCut.isEmpty()) {
                iAction->setShortcut(QKeySequence());
            }
            actionCollection()->addAction(iIdentifier, iAction);
            if (!shortCut.isEmpty()) {
                actionCollection()->setDefaultShortcut(iAction, shortCut);
            }
        }
    }
}

void SKGMainPanel::unRegisterGlobalAction(QObject *iAction)
{
    auto *act = qobject_cast<QAction *>(iAction);
    if (act != nullptr) {
        const auto keys = d->m_registeredGlogalAction.keys();
        for (const auto &id : keys) {
            if (d->m_registeredGlogalAction.value(id).action == QPointer<QAction>(act)) {
                d->m_registeredGlogalAction.remove(id);
            }
        }
    }
}

QPointer<QAction> SKGMainPanel::getGlobalAction(const QString &iIdentifier, bool iWarnIfNotExist)
{
    QAction *act = d->m_registeredGlogalAction.value(iIdentifier).action;
    if (act == nullptr && iWarnIfNotExist) {
        SKGTRACE << "WARNING: getGlobalAction(" << iIdentifier << ")=nullptr" << Qt::endl;
    }
    return act;
}

QList<QPointer<QAction>> SKGMainPanel::getActionsForContextualMenu(const QString &iTable)
{
    // Filter action
    QVector<actionDetails> tmp;
    for (const auto &actDetails : std::as_const(d->m_registeredGlogalAction)) {
        if (actDetails.ranking > 0 && actDetails.min > 0) {
            if (actDetails.tables.isEmpty() || actDetails.tables.contains(iTable)) {
                tmp.push_back(actDetails);
            } else if (actDetails.tables.count() == 1 && actDetails.tables.at(0).startsWith(QStringLiteral("query:"))) {
                // Dynamic mode
                QStringList tmpListTable;
                getDocument()->getDistinctValues(QStringLiteral("sqlite_master"),
                                                 QStringLiteral("name"),
                                                 actDetails.tables.at(0).right(actDetails.tables.at(0).size() - 6),
                                                 tmpListTable);
                if (tmpListTable.contains(iTable)) {
                    tmp.push_back(actDetails);
                }
            }
        }
    }

    // Sort
    std::sort(tmp.begin(), tmp.end(), [&](const actionDetails &a, const actionDetails &b) {
        return a.ranking < b.ranking;
    });

    // Generate output
    int previousGroup = -1;
    QList<QPointer<QAction>> output;
    output.reserve(tmp.count());
    for (const auto &actDetails : std::as_const(tmp)) {
        int currentGroup = (actDetails.ranking) / 100;
        if (currentGroup != previousGroup) {
            output.push_back(nullptr);
            previousGroup = currentGroup;
        }
        output.push_back(actDetails.action);
    }
    return output;
}

QMap<QString, QPointer<QAction>> SKGMainPanel::getGlobalActions() const
{
    QMap<QString, QPointer<QAction>> map;
    const auto keys = d->m_registeredGlogalAction.keys();
    for (const auto &id : keys) {
        map[id] = d->m_registeredGlogalAction[id].action;
    }
    return map;
}

SKGInterfacePlugin *SKGMainPanel::getPluginByIndex(int iIndex)
{
    SKGInterfacePlugin *output = nullptr;
    if (iIndex >= 0 && iIndex < d->m_pluginsList.count()) {
        output = d->m_pluginsList.value(iIndex);
    }

    return output;
}

SKGInterfacePlugin *SKGMainPanel::getPluginByName(const QString &iName)
{
    SKGInterfacePlugin *output = nullptr;
    int nbplugin = d->m_pluginsList.count();
    QString name = iName.toLower();
    for (int j = 0; (output == nullptr) && j < nbplugin; ++j) {
        QString namep = d->m_pluginsList.at(j)->objectName().toLower();
        if (namep == name || namep.replace(' ', '_') == name) {
            output = d->m_pluginsList.at(j);
        }
    }

    return output;
}

void SKGMainPanel::setupActions()
{
    SKGTRACEINFUNC(1)

    // Std File
    KStandardAction::quit(this, SLOT(onQuitAction()), actionCollection());
    KStandardAction::configureNotifications(this, SLOT(onConfigureNotifications()), actionCollection());

    // Preferences
    KStandardAction::preferences(this, SLOT(optionsPreferences()), actionCollection());

    // New Tab
    auto actAddTab = new QAction(SKGServices::fromTheme(QStringLiteral("tab-new-background")), i18nc("Noun, user action", "New Tab"), this);
    actionCollection()->setDefaultShortcut(actAddTab, Qt::CTRL | Qt::SHIFT | Qt::Key_W);
    connect(actAddTab, &QAction::triggered, this, &SKGMainPanel::addTab);
    registerGlobalAction(QStringLiteral("new_tab"), actAddTab, true, QStringList(), -1);

    // Add new tab widget
    auto addTabButton = new QToolButton(this);
    addTabButton->setIcon(actAddTab->icon());
    addTabButton->setAutoRaise(true);
    addTabButton->raise();
    addTabButton->setDefaultAction(actAddTab);
    addTabButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    addTabButton->setFocusPolicy(Qt::NoFocus);
    d->m_tabWidget->setCornerWidget(addTabButton);

    d->m_actLock = new QAction(SKGServices::fromTheme(QStringLiteral("document-encrypt")), i18nc("Verb", "Lock panels"), this);
    connect(d->m_actLock, &QAction::triggered, this, &SKGMainPanel::onLockDocks);
    registerGlobalAction(QStringLiteral("view_lock"), d->m_actLock);

    d->m_actUnLock = new QAction(SKGServices::fromTheme(QStringLiteral("document-decrypt")), i18nc("Verb", "Unlock panels"), this);
    connect(d->m_actUnLock, &QAction::triggered, this, &SKGMainPanel::onUnlockDocks);
    registerGlobalAction(QStringLiteral("view_unlock"), d->m_actUnLock);

    d->m_switchPinState = new QAction(SKGServices::fromTheme(QStringLiteral("document-encrypt")), i18nc("Noun, user action", "Pin this page"), this);
    connect(d->m_switchPinState, &QAction::triggered, this, [this] {
        this->switchPinPage(nullptr);
    });
    registerGlobalAction(QStringLiteral("tab_switchpin"), d->m_switchPinState);

    //
    d->m_closePageAction = actionCollection()->addAction(KStandardAction::Close, QStringLiteral("tab_close"), this, SLOT(closeCurrentPage()));
    registerGlobalAction(QStringLiteral("tab_close"), d->m_closePageAction);

    //
    auto actCloseAllPages = new QAction(SKGServices::fromTheme(QStringLiteral("window-close")), i18nc("Noun, user action", "Close All"), this);
    actionCollection()->setDefaultShortcut(actCloseAllPages, Qt::ALT | Qt::Key_W);
    connect(actCloseAllPages, &QAction::triggered, this, &SKGMainPanel::closeAllPages);
    registerGlobalAction(QStringLiteral("tab_closeall"), actCloseAllPages, true, QStringList(), -1);

    //
    d->m_closeAllOtherPagesAction = new QAction(SKGServices::fromTheme(QStringLiteral("window-close")), i18nc("Noun, user action", "Close All Other"), this);
    actionCollection()->setDefaultShortcut(d->m_closeAllOtherPagesAction, Qt::CTRL | Qt::ALT | Qt::Key_W);
    connect(d->m_closeAllOtherPagesAction, &QAction::triggered, this, [this] {
        this->closeAllOtherPages(nullptr);
    });
    registerGlobalAction(QStringLiteral("tab_closeallother"), d->m_closeAllOtherPagesAction);

    //
    d->m_saveDefaultStateAction = new QAction(SKGServices::fromTheme(QStringLiteral("document-save")), i18nc("Noun, user action", "Save page state"), this);
    actionCollection()->setDefaultShortcut(d->m_saveDefaultStateAction, Qt::CTRL | Qt::ALT | Qt::Key_S);
    connect(d->m_saveDefaultStateAction, &QAction::triggered, this, &SKGMainPanel::saveDefaultState);
    registerGlobalAction(QStringLiteral("tab_savedefaultstate"), d->m_saveDefaultStateAction);

    //
    d->m_resetDefaultStateAction = new QAction(SKGServices::fromTheme(QStringLiteral("edit-clear")), i18nc("Noun, user action", "Reset page state"), this);
    actionCollection()->setDefaultShortcut(d->m_resetDefaultStateAction, Qt::CTRL | Qt::ALT | Qt::Key_R);
    connect(d->m_resetDefaultStateAction, &QAction::triggered, this, &SKGMainPanel::resetDefaultState);
    registerGlobalAction(QStringLiteral("tab_resetdefaultstate"), d->m_resetDefaultStateAction);

    //
    d->m_reopenLastClosed = new QAction(SKGServices::fromTheme(QStringLiteral("tab-new")), i18nc("Noun, user action", "Reopen last page closed"), this);
    actionCollection()->setDefaultShortcut(d->m_reopenLastClosed, Qt::CTRL | Qt::ALT | Qt::Key_T);
    connect(d->m_reopenLastClosed, &QAction::triggered, this, &SKGMainPanel::onReopenLastClosed);
    registerGlobalAction(QStringLiteral("tab_reopenlastclosed"), d->m_reopenLastClosed);

    //
    QStringList overlay;
    overlay.push_back(QStringLiteral("bookmarks"));
    d->m_overwriteBookmarkStateAction =
        new QAction(SKGServices::fromTheme(QStringLiteral("document-save"), overlay), i18nc("Noun, user action", "Overwrite bookmark state"), this);
    connect(d->m_overwriteBookmarkStateAction, &QAction::triggered, this, &SKGMainPanel::overwriteBookmarkState);
    actionCollection()->setDefaultShortcut(d->m_overwriteBookmarkStateAction, Qt::CTRL | Qt::ALT | Qt::Key_B);
    registerGlobalAction(QStringLiteral("tab_overwritebookmark"), d->m_overwriteBookmarkStateAction);

    //
    d->m_configureAction = new QAction(SKGServices::fromTheme(QStringLiteral("configure")), i18nc("Noun, user action", "Configure…"), this);
    connect(d->m_configureAction, &QAction::triggered, this, [this] {
        this->optionsPreferences();
    });
    registerGlobalAction(QStringLiteral("tab_configure"), d->m_configureAction);

    // Menu
    d->m_buttonMenuAction = new KToolBarPopupAction(SKGServices::fromTheme(QStringLiteral("configure")), QString(), this);
    d->m_buttonMenuAction->setToolTip(i18nc("Noun, user action", "Menu"));
    d->m_buttonMenu = d->m_buttonMenuAction->popupMenu();
    connect(d->m_buttonMenu, &QMenu::aboutToShow, this, &SKGMainPanel::onShowButtonMenu);
    registerGlobalAction(QStringLiteral("view_menu"), d->m_buttonMenuAction);

    d->m_showMenuBarAction = KStandardAction::showMenubar(this, SLOT(onShowMenuBar()), actionCollection());
    KConfigGroup pref = getMainConfigGroup();
    d->m_showMenuBarAction->setChecked(pref.readEntry("menubar_shown", true));
    QTimer::singleShot(200, Qt::CoarseTimer, this, &SKGMainPanel::onShowMenuBar);
    registerGlobalAction(QStringLiteral("options_show_menubar"), d->m_showMenuBarAction);

    //
    d->m_previousAction = new KToolBarPopupAction(SKGServices::fromTheme(QStringLiteral("go-previous")), i18nc("Noun, user action", "Previous"), this);
    connect(d->m_previousAction, &QAction::triggered, this, &SKGMainPanel::onPrevious);
    actionCollection()->setDefaultShortcut(d->m_previousAction, Qt::ALT | Qt::Key_Left);
    d->m_previousAction->setPriority(QAction::LowPriority);
    d->m_previousMenu = d->m_previousAction->popupMenu();
    connect(d->m_previousMenu, &QMenu::aboutToShow, this, &SKGMainPanel::onShowPreviousMenu);

    d->m_previousAction->setData(0);
    registerGlobalAction(QStringLiteral("go_previous"), d->m_previousAction);

    //
    d->m_nextAction = new KToolBarPopupAction(SKGServices::fromTheme(QStringLiteral("go-next")), i18nc("Noun, user action", "Next"), this);
    connect(d->m_nextAction, &QAction::triggered, this, &SKGMainPanel::onNext);
    actionCollection()->setDefaultShortcut(d->m_nextAction, Qt::ALT | Qt::Key_Right);
    d->m_nextAction->setPriority(QAction::LowPriority);
    d->m_nextMenu = d->m_nextAction->popupMenu();
    connect(d->m_nextMenu, &QMenu::aboutToShow, this, &SKGMainPanel::onShowNextMenu);

    d->m_nextAction->setData(0);
    registerGlobalAction(QStringLiteral("go_next"), d->m_nextAction);

    //
    d->m_fullScreenAction = actionCollection()->addAction(KStandardAction::FullScreen, QStringLiteral("fullscreen"), this, SLOT(onFullScreen()));
    registerGlobalAction(QStringLiteral("fullscreen"), d->m_fullScreenAction);

    //
    d->m_enableEditorAction = new QAction(SKGServices::fromTheme(QStringLiteral("appointment-new")), i18nc("Noun, user action", "Enable editor"), this);
    actionCollection()->setDefaultShortcut(d->m_enableEditorAction, Qt::CTRL | Qt::Key_Insert);
    connect(d->m_enableEditorAction, &QAction::triggered, this, &SKGMainPanel::enableEditor);
    registerGlobalAction(QStringLiteral("enable_editor"), d->m_enableEditorAction);

    //
    auto migrateSQLCipher = new QAction(SKGServices::fromTheme(QStringLiteral("run-build")), i18nc("Noun, user action", "Migrate to SQLCipher format"), this);
    connect(migrateSQLCipher, &QAction::triggered, this, &SKGMainPanel::onMigrateToSQLCipher);
    registerGlobalAction(QStringLiteral("migrate_sqlcipher"), migrateSQLCipher);

    // Contextual menu
    d->m_tabWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("new_tab")));
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_reopenlastclosed")));
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_close")));
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_closeall")));
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_closeallother")));
    {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        d->m_tabWidget->insertAction(nullptr, sep);
    }
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_switchpin")));
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_resetdefaultstate")));
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_savedefaultstate")));
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_overwritebookmark")));
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("tab_configure")));
    {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        d->m_tabWidget->insertAction(nullptr, sep);
    }
    d->m_tabWidget->insertAction(nullptr, getGlobalAction(QStringLiteral("fullscreen")));
}

void SKGMainPanel::enableEditor()
{
    SKGTabPage *cPage = currentPage();
    if (cPage != nullptr) {
        cPage->activateEditor();
    }
}

void SKGMainPanel::onPrevious()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    SKGTabPage *cPage = currentPage();
    if (cPage != nullptr) {
        // Get index in history of page to refresh
        int indexPrevious = qobject_cast<QAction *>(sender())->data().toInt();

        // Get lists
        SKGTabPage::SKGPageHistoryItemList listPrevious = cPage->getPreviousPages();
        if (indexPrevious < listPrevious.count()) {
            SKGTabPage::SKGPageHistoryItemList listNext = cPage->getNextPages();
            SKGTabPage::SKGPageHistoryItem current = currentPageHistoryItem();

            // Get item to refresh
            SKGTabPage::SKGPageHistoryItem item = listPrevious.at(indexPrevious);

            // Open page
            cPage = openPage(getPluginByName(item.plugin), currentPageIndex(), item.state, item.name, item.bookmarkID);
            if (cPage != nullptr) {
                cPage->setBookmarkID(item.bookmarkID);

                // Update lists
                listNext.insert(0, current);
                listPrevious.removeAt(indexPrevious);
                for (int i = 0; i < indexPrevious; ++i) {
                    SKGTabPage::SKGPageHistoryItem itemPrevious = listPrevious.at(0); // Because the list is modified
                    listNext.insert(0, itemPrevious);
                    listPrevious.removeAt(0);
                }

                // Set lists
                cPage->setPreviousPages(listPrevious);
                cPage->setNextPages(listNext);
            }

            refresh();
        }
    }
}

void SKGMainPanel::onShowPreviousMenu()
{
    if (d->m_previousMenu != nullptr) {
        d->m_previousMenu->clear();

        SKGTabPage *cPage = currentPage();
        if (cPage != nullptr) {
            SKGTabPage::SKGPageHistoryItemList list = cPage->getPreviousPages();
            int nb = list.count();
            for (int i = 0; i < nb; ++i) {
                QAction *act = d->m_previousMenu->addAction(SKGServices::fromTheme(list.at(i).icon), list.at(i).name);
                if (act != nullptr) {
                    act->setData(i);
                    connect(act, &QAction::triggered, this, &SKGMainPanel::onPrevious);
                }
            }
        }
    }
}

void SKGMainPanel::onShowMenuBar()
{
    bool test = d->m_showMenuBarAction->isChecked();
    menuBar()->setVisible(test);
    d->m_buttonMenuAction->setVisible(!test);

    KConfigGroup pref = getMainConfigGroup();
    pref.writeEntry("menubar_shown", test);
}

void SKGMainPanel::onFullScreen()
{
    auto *p = d->m_tabWidget;
    if (p != nullptr) {
        if (!d->m_fullScreenAction->isChecked()) {
            // No Full screen
            p->setWindowState(p->windowState() & ~Qt::WindowFullScreen); // reset
            d->m_mainLayout->addWidget(d->m_tabWidget);
        } else {
            bool atLeastOnePageOpened = (d->m_tabWidget->count() > 0);
            if (atLeastOnePageOpened) {
                // Activate Full screen mode
                p->setParent(nullptr);
                p->setWindowFlags(p->windowFlags() | Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowMaximizeButtonHint
                                  | Qt::WindowCloseButtonHint);
                p->setWindowState(p->windowState() | Qt::WindowFullScreen); // set
                p->show();

                displayMessage(i18nc("Information message",
                                     "You can exit full screen mode with %1 or with the contextual menu",
                                     d->m_fullScreenAction->shortcut().toString()));
            } else {
                d->m_fullScreenAction->setChecked(false);
                displayMessage(i18nc("Information message", "At least one page must be opened to enable full screen mode"), SKGDocument::Error);
            }
        }
    }
}

void SKGMainPanel::onNext()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    SKGTabPage *cPage = currentPage();
    if (cPage != nullptr) {
        // Get index in history of page to refresh
        int posNext = qobject_cast<QAction *>(sender())->data().toInt();

        // Get lists
        SKGTabPage::SKGPageHistoryItemList listPrevious = cPage->getPreviousPages();
        SKGTabPage::SKGPageHistoryItemList listNext = cPage->getNextPages();
        SKGTabPage::SKGPageHistoryItem current = currentPageHistoryItem();

        // Get item to refresh
        SKGTabPage::SKGPageHistoryItem item = listNext.at(posNext);

        // Open page
        cPage = openPage(getPluginByName(item.plugin), currentPageIndex(), item.state, item.name, item.bookmarkID);
        if (cPage != nullptr) {
            cPage->setBookmarkID(item.bookmarkID);

            // Update lists
            listPrevious.insert(0, current);
            listNext.removeAt(posNext);
            for (int i = 0; i < posNext; ++i) {
                SKGTabPage::SKGPageHistoryItem itemNext = listNext.at(0); // Because the list is modified
                listPrevious.insert(0, itemNext);
                listNext.removeAt(0);
            }

            // Set lists
            cPage->setPreviousPages(listPrevious);
            cPage->setNextPages(listNext);
        }

        refresh();
    }
}

void SKGMainPanel::onReopenLastClosed()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    SKGTabPage::SKGPageHistoryItem current = currentPageHistoryItem();

    // Get item to refresh
    historyPage item = d->m_historyClosedPages.takeLast();

    // Open page
    SKGTabPage *cPage = openPage(getPluginByName(item.current.plugin), -1, item.current.state, item.current.name, item.current.bookmarkID);
    if (cPage != nullptr) {
        cPage->setBookmarkID(item.current.bookmarkID);
        cPage->setNextPages(item.next);
        cPage->setPreviousPages(item.previous);
    }

    refresh();
}

void SKGMainPanel::onLockDocks()
{
    QObjectList cs = children();
    for (auto c : std::as_const(cs)) {
        auto *doc = qobject_cast<QDockWidget *>(c);
        if (doc != nullptr) {
            doc->setFeatures(QDockWidget::NoDockWidgetFeatures);
        }
    }

    KConfigGroup pref = getMainConfigGroup();
    pref.writeEntry("docks_locked", true);

    refresh();
}

void SKGMainPanel::onUnlockDocks()
{
    QObjectList cs = children();
    for (auto c : std::as_const(cs)) {
        auto *doc = qobject_cast<QDockWidget *>(c);
        if (doc != nullptr) {
            doc->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable
                             | QDockWidget::DockWidgetVerticalTitleBar);
        }
    }

    KConfigGroup pref = getMainConfigGroup();
    pref.writeEntry("docks_locked", false);

    refresh();
}

void SKGMainPanel::onConfigureNotifications()
{
    KNotifyConfigWidget::configure(this);
}

void SKGMainPanel::onShowNextMenu()
{
    if (d->m_nextMenu != nullptr) {
        d->m_nextMenu->clear();

        SKGTabPage *cPage = currentPage();
        if (cPage != nullptr) {
            SKGTabPage::SKGPageHistoryItemList list = cPage->getNextPages();
            int nb = list.count();
            for (int i = 0; i < nb; ++i) {
                QAction *act = d->m_nextMenu->addAction(SKGServices::fromTheme(list.at(i).icon), list.at(i).name);
                if (act != nullptr) {
                    act->setData(i);
                    connect(act, &QAction::triggered, this, &SKGMainPanel::onNext);
                }
            }
        }
    }
}

void SKGMainPanel::onShowButtonMenu()
{
    if (d->m_buttonMenu != nullptr) {
        d->m_buttonMenu->clear();

        QMenuBar *mb = menuBar();
        if (mb != nullptr) {
            d->m_buttonMenu->addActions(mb->actions());
        }
    }
}

bool SKGMainPanel::queryClose()
{
    SKGTRACEINFUNC(1)
    // Bug 2777697: To be sure that all page modifications are closed
    closeAllPages();
    // Bug 2777697:

    bool output = queryFileClose();

    // To be sure that the application is not closed in fullscreen mode
    if (output) {
        if (d->m_fullScreenAction->isChecked()) {
            d->m_fullScreenAction->trigger();
        }
    }

    return output;
}

void SKGMainPanel::setSaveOnClose(bool iSaveOnClose)
{
    d->m_saveOnClose = iSaveOnClose;
}

bool SKGMainPanel::queryFileClose()
{
    SKGTRACEINFUNC(1)
    bool output = true;
    if (getDocument()->getCurrentTransaction() != 0) {
        displayMessage(i18nc("skgtestimportskg", "The application cannot be closed when a transaction is running."), SKGDocument::Error);
        output = false;
    } else if (getDocument()->isFileModified()) {
        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
        int code = KMessageBox::PrimaryAction;
        QString fileName = getDocument()->getCurrentFileName();
        QAction *save = getGlobalAction(fileName.isEmpty() ? QStringLiteral("file_save_as") : QStringLiteral("file_save"));
        if (save != nullptr) {
            if (!d->m_saveOnClose) {
                code = KMessageBox::questionTwoActionsCancel(
                    this,
                    i18nc("Question", "The document has been modified.\nDo you want to save it before closing?"),
                    QString(),
                    KGuiItem(fileName.isEmpty() ? i18nc("Question", "Save as") : i18nc("Question", "Save"),
                             SKGServices::fromTheme(fileName.isEmpty() ? QStringLiteral("document-save-as") : QStringLiteral("document-save"))),
                    KGuiItem(i18nc("Question", "Do not save")));
            }
            if (code == KMessageBox::PrimaryAction) {
                save->trigger();
            }
            output = (code == KMessageBox::SecondaryAction || code == KMessageBox::PrimaryAction);
        } else {
            code = KMessageBox::questionTwoActions(this,
                                                   i18nc("Question", "Current modifications will not be saved.\nDo you want to continue?"),
                                                   i18nc("Question", "Close confirmation"),
                                                   KStandardGuiItem::apply(),
                                                   KStandardGuiItem::cancel());
            output = (code == KMessageBox::PrimaryAction);
        }
        QApplication::restoreOverrideCursor();
    }

    return output;
}

SKGObjectBase SKGMainPanel::getFirstSelectedObject() const
{
    SKGObjectBase selection;
    SKGWidget *cPage = (d->m_widgetHavingSelection != nullptr ? d->m_widgetHavingSelection : currentPage());
    if (cPage != nullptr) {
        selection = cPage->getFirstSelectedObject();
    }
    return selection;
}

SKGObjectBase::SKGListSKGObjectBase SKGMainPanel::getSelectedObjects() const
{
    SKGObjectBase::SKGListSKGObjectBase selection;
    SKGWidget *cPage = (d->m_widgetHavingSelection != nullptr ? d->m_widgetHavingSelection : currentPage());
    if (cPage != nullptr) {
        selection = cPage->getSelectedObjects();
    }
    return selection;
}

int SKGMainPanel::getNbSelectedObjects() const
{
    int nb = 0;
    SKGWidget *cPage = (d->m_widgetHavingSelection != nullptr ? d->m_widgetHavingSelection : currentPage());
    if (cPage != nullptr) {
        nb = cPage->getNbSelectedObjects();
    }
    return nb;
}

bool SKGMainPanel::hasSelectionWithFocus()
{
    bool output = false;
    SKGWidget *cPage = (d->m_widgetHavingSelection != nullptr ? d->m_widgetHavingSelection : currentPage());
    if (cPage != nullptr) {
        output = cPage->hasSelectionWithFocus();
    }
    return output;
}

SKGAdviceList SKGMainPanel::getAdvice() const
{
    SKGTRACEINFUNC(1)
    // Get list of ignored advice
    QString currentMonth = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));
    QStringList ignoredAdvice = getDocument()->getParameters(QStringLiteral("advice"), "t_value='I' OR t_value='I_" % currentMonth % u'\'');

    // Build the list of all advice by requesting all plugins
    SKGAdviceList globalAdviceList;
    int index = 0;
    while (index >= 0) {
        SKGInterfacePlugin *plugin = SKGMainPanel::getMainPanel()->getPluginByIndex(index);
        if (plugin != nullptr) {
            const auto list = plugin->advice(ignoredAdvice);
            for (const auto &ad : list) {
                if (!ignoredAdvice.contains(ad.getUUID()) && !ignoredAdvice.contains(SKGServices::splitCSVLine(ad.getUUID(), '|').at(0))) {
                    globalAdviceList.push_back(ad);
                }
            }
        } else {
            index = -2;
        }
        ++index;
    }
    std::sort(globalAdviceList.begin(), globalAdviceList.end(), SKGMainPanelPrivate::adviceLessThan);

    return globalAdviceList;
}

KConfigGroup SKGMainPanel::getMainConfigGroup()
{
    KSharedConfigPtr config = KSharedConfig::openConfig();
    return config->group(QStringLiteral("Main Panel"));
}

void SKGMainPanel::optionsPreferences(const QString &iPluginName)
{
    SKGTRACEINFUNC(1)
    // Compute page
    QString pluginName = iPluginName;
    if (pluginName.isEmpty()) {
        auto *act = qobject_cast<QAction *>(sender());
        if (act != nullptr) {
            pluginName = act->property("page").toString();
        }
    }
    if (pluginName.isEmpty() && (this->currentPage() != nullptr)) {
        pluginName = this->currentPage()->objectName();
    }
    SKGTRACEL(1) << "Open setting page: " << pluginName << Qt::endl;
    // Synchronize setting with confirmation panel
    if (skgbasegui_settings::update_modified_bookmarks() == 0) {
        KMessageBox::ButtonCode confirm;
        bool ask = KMessageBox::shouldBeShownTwoActions(QStringLiteral("updateBookmarkOnClose"), confirm);
        KConfigGroup pref = getMainConfigGroup();
        if (ask) {
            pref.writeEntry("update_modified_bookmarks", 0, KConfigBase::Normal);
            SKGTRACEL(1) << "update_modified_bookmarks set to ASK" << Qt::endl;
        } else if (confirm == KMessageBox::PrimaryAction) {
            pref.writeEntry("update_modified_bookmarks", 1, KConfigBase::Normal);
            SKGTRACEL(1) << "update_modified_bookmarks set to ALWAYS" << Qt::endl;
        } else {
            pref.writeEntry("update_modified_bookmarks", 2, KConfigBase::Normal);
            SKGTRACEL(1) << "update_modified_bookmarks set to NEVER" << Qt::endl;
        }
    }

    if (skgbasegui_settings::update_modified_contexts() == 0) {
        KMessageBox::ButtonCode confirm;
        bool ask = KMessageBox::shouldBeShownTwoActions(QStringLiteral("updateContextOnClose"), confirm);
        KConfigGroup pref = getMainConfigGroup();
        if (ask) {
            pref.writeEntry("update_modified_contexts", 0, KConfigBase::Normal);
            SKGTRACEL(1) << "update_modified_contexts set to ASK" << Qt::endl;
        } else if (confirm == KMessageBox::PrimaryAction) {
            pref.writeEntry("update_modified_contexts", 1, KConfigBase::Normal);
            SKGTRACEL(1) << "update_modified_contexts set to ALWAYS" << Qt::endl;
        } else {
            pref.writeEntry("update_modified_contexts", 2, KConfigBase::Normal);
            SKGTRACEL(1) << "update_modified_contexts set to NEVER" << Qt::endl;
        }
    }
    skgbasegui_settings::self()->load();

    if (KConfigDialog::showDialog(QStringLiteral("settings"))) {
        return;
    }

    auto dialog = new KConfigDialog(this, QStringLiteral("settings"), skgbasegui_settings::self());

    // Add main
    auto w = new QWidget();
    d->uipref.setupUi(w);
    d->uipref.kcfg_date_format->addItem(i18nc("Date format",
                                              "Short date (%1, %2)",
                                              QLocale().toString(QDate::currentDate(), QLocale::ShortFormat),
                                              QLocale().toString(QDate::currentDate().addDays(-10), QLocale::ShortFormat)));
    d->uipref.kcfg_date_format->addItem(i18nc("Date format",
                                              "Long date (%1, %2)",
                                              QLocale().toString(QDate::currentDate(), QLocale::LongFormat),
                                              QLocale().toString(QDate::currentDate().addDays(-10), QLocale::LongFormat)));
    d->uipref.kcfg_date_format->addItem(i18nc("Date format",
                                              "Fancy short date (%1, %2)",
                                              KFormat().formatRelativeDate(QDate::currentDate(), QLocale::ShortFormat),
                                              KFormat().formatRelativeDate(QDate::currentDate().addDays(-10), QLocale::ShortFormat)));
    d->uipref.kcfg_date_format->addItem(i18nc("Date format",
                                              "Fancy long date (%1, %2)",
                                              KFormat().formatRelativeDate(QDate::currentDate(), QLocale::LongFormat),
                                              KFormat().formatRelativeDate(QDate::currentDate().addDays(-10), QLocale::LongFormat)));
    d->uipref.kcfg_date_format->addItem(
        i18nc("Date format", "ISO date (%1, %2)", QDate::currentDate().toString(Qt::ISODate), QDate::currentDate().addDays(-10).toString(Qt::ISODate)));
    dialog->addPage(w, skgbasegui_settings::self(), i18nc("Noun", "General"), QStringLiteral("preferences-other"));

    // Add plugin in client in right order
    int nbplugin = d->m_pluginsList.count();
    for (int j = 0; j < nbplugin; ++j) {
        SKGInterfacePlugin *pluginInterface = getPluginByIndex(j);
        if (pluginInterface != nullptr) {
            QWidget *w2 = pluginInterface->getPreferenceWidget();
            if (w2 != nullptr) {
                auto icon = SKGServices::fromTheme(pluginInterface->icon());
                KPageWidgetItem *p = dialog->addPage(w2, pluginInterface->getPreferenceSkeleton(), pluginInterface->title(), icon.name());
                if ((p != nullptr) && pluginName == pluginInterface->objectName()) {
                    dialog->setCurrentPage(p);
                }
            }
        }
    }

    connect(dialog, &KConfigDialog::settingsChanged, this, &SKGMainPanel::onSettingsChanged);

    dialog->show();

    // Refresh
    refresh();
}

void SKGMainPanel::onSettingsChanged()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)
    {
        int nb = d->m_pluginsList.count();
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Save settings"), err, nb)

        // Refresh plugins
        for (int i = 0; !err && i < nb; ++i) {
            err = getPluginByIndex(i)->savePreferences();
            IFOKDO(err, getDocument()->stepForward(i + 1))
        }

        // Setting for tab position
        d->refreshTabPosition();

        // Setting for bookmarks modification
        {
            int option = skgbasegui_settings::update_modified_bookmarks();
            if (option == 0) {
                // ASK: remove following setting
                KMessageBox::enableMessage(QStringLiteral("updateBookmarkOnClose"));
                SKGTRACEL(1) << "updateBookmarkOnClose set to ASK" << Qt::endl;
            } else if (option == 1) {
                // ALWAYS: set following setting
                KMessageBox::saveDontShowAgainTwoActions(QStringLiteral("updateBookmarkOnClose"), KMessageBox::PrimaryAction);
                SKGTRACEL(1) << "updateBookmarkOnClose set to Yes" << Qt::endl;
            } else {
                // NEVER: set following setting
                KMessageBox::saveDontShowAgainTwoActions(QStringLiteral("updateBookmarkOnClose"), KMessageBox::SecondaryAction);
                SKGTRACEL(1) << "updateBookmarkOnClose set to No" << Qt::endl;
            }
        }
        {
            int option = skgbasegui_settings::update_modified_contexts();
            if (option == 0) {
                // ASK: remove following setting
                KMessageBox::enableMessage(QStringLiteral("updateContextOnClose"));
                SKGTRACEL(1) << "updateContextOnClose set to ASK" << Qt::endl;
            } else if (option == 1) {
                // ALWAYS: set following setting
                KMessageBox::saveDontShowAgainTwoActions(QStringLiteral("updateContextOnClose"), KMessageBox::PrimaryAction);
                SKGTRACEL(1) << "updateContextOnClose set to Yes" << Qt::endl;
            } else {
                // NEVER: set following setting
                KMessageBox::saveDontShowAgainTwoActions(QStringLiteral("updateContextOnClose"), KMessageBox::SecondaryAction);
                SKGTRACEL(1) << "updateContextOnClose set to No" << Qt::endl;
            }
        }
        skgbasegui_settings::self()->load();
    }

    // Rebuild system tray
    d->rebuildSystemTray();

    Q_EMIT settingsChanged();

    // Display error
    displayErrorMessage(err);
}

void SKGMainPanel::refresh()
{
    SKGTRACEINFUNC(1)

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Show/hide main widget
    bool atLeastOnePageOpened = (d->m_tabWidget->count() > 0);
    d->m_tabWidget->setVisible(atLeastOnePageOpened);
    if (d->m_mainWidget != nullptr) {
        d->m_mainWidget->setVisible(!atLeastOnePageOpened);
    }

    // Refresh actions
    d->m_widgetHavingSelection = qobject_cast<SKGWidget *>(sender());
    SKGObjectBase selection = SKGMainPanel::getMainPanel()->getFirstSelectedObject();
    int nbSelectedItems = SKGMainPanel::getMainPanel()->getNbSelectedObjects();
    bool hasFocus = SKGMainPanel::getMainPanel()->hasSelectionWithFocus();

    QString selectedTable = (nbSelectedItems > 0 ? selection.getRealTable() : QString());
    for (const auto &actDetails : std::as_const(d->m_registeredGlogalAction)) {
        QStringList tables = actDetails.tables;
        if (tables.count() == 1 && tables.at(0).startsWith(QStringLiteral("query:"))) {
            // Dynamic mode
            getDocument()->getDistinctValues(QStringLiteral("sqlite_master"), QStringLiteral("name"), tables.at(0).right(tables.at(0).size() - 6), tables);
        }

        bool enabled = (tables.contains(selectedTable) || tables.isEmpty()) && (nbSelectedItems >= actDetails.min)
            && (nbSelectedItems <= actDetails.max || actDetails.max == -1) && (!actDetails.focus || hasFocus);

        if (enabled && nbSelectedItems == 0 && (actDetails.min == 0 || actDetails.min == -1)) {
            // Check if a page is opened
            SKGTabPage *page = SKGMainPanel::getMainPanel()->currentPage();
            if (page != nullptr) {
                auto *view = qobject_cast<QAbstractItemView *>(page->mainWidget());
                enabled = (actDetails.min == -1 || view != nullptr);
            } else {
                enabled = false;
            }
        }
        if (actDetails.action != nullptr) {
            actDetails.action->setEnabled(enabled);
        }
    }

    // Refresh plugins
    int nb = d->m_pluginsList.count();
    for (int i = 0; i < nb; ++i) {
        getPluginByIndex(i)->refresh();
    }

    // Enable addTabeAction
    SKGTabPage *toSave = currentPage();
    if (d->m_switchPinState != nullptr) {
        if ((toSave != nullptr) && toSave->isPin()) {
            d->m_switchPinState->setText(i18nc("Noun, user action", "Unpin this page"));
        } else {
            d->m_switchPinState->setText(i18nc("Noun, user action", "Pin this page"));
        }
    }
    if (d->m_closePageAction != nullptr) {
        d->m_closePageAction->setEnabled(atLeastOnePageOpened && (toSave != nullptr) && !toSave->isPin());
    }
    if (d->m_switchPinState != nullptr) {
        d->m_switchPinState->setEnabled(atLeastOnePageOpened);
    }
    if (d->m_closeAllOtherPagesAction != nullptr) {
        d->m_closeAllOtherPagesAction->setEnabled(d->m_tabWidget->count() > 1);
    }
    if (d->m_reopenLastClosed != nullptr) {
        d->m_reopenLastClosed->setEnabled(!d->m_historyClosedPages.isEmpty());
    }

    if (d->m_saveDefaultStateAction != nullptr) {
        d->m_saveDefaultStateAction->setEnabled((toSave != nullptr) && !toSave->getDefaultStateAttribute().isEmpty());
    }
    if (d->m_resetDefaultStateAction != nullptr) {
        d->m_resetDefaultStateAction->setEnabled((toSave != nullptr) && !toSave->getDefaultStateAttribute().isEmpty());
    }
    if (d->m_overwriteBookmarkStateAction != nullptr) {
        d->m_overwriteBookmarkStateAction->setEnabled((toSave != nullptr) && !toSave->getBookmarkID().isEmpty());
    }
    if (d->m_enableEditorAction != nullptr) {
        d->m_enableEditorAction->setEnabled((toSave != nullptr) && toSave->isEditor());
    }
    if (d->m_zoomSelector != nullptr) {
        d->m_zoomSelector->setVisible((toSave != nullptr) && toSave->isZoomable());
        if (toSave != nullptr) {
            d->m_zoomSelector->setValue(toSave->zoomPosition());
            QWidget *zoomWidget = toSave->zoomableWidget();
            auto *treeView = qobject_cast<SKGTreeView *>(zoomWidget);
            if (treeView != nullptr) {
                disconnect(treeView, &SKGTreeView::zoomChanged, this, nullptr);
                connect(treeView, &SKGTreeView::zoomChanged, this, [this](int val) {
                    d->m_zoomSelector->setValue(val);
                });
            } else {
                auto *webView = qobject_cast<SKGWebView *>(zoomWidget);
                if (webView != nullptr) {
                    disconnect(webView, &SKGWebView::zoomChanged, this, nullptr);
                    connect(webView, &SKGWebView::zoomChanged, this, [this](int val) {
                        d->m_zoomSelector->setValue(val);
                    });
                }
            }
        }
    }

    if (d->m_actLock != nullptr) {
        d->m_actLock->setVisible(d->ui.kDockContext->features() != QDockWidget::NoDockWidgetFeatures);
    }
    if (d->m_actUnLock != nullptr) {
        d->m_actUnLock->setVisible(d->ui.kDockContext->features() == QDockWidget::NoDockWidgetFeatures);
    }

    if (d->m_previousAction != nullptr) {
        SKGTabPage::SKGPageHistoryItemList list;
        if (toSave != nullptr) {
            list = toSave->getPreviousPages();
        }
        d->m_previousAction->setEnabled(!list.isEmpty());
    }
    if (d->m_nextAction != nullptr) {
        SKGTabPage::SKGPageHistoryItemList list;
        if (toSave != nullptr) {
            list = toSave->getNextPages();
        }
        d->m_nextAction->setEnabled(!list.isEmpty());
    }

    // Set current selection of context
    d->ui.kContextList->clearSelection();
    if (toSave != nullptr) {
        // Get plugin of current page
        SKGInterfacePlugin *plugin = getPluginByName(toSave->objectName());
        int index = (plugin != nullptr ? plugin->property("contextItem").toInt() : -1);
        if (index != -1) {
            d->ui.kContextList->setCurrentItem(d->ui.kContextList->item(index));
        }
    }

    // Set window title
    QString modified;
    if (getDocument()->isFileModified()) {
        modified += i18nc("Noun, indicate that current document is modified", " [modified]");
    }
    if (getDocument()->isReadOnly()) {
        modified += i18nc("Noun, indicate that current document is loaded in read only", " [read only]");
    }
    QString fileName = getDocument()->getCurrentFileName();
    if (fileName.isEmpty()) {
        fileName = i18nc("Noun, default name for a new document", "Untitled");
    } else {
        if (fileName != d->m_fileName) {
            // The file name has been changed
            onClearMessages();

            d->m_fileName = fileName;
#ifdef HAS_ACTIVITIES
            if (!d->m_fileName.isEmpty()) {
                d->m_activityResourceInstance->setUri(d->m_fileName);
            }
#endif
        }
    }
    setWindowTitle(i18nc("Title of the main window", "%1%2", fileName, modified));

    QApplication::restoreOverrideCursor();
}

SKGTabPage::SKGPageHistoryItem SKGMainPanel::currentPageHistoryItem() const
{
    SKGTabPage::SKGPageHistoryItem cpage;
    int currentIndex = currentPageIndex();
    SKGTabPage *cPage = currentPage();
    if (currentIndex >= 0 && (cPage != nullptr)) {
        cpage.plugin = cPage->objectName();
        SKGInterfacePlugin *plugin = SKGMainPanel::getMainPanel()->getPluginByName(cpage.plugin);
        if (plugin != nullptr) {
            cpage.name = d->m_tabWidget->tabText(currentIndex);
            cpage.icon = plugin->icon();
        }
        cpage.state = cPage->getState();
        cpage.bookmarkID = cPage->getBookmarkID();
    }

    return cpage;
}

SKGTabPage *SKGMainPanel::openPage(SKGInterfacePlugin *plugin, int index, const QString &parameters, const QString &title, const QString &iID, bool iSetCurrent)
{
    SKGTRACEINFUNC(1)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    bool previous = d->m_tabWidget->blockSignals(true);
    // If the current page is pin, then open new page
    SKGTabPage *cPage = currentPage();
    if ((cPage != nullptr) && cPage->isPin()) {
        index = -1;
        iSetCurrent = true;
    }

    SKGTabPage *w = nullptr;

    SKGTabPage::SKGPageHistoryItemList previousPages;
    if (index != -1) {
        int currentIndex = currentPageIndex();
        if (currentIndex >= 0 && (cPage != nullptr)) {
            previousPages = cPage->getPreviousPages();
            previousPages.insert(0, currentPageHistoryItem());

            d->m_tabWidget->removeTab(currentIndex);
            closePage(cPage);

            // Repair the history of closed page
            if (!d->m_historyClosedPages.isEmpty()) {
                d->m_historyClosedPages.removeLast();
            }
        }
    }

    if (plugin != nullptr) {
        w = plugin->getWidget();
        if (w != nullptr) {
            // Title
            QString title2 = (title.isEmpty() ? plugin->title() : title);
            w->setObjectName(plugin->objectName());
            if (!iID.isEmpty()) {
                w->setBookmarkID(iID);
            }

            QString param = parameters;
            if (param.isEmpty()) {
                QString def = w->getDefaultStateAttribute();
                if (!def.isEmpty()) {
                    param = getDocument()->getParameter(def);
                }
            }
            SKGTRACEL(10) << "state=[" << param << "]" << Qt::endl;
            w->setState(param);
            connect(w, &SKGTabPage::selectionChanged, this, &SKGMainPanel::refresh);
            connect(w, &SKGTabPage::selectionChanged, this, &SKGMainPanel::selectionChanged);
            connect(w, &SKGTabPage::selectionFocusChanged, this, &SKGMainPanel::refresh);

            if (index == -1) {
                SKGTRACEINFUNC(20)
                d->m_tabWidget->addTab(w, SKGServices::fromTheme(plugin->icon()), title2);
                if (iSetCurrent) {
                    d->m_tabWidget->setCurrentWidget(w);
                }
            } else {
                SKGTRACEINFUNC(20)
                d->m_tabWidget->insertTab(index, w, SKGServices::fromTheme(plugin->icon()), title2);
                if (iSetCurrent) {
                    d->m_tabWidget->setCurrentWidget(w);
                }

                w->setPreviousPages(previousPages);
                SKGTabPage::SKGPageHistoryItemList empty;
                w->setNextPages(empty);
            }
            SKGTRACEL(1) << "opening plugin [" << plugin->objectName() << ']' << Qt::endl;
            Q_EMIT pageOpened();
        }
    } else {
        getDocument()->sendMessage(i18nc("An information message", "Impossible to open the page because the plugin was not found"), SKGDocument::Error);
        notify(); // Due to sendMessage not in a transaction
    }

    // Show/hide main widget
    bool atLeastOnePageOpened = (d->m_tabWidget->count() > 0);
    d->m_tabWidget->setVisible(atLeastOnePageOpened);
    if (d->m_mainWidget != nullptr) {
        d->m_mainWidget->setVisible(!atLeastOnePageOpened);
    }

    d->m_tabWidget->blockSignals(previous);
    if (iSetCurrent) {
        Q_EMIT currentPageChanged();
    }
    QApplication::restoreOverrideCursor();
    return w;
}

int SKGMainPanel::currentPageIndex() const
{
    return d->m_tabWidget->currentIndex();
}

SKGTabPage *SKGMainPanel::currentPage() const
{
    return qobject_cast<SKGTabPage *>(d->m_tabWidget->currentWidget());
}

int SKGMainPanel::pageIndex(SKGTabPage *iPage) const
{
    int nb = countPages();
    for (int i = 0; i < nb; ++i) {
        if (page(i) == iPage) {
            return i;
        }
    }

    return -1;
}

QSplashScreen *SKGMainPanel::splashScreen() const
{
    return d->m_splashScreen;
}

int SKGMainPanel::countPages() const
{
    return d->m_tabWidget->count();
}

SKGTabPage *SKGMainPanel::page(int iIndex) const
{
    return qobject_cast<SKGTabPage *>(d->m_tabWidget->widget(iIndex));
}

void SKGMainPanel::setCurrentPage(int iIndex)
{
    d->m_tabWidget->setCurrentIndex(iIndex);
}

void SKGMainPanel::onBeforeOpenContext()
{
    d->m_middleClick = ((QApplication::mouseButtons() & Qt::MiddleButton) != 0u);
}

bool SKGMainPanel::openPage(const QUrl &iUrl, bool iNewPage)
{
    const QUrl &url(iUrl);
    if (url.scheme() == QStringLiteral("skg")) {
        // Get plugin
        SKGInterfacePlugin *plugin = getPluginByName(url.host());
        if (plugin != nullptr) {
            // Open special page
            SKGTabPage *w = plugin->getWidget();
            if (w != nullptr) {
                // Create xml
                QString path = url.path().remove('/');
                QDomDocument doc(QStringLiteral("SKGML"));
                doc.setContent(getDocument()->getParameter(path.isEmpty() ? w->getDefaultStateAttribute() : path));
                QDomElement root = doc.documentElement();
                if (root.isNull()) {
                    root = doc.createElement(QStringLiteral("parameters"));
                    doc.appendChild(root);
                }
                auto params = QUrlQuery(url).queryItems();
                for (const auto &p : std::as_const(params)) {
                    QString value = QUrl::fromPercentEncoding(p.second.toUtf8());
                    SKGMainPanelPrivate::setAttribute(root, p.first, value);
                }

                // Open page
                openPage(plugin, iNewPage ? -1 : currentPageIndex(), doc.toString(), url.fragment());
                return true;
            }
        } else {
            // Trigger action
            QAction *act = SKGMainPanel::getMainPanel()->getGlobalAction(url.host());
            if (act != nullptr) {
                auto params = QUrlQuery(url).queryItems();
                for (const auto &p : std::as_const(params)) {
                    QString value = QUrl::fromPercentEncoding(p.second.toUtf8());
                    act->setProperty(p.first.toUtf8().data(), value);
                }

                act->trigger();
                return true;
            }
        }
    } else {
        QDesktopServices::openUrl(iUrl);
        return true;
    }
    displayErrorMessage(SKGError(ERR_ABORT, i18nc("Error message", "Unknown plugin or action [%1] in url [%2]", url.host(), iUrl.toString())));

    return false;
}

bool SKGMainPanel::openPage()
{
    return openPage(QString());
}

bool SKGMainPanel::openPage(const QString &iUrl, bool iNewPage)
{
    // Get the url
    QString urlString(iUrl);
    if (urlString.isEmpty()) {
        auto *act = qobject_cast<QAction *>(sender());
        if (act != nullptr) {
            urlString = act->data().toString();
        }
    }

    return openPage(QUrl(urlString), iNewPage);
}

SKGTabPage *SKGMainPanel::openPage(int iPage, bool iNewPage)
{
    SKGTRACEINFUNC(1)
    SKGTRACEL(1) << "iPage=" << iPage << Qt::endl;
    int index = d->ui.kContextList->item(iPage)->data(12).toInt();
    return openPage(getPluginByIndex(index), iNewPage ? -1 : currentPageIndex());
}

void SKGMainPanel::onOpenContext()
{
    SKGTRACEINFUNC(1)
    if (!(QApplication::mouseButtons() & Qt::RightButton)) {
        int cpage = -1;
        auto *s = qobject_cast<QAction *>(this->sender());
        if (s != nullptr) {
            cpage = s->data().toInt();
        } else {
            cpage = d->ui.kContextList->currentRow();
        }
        if (cpage != -1) {
            openPage(cpage,
                     ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u) || d->m_middleClick
                         || ((QGuiApplication::mouseButtons() & Qt::MiddleButton) != 0u));
        }
    }
    d->m_middleClick = false;
}

void SKGMainPanel::switchPinPage(QWidget *iWidget)
{
    auto *toSwitch = qobject_cast<SKGTabPage *>(iWidget);
    if (toSwitch == nullptr) {
        toSwitch = currentPage();
    }

    if (toSwitch != nullptr) {
        toSwitch->setPin(!toSwitch->isPin());
        Q_EMIT currentPageChanged();
    }
}

void SKGMainPanel::closePageByIndex(int iIndex)
{
    QWidget *w = nullptr;
    if (iIndex >= 0) {
        w = d->m_tabWidget->widget(iIndex);
    } else {
        w = d->m_tabWidget->currentWidget();
    }

    closePage(w);
}

void SKGMainPanel::closeCurrentPage()
{
    closePage(nullptr);
}

void SKGMainPanel::closePage(QWidget *iWidget, bool iForce)
{
    SKGTRACEINFUNC(1)
    if (getDocument()->getCurrentTransaction() != 0) {
        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
        displayMessage(i18nc("Information message", "A page cannot be closed when a transaction is running."), SKGDocument::Information);
        QApplication::restoreOverrideCursor();
    } else {
        auto *toRemove = qobject_cast<SKGTabPage *>(iWidget);
        if (toRemove == nullptr) {
            toRemove = currentPage();
        }
        if ((toRemove != nullptr) && toRemove->close(iForce)) {
            historyPage item;
            item.current = currentPageHistoryItem();
            item.next = toRemove->getNextPages();
            item.previous = toRemove->getPreviousPages();
            d->m_historyClosedPages.push_back(item);
            delete toRemove;

            Q_EMIT pageClosed();
        }
    }

    // Show/hide main widget
    bool atLeastOnePageOpened = (d->m_tabWidget->count() > 0);
    d->m_tabWidget->setVisible(atLeastOnePageOpened);
    if (d->m_mainWidget != nullptr) {
        d->m_mainWidget->setVisible(!atLeastOnePageOpened);
    }

    if (!atLeastOnePageOpened) {
        d->m_fullScreenAction->setChecked(false);
        onFullScreen();
    }
}

void SKGMainPanel::closeAllPages(bool iForce)
{
    SKGTRACEINFUNC(1)
    bool previous = d->m_tabWidget->blockSignals(true);
    int nb = d->m_tabWidget->count();
    for (int i = nb - 1; i >= 0; --i) {
        auto *w = qobject_cast<SKGTabPage *>(d->m_tabWidget->widget(i));
        if ((w != nullptr) && (iForce || !w->isPin())) {
            closePage(w, iForce);
        }
    }
    d->m_tabWidget->blockSignals(previous);
    KMessageBox::enableMessage(QStringLiteral("closepinnedpage"));
    Q_EMIT currentPageChanged();
}

void SKGMainPanel::closeAllOtherPages(QWidget *iWidget)
{
    SKGTRACEINFUNC(1)
    bool previous = d->m_tabWidget->blockSignals(true);
    QWidget *toKeep = iWidget;
    if (toKeep == nullptr) {
        toKeep = currentPage();
    }

    int nb = d->m_tabWidget->count();
    for (int i = nb - 1; i >= 0; --i) {
        auto *w = qobject_cast<SKGTabPage *>(d->m_tabWidget->widget(i));
        if ((w != nullptr) && w != toKeep && !w->isPin()) {
            closePage(w);
        }
    }
    d->m_tabWidget->blockSignals(previous);
    Q_EMIT currentPageChanged();
}

void SKGMainPanel::saveDefaultState()
{
    SKGTRACEINFUNC(1)
    SKGError err;
    SKGTabPage *toSave = currentPage();
    if (toSave != nullptr) {
        // Get bookmarks uuid
        QString uuid = toSave->getBookmarkID();

        // Reset bookmarks uuid to overwrite page state
        toSave->setBookmarkID(QString());

        // Overwrite
        toSave->overwrite(false);

        // Set original bookmarks uuid
        toSave->setBookmarkID(uuid);
    }
}

void SKGMainPanel::overwriteBookmarkState()
{
    SKGTRACEINFUNC(1)
    SKGError err;
    SKGTabPage *toSave = currentPage();
    if (toSave != nullptr) {
        // Get bookmarks uuid
        QString uuid = toSave->getBookmarkID();
        if (!uuid.isEmpty()) {
            // Overwrite
            toSave->overwrite(false);
        }
    }
}

void SKGMainPanel::resetDefaultState()
{
    SKGTRACEINFUNC(1)
    SKGError err;
    SKGTabPage *toSave = currentPage();
    if (toSave != nullptr) {
        QString name = toSave->getDefaultStateAttribute();
        if (!name.isEmpty()) {
            SKGBEGINLIGHTTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Reset default state"), err)
            IFOKDO(err, getDocument()->setParameter(name, QStringLiteral("<!DOCTYPE SKGML>")))

            // Refresh panel
            IFOK(err) toSave->setState(QString());
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Default state has been reset")))
    displayErrorMessage(err);
}

void SKGMainPanel::addTab()
{
    SKGTRACEINFUNC(1)
    SKGTabPage *cPage = currentPage();
    if (cPage != nullptr) {
        openPage(getPluginByName(cPage->objectName()));
    }
}

bool SKGMainPanel::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iObject != nullptr) && (iEvent != nullptr) && iEvent->type() == QEvent::Resize) {
        auto *rEvent = dynamic_cast<QResizeEvent *>(iEvent);
        if (rEvent != nullptr) {
            QSize newSize = rEvent->size();

            // Compute icon size
            int s = qMax(qMin(newSize.width() / 5, 64), 16);
            d->ui.kContextList->setIconSize(QSize(s, s));
        }
    }
    return KXmlGuiWindow::eventFilter(iObject, iEvent);
}

QStringList SKGMainPanel::getTipsOfDay() const
{
    return d->m_tipsOfTheDay;
}

QString SKGMainPanel::getTipOfDay() const
{
    auto tips = getTipsOfDay();
    return SKGServices::htmlToString(tips.at(QRandomGenerator::global()->bounded(tips.size())));
}

void SKGMainPanel::notify(int iTransaction)
{
    SKGTRACEINFUNC(1)
    SKGTRACEL(1) << "iTransaction=" << iTransaction << Qt::endl;

    // Notify
    SKGObjectBase transaction(getDocument(), QStringLiteral("doctransaction"), iTransaction);
    if (iTransaction == 0 || transaction.getAttribute(QStringLiteral("t_mode")) != QStringLiteral("R")) {
        SKGDocument::SKGMessageList msg;
        getDocument()->getMessages(iTransaction, msg, false);
        int nbMessages = msg.count();
        if (nbMessages != 0) {
            // Build list of types
            SKGDocument::MessageType maxType = SKGDocument::Positive;
            QList<SKGDocument::MessageType> listGroups;
            listGroups.reserve(nbMessages);
            for (int i = 0; i < nbMessages; ++i) {
                SKGDocument::SKGMessage m = msg.at(i);
                // if the message has an action, it can not be grouped
                if (!m.Action.isEmpty()) {
                    displayMessage(m.Text, m.Type, m.Action);
                    msg.removeAt(i);
                    i--;
                    nbMessages--;
                } else {
                    if (listGroups.isEmpty() || m.Type != listGroups.at(listGroups.count() - 1)) {
                        listGroups.push_back(m.Type);
                    }
                    if (static_cast<int>(m.Type) >= static_cast<int>(maxType)) {
                        maxType = m.Type;
                    }
                }
            }

            // Is the number of type acceptable?
            bool modeGrouped = false;
            if (listGroups.count() > 5 || nbMessages > 20) {
                // Too many group ==> simplification
                listGroups.clear();
                listGroups.push_back(maxType);
                modeGrouped = true;
            }

            // Build message
            if (nbMessages != 0) {
                QString message;
                int indexGroup = 0;
                for (int i = 0; i < nbMessages; ++i) {
                    auto m = msg.at(i);
                    auto t = m.Type;
                    if (modeGrouped) {
                        if (t == SKGDocument::Warning) {
                            m.Text = i18nc("Warning header", "Warning: %1", m.Text);
                        } else if (t == SKGDocument::Error) {
                            m.Text = i18nc("Error header", "Error: %1", m.Text);
                        } else if (t == SKGDocument::Information) {
                            m.Text = i18nc("Information header", "Information: %1", m.Text);
                        } else if (t == SKGDocument::Positive) {
                            m.Text = i18nc("Done header", "Done: %1", m.Text);
                        }
                    }
                    if (modeGrouped || t == listGroups.at(indexGroup)) {
                        // Same group
                        if (!message.isEmpty()) {
                            message += QStringLiteral("<br>");
                        }
                        message += m.Text;
                    } else {
                        // Different group
                        displayMessage(message, listGroups.at(indexGroup));

                        // Reset message
                        message = m.Text;

                        indexGroup++;
                    }
                }

                if (nbMessages < 21 || !SKGServices::getEnvVariable(QStringLiteral("SKGTEST")).isEmpty()) {
                    displayMessage(message, listGroups.at(indexGroup));
                } else {
                    // Too many message, display a warning panel
                    KMessageBox::information(SKGMainPanel::getMainPanel(), message, i18nc("Noun", "Notification"));
                }
            }
        }
    }
}

void SKGMainPanel::changeEvent(QEvent *e)
{
    KXmlGuiWindow::changeEvent(e);
}

QLabel *SKGMainPanel::statusNormalMessage() const
{
    return d->m_kNormalMessage;
}

KMessageWidget *SKGMainPanel::getMessageWidget(const QString &iMessage, SKGDocument::MessageType iType, const QString &iAction, bool iAutoKillOnClick)
{
    KMessageWidget *msg = nullptr;
    if (!iMessage.isEmpty()) {
        msg = new KMessageWidget(this);
        msg->setText(iMessage);
        msg->setIcon(SKGServices::fromTheme(iType == SKGDocument::Positive          ? QStringLiteral("dialog-positive")
                                                : iType == SKGDocument::Information ? QStringLiteral("dialog-information")
                                                : iType == SKGDocument::Warning     ? QStringLiteral("dialog-warning")
                                                                                    : QStringLiteral("dialog-error")));
        msg->setMessageType(static_cast<KMessageWidget::MessageType>(iType));
        msg->setPosition(KMessageWidget::Position::Header);
        if (!iAction.isEmpty()) {
            QUrl url(iAction);
            if (url.scheme() == QStringLiteral("skg")) {
                QAction *action = SKGMainPanel::getMainPanel()->getGlobalAction(url.host(), false);
                QAction *act = nullptr;
                if (action != nullptr) {
                    // This is an action
                    act = new QAction(action->icon(), action->text(), SKGMainPanel::getMainPanel());
                } else {
                    // This is a default action
                    act = new QAction(SKGServices::fromTheme(QStringLiteral("open")), i18nc("Verb", "Open …"), SKGMainPanel::getMainPanel());
                }

                act->setData(iAction);

                msg->addAction(act);
                connect(act, &QAction::triggered, this, [this] {
                    openPage(QUrl(qobject_cast<QAction *>(sender())->data().toString()), true);
                });
                if (iAutoKillOnClick) {
                    connect(act, &QAction::triggered, msg, &KMessageWidget::deleteLater, Qt::QueuedConnection);
                }
            }
        }
    }
    return msg;
}

KMessageWidget *SKGMainPanel::displayMessage(const QString &iMessage, SKGDocument::MessageType iType, const QString &iAction)
{
    // Create message widget
    KMessageWidget *msg = nullptr;
    if (!iMessage.isEmpty()) {
        msg = getMessageWidget(iMessage, iType, iAction, true);
        QTimer::singleShot(iType == SKGDocument::Positive          ? 5000
                               : iType == SKGDocument::Information ? 10000
                                                                   : 20000,
                           Qt::CoarseTimer,
                           msg,
                           &KMessageWidget::deleteLater);
        msg->show();
        d->m_mainLayout->insertWidget(qMax(d->m_mainLayout->indexOf(d->m_mainWidget) - 1, 0), msg);

        // Store message
        auto msg2 = getMessageWidget(iMessage, iType, iAction, false);
        auto *l = qobject_cast<QVBoxLayout *>(d->ui.kMessagesLayout->layout());
        if (l != nullptr) {
            l->insertWidget(0, msg2);
        }
    }

    // Emit message
    //  [Event/error]
    //  [Event/neutral]
    //  [Event/positive]
    auto notification =
        new KNotification(iType == SKGDocument::Error
                              ? QStringLiteral("error")
                              : (iType == SKGDocument::Positive ? QStringLiteral("positive")
                                                                : (iType == SKGDocument::Warning ? QStringLiteral("negative") : QStringLiteral("neutral"))));

    notification->setText(iMessage);
    notification->sendEvent();

    // Alert
    if (iType == SKGDocument::Error || iType == SKGDocument::Warning) {
        qApp->alert(this);
    }

    return msg;
}

KMessageWidget *SKGMainPanel::displayErrorMessage(const QString &iMessage)
{
    QString msg = iMessage;
    if (msg.isEmpty()) {
        auto *act = qobject_cast<QAction *>(sender());
        if (act != nullptr) {
            msg = act->data().toString();
        }
    }
    return displayMessage(msg, SKGDocument::Error);
}

KMessageWidget *SKGMainPanel::displayErrorMessage(const SKGError &iError, bool iNotifyIfNoError)
{
    return displayErrorMessage(iError, nullptr, iNotifyIfNoError);
}

KMessageWidget *SKGMainPanel::displayErrorMessage(const SKGError &iError, QAction *iAction, bool iNotifyIfNoError)
{
    SKGTRACEINFUNC(1)
    KMessageWidget *msg = nullptr;
    SKGMainPanel *parent = SKGMainPanel::getMainPanel();
    if (parent != nullptr) {
        if (iError) {
            // Get the message
            // BUG 423311: Remove history button
            msg = parent->displayMessage(iError.getFullMessageWithHistorical(), SKGDocument::Error, iError.getAction());

            // Add history action in case of
            /*if (iError.getHistoricalSize() != 0) {
                auto history = new QAction(i18nc("Noun", "History"), msg);
                history->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-information")));
                history->setData(iError.getFullMessageWithHistorical());
                msg->addAction(history);
                connect(history, &QAction::triggered, parent, [ = ] { parent->displayErrorMessage();});
                connect(history, &QAction::triggered, msg, &KMessageWidget::deleteLater, Qt::QueuedConnection);
            }*/

            // Add the additional action
            if (iAction != nullptr) {
                iAction->setParent(msg);
                msg->addAction(iAction);
                connect(iAction, &QAction::triggered, msg, &KMessageWidget::deleteLater, Qt::QueuedConnection);
            }
        } else {
            if (iNotifyIfNoError) {
                auto notification = new KNotification(QStringLiteral("positive"));
                notification->setText(iError.getFullMessage());
                notification->sendEvent();
            }
            // Status bar
            QLabel *label = parent->statusNormalMessage();
            QString message = iError.getMessage();
            if ((label != nullptr) && !message.isEmpty()) {
                label->setText(message);
            }
        }
    }
    return msg;
}

void SKGMainPanel::onCancelCurrentAction()
{
    SKGMainPanelPrivate::m_currentActionCanceled = true;
}

void SKGMainPanel::onQuitAction()
{
    // Bug 2777697: To be sure that all page modifications are closed
    closeAllPages(true);
    // Bug 2777697:

    qApp->closeAllWindows();
}

QString SKGMainPanel::getSaveFileName(const QString &iStartDir, const QString &iFilter, QWidget *iParent, QString *iCodec)
{
    QString fileName;

    QString lastCodecUsed = QTextCodec::codecForLocale()->name();
    KEncodingFileDialog::Result result =
        KEncodingFileDialog::getSaveUrlAndEncoding(lastCodecUsed, QUrl::fromLocalFile(QUrl(iStartDir).toLocalFile()), iFilter, iParent);
    if (!result.URLs.isEmpty()) {
        fileName = result.URLs.at(0).toLocalFile();
    }
    if (iCodec != nullptr) {
        *iCodec = result.encoding;
    }
    if (fileName.isEmpty()) {
        return QString();
    }
    QFile f(fileName);
    if (f.exists()
        && KMessageBox::warningContinueCancel(iParent,
                                              i18nc("Question", "File <b>%1</b> already exists. Do you really want to overwrite it?", fileName),
                                              i18nc("Question", "Warning"),
                                              KGuiItem(i18nc("Verb", "Save"), SKGServices::fromTheme(QStringLiteral("document-save"))))
            != KMessageBox::Continue) {
        return QString();
    }

    return fileName;
}

void SKGMainPanel::fillWithDistinctValue(const QList<QWidget *> &iWidgets,
                                         SKGDocument *iDoc,
                                         const QString &iTable,
                                         const QString &iAttribut,
                                         const QString &iWhereClause,
                                         bool iAddoperators)
{
    SKGTRACEINFUNC(10)

    if (iDoc != nullptr) {
        {
            // Get list
            QStringList list;
            {
                SKGTRACEIN(10, "SKGMainPanel::fillWithDistinctValue-build list " % iTable % " " % iAttribut)
                iDoc->getDistinctValues(iTable, iAttribut, iWhereClause, list);
                if (!list.isEmpty() && !list.at(0).isEmpty()) {
                    list.insert(0, QString());
                }

                // Sorting list
                {
                    SKGTRACEIN(10, "SKGMainPanel::fillWithDistinctValue-build list sorting " % iTable % " " % iAttribut)
                    // Correction bug 202341 vvv
                    QCollator c;
                    std::sort(list.begin(), list.end(), [&](const QString &a, const QString &b) {
                        return c.compare(a, b) < 0;
                    });
                }

                // Add operator
                if (iAddoperators) {
                    list.push_back(u'=' % i18nc("Key word to modify a string into a field", "capitalize"));
                    list.push_back(u'=' % i18nc("Key word to modify a string into a field", "capwords"));
                    list.push_back(u'=' % i18nc("Key word to modify a string into a field", "lower"));
                    list.push_back(u'=' % i18nc("Key word to modify a string into a field", "trim"));
                    list.push_back(u'=' % i18nc("Key word to modify a string into a field", "upper"));
                }
            }

            {
                SKGTRACEIN(10, "SKGMainPanel::fillWithDistinctValue-fill " % iTable % " " % iAttribut)
                SKGTRACEL(10) << "list.count()=" << list.count() << Qt::endl;
                for (auto w : std::as_const(iWidgets)) {
                    auto comp = new QCompleter(list, w);
                    if (comp != nullptr) {
                        comp->setCaseSensitivity(Qt::CaseInsensitive);
                        comp->setFilterMode(Qt::MatchContains);

                        // Fill completion
                        auto *kcmb = qobject_cast<KComboBox *>(w);
                        if (kcmb != nullptr) {
                            // Fill combo
                            kcmb->clear();
                            kcmb->addItems(list);

                            if (kcmb->isEditable()) {
                                kcmb->setCompleter(comp);
                            }
                        } else {
                            auto *kline = qobject_cast<QLineEdit *>(w);
                            if (kline != nullptr) {
                                kline->setClearButtonEnabled(true);
                                kline->setCompleter(comp);
                            }
                        }
                    }
                }
            }
        }
    }
}

SKGMainPanel *SKGMainPanel::getMainPanel()
{
    return SKGMainPanelPrivate::m_mainPanel;
}

void SKGMainPanel::onZoomChanged()
{
    SKGTabPage *toSave = currentPage();
    if (toSave != nullptr) {
        toSave->setZoomPosition(d->m_zoomSelector->value());
        d->m_zoomSelector->setValue(toSave->zoomPosition()); // In case of a limit is reached
    }
}

void SKGMainPanel::setMainWidget(QWidget *iWidget)
{
    if (d->m_mainWidget == nullptr && d->m_mainLayout != nullptr && iWidget != nullptr) {
        d->m_mainWidget = iWidget;
        d->m_mainLayout->addWidget(d->m_mainWidget);

        // Show/hide main widget
        d->m_tabWidget->setVisible(d->m_tabWidget->count() != 0);
        if (d->m_mainWidget != nullptr) {
            d->m_mainWidget->setVisible(!d->m_tabWidget->isVisible());
        }
    }
}

SKGTabWidget *SKGMainPanel::getTabWidget() const
{
    return d->m_tabWidget;
}

void SKGMainPanel::onClearMessages()
{
    QLayout *l = d->ui.kMessagesLayout->layout();
    if (l != nullptr) {
        // Remove all item of the layout
        while (l->count() > 1) {
            QLayoutItem *child = l->takeAt(0);
            if (child != nullptr) {
                QWidget *w = child->widget();
                delete w;
                delete child;
            }
        }
    }
}

void SKGMainPanel::onMigrateToSQLCipher()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (getDocument()->isFileModified()) {
        err = SKGError(ERR_ABORT, i18nc("An information message", "The document must be saved to be migrated."), QStringLiteral("skg://file_save"));
    } else {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        // Set parameters
        QString input = getDocument()->getCurrentFileName();
        QString tmp = input % ".sqlcipher";
        QString output = input % "_migrated.skg";
        output = output.replace(QStringLiteral(".skg_migrated"), QStringLiteral("_migrated"));

        // Build argument
        QStringList arg;
        arg.push_back(QStringLiteral("--in"));
        arg.push_back(input);
        arg.push_back(QStringLiteral("--out"));
        arg.push_back(tmp);

        QString password = getDocument()->getPassword();
        if (!password.isEmpty()) {
            arg.push_back(QStringLiteral("--param"));
            arg.push_back(QStringLiteral("password"));
            arg.push_back(QStringLiteral("--value"));
            arg.push_back(password);
            password = " --param password --value \"" % password % "\"";
        }

        // Conversion skg => sqlcipher
        QString cmd = "skroogeconvert --in \"" % input % "\" --out \"" % tmp % "\"" % password;
        int rc = QProcess::execute(QStringLiteral("skroogeconvert"), arg);
        if (rc != 0) {
            err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "The following command line failed with code %2:\n'%1'", cmd, rc));
        } else {
            cmd = "skroogeconvert --in \"" % tmp % "\" --out \"" % output % "\"" % password;
            arg[1] = tmp;
            arg[3] = output;
            rc = QProcess::execute(QStringLiteral("skroogeconvert"), arg);
            if (rc != 0) {
                err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "The following command line failed with code %2:\n'%1'", cmd, rc));
            } else {
                getDocument()->sendMessage(i18nc("Positive message", "Your document has been migrated.\nHere is the new file:\n%1", output),
                                           SKGDocument::Positive,
                                           "skg://file_open/?filename=" % output);
                notify();
            }
        }
        QFile(tmp).remove();

        QApplication::restoreOverrideCursor();
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

QString SKGMainPanel::dateToString(QDate iDate)
{
    switch (skgbasegui_settings::date_format()) {
    case 0:
        return QLocale().toString(iDate, QLocale::ShortFormat);
    case 1:
        return QLocale().toString(iDate, QLocale::LongFormat);
    case 3:
        return KFormat().formatRelativeDate(iDate, QLocale::LongFormat);
    case 4:
        return iDate.toString(Qt::ISODate);
    case 2:
    default:
        return KFormat().formatRelativeDate(iDate, QLocale::ShortFormat);
    }
}
