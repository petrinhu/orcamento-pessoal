/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for import and export operation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportexportplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kencodingfiledialog.h>
#include <kmessagebox.h>
#include <kmessagewidget.h>
#include <kpassworddialog.h>
#include <kpluginfactory.h>
#include <kpluginmetadata.h>

#include <QtCore5Compat/QTextCodec>
#include <qaction.h>
#include <qdiriterator.h>
#include <qdom.h>
#include <qregularexpression.h>
#include <qstandardpaths.h>
#include <qsysinfo.h>
#include <qvariant.h>

#include "skgbankincludes.h"
#include "skgerror.h"
#include "skgimportexport_settings.h"
#include "skgmainpanel.h"
#include "skgoperationobject.h"
#include "skgtraces.h"
#include <utility>

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportExportPlugin, "metadata.json")

SKGImportExportPlugin::SKGImportExportPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
    , m_install(false)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGImportExportPlugin::~SKGImportExportPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGImportExportPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    // Tell the host application to load my GUI component
    setComponentName(QStringLiteral("skrooge_importexport"), title());
    setXMLFile(QStringLiteral("skrooge_importexport.rc"));

    // Imports
    auto imports =
        new KActionMenu(SKGServices::fromTheme(QStringLiteral("document-import")), i18nc("Verb, action to import items from another format", "Import"), this);
    registerGlobalAction(QStringLiteral("import"), imports);

    // Import
    QStringList overlay;
    overlay.push_back(QStringLiteral("skrooge"));

    auto actImport = new QAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlay),
                                 i18nc("Verb, action to import items from another format", "Import…"),
                                 this);
    actionCollection()->setDefaultShortcut(actImport, Qt::CTRL | Qt::META | Qt::Key_I);
    connect(actImport, &QAction::triggered, this, [this]() {
        this->importFiles();
    });
    imports->addAction(actImport);
    registerGlobalAction(QStringLiteral("import_operation"), actImport);

    // Import backends
    QStringList overlay2;
    overlay.push_back(QStringLiteral("download"));

    auto actImportBackend = new QAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlay2),
                                        i18nc("Verb, action to import items from another format", "Import with backends"),
                                        this);
    actionCollection()->setDefaultShortcut(actImportBackend, Qt::CTRL | Qt::META | Qt::Key_W);
    connect(actImportBackend, &QAction::triggered, this, &SKGImportExportPlugin::importbackends);
    imports->addAction(actImportBackend);
    registerGlobalAction(QStringLiteral("import_backends"), actImportBackend);

    // Import CSV Unit
    QStringList overlaycsv;
    overlaycsv.push_back(QStringLiteral("text-csv"));
    auto actImportCsvUnit =
        new QAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlaycsv), i18nc("Verb, action to import", "Import currency values…"), this);
    connect(actImportCsvUnit, &QAction::triggered, this, [this]() {
        this->importFiles(QList<QUrl>(), 2);
    });
    imports->addAction(actImportCsvUnit);
    registerGlobalAction(QStringLiteral("import_csv_unit"), actImportCsvUnit);

    // Import CSV Rule
    auto actImportCsvRule =
        new QAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlaycsv), i18nc("Verb, action to import", "Import rules…"), this);
    connect(actImportCsvRule, &QAction::triggered, this, [this]() {
        this->importFiles(QList<QUrl>(), 3);
    });
    imports->addAction(actImportCsvRule);
    registerGlobalAction(QStringLiteral("import_csv_rule"), actImportCsvRule);

    // Exports
    auto exports =
        new KActionMenu(SKGServices::fromTheme(QStringLiteral("document-export")), i18nc("Verb, action to export items in another format", "Export"), this);
    registerGlobalAction(QStringLiteral("export"), exports);

    // Export
    auto actExportFile =
        new QAction(SKGServices::fromTheme(QStringLiteral("document-export")), i18nc("Verb, action to export items to another format", "Export…"), this);
    connect(actExportFile, &QAction::triggered, this, &SKGImportExportPlugin::exportFile);
    exports->addAction(actExportFile);
    actionCollection()->setDefaultShortcut(actExportFile, Qt::CTRL | Qt::META | Qt::Key_E);
    registerGlobalAction(QStringLiteral("export_operation"), actExportFile);

    // Processing
    auto processing = new KActionMenu(SKGServices::fromTheme(QStringLiteral("tools-wizard")),
                                      i18nc("Noun, apply some kind of transformation on an item", "Processing"),
                                      this);
    registerGlobalAction(QStringLiteral("processing"), processing);

    // Processing found and group
    QStringList overlaytransfers;
    overlaytransfers.push_back(QStringLiteral("exchange-positions"));

    auto actProcessingFoundTransfer = new QAction(SKGServices::fromTheme(QStringLiteral("tools-wizard"), overlaytransfers),
                                                  i18nc("Verb, action to find and group transfers", "Find and group transfers"),
                                                  this);
    connect(actProcessingFoundTransfer, &QAction::triggered, this, &SKGImportExportPlugin::findTransfers);
    processing->addAction(actProcessingFoundTransfer);
    actionCollection()->setDefaultShortcut(actProcessingFoundTransfer, Qt::CTRL | Qt::META | Qt::Key_G);
    registerGlobalAction(QStringLiteral("process_foundtransfer"), actProcessingFoundTransfer);

    auto actProcessingAnonymize =
        new QAction(SKGServices::fromTheme(QStringLiteral("tools-wizard"), overlaytransfers), i18nc("Verb, action to anonymize a document", "Anonymize"), this);
    connect(actProcessingAnonymize, &QAction::triggered, this, &SKGImportExportPlugin::anonymize);
    processing->addAction(actProcessingAnonymize);
    registerGlobalAction(QStringLiteral("process_anonymize"), actProcessingAnonymize);

    // Processing banks
    auto actProcessingBank =
        new QAction(SKGServices::fromTheme(QStringLiteral("tools-wizard")), i18nc("Verb, action to clean an import", "Clean bank's imports"), this);
    connect(actProcessingBank, &QAction::triggered, this, &SKGImportExportPlugin::cleanBanks);
    processing->addAction(actProcessingBank);
    registerGlobalAction(QStringLiteral("process_banks"), actProcessingBank);

    // Processing banks
    QStringList overlayValidate;
    overlayValidate.push_back(QStringLiteral("dialog-ok"));
    auto actSwithValidationImportedOperations =
        new QAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlayValidate),
                    i18nc("Verb, action to validate imported transactions", "Switch validation of imported transactions"),
                    this);
    connect(actSwithValidationImportedOperations, &QAction::triggered, this, &SKGImportExportPlugin::swithvalidationImportedOperations);
    actionCollection()->setDefaultShortcut(actSwithValidationImportedOperations, Qt::CTRL | Qt::SHIFT | Qt::Key_V);
    registerGlobalAction(QStringLiteral("switch_validation_imported_operation"),
                         actSwithValidationImportedOperations,
                         QStringList() << QStringLiteral("operation"),
                         1,
                         -1,
                         318);

    auto act = new QAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlayValidate),
                           i18nc("Verb, action to merge", "Validate transactions that do not require further action"),
                           this);
    connect(act, &QAction::triggered, this, &SKGImportExportPlugin::validateAllOperations);
    registerGlobalAction(QStringLiteral("process_validate"), act, QStringList(), -2, -1);

    QStringList overlayopen;
    overlayopen.push_back(QStringLiteral("quickopen"));
    auto actOpenNotValidated = new QAction(SKGServices::fromTheme(QStringLiteral("document-import"), overlayopen),
                                           i18nc("Verb, action to open", "Open imported transactions not yet validated…"),
                                           this);
    actOpenNotValidated->setData(QString(
        "skg://skrooge_operation_plugin/?title=" % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions imported and not yet validated"))
        % "&title_icon=" % SKGServices::encodeForUrl(icon()) % "&operationWhereClause=" % SKGServices::encodeForUrl(QStringLiteral("t_imported='P'"))));
    connect(actOpenNotValidated, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    actionCollection()->setDefaultShortcut(actOpenNotValidated, Qt::META | Qt::Key_V);
    registerGlobalAction(QStringLiteral("view_open_not_validated"), actOpenNotValidated);

    auto actMergeImportedOperation =
        new QAction(SKGServices::fromTheme(QStringLiteral("merge")), i18nc("Verb, action to merge", "Merge imported transactions"), this);
    connect(actMergeImportedOperation, &QAction::triggered, this, &SKGImportExportPlugin::mergeImportedOperation);
    actionCollection()->setDefaultShortcut(actMergeImportedOperation, Qt::CTRL | Qt::ALT | Qt::Key_M);
    registerGlobalAction(QStringLiteral("merge_imported_operation"), actMergeImportedOperation, QStringList() << QStringLiteral("operation"), 1, -1, 319);

    auto force = new QAction(i18nc("Noun", "Force the merge"), this);
    force->setIcon(SKGServices::fromTheme(QStringLiteral("merge")));
    force->setData(1);
    connect(force, &QAction::triggered, this, &SKGImportExportPlugin::mergeImportedOperation);
    registerGlobalAction(QStringLiteral("merge_imported_operation_force"), force);

    // Get last argument
    connect(
        this,
        &SKGImportExportPlugin::importFileName,
        this,
        [this](const QString &iFileName) {
            this->importFile(iFileName, true);
        },
        Qt::QueuedConnection);

    // Krunner transactions
    QString dirName = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QStringList fileList = QDir(dirName).entryList(QStringList() << QStringLiteral("add_operation_*.txt"), QDir::Files);
    if (!fileList.isEmpty()) {
        m_currentBankDocument->sendMessage(i18nc("Information message", "You have some krunner's transactions to import"));
    }

    return true;
}

