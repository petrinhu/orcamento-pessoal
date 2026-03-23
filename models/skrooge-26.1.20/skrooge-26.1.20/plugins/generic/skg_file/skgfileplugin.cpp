/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin for file operation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgfileplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kmessagewidget.h>
#include <knewpassworddialog.h>
#include <kpassworddialog.h>
#include <kpluginfactory.h>
#include <krecentfilesaction.h>
#include <kstandardaction.h>
#include <kwallet.h>

#include <qfile.h>
#include <qfiledialog.h>
#include <qsplashscreen.h>
#include <qwidget.h>

#include "skgerror.h"
#include "skgfile_settings.h"
#include "skgmainpanel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGFilePlugin, "metadata.json")

SKGFilePlugin::SKGFilePlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_saveAction(nullptr)
    , m_recentFiles(nullptr)
    , m_currentDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)

    // Set save on close mode
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGMainPanel::getMainPanel()->setSaveOnClose(skgfile_settings::saveonclose());
    }
}

SKGFilePlugin::~SKGFilePlugin()
{
    SKGTRACEINFUNC(10)
    if (m_recentFiles != nullptr) {
        m_recentFiles->saveEntries(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("RecentFiles")));
    }

    m_currentDocument = nullptr;
    m_recentFiles = nullptr;
    m_saveAction = nullptr;
}

bool SKGFilePlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;
    if (m_currentDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skg_file"), title());
    setXMLFile(QStringLiteral("skg_file.rc"));

    // Menu
    registerGlobalAction(QStringLiteral("file_new"), KStandardAction::openNew(this, SLOT(onNew()), actionCollection()));
    registerGlobalAction(QStringLiteral("file_open"), KStandardAction::open(this, SLOT(onOpen()), actionCollection()));

    m_saveAction = KStandardAction::save(this, SLOT(onSave()), actionCollection());
    registerGlobalAction(QStringLiteral("file_save"), m_saveAction);
    registerGlobalAction(QStringLiteral("file_save_as"), KStandardAction::saveAs(this, SLOT(onSaveAs()), actionCollection()));

    auto actChangePassword = new QAction(SKGServices::fromTheme(QStringLiteral("document-encrypt")),
                                         i18nc("Action allowing the user to change his document password", "Change password…"),
                                         this);
    connect(actChangePassword, &QAction::triggered, this, &SKGFilePlugin::onChangePassword);
    actionCollection()->setDefaultShortcut(actChangePassword, Qt::CTRL | Qt::Key_K);
    registerGlobalAction(QStringLiteral("file_change_password"), actChangePassword);

    // Recent file
    m_recentFiles = KStandardAction::openRecent(this, SLOT(onOpen(QUrl)), actionCollection());
    if (m_recentFiles != nullptr) {
        m_recentFiles->loadEntries(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("RecentFiles")));
    }

    // Get last argument
    connect(this, &SKGFilePlugin::loadFile, this, &SKGFilePlugin::onOpen, Qt::QueuedConnection);

    return true;
}