QStringList SKGImportExportPlugin::processArguments(const QStringList &iArgument)
{
    SKGTRACEINFUNC(10)
    QStringList output = iArgument;
    int nbArg = output.count();
    if (nbArg != 0) {
        QString filename = output.at(nbArg - 1);
        QString extension = QFileInfo(filename).suffix().toUpper();
        QString extensionDocument = m_currentBankDocument->getFileExtension().toUpper();
        if (QFile(filename).exists() && extension != extensionDocument) {
            int rc = KMessageBox::questionTwoActions(SKGMainPanel::getMainPanel(),
                                                     i18nc("Question", "Do you really want to import %1 into the current document ?", filename),
                                                     i18nc("Question", "Import confirmation"),
                                                     KStandardGuiItem::apply(),
                                                     KStandardGuiItem::cancel(),
                                                     QStringLiteral("importconfirmation"));
            if (rc == KMessageBox::PrimaryAction) {
                Q_EMIT importFileName(filename);
                output.pop_back();
            }
        }
    }
    return output;
}

void SKGImportExportPlugin::initPreferences()
{
    // Read Setting
    if (m_currentBankDocument != nullptr) {
        KSharedConfigPtr config = KSharedConfig::openConfig();
        KConfigGroup pref = config->group(QStringLiteral("skrooge_importexport"));
        auto backends = m_currentBankDocument->getParameter(QStringLiteral("SKG_BACKENDS"));
        pref.writeEntry("backends", backends);

        skgimportexport_settings::self()->read();
    }
}

QWidget *SKGImportExportPlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    // Create widget
    auto w = new QWidget();
    ui.setupUi(w);
    connect(ui.kcfg_automatic_search_header, &QCheckBox::toggled, ui.kHeaderPositionFrm, &QFrame::setHidden);
    connect(ui.kcfg_automatic_search_columns, &QCheckBox::toggled, ui.kColumnsPositionsFrm, &QFrame::setHidden);
    connect(ui.kcfg_automatic_search_columns, &QCheckBox::clicked, ui.kCsvMappingFrm, &QFrame::hide);
    connect(ui.kcfg_automatic_search_columns, &QCheckBox::toggled, ui.More, &QPushButton::setEnabled);
    connect(ui.More, &QPushButton::toggled, ui.kCsvMappingFrm, &QFrame::setVisible);
    connect(ui.kcfg_automatic_search_columns, &QCheckBox::clicked, ui.More, &QPushButton::setChecked);
    connect(ui.kcfg_download_on_open, &QCheckBox::toggled, ui.kcfg_download_frequency, &KComboBox::setEnabled);

    ui.kHeaderPositionFrm->hide();
    ui.kColumnsPositionsFrm->hide();
    ui.kCsvMappingFrm->hide();

    // Build list of known backends
    QString doc;
    const auto services = SKGServices::findDataPlugins(QStringLiteral("skrooge_import_backend"));
    for (const auto &service : services) {
        auto os_supported = SKGServices::splitCSVLine(service.value(QStringLiteral("X-SKROOGE-ossupported"), QString()).toLower(), u',');
        if (os_supported.isEmpty() || os_supported.contains(QSysInfo::kernelType().toLower())) {
            doc += "<br/><b>" + service.name() + "(" + service.pluginId().remove(QStringLiteral("org.kde.skrooge-import-backend-")) + ")</b><br/>";
            doc += service.description() + "<br/>";
        }
    }
    auto text = i18nc("Information",
                      "You must enter the list of backends to use separated by a ';'.\nA backend can have parameters. You can pass the parameters in "
                      "parenthesis separated by comma.\n\nExample: backendA;backendB(parameter1,parameter2).\n\nHere is the list of known backends: %1.",
                      doc);
    text = text.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
    ui.kbackendText->setText(text);

    // Add date formats
    QStringList dateFormats;
    dateFormats << i18nc("Format date", "Automatic detection") << QStringLiteral("YYYYMMDD") << QStringLiteral("MMDDYYYY") << QStringLiteral("DDMMYYYY")
                << QStringLiteral("MM-DD-YY") << QStringLiteral("DD-MM-YY") << QStringLiteral("MM-DD-YYYY") << QStringLiteral("DD-MM-YYYY")
                << QStringLiteral("YYYY-MM-DD") << QStringLiteral("DDMMMYYYY") << QStringLiteral("DD-MMM-YY") << QStringLiteral("DD-MMM-YYYY");
    ui.kcfg_qif_date_format->addItems(dateFormats);
    ui.kcfg_csv_date_format->addItems(dateFormats);

    return w;
}

KConfigSkeleton *SKGImportExportPlugin::getPreferenceSkeleton()
{
    return skgimportexport_settings::self();
}

SKGError SKGImportExportPlugin::savePreferences() const
{
    SKGError err;
    if (m_currentBankDocument != nullptr) {
        // Read Setting
        auto backends = skgimportexport_settings::backends();

        // Save setting in document
        if (backends != m_currentBankDocument->getParameter(QStringLiteral("SKG_BACKENDS"))) {
            err = m_currentBankDocument->setParameter(QStringLiteral("SKG_BACKENDS"), backends);
        }
    }
    return err;
}

QString SKGImportExportPlugin::title() const
{
    return i18nc("Noun", "Import / Export");
}

QString SKGImportExportPlugin::icon() const
{
    return QStringLiteral("utilities-file-archiver");
}

QString SKGImportExportPlugin::toolTip() const
{
    return i18nc("Noun", "Import / Export management");
}

QStringList SKGImportExportPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips",
                           "<p>… skrooge is able to detect <a href=\"skg://tab_configure?page=Skrooge import and export plugin\">automatically</a> transfers "
                           "after an import.</p>"));
    output.push_back(
        i18nc("Description of a tips",
              "<p>… you can automatically import transaction with <a href=\"skg://tab_configure?page=Skrooge import and export plugin\">backend</a>.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can <a href=\"skg://import_operation\">import</a> many files in one shot.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… unit amounts can be imported through a CSV file.</p>"));
    output.push_back(i18nc("Description of a tips",
                           "<p>… you can customize your CSV import with regular expressions defined in <a href=\"skg://tab_configure?page=Skrooge import and "
                           "export plugin\">setting</a> panel.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can export the full content of your document into a XML file.</p>"));
    output.push_back(i18nc(
        "Description of a tips",
        "<p>… you can export some accounts or transactions just be selecting them before to launch the <a href=\"skg://export\">export_operation</a>.</p>"));
    output.push_back(i18nc("Description of a tips",
                           "<p>… you can apply <a href=\"skg://skrooge_search_plugin\">automatic rules</a> after an import to set the right categories.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can convert file by using the batch tool '%1'.</p>", "skroogeconvert"));
    output.push_back(i18nc("Description of a tips", "<p>… skrooge uses the name of the imported file to find the target account.</p>"));
    return output;
}

QStringList SKGImportExportPlugin::subPlugins() const
{
    return QStringList() << QStringLiteral("skrooge_import") << QStringLiteral("skrooge_import_backend");
}

int SKGImportExportPlugin::getOrder() const
{
    return 70;
}