QStringList SKGFilePlugin::processArguments(const QStringList &iArgument)
{
    SKGTRACEINFUNC(10)
    QStringList output = iArgument;
    if (m_currentDocument->getCurrentFileName().isEmpty()) {
        int nbArg = iArgument.count();
        int openMode = 1; // 0=no open, 1=open last opened if settings set, 2=new document
        if (nbArg != 0) {
            openMode = 2;
            QString filename = iArgument.at(nbArg - 1);
            QString extension = QFileInfo(filename).suffix().toUpper();
            auto inputFile = QFileInfo(filename);
            if (!inputFile.isAbsolute()) {
                filename = QFileInfo(QDir::currentPath(), filename).absoluteFilePath();
            }

            QString extensionDocument = m_currentDocument->getFileExtension().toUpper();
            if (extension == extensionDocument) {
                if (QFile(filename).exists()) {
                    if (SKGMainPanel::getMainPanel() != nullptr) {
                        QSplashScreen *splashScreen = SKGMainPanel::getMainPanel()->splashScreen();
                        if (splashScreen != nullptr) {
                            splashScreen->showMessage(i18nc("Splash screen message", "Opening file %1…", filename),
                                                      Qt::AlignLeft,
                                                      QColor(221, 130, 8)); // krazy:exclude=qmethods
                        }
                    }
                    Q_EMIT loadFile(QUrl::fromLocalFile(filename));
                    output.pop_back();
                    openMode = 0;
                } else {
                    m_currentDocument->sendMessage(i18nc("Error Message", "File %1 not found! Impossible to open it.", filename), SKGDocument::Error);
                }
            }
        }

        if (openMode != 0) {
            // Read Setting
            bool openlastfile = skgfile_settings::openlastfile();
            if (openMode == 1 && openlastfile) {
                QString lastOpenedFile = skgfile_settings::lastfilepath();
                if (!lastOpenedFile.isEmpty() && QFile(lastOpenedFile).exists()) {
                    if (SKGMainPanel::getMainPanel() != nullptr) {
                        QSplashScreen *splashScreen = SKGMainPanel::getMainPanel()->splashScreen();
                        if (splashScreen != nullptr) {
                            splashScreen->showMessage(i18nc("Splash screen message", "Opening file %1…", lastOpenedFile),
                                                      Qt::AlignLeft,
                                                      QColor(221, 130, 8)); // krazy:exclude=qmethods
                        }
                    }
                    Q_EMIT loadFile(QUrl::fromLocalFile(lastOpenedFile));
                } else {
                    openMode = 2;
                }
            } else {
                openMode = 2;
            }

            if (openMode == 2 && m_currentDocument->getMainDatabase() == nullptr) {
                onNew();
            }
        }

        // To be sure that the document has the right parameters
        savePreferences();
    }

    return output;
}

QWidget *SKGFilePlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    auto w = new QWidget();
    ui.setupUi(w);
    connect(ui.kcfg_backup_enabled, &QCheckBox::toggled, ui.kcfg_prefix, &SKGComboBox::setEnabled);
    connect(ui.kcfg_backup_enabled, &QCheckBox::toggled, ui.kcfg_suffix, &SKGComboBox::setEnabled);
    connect(ui.kcfg_storeInKdeWallet, &QCheckBox::toggled, ui.kcfg_selectedWallet, &SKGComboBox::setEnabled);

    ui.kcfg_prefix->addItem(QString());
    ui.kcfg_prefix->addItem(QStringLiteral("."));

    ui.kcfg_suffix->addItem(QStringLiteral(".old"));
    ui.kcfg_suffix->addItem(QStringLiteral(".back"));
    ui.kcfg_suffix->addItem(QStringLiteral(".<DATE>.back"));
    ui.kcfg_suffix->addItem(QStringLiteral(".<DATE>.old"));
    ui.kcfg_suffix->addItem(QStringLiteral("~"));

    ui.kcfg_selectedWallet->addItems(KWallet::Wallet::walletList());

    return w;
}

KConfigSkeleton *SKGFilePlugin::getPreferenceSkeleton()
{
    return skgfile_settings::self();
}

SKGError SKGFilePlugin::savePreferences() const
{
    SKGError err;
    if (m_currentDocument != nullptr) {
        // Read Setting
        QString prefix;
        QString suffix;
        if (skgfile_settings::backup_enabled()) {
            prefix = skgfile_settings::prefix();
            suffix = skgfile_settings::suffix();
        }

        // Save setting in document
        m_currentDocument->setBackupParameters(prefix, suffix);

        // Set save on close mode
        if (SKGMainPanel::getMainPanel()) {
            SKGMainPanel::getMainPanel()->setSaveOnClose(skgfile_settings::saveonclose());
        }
    }
    return err;
}

void SKGFilePlugin::refresh()
{
    SKGTRACEINFUNC(10)

    // Refresh action status
    if (m_currentDocument != nullptr) {
        if (m_saveAction != nullptr) {
            m_saveAction->setEnabled(m_currentDocument->isFileModified());
        }
    }
}

QString SKGFilePlugin::title() const
{
    return i18nc("Noun, a file as in a text file", "File");
}

QString SKGFilePlugin::icon() const
{
    return QStringLiteral("document-save");
}

QString SKGFilePlugin::toolTip() const
{
    return i18nc("File Management, as in Save File, Save As…", "File management");
}

QStringList SKGFilePlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc(
        "Description of a tip",
        "<p>… the last opened file can be <a href=\"skg://tab_configure?page=File plugin\">open automatically</a> when the application is launched.</p>"));
    output.push_back(i18nc("Description of a tip", "<p>… you can secure your document with a <a href=\"skg://file_change_password\">password</a>.</p>"));
    return output;
}