void SKGImportExportPlugin::refresh()
{
    SKGTRACEINFUNC(10)

    if ((m_currentBankDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        bool test = (m_currentBankDocument->getMainDatabase() != nullptr);

        // Automatic download
        if (test) {
            QString doc_id = m_currentBankDocument->getUniqueIdentifier();
            if (m_docUniqueIdentifier != doc_id) {
                m_docUniqueIdentifier = std::move(doc_id);

                SKGError err;

                if (skgimportexport_settings::download_on_open()) {
                    // Check frequency
                    QString lastAutomaticDownload = m_currentBankDocument->getParameter(QStringLiteral("SKG_LAST_BACKEND_AUTOMATIC_DOWNLOAD"));
                    if (!lastAutomaticDownload.isEmpty()) {
                        // The automatic import is not done if at least one manual import has not been done
                        QDate lastAutomaticDownloadDate = QDate::fromString(lastAutomaticDownload, QStringLiteral("yyyy-MM-dd"));
                        if ((lastAutomaticDownloadDate.daysTo(QDate::currentDate()) >= 1 && skgimportexport_settings::download_frequency() == 0)
                            || (lastAutomaticDownloadDate.daysTo(QDate::currentDate()) >= 7 && skgimportexport_settings::download_frequency() == 1)
                            || (lastAutomaticDownloadDate.daysTo(QDate::currentDate()) >= 30 && skgimportexport_settings::download_frequency() == 2))

                        {
                            // Import
                            importbackends();
                        }
                    }
                }
            }
        }
    }
}

SKGError SKGImportExportPlugin::importbackends()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (m_currentBankDocument != nullptr) {
        // Check if already in a transaction
        if (!m_currentBankDocument->checkExistingTransaction()) {
            // Repeat later
            QTimer::singleShot(300, Qt::CoarseTimer, this, &SKGImportExportPlugin::importbackends);
            return err;
        }

        // Get backends list to used
        QStringList backends = SKGServices::splitCSVLine(skgimportexport_settings::backends());
        int nbBackends = backends.count();

        // Import
        SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Import with backends"), err, nbBackends)
        for (int i = 0; !err && i < nbBackends; ++i) {
            QString backend = backends.at(i).trimmed();
            QString parameter;

            if (backend.contains(QStringLiteral("("))) {
                static QRegularExpression reg(QStringLiteral("^(.+)\\((.+)\\)$"));
                auto match = reg.match(backend);
                if (match.hasMatch()) {
                    backend = match.captured(1);
                    parameter = match.captured(2);
                } else {
                    err = SKGError(ERR_FAIL, i18nc("Error message", "Syntax error in backend \"%1\"", backend));
                    break;
                }
            }

            // Is password needed?
            QString pwd;
            QHash<QString, QString> properties;
            const auto services = SKGServices::findDataPlugins(QStringLiteral("skrooge_import_backend"));
            for (const auto &service : services) {
                auto os_supported = SKGServices::splitCSVLine(service.value(QStringLiteral("X-SKROOGE-ossupported"), QString()).toLower(), u',');
                if ((os_supported.isEmpty() || os_supported.contains(QSysInfo::kernelType().toLower()))
                    && service.pluginId().remove(QStringLiteral("org.kde.skrooge-import-backend-")) != backend) {
                    properties[QStringLiteral("x-skrooge-csvcolumns")] = service.value(QStringLiteral("X-SKROOGE-csvcolumns"));
                    properties[QStringLiteral("x-skrooge-getaccountbalance")] = service.value(QStringLiteral("X-SKROOGE-getaccountbalance"));
                    properties[QStringLiteral("x-skrooge-getaccountid")] = service.value(QStringLiteral("X-SKROOGE-getaccountid"));
                    properties[QStringLiteral("x-skrooge-getaccountname")] = service.value(QStringLiteral("X-SKROOGE-getaccountname"));
                    properties[QStringLiteral("x-skrooge-getaccounts")] = service.value(QStringLiteral("X-SKROOGE-getaccounts"));
                    properties[QStringLiteral("x-skrooge-getoperations")] = service.value(QStringLiteral("X-SKROOGE-getoperations"));
                }
            }

            if (!err
                && (properties[QStringLiteral("x-skrooge-getaccounts")].contains(QStringLiteral("%3"))
                    || properties[QStringLiteral("x-skrooge-getoperations")].contains(QStringLiteral("%3")))) {
                QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
                QPointer<KPasswordDialog> dlg = new KPasswordDialog(SKGMainPanel::getMainPanel());
                dlg->setPrompt(i18nc("Question", "The backend '%1' needs a password.\nPlease enter the password.", backend));
                int rc = dlg->exec();
                pwd = dlg->password();
                delete dlg;
                QApplication::restoreOverrideCursor();

                if (rc != QDialog::Accepted) {
                    continue;
                }
            }

            QString codec = m_currentBankDocument->getParameter(QStringLiteral("SKG_LAST_CODEC_USED_FOR_IMPORT"));
            if (codec.isEmpty()) {
                codec = QTextCodec::codecForLocale()->name();
            }
            IFOKDO(err, m_currentBankDocument->setParameter(QStringLiteral("SKG_LAST_CODEC_USED_FOR_IMPORT"), codec))

            SKGImportExportManager imp1(m_currentBankDocument, QUrl("." % backend));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("password")] = pwd;
            auto params = SKGServices::splitCSVLine(parameter, u',');
            int nbp = params.count();
            for (int j = 0; j < nbp; ++j) {
                parameters[QStringLiteral("parameter") % SKGServices::intToString(j + 1)] = params.at(j).trimmed();
            }
            imp1.setImportParameters(parameters);
            imp1.setAutomaticValidation(skgimportexport_settings::automatic_validation());
            imp1.setAutomaticApplyRules(skgimportexport_settings::apply_rules());
            // This option is not used with backend import
            imp1.setSinceLastImportDate(false);
            imp1.setCodec(codec);
            IFOKDO(err, imp1.importFile())
            IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
        }

        // Memorize the last download date
        IFOKDO(err,
               m_currentBankDocument->setParameter(QStringLiteral("SKG_LAST_BACKEND_AUTOMATIC_DOWNLOAD"),
                                                   QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))))
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);

    // Open last modified transactions if setting activated
    IFOK(err) openLastModifiedIfSetting();

    return err;
}

void SKGImportExportPlugin::importFile(const QString &iFile, bool iBlockOpenLastModified)
{
    importFiles(QList<QUrl>() << QUrl::fromLocalFile(iFile), static_cast<int>(iBlockOpenLastModified));
}

void SKGImportExportPlugin::importFiles(const QList<QUrl> &iFiles, int mode, bool iBlockOpenLastModified)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (m_currentBankDocument != nullptr) {
        QList<QUrl> fileNames;
        QString lastCodecUsed = m_currentBankDocument->getParameter(QStringLiteral("SKG_LAST_CODEC_USED_FOR_IMPORT"));
        if (lastCodecUsed.isEmpty()) {
            lastCodecUsed = QTextCodec::codecForLocale()->name();
        }
        QString codec;

        if (iFiles.isEmpty()) {
            // Panel to ask files
            KEncodingFileDialog::Result result = KEncodingFileDialog::getOpenUrlsAndEncoding(
                lastCodecUsed,
                QUrl::fromLocalFile(QUrl(QStringLiteral("kfiledialog:///IMPEXP")).toLocalFile()),
                mode == 2 || mode == 3 ? QString("*.csv|" % i18nc("A file format", "CSV Files")) : SKGImportExportManager().getImportMimeTypeFilter(),
                SKGMainPanel::getMainPanel());
            const auto &urls = result.URLs;
            fileNames.reserve(urls.count());
            for (const auto &u : std::as_const(urls)) {
                fileNames.append(QUrl(u.url()));
            }
            codec = result.encoding;
        } else {
            fileNames = iFiles;
            codec = std::move(lastCodecUsed);
        }

        int nbFiles = fileNames.count();
        if (nbFiles != 0) {
            {
                SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Import with codec %1", codec), err, nbFiles)

                // Read Setting
                bool automatic_validation = skgimportexport_settings::automatic_validation();
                bool automatic_rule = skgimportexport_settings::apply_rules();
                bool since_last = skgimportexport_settings::since_last_import();

                IFOKDO(err, m_currentBankDocument->setParameter(QStringLiteral("SKG_LAST_CODEC_USED_FOR_IMPORT"), codec))

                for (int i = 0; !err && i < nbFiles; ++i) {
                    // Get Filename
                    const QUrl &fileName = fileNames.at(i);

                    // Import
                    SKGImportExportManager imp1(m_currentBankDocument, fileName);
                    imp1.setAutomaticValidation(automatic_validation);
                    imp1.setAutomaticApplyRules(automatic_rule);
                    imp1.setSinceLastImportDate(since_last);
                    imp1.setCodec(codec);

                    if (QFileInfo(fileName.path()).suffix().toUpper() == QStringLiteral("CSV")) {
                        QMap<QString, QString> parameters = imp1.getImportParameters();

                        parameters[QStringLiteral("automatic_search_header")] =
                            (skgimportexport_settings::automatic_search_header() ? QStringLiteral("Y") : QStringLiteral("N"));
                        parameters[QStringLiteral("header_position")] = SKGServices::intToString(skgimportexport_settings::header_position());

                        parameters[QStringLiteral("mapping_date")] = skgimportexport_settings::mapping_date();
                        parameters[QStringLiteral("mapping_account")] = skgimportexport_settings::mapping_account();
                        parameters[QStringLiteral("mapping_number")] = skgimportexport_settings::mapping_number();
                        parameters[QStringLiteral("mapping_mode")] = skgimportexport_settings::mapping_mode();
                        parameters[QStringLiteral("mapping_payee")] = skgimportexport_settings::mapping_payee();
                        parameters[QStringLiteral("mapping_comment")] = skgimportexport_settings::mapping_comment();
                        parameters[QStringLiteral("mapping_status")] = skgimportexport_settings::mapping_status();
                        parameters[QStringLiteral("mapping_bookmarked")] = skgimportexport_settings::mapping_bookmarked();
                        parameters[QStringLiteral("mapping_category")] = skgimportexport_settings::mapping_category();
                        parameters[QStringLiteral("mapping_amount")] = skgimportexport_settings::mapping_amount();
                        parameters[QStringLiteral("mapping_quantity")] = skgimportexport_settings::mapping_quantity();
                        parameters[QStringLiteral("mapping_unit")] = skgimportexport_settings::mapping_unit();
                        parameters[QStringLiteral("mapping_idtransaction")] = skgimportexport_settings::mapping_idtransaction();
                        parameters[QStringLiteral("mapping_idgroup")] = skgimportexport_settings::mapping_idgroup();
                        parameters[QStringLiteral("mapping_sign")] = skgimportexport_settings::mapping_sign();
                        parameters[QStringLiteral("mapping_debit")] = skgimportexport_settings::mapping_debit();
                        parameters[QStringLiteral("mapping_property")] = skgimportexport_settings::mapping_property();

                        parameters[QStringLiteral("automatic_search_columns")] =
                            (skgimportexport_settings::automatic_search_columns() ? QStringLiteral("Y") : QStringLiteral("N"));
                        parameters[QStringLiteral("columns_positions")] = skgimportexport_settings::columns_positions();
                        parameters[QStringLiteral("mode_csv_unit")] = (mode == 2 ? QStringLiteral("Y") : QStringLiteral("N"));
                        parameters[QStringLiteral("mode_csv_rule")] = (mode == 3 ? QStringLiteral("Y") : QStringLiteral("N"));
                        parameters[QStringLiteral("date_format")] = skgimportexport_settings::csv_date_format();
                        if (!parameters[QStringLiteral("date_format")].contains(QStringLiteral("YY"))) {
                            parameters[QStringLiteral("date_format")] = QString();
                        }

                        imp1.setImportParameters(parameters);
                    } else if (QFileInfo(fileName.path()).suffix().toUpper() == QStringLiteral("QIF")) {
                        QMap<QString, QString> parameters = imp1.getImportParameters();

                        parameters[QStringLiteral("date_format")] = skgimportexport_settings::qif_date_format();
                        if (!parameters[QStringLiteral("date_format")].contains(QStringLiteral("YY"))) {
                            parameters[QStringLiteral("date_format")] = QString();
                        }

                        imp1.setImportParameters(parameters);
                    } else if (QFileInfo(fileName.path()).suffix().toUpper() == QStringLiteral("LEDGER")) {
                        QMap<QString, QString> parameters = imp1.getImportParameters();
                        parameters[QStringLiteral("ledger_account_identification")] = skgimportexport_settings::ledger_account_identification();
                        imp1.setImportParameters(parameters);
                    }

                    // Optimization
                    if (i != nbFiles - 1) {
                        QMap<QString, QString> parameters = imp1.getImportParameters();
                        parameters[QStringLiteral("donotfinalize")] = 'Y';
                        imp1.setImportParameters(parameters);
                    }

                    if (m_install) {
                        QMap<QString, QString> parameters = imp1.getImportParameters();
                        parameters[QStringLiteral("install_sunriise")] = 'Y';
                        imp1.setImportParameters(parameters);
                    }

                    IFOKDO(err, imp1.importFile())
                    if (err && err.getReturnCode() == ERR_ENCRYPTION) {
                        QString pwd;
                        QString additionalMessage;
                        do {
                            // Reset error
                            err = SKGError(ERR_FAIL, i18nc("Error message", "Import of file named '%1' failed", fileName.toDisplayString()));
                            pwd = QString();

                            // Use password dialog
                            QApplication::restoreOverrideCursor();
                            QPointer<KPasswordDialog> dlg = new KPasswordDialog(SKGMainPanel::getMainPanel());
                            dlg->setPrompt(additionalMessage % i18nc("Question", "This file seems to be protected.\nPlease enter the password."));
                            if (dlg->exec() == QDialog::Accepted) {
                                pwd = dlg->password();
                            }
                            delete dlg;
                            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

                            // Load file
                            if (!pwd.isEmpty()) {
                                QMap<QString, QString> parameters = imp1.getImportParameters();
                                parameters[QStringLiteral("password")] = pwd;
                                imp1.setImportParameters(parameters);
                                err = imp1.importFile();
                                IFKO(err)
                                {
                                    if (err.getReturnCode() == ERR_ENCRYPTION) {
                                        additionalMessage = i18nc("The user did not provide the correct password", "<b>Wrong password.</b>\n");
                                    } else {
                                        // Import error
                                        break;
                                    }
                                }
                            } else {
                                err = SKGError(ERR_FAIL, i18nc("Error message", "Import canceled by user"));
                                break;
                            }
                        } while (err);
                    }

                    if (err && err.getReturnCode() != ERR_INSTALL) {
                        err.addError(ERR_FAIL, i18nc("Error message", "Import of file named '%1' failed", fileName.toDisplayString()));
                    }

                    IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
                }
            }

            // status bar
            IFOK(err)
            {
                err = SKGError(0, i18np("%1 file successfully imported.", "%1 files successfully imported.", nbFiles));
            }

            // Display error
            m_install = false;
            KMessageWidget *msg = SKGMainPanel::displayErrorMessage(err);
            if (err.getReturnCode() == ERR_INSTALL && (msg != nullptr)) {
                QString application = err.getProperty();
                auto install = new QAction(i18nc("Noun", "Install %1", application), msg);
                install->setIcon(SKGServices::fromTheme(QStringLiteral("download")));
                msg->addAction(install);
                connect(install, &QAction::triggered, this, &SKGImportExportPlugin::onInstall);
                connect(install, &QAction::triggered, msg, &KMessageWidget::deleteLater, Qt::QueuedConnection);
            }

            // Open last modified transactions if setting activated
            if (!iBlockOpenLastModified) {
                IFOK(err) openLastModifiedIfSetting();
            }
        }
    }
}