int SKGFilePlugin::getOrder() const
{
    // Must be one of the first
    return 1;
}

void SKGFilePlugin::onNew()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentDocument != nullptr) && SKGMainPanel::getMainPanel()->queryFileClose()) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        SKGMainPanel::getMainPanel()->closeAllPages(true);

        err = m_currentDocument->initialize();

        IFOKDO(err, m_currentDocument->setLanguage(QLocale::languageToString(QLocale().language())))
        QApplication::restoreOverrideCursor();

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after creating a document", "Document successfully created.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message: Could not create a document", "Document creation failed."));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGFilePlugin::onSave()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((m_currentDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        if (m_currentDocument->getCurrentFileName().isEmpty()) {
            onSaveAs();
        } else {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            err = m_currentDocument->save();
            QApplication::restoreOverrideCursor();

            // Refresh
            SKGMainPanel::getMainPanel()->refresh();

            // status bar
            IFOKDO(err, SKGError(0, i18nc("Successfully saved a file", "File successfully saved.")))
            else
            {
                err.addError(ERR_FAIL, i18nc("Error message: Could not save a file", "Cannot save file"));
            }

            // Display error
            SKGMainPanel::displayErrorMessage(err);
        }
    }
}

void SKGFilePlugin::onSaveAs()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((m_currentDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        QString fileName = SKGMainPanel::getSaveFileName(
            "kfiledialog:///" % m_currentDocument->objectName(),
            "*." % m_currentDocument->getFileExtension() % u'|'
                % i18nc("Associated with the file extension : for example, .csv --> CSV document", "%1 document", KAboutData::applicationData().displayName()),
            SKGMainPanel::getMainPanel());
        if (fileName.isEmpty()) {
            return;
        }

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        err = m_currentDocument->saveAs(fileName, true);
        QApplication::restoreOverrideCursor();

        // Refresh
        SKGMainPanel::getMainPanel()->refresh();

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successfully saved a file", "File '%1' saved.", fileName));
            // Add in recentFiles
            if (m_recentFiles != nullptr) {
                m_recentFiles->addUrl(QUrl::fromLocalFile(fileName));
                m_recentFiles->saveEntries(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("RecentFiles")));
            }
            // Set as last open file in kcfg
            KSharedConfigPtr config = KSharedConfig::openConfig();
            KConfigGroup pref = config->group(QStringLiteral("File"));
            pref.writePathEntry("lastfilepath", fileName);
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message: Could not save a file", "Failed to save '%1'.", fileName));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGFilePlugin::onReOpen()
{
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        QString filename = act->data().toString();
        QFile(SKGDocument::getTemporaryFile(filename)).remove();
        onOpen(QUrl::fromLocalFile(filename));
    }
}