void SKGImportExportPlugin::onInstall()
{
    m_install = true;
    SKGMainPanel::getMainPanel()->displayMessage(i18nc("Information message", "The installation will be done during the next import"));
}

void SKGImportExportPlugin::exportFile()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (m_currentBankDocument != nullptr) {
        QString lastCodecUsed = m_currentBankDocument->getParameter(QStringLiteral("SKG_LAST_CODEC_USED_FOR_IMPORT"));
        if (lastCodecUsed.isEmpty()) {
            lastCodecUsed = QTextCodec::codecForLocale()->name();
        }
        QString fileName = SKGMainPanel::getSaveFileName(QStringLiteral("kfiledialog:///IMPEXP"),
                                                         SKGImportExportManager().getExportMimeTypeFilter(),
                                                         SKGMainPanel::getMainPanel(),
                                                         &lastCodecUsed);
        if (fileName.isEmpty() || (m_currentBankDocument == nullptr)) {
            return;
        }

        QString uuids;
        const auto objects = SKGMainPanel::getMainPanel()->getSelectedObjects();
        for (const auto &obj : objects) {
            if (!uuids.isEmpty()) {
                uuids.append(";");
            }
            uuids.append(obj.getUniqueID());
        }

        {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Export"), err)
            IFOK(err)
            {
                SKGImportExportManager imp1(m_currentBankDocument, QUrl::fromLocalFile(fileName));
                imp1.setCodec(lastCodecUsed);
                QMap<QString, QString> params;
                params[QStringLiteral("uuid_of_selected_accounts_or_operations")] = uuids;
                imp1.setExportParameters(params);
                err = imp1.exportFile();
            }
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "File '%1' successfully exported.", fileName)))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Export of '%1' failed", fileName));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGImportExportPlugin::anonymize()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (m_currentBankDocument != nullptr) {
        QString pwd;
        QPointer<KPasswordDialog> dlg = new KPasswordDialog(SKGMainPanel::getMainPanel());
        dlg->setPrompt(i18nc(
            "Question",
            "The file can be made anonymous in two ways.<br/><b>Reversibly:</b> enter a key and memorize it, it will be used to go back.<br/><b>Irreversibly "
            "(recommended):</b> do not enter a key.<br/><br/>To reverse an anonymous file, simply try to anonymize it with the same key."));
        if (dlg->exec() == QDialog::Accepted) {
            pwd = dlg->password();
        }

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        SKGImportExportManager imp1(m_currentBankDocument);
        err = imp1.anonymize(pwd);
        QApplication::restoreOverrideCursor();

        // status bar
        IFOKDO(err, SKGError(0, i18nc("An anonymized document is a document where all private data has been removed", "Document anonymized.")))

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGImportExportPlugin::findTransfers()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (m_currentBankDocument != nullptr) {
        int NbOperationsMerged = 0;
        {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Find and group transfers"), err)
            IFOK(err)
            {
                SKGImportExportManager imp1(m_currentBankDocument);
                err = imp1.findAndGroupTransfers(NbOperationsMerged);
            }
        }

        // status bar
        IFOK(err)
        {
            if (NbOperationsMerged != 0) {
                err = SKGError(0,
                               i18np("Document successfully processed. %1 transfer created.",
                                     "Document successfully processed. %1 transfers created.",
                                     NbOperationsMerged));
            } else {
                err = m_currentBankDocument->sendMessage(i18nc("Information message", "No transfers found"));
            }
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Processing failed."));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);

        // Open last modified transactions if setting activated
        if (!err && (NbOperationsMerged != 0)) {
            openLastModifiedIfSetting();
        }
    }
}