void SKGFilePlugin::onOpen(const QUrl &iUrl)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentDocument != nullptr) && SKGMainPanel::getMainPanel()->queryFileClose()) {
        bool useKWallet = skgfile_settings::storeInKdeWallet();
        QString pwd;
        QString programName = KAboutData::applicationData().displayName();
        QString fileName = iUrl.toLocalFile();
        if (fileName.isEmpty()) {
            auto *act = qobject_cast<QAction *>(sender());
            if (act != nullptr) {
                fileName = act->property("filename").toString();
            }
        }
        if (fileName.isEmpty()) {
            fileName = QFileDialog::getOpenFileUrl(SKGMainPanel::getMainPanel(),
                                                   i18nc("Panel title", "Open file"),
                                                   QUrl(),
                                                   i18nc("File format for open dialog panel", "%1 document", programName) % "(*."
                                                       % m_currentDocument->getFileExtension() % ")")
                           .toLocalFile();
        }
        if (!fileName.isEmpty()) {
            // Check if temporary file exists
            bool restoreTmpFile = false;
            QString tmpFile = SKGDocument::getTemporaryFile(fileName);
            if (QFile(tmpFile).exists()) {
                KMessageWidget *msg = SKGMainPanel::getMainPanel()->displayMessage(
                    i18nc("Warning message", "Your document has been restored from its temporary file. You can decide to reopen the original document."),
                    SKGDocument::Warning);
                auto reopen = new QAction(i18nc("Noun", "Reopen"), msg);
                reopen->setIcon(SKGServices::fromTheme(QStringLiteral("quickopen")));
                reopen->setData(fileName);
                msg->addAction(reopen);
                connect(reopen, &QAction::triggered, this, &SKGFilePlugin::onReOpen);
                connect(reopen, &QAction::triggered, msg, &KMessageWidget::deleteLater, Qt::QueuedConnection);

                restoreTmpFile = true;
            }

            // Open
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            SKGMainPanel::getMainPanel()->closeAllPages(true);
            err = m_currentDocument->load(fileName, QString(), restoreTmpFile);
            QApplication::restoreOverrideCursor();

            if (err && err.getReturnCode() == ERR_ENCRYPTION) {
                m_currentDocument->close();

                // Open failed
                // Password must be asked
                QString additionalMessage;
                do {
                    // Reset error
                    err = SKGError(0, QString());
                    pwd = QString();

                    // Get password
                    if (useKWallet) {
                        SKGTRACEL(10) << "Use KWallet" << Qt::endl;
                        // Use KWallet
                        QString walletName = skgfile_settings::selectedWallet();
                        if (!KWallet::Wallet::walletList().contains(walletName)) {
                            walletName = KWallet::Wallet::walletList().value(SKGServices::stringToInt(skgfile_settings::selectedWallet()));
                            if (walletName.isEmpty()) {
                                walletName = QStringLiteral("kdewallet");
                            }
                        }
                        KWallet::Wallet *w = KWallet::Wallet::openWallet(walletName, SKGMainPanel::getMainPanel()->winId());
                        if (w != nullptr) {
                            // Change folder
                            if (!w->hasFolder(programName)) {
                                w->createFolder(programName);
                            }
                            w->setFolder(programName);

                            // Read password
                            w->readPassword(fileName, pwd);
                            if (pwd.isEmpty()) {
                                SKGTRACEL(10) << "Password not found in KWallet for " << fileName << Qt::endl;
                                useKWallet = false;
                            }

                            delete w;
                            w = nullptr;
                        }
                    }

                    if (!useKWallet) {
                        SKGTRACEL(10) << "Ask password" << Qt::endl;
                        // Use password dialog
                        QPointer<KPasswordDialog> dlg = new KPasswordDialog(SKGMainPanel::getMainPanel());
                        dlg->setPrompt(additionalMessage % i18nc("Question", "This file seems to be protected.\nPlease enter the password."));
                        if (dlg->exec() == QDialog::Accepted) {
                            pwd = dlg->password();
                        }
                        delete dlg;
                    }

                    // Load file
                    if (!pwd.isEmpty()) {
                        QSplashScreen *splashScreen = SKGMainPanel::getMainPanel()->splashScreen();
                        if (splashScreen != nullptr) {
                            splashScreen->hide();
                        }

                        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                        err = m_currentDocument->load(fileName, pwd, restoreTmpFile);
                        IFKO(err)
                        {
                            if (err.getReturnCode() == ERR_ENCRYPTION) {
                                additionalMessage = i18nc("The user did not provide the correct password", "<b>Wrong password.</b>\n");
                                useKWallet = false;
                            } else {
                                // Load error
                                QApplication::restoreOverrideCursor();
                                break;
                            }
                        }
                        QApplication::restoreOverrideCursor();

                        if (splashScreen != nullptr) {
                            splashScreen->show();
                        }
                    }
                } while (err);
            }

            IFOKDO(err, m_currentDocument->setLanguage(QLocale::languageToString(QLocale().language())))

            // status bar
            IFOK(err)
            {
                err = SKGError(0, i18nc("Successfully opened a file", "File '%1' opened.", fileName));
                // Add in recentFiles
                if (m_recentFiles != nullptr) {
                    m_recentFiles->addUrl(QUrl::fromLocalFile(fileName));
                    m_recentFiles->saveEntries(KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("RecentFiles")));
                }
                // Set as last open file in kcfg
                KSharedConfigPtr config = KSharedConfig::openConfig();
                KConfigGroup pref = config->group(QStringLiteral("File"));
                pref.writePathEntry("lastfilepath", fileName);

                // Store password if KDE wallet if needed
                if (skgfile_settings::storeInKdeWallet() && !useKWallet) {
                    // Use KWallet
                    QString walletName = skgfile_settings::selectedWallet();
                    if (!KWallet::Wallet::walletList().contains(walletName)) {
                        walletName = KWallet::Wallet::walletList().value(SKGServices::stringToInt(skgfile_settings::selectedWallet()));
                        if (walletName.isEmpty()) {
                            walletName = QStringLiteral("kdewallet");
                        }
                    }
                    KWallet::Wallet *w = KWallet::Wallet::openWallet(walletName, SKGMainPanel::getMainPanel()->winId());
                    if (w != nullptr) {
                        // Change folder
                        w->setFolder(programName);

                        // Write password
                        w->writePassword(fileName, pwd);

                        delete w;
                        w = nullptr;
                    }
                }
            }
            else
            {
                this->onNew();
                if (err.getReturnCode() != ERR_CORRUPTION) {
                    err.addError(ERR_FAIL, i18nc("Error message: Could not open a file", "Failed to open '%1'.", fileName));
                }
            }

            // Display error
            QAction *recovery = nullptr;
#ifdef Q_OS_UNIX
            if (true || err.getReturnCode() == ERR_CORRUPTION) {
                recovery = new QAction(i18nc("Noun", "Try a recovery"), this);
                recovery->setIcon(SKGServices::fromTheme(QStringLiteral("games-solve")));
                recovery->setData(SKGServices::stringsToCsv(QStringList() << fileName << pwd));
                connect(recovery, &QAction::triggered, this, &SKGFilePlugin::onRecover);
            }
#endif
            // Initialize preferences
            int index = 0;
            while (index >= 0) {
                SKGInterfacePlugin *plugin = SKGMainPanel::getMainPanel()->getPluginByIndex(index);
                if (plugin != nullptr) {
                    plugin->initPreferences();
                } else {
                    index = -2;
                }
                ++index;
            }

            SKGMainPanel::displayErrorMessage(err, recovery);
        }
    }
}