void SKGImportExportPlugin::cleanBanks()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Clean bank's imports"), err)
        IFOK(err)
        {
            SKGImportExportManager imp1(m_currentBankDocument);
            err = imp1.cleanBankImport();
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Document successfully cleaned.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Clean failed."));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);

    // Open last modified transactions if setting activated
    IFOK(err) openLastModifiedIfSetting();
}

void SKGImportExportPlugin::swithvalidationImportedOperations()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        {
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Switch validation of imported transactions"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(selection.at(i));
                if (op.getAttribute(QStringLiteral("t_imported")) == QStringLiteral("P")) {
                    err = op.setImported(true);
                    IFOKDO(err, op.save())
                } else if (op.getAttribute(QStringLiteral("t_imported")) == QStringLiteral("Y")) {
                    err = op.setAttribute(QStringLiteral("t_imported"), QStringLiteral("P"));
                    IFOKDO(err, op.save())
                }
                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Imported transactions validated.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Validation failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGImportExportPlugin::mergeImportedOperation()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        err = SKGError(ERR_INVALIDARG,
                       i18nc("Error message", "Invalid selection, you must select one imported transaction and one manual transaction with same amounts"));
        if (nb == 2) {
            SKGOperationObject opImported(selection.at(0));
            SKGOperationObject opManual(selection.at(1));
            if (opImported.isImported() || opManual.isImported()) {
                if (opImported.isImported() && opManual.isImported()) {
                    // Both are imports, so the "imported" on is the last one
                    if (opImported.getID() < opManual.getID()) {
                        qSwap(opImported, opManual);
                    }
                } else if (!opImported.isImported()) {
                    qSwap(opImported, opManual);
                }

                // Mode force?
                bool modeForce = false;
                auto *act = qobject_cast<QAction *>(sender());
                if (act != nullptr) {
                    modeForce = (act->data().toInt() == 1);
                }

                if (!modeForce
                    && m_currentBankDocument->formatMoney(opImported.getCurrentAmount(), m_currentBankDocument->getPrimaryUnit())
                        != m_currentBankDocument->formatMoney(opManual.getCurrentAmount(), m_currentBankDocument->getPrimaryUnit())) {
                    SKGMainPanel::getMainPanel()->displayMessage(i18nc("Question", "Amounts are not equals. Do you want to force the merge ?"),
                                                                 SKGDocument::Error,
                                                                 QStringLiteral("skg://merge_imported_operation_force"));
                    err = SKGError();
                } else {
                    SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Merge imported transactions"), err)
                    err = opManual.mergeAttribute(opImported);
                    IFKO(err) err.addError(ERR_FAIL, i18nc("Error message", "Merge failed"));
                }
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Imported transactions merged.")))

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGImportExportPlugin::openLastModifiedIfSetting()
{
    // Read Setting
    bool open_after_import_or_processing = skgimportexport_settings::open_after_import_or_processing();
    if (open_after_import_or_processing) {
        // Open last transactions
        QAction *act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("view_open_last_modified"));
        if (act != nullptr) {
            act->trigger();
        }
    }
}

void SKGImportExportPlugin::validateAllOperations()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Validate all transactions"), err)
        err = m_currentBankDocument->executeSqliteOrder(QStringLiteral("UPDATE operation SET t_imported='Y' WHERE t_imported='P'"));
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Transactions validated.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Validation failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

SKGAdviceList SKGImportExportPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;

    // Check transactions not validated
    if (!iIgnoredAdvice.contains(QStringLiteral("skgimportexportplugin_notvalidated"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(QStringLiteral("operation"), QStringLiteral("t_imported='P'"), exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgimportexportplugin_notvalidated"));
            ad.setPriority(4);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Many transactions imported and not yet validated"));
            ad.setLongMessage(i18nc("Advice on making the best (long)",
                                    "After importing transactions, you should review them, make corrections on, for instance, category, payee. Once done, you "
                                    "should mark the imported transaction as validated, so that you know the transaction has been fully processed."));
            SKGAdvice::SKGAdviceActionList autoCorrections;
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = QStringLiteral("skg://view_open_not_validated");
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = QStringLiteral("skg://process_validate");
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Krunner transactions
    QDir dirName = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    QStringList fileList = dirName.entryList(QStringList() << QStringLiteral("add_operation_*.txt"), QDir::Files);
    int nb = fileList.count();
    if (nb != 0) {
        QStringList listAccounts;
        m_currentBankDocument->getDistinctValues(QStringLiteral("account"),
                                                 QStringLiteral("t_name"),
                                                 QStringLiteral("t_type IN ('C', 'D', 'W') and t_close='N'"),
                                                 listAccounts);
        int nbAccounts = listAccounts.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        autoCorrections.reserve(nbAccounts + 1);
        for (int i = 0; i < nb; ++i) {
            QString fileName = dirName.filePath(fileList.at(i));
            QFile file(fileName);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream stream(&file);
                stream.readLine(); // action not used yet
                QString date = QLocale().toString(SKGServices::stringToDate(stream.readLine().trimmed()), QLocale::ShortFormat);
                QString amount =
                    m_currentBankDocument->formatMoney(SKGServices::stringToDouble(stream.readLine().trimmed()), m_currentBankDocument->getPrimaryUnit());
                QString payee = stream.readLine().trimmed();
                SKGAdvice ad;
                ad.setUUID("skgimportexportplugin_krunner_" % fileName);
                ad.setPriority(8);
                ad.setShortMessage(i18nc("Advice on making the best (short)", "Krunner's transaction ongoing [%1 %2 %3]", date, amount, payee));
                ad.setLongMessage(i18nc("Advice on making the best (long)", "Transactions created through krunner have to be fully created in skrooge."));
                autoCorrections.resize(0);
                for (int j = 0; j < nbAccounts; ++j) {
                    SKGAdvice::SKGAdviceAction a;
                    a.Title = i18nc("Advice on making the best (action)", "Import transaction in %1", listAccounts.at(j));
                    a.IconName = icon();
                    a.IsRecommended = false;
                    autoCorrections.push_back(a);
                }
                {
                    SKGAdvice::SKGAdviceAction a;
                    a.Title = i18nc("Advice on making the best (action)", "Remove transaction");
                    a.IconName = QStringLiteral("edit-delete");
                    a.IsRecommended = false;
                    autoCorrections.push_back(a);
                }
                ad.setAutoCorrections(autoCorrections);
                output.push_back(ad);

                // Close file
                file.close();
            }
        }
    }
    return output;
}

SKGError SKGImportExportPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    if (iAdviceIdentifier.startsWith(QStringLiteral("skgimportexportplugin_krunner_")) && (m_currentBankDocument != nullptr)) {
        SKGError err;
        // Get file name
        QString fileName = iAdviceIdentifier.right(iAdviceIdentifier.length() - 30);
        QFile file(fileName);

        // Get accounts
        QStringList listAccounts;
        m_currentBankDocument->getDistinctValues(QStringLiteral("account"),
                                                 QStringLiteral("t_name"),
                                                 QStringLiteral("t_type IN ('C', 'D', 'W') and t_close='N'"),
                                                 listAccounts);
        if (iSolution < listAccounts.count()) {
            // Addition in an account

            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                err = SKGError(ERR_FAIL, i18nc("An error message", "Open file '%1' failed", fileName));
            } else {
                QTextStream stream(&file);
                stream.readLine(); // action is not used yet
                QDate date = SKGServices::stringToDate(stream.readLine().trimmed());
                double amount = SKGServices::stringToDouble(stream.readLine().trimmed());
                QString payee = stream.readLine().trimmed();

                SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Import krunner's transaction"), err)

                // Get account
                SKGAccountObject act(m_currentBankDocument);
                err = act.setName(listAccounts.at(iSolution));
                IFOKDO(err, act.load())

                // Get unit
                SKGUnitObject unit(m_currentBankDocument);
                IFOKDO(err, unit.setName(m_currentBankDocument->getPrimaryUnit().Name))
                IFOKDO(err, unit.load())

                // Add transaction
                SKGOperationObject op;
                IFOKDO(err, act.addOperation(op))
                IFOKDO(err, op.setDate(date))
                IFOKDO(err, op.setUnit(unit))

                if (!payee.isEmpty()) {
                    // Get payee
                    SKGPayeeObject pa;
                    IFOKDO(err, SKGPayeeObject::createPayee(m_currentBankDocument, payee, pa, true))
                    IFOKDO(err, op.setPayee(pa))
                }
                IFOK(err)
                {
                    int pos1 = fileName.indexOf(QStringLiteral("{"));
                    int pos2 = fileName.indexOf(QStringLiteral("}"));
                    if (pos1 != -1 && pos2 > pos1) {
                        err = op.setImportID("KR-" % fileName.mid(pos1 + 1, pos2 - pos1 - 1));
                    }
                }
                IFOKDO(err, op.save())

                // Add suboperation
                SKGSubOperationObject sop;
                IFOKDO(err, op.addSubOperation(sop))
                IFOKDO(err, sop.setQuantity(-amount))
                IFOKDO(err, sop.save())

                // Finalize the importation
                IFOK(err)
                {
                    bool automatic_validation = skgimportexport_settings::automatic_validation();
                    bool automatic_rule = skgimportexport_settings::apply_rules();
                    bool since_last = skgimportexport_settings::since_last_import();

                    SKGImportExportManager imp1(m_currentBankDocument);
                    imp1.setAutomaticValidation(automatic_validation);
                    imp1.setAutomaticApplyRules(automatic_rule);
                    imp1.setSinceLastImportDate(since_last);
                    err = imp1.finalizeImportation();
                }

                // Send message
                IFOKDO(err,
                       op.getDocument()->sendMessage(i18nc("An information to the user", "The transaction '%1' has been added", op.getDisplayName()),
                                                     SKGDocument::Hidden))

                // Close file
                file.close();
            }

            // status bar
            IFOK(err)
            {
                err = SKGError(0, i18nc("Message for successful user action", "Transactions imported."));
                QFile::remove(fileName);
            }
            else
            {
                err.addError(ERR_FAIL, i18nc("Error message", "Import failed"));
            }
        } else {
            err = SKGError(0, i18nc("Message for successful user action", "Transactions removed."));
            QFile::remove(fileName);
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
        return SKGError();
    }
    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

#include <skgimportexportplugin.moc>