void SKGFilePlugin::onRecover()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    auto *act = qobject_cast<QAction *>(sender());
    if ((act != nullptr) && (m_currentDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        QStringList params = SKGServices::splitCSVLine(act->data().toString());
        QString recoveredFileName;
        err = m_currentDocument->recover(params.at(0), params.at(1), recoveredFileName);

        IFOK(err)
        {
            // Display recovery message
            KMessageWidget *msg = SKGMainPanel::getMainPanel()->displayMessage(
                i18nc("Positive message", "Your document has been recovered here: %1\nTake care the recovery could be not perfect", recoveredFileName),
                SKGDocument::Positive);
            auto reopen = new QAction(i18nc("Noun", "Open the recovered file"), msg);
            reopen->setIcon(SKGServices::fromTheme(QStringLiteral("quickopen")));
            reopen->setData(recoveredFileName);
            msg->addAction(reopen);
            connect(reopen, &QAction::triggered, this, &SKGFilePlugin::onReOpen);
            connect(reopen, &QAction::triggered, msg, &KMessageWidget::deleteLater, Qt::QueuedConnection);
        }
        else
        {
            // Display error
            SKGMainPanel::displayErrorMessage(err);
        }
    }
}

void SKGFilePlugin::onChangePassword()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((m_currentDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        QPointer<KNewPasswordDialog> dlg = new KNewPasswordDialog(SKGMainPanel::getMainPanel());
        dlg->setPrompt(
            i18n("Take care, if you lose your <b>password</b> then it will be <u><b>impossible</b></u> to open your document. Warning, this action can not be "
                 "undone excepted by changing the password again."));
        if (dlg->exec() == 0) {
            err = SKGError(0, i18nc("Successfully changed the document password", "Changing password was canceled."));
        } else {
            QString newPassword = dlg->password();
            IFOKDO(err, m_currentDocument->changePassword(newPassword))

            // status
            IFOKDO(err, SKGError(0, i18nc("Successfully changed the document password", "Password changed.")))
            else
            {
                err.addError(ERR_FAIL, i18nc("Error message: Could not change the document password", "Failed to change password."));
            }
        }
        delete dlg;

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

SKGAdviceList SKGFilePlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;

    // Backup
    if (!iIgnoredAdvice.contains(QStringLiteral("skgfileplugin_notvalidated"))) {
        SKGAdvice ad;
        ad.setUUID(QStringLiteral("skgfileplugin_notvalidated"));
        ad.setPriority(2);
        ad.setShortMessage(i18nc("Advice to the user that he should backup his document", "Backup your document"));
        ad.setLongMessage(i18nc("Explain the user that he should backup his document", "Do not forget to backup your document on another device."));
        output.push_back(ad);
    }

    return output;
}

#include <skgfileplugin.moc>
