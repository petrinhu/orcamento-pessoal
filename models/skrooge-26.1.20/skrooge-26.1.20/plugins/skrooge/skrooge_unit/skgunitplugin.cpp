/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for unit management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgunitplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kpluginmetadata.h>

#include <qaction.h>
#include <qinputdialog.h>
#include <qprocess.h>
#include <qvariant.h>

#include "skgdocumentbank.h"
#include "skghtmlboardwidget.h"
#include "skgmainpanel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include "skgunit_settings.h"
#include "skgunitboardwidget.h"
#include "skgunitpluginwidget.h"
#include <utility>

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGUnitPlugin, "metadata.json")

SKGUnitPlugin::SKGUnitPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGUnitPlugin::~SKGUnitPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGUnitPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skrooge_unit"), title());
    setXMLFile(QStringLiteral("skrooge_unit.rc"));

    // Menu
    auto actSplitShare = new QAction(SKGServices::fromTheme(QStringLiteral("format-text-strikethrough")), i18nc("Verb", "Split share…"), this);
    connect(actSplitShare, &QAction::triggered, this, &SKGUnitPlugin::onSplitShare);
    actionCollection()->setDefaultShortcut(actSplitShare, Qt::ALT | Qt::Key_Slash);
    registerGlobalAction(QStringLiteral("edit_split_stock"),
                         actSplitShare,
                         QStringList() << QStringLiteral("unit"),
                         1,
                         1,
                         310); // TODO(Stephane MANKOWSKI): must be a share

    // -----------
    auto act = new QAction(SKGServices::fromTheme(icon()), i18nc("Verb", "Delete unused units"), this);
    connect(act, &QAction::triggered, this, &SKGUnitPlugin::deleteUnusedUnits);
    registerGlobalAction(QStringLiteral("clean_delete_unused_units"), act);
    return true;
}

int SKGUnitPlugin::getNbDashboardWidgets()
{
    return 2;
}

QString SKGUnitPlugin::getDashboardWidgetTitle(int iIndex)
{
    if (iIndex == 0) {
        return i18nc("Noun, the title of a section", "Quotes");
    }
    return i18nc("Noun, the title of a section", "Stock portfolio");
}

SKGBoardWidget *SKGUnitPlugin::getDashboardWidget(int iIndex)
{
    if (iIndex == 0) {
        return new SKGUnitBoardWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
    }

    auto listForFilter =
        std::move(QStringList() << QStringLiteral("t_name") << QStringLiteral("t_symbol") << QStringLiteral("t_country") << QStringLiteral("t_type")
                                << QStringLiteral("t_internet_code") << QStringLiteral("t_source") << QStringLiteral("t_bookmarked"));

    return new SKGHtmlBoardWidget(SKGMainPanel::getMainPanel(),
                                  m_currentBankDocument,
                                  getDashboardWidgetTitle(iIndex),
                                  QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/portfolio.qml")),
                                  QStringList() << QStringLiteral("v_operation_display"),
                                  SKGSimplePeriodEdit::NONE,
                                  listForFilter);
}

void SKGUnitPlugin::refresh()
{
    SKGTRACEINFUNC(10)
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        // Automatic download
        QString doc_id = m_currentBankDocument->getUniqueIdentifier();
        if (m_docUniqueIdentifier != doc_id) {
            m_docUniqueIdentifier = std::move(doc_id);
            // Check if current unit is existing
            bool exist = false;
            SKGError err = m_currentBankDocument->existObjects(QStringLiteral("unit"), QString(), exist);
            IFOK(err)
            {
                if (!exist) {
                    SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Create default unit"), err)
                    IFOK(err)
                    {
                        // Create default unit
                        SKGUnitObject unit;
                        QString unitS = QLocale().currencySymbol(QLocale::CurrencyIsoCode);
                        if (!unitS.isEmpty()) {
                            err = SKGUnitObject::createCurrencyUnit(m_currentBankDocument, unitS, unit);
                        }

                        // The file is considered has not modified
                        m_currentBankDocument->setFileNotModified();
                    }
                } else if (skgunit_settings::download_on_open()) {
                    // Check frequency
                    QString lastAutomaticDownload = m_currentBankDocument->getParameter(QStringLiteral("SKG_LAST_UNIT_AUTOMATIC_DOWNLOAD"));
                    if (lastAutomaticDownload.isEmpty()) {
                        lastAutomaticDownload = QStringLiteral("1970-01-01");
                    }
                    QDate lastAutomaticDownloadDate = QDate::fromString(lastAutomaticDownload, QStringLiteral("yyyy-MM-dd"));
                    if ((lastAutomaticDownloadDate.daysTo(QDate::currentDate()) >= 1 && skgunit_settings::download_frequency() == 0)
                        || (lastAutomaticDownloadDate.daysTo(QDate::currentDate()) >= 7 && skgunit_settings::download_frequency() == 1)
                        || (lastAutomaticDownloadDate.daysTo(QDate::currentDate()) >= 30 && skgunit_settings::download_frequency() == 2))

                    {
                        // Download all units
                        SKGObjectBase::SKGListSKGObjectBase selection;
                        err = m_currentBankDocument->getObjects(QStringLiteral("unit"), QString(), selection);
                        int nb = selection.count();
                        SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Automatic download of units"), err, nb)
                        for (int i = 0; !err && i < nb; ++i) {
                            SKGUnitObject unit(selection.at(i));
                            err = SKGUnitPluginWidget::downloadUnitValue(unit, SKGUnitPluginWidget::getDownloadModeFromSettings());

                            // Send message
                            IFOKDO(err,
                                   m_currentBankDocument->sendMessage(
                                       i18nc("An information to the user", "The unit '%1' has been downloaded", unit.getDisplayName()),
                                       SKGDocument::Hidden))

                            IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
                        }

                        // Memorize the last download date
                        IFOKDO(err,
                               m_currentBankDocument->setParameter(QStringLiteral("SKG_LAST_UNIT_AUTOMATIC_DOWNLOAD"),
                                                                   QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))))
                    }
                }
            }

            // Display error
            SKGMainPanel::displayErrorMessage(err);
        }
    }
}

QStringList SKGUnitPlugin::subPlugins() const
{
    return QStringList() << QStringLiteral("skrooge_source");
}

SKGTabPage *SKGUnitPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGUnitPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QWidget *SKGUnitPlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    auto w = new QWidget();
    ui.setupUi(w);

    // Get sources from.desktop file
    QStringList sources;
    const auto list2 = SKGServices::findDataPlugins(QStringLiteral("skrooge_source"));
    for (const auto &service : list2) {
        auto name = service.pluginId().remove(QStringLiteral("org.kde.skrooge-source-")).toLower();
        auto keyAPI = service.value(QStringLiteral("X-SKROOGE-keyAPI"), false);
        if (!sources.contains(name) && keyAPI) {
            sources.push_back(name);
        }
    }
    sources.sort();

    auto nb = sources.count();
    auto lwidgets = new QHash<QString, QLineEdit *>();
    for (auto i = 0; i < nb; ++i) {
        // Get Current value
        auto ln = new QLineEdit(w);
        ln->setText(m_currentBankDocument->getParameter("KEYAPI_" + sources[i]));
        lwidgets->insert(sources.value(i), ln);

        ui.kAPIKeyLayout->addWidget(new QLabel(sources[i] + ':', w), i, 0);
        ui.kAPIKeyLayout->addWidget(ln, i, 1);
    }

    connect(ui.kcfg_download_on_open, &QCheckBox::toggled, ui.kcfg_download_frequency, &KComboBox::setEnabled);
    connect(ui.kSave, &QPushButton::clicked, this, [this, lwidgets]() {
        SKGError err;
        {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Save API keys"), err)
            auto keys = lwidgets->keys();
            for (const auto &k : std::as_const(keys)) {
                m_currentBankDocument->setParameter("KEYAPI_" + k, lwidgets->value(k)->text());
            }
        }
        // Display error
        SKGMainPanel::displayErrorMessage(err);
    });

    return w;
}

KConfigSkeleton *SKGUnitPlugin::getPreferenceSkeleton()
{
    return skgunit_settings::self();
}

QString SKGUnitPlugin::title() const
{
    return i18nc("Noun, units for transactions, usually currencies or a shares", "Units");
}

QString SKGUnitPlugin::icon() const
{
    return QStringLiteral("taxes-finances");
}

QString SKGUnitPlugin::toolTip() const
{
    return i18nc("A tool tip", "Unit management");
}

QStringList SKGUnitPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… you can download <a href=\"skg://skrooge_unit_plugin\">units</a>.</p>"));
    output.push_back(i18nc("Description of a tips",
                           "<p>… <a href=\"skg://skrooge_unit_plugin\">units</a> can be downloaded <a href=\"skg://tab_configure?page=Skrooge Unit "
                           "Plugin\">automatically</a> when a document is opened.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can split a <a href=\"skg://skrooge_unit_plugin\">share</a>.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… <a href=\"skg://skrooge_unit_plugin\">units</a> can be merged by drag & drop.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can download more <a href=\"skg://skrooge_unit_plugin\">sources</a> of quote.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can create and share your own source of quote.</p>"));

    return output;
}

int SKGUnitPlugin::getOrder() const
{
    return 60;
}

bool SKGUnitPlugin::isInPagesChooser() const
{
    return true;
}

void SKGUnitPlugin::onSplitShare()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Get Selection
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        if (nb == 1) {
            bool ok = false;
            double ratio = QInputDialog::getDouble(SKGMainPanel::getMainPanel(),
                                                   i18nc("Question", "Split share"),
                                                   i18nc("Question", "Ratio (2 means 2-for-1, 0.5 means 1-for-2):"),
                                                   2.0,
                                                   0,
                                                   std::numeric_limits<double>::max(),
                                                   8,
                                                   &ok);
            if (ok) {
                SKGUnitObject unit(selection.at(0));
                SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Split stock '%1' by '%2'", unit.getName(), ratio), err)
                IFOKDO(err, unit.split(ratio))
            }
        }

        // status
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Stock split.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Splitting stock failed."));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

SKGAdviceList SKGUnitPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    output.reserve(20);

    // Get all currencies
    SKGStringListList result;
    m_currentBankDocument->executeSelectSqliteOrder(QStringLiteral("SELECT (SELECT count(1) FROM operation WHERE operation.rc_unit_id=unit.id), unit.t_name "
                                                                   "FROM unit WHERE t_type='C' GROUP BY t_name ORDER BY count(1) DESC"),
                                                    result);
    int nb = result.count();

    // Check primary unit
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_primaryunit"))) {
        if (m_currentBankDocument->getPrimaryUnit().Name.isEmpty() && nb > 1) {
            // Get unit
            QString unit = result.at(1).at(1);

            SKGAdvice ad;
            ad.setUUID("skgunitplugin_primaryunit|" % unit);
            ad.setPriority(8);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Define a primary currency"));
            ad.setLongMessage(i18nc("Advice on making the best (long)",
                                    "To avoid misunderstanding and conflicts between units at conversion time, you should define a primary currency. It is the "
                                    "currency against which all other will be converted"));
            SKGAdvice::SKGAdviceActionList autoCorrections;
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Set '%1' as primary currency", unit);
                a.IconName = icon();
                a.IsRecommended = true;
                autoCorrections.push_back(a);
            }
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Edit units");
                a.IconName = icon();
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);

            --nb;
        }
    }

    // Check secondary unit
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_secondaryunit"))) {
        if (m_currentBankDocument->getSecondaryUnit().Name.isEmpty() && nb > 1) {
            // Get unit
            QString unit = result.at(1).at(1);

            SKGAdvice ad;
            ad.setUUID("skgunitplugin_secondaryunit|" % unit);
            ad.setPriority(2);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Define a secondary currency"));
            ad.setLongMessage(
                i18nc("Advice on making the best (long)", "When a secondary unit is defined, Skrooge will display it as an additional amount information."));
            SKGAdvice::SKGAdviceActionList autoCorrections;
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Set '%1' as secondary currency", unit);
                a.IconName = icon();
                a.IsRecommended = true;
                autoCorrections.push_back(a);
            }
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Edit units");
                a.IconName = icon();
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Shares not downloaded
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_notdownloaded"))) {
        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral(
                "SELECT t_name, t_internet_code from unit WHERE t_internet_code<>'' AND (julianday('now', 'localtime')-(SELECT MAX(julianday(d_date)) FROM "
                "unitvalue WHERE rd_unit_id=unit.id ))>30 OR NOT EXISTS (SELECT 1 FROM unitvalue WHERE unitvalue.rd_unit_id=unit.id)"),
            result);
        nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        autoCorrections.reserve(nb);
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            const QString &unit = line.at(0);
            const QString &internet_code = line.at(1);

            SKGAdvice ad;
            ad.setUUID("skgunitplugin_notdownloaded|" % unit);
            ad.setPriority(5);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Unit '%1' has not been downloaded for more than a month", unit));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "Do not forget download units to have a better view of your accounts"));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Edit units");
                a.IconName = icon();
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            if (!internet_code.isEmpty()) {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Download '%1'", unit);
                a.IconName = QStringLiteral("download");
                a.IsRecommended = true;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Check unused units
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_unused"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(
            QStringLiteral("unit"),
            QStringLiteral("t_type NOT IN ('I', '1', '2') AND NOT EXISTS (SELECT 1 FROM operation WHERE operation.rc_unit_id=unit.id) AND NOT EXISTS (SELECT 1 "
                           "FROM unit as unit2 WHERE unit2.rd_unit_id=unit.id)"),
            exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgunitplugin_unused"));
            ad.setPriority(5);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Many unused units"));
            ad.setLongMessage(
                i18nc("Advice on making the best (long)", "You can improve performances by removing units for which no transaction is registered."));
            QStringList autoCorrections;
            autoCorrections.push_back(QStringLiteral("skg://clean_delete_unused_units"));
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Check unit too complex
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_amountnotdefined"))) {
        m_currentBankDocument->executeSelectSqliteOrder(QStringLiteral("SELECT t_name FROM v_unit WHERE t_type IN ('2','C') AND f_CURRENTAMOUNT=1"), result);
        nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        autoCorrections.reserve(nb);
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            const QString &unit = line.at(0);

            SKGAdvice ad;
            ad.setUUID("skgunitplugin_amountnotdefined|" % unit);
            ad.setPriority(9);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "The amount of the unit '%1' is not defined", unit));
            ad.setLongMessage(
                i18nc("Advice on making the best (long)",
                      "'%1' has an amount defined at 1. Most of the time this is not normal and causes wrong computation. Check if this is normal.",
                      unit));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Edit units");
                a.IconName = icon();
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Check unit too complex
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_toocomplex"))) {
        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT A.t_name FROM unit A, unit B, unit C, unit D WHERE A.rd_unit_id=B.id AND B.rd_unit_id=C.id AND C.rd_unit_id=D.id"),
            result);
        nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        autoCorrections.reserve(nb);
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            const QString &unit = line.at(0);

            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgunitplugin_toocomplex"));
            ad.setPriority(9);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "The definition of the unit '%1' is too complex", unit));
            ad.setLongMessage(
                i18nc("Advice on making the best (long)",
                      "'%1' is defined relatively to another unit defined relatively to a third one. This is too complex and not supported by Skrooge.",
                      unit));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Edit units");
                a.IconName = icon();
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Unit with very old values
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_veryold"))) {
        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT t_name from unit WHERE (SELECT COUNT(*) FROM unitvalue WHERE unitvalue.rd_unit_id=unit.id)>1 AND EXISTS (SELECT 1 FROM "
                           "unitvalue WHERE unitvalue.rd_unit_id=unit.id AND unitvalue.d_date<=(date('now', 'localtime', '-50 year')))"),
            result);
        nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        autoCorrections.reserve(nb);
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            const QString &unit = line.at(0);

            SKGAdvice ad;
            ad.setUUID("skgunitplugin_veryold|" % unit);
            ad.setPriority(3);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Unit '%1' has very old values", unit));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "Unit '%1' has very old values. Check if this is normal.", unit));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Edit units");
                a.IconName = icon();
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Unit with very old values
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_too_long_symbol"))) {
        m_currentBankDocument->executeSelectSqliteOrder(QStringLiteral("SELECT t_name from unit WHERE length(t_symbol)>7"), result);
        nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        autoCorrections.reserve(nb);
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            const QString &unit = line.at(0);

            SKGAdvice ad;
            ad.setUUID("skgunitplugin_too_long_symbol|" % unit);
            ad.setPriority(3);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Unit '%1' has a too long symbol", unit));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "Unit '%1' has a too long symbol. You should shorten it.", unit));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Advice on making the best (action)", "Edit units");
                a.IconName = icon();
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // No decimal settings
    if (!iIgnoredAdvice.contains(QStringLiteral("skgunitplugin_decimalsymbol")) && QLocale().decimalPoint().isNull()) {
        SKGAdvice ad;
        ad.setUUID(QStringLiteral("skgunitplugin_decimalsymbol"));
        ad.setPriority(9);
        ad.setShortMessage(i18nc("Advice on making the best (short)", "No decimal symbol defined"));
        ad.setLongMessage(i18nc("Advice on making the best (long)",
                                "In KDE localization settings, there is no decimal symbol defined for currencies. This could be confusing."));
        SKGAdvice::SKGAdviceActionList autoCorrections;
        {
            SKGAdvice::SKGAdviceAction a;
            a.Title = i18nc("Advice on making the best (action)", "Edit KDE settings");
            a.IconName = QStringLiteral("configure");
            a.IsRecommended = false;
            autoCorrections.push_back(a);
        }
        ad.setAutoCorrections(autoCorrections);
        output.push_back(ad);
    }

    return output;
}

SKGError SKGUnitPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgunitplugin_primaryunit|"))) {
        if (iSolution == 1) {
            SKGMainPanel::getMainPanel()->openPage(QStringLiteral("skg://skrooge_unit_plugin"));
        } else {
            // Get parameters
            QString unit = iAdviceIdentifier.right(iAdviceIdentifier.length() - 26);

            SKGError err;
            {
                SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Define primary currency"), err)
                SKGUnitObject unitObj(m_currentBankDocument);
                err = unitObj.setName(unit);
                IFOKDO(err, unitObj.load())
                IFOKDO(err, unitObj.setType(SKGUnitObject::PRIMARY))
                IFOKDO(err, unitObj.save())

                // Send message
                IFOKDO(
                    err,
                    unitObj.getDocument()->sendMessage(i18nc("An information to the user", "The unit '%1' is now the primary unit", unitObj.getDisplayName()),
                                                       SKGDocument::Hidden))
            }

            // status bar
            IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Primary currency defined.")))
            else
            {
                err.addError(ERR_FAIL, i18nc("Error message", "Primary currency definition failed"));
            }

            // Display error
            SKGMainPanel::displayErrorMessage(err);
        }

        return SKGError();
    }
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgunitplugin_secondaryunit|"))) {
        if (iSolution == 1) {
            SKGMainPanel::getMainPanel()->openPage(QStringLiteral("skg://skrooge_unit_plugin"));
        } else {
            // Get parameters
            QString unit = iAdviceIdentifier.right(iAdviceIdentifier.length() - 28);

            SKGError err;
            {
                SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Define secondary currency"), err)
                SKGUnitObject unitObj(m_currentBankDocument);
                err = unitObj.setName(unit);
                IFOKDO(err, unitObj.load())
                IFOKDO(err, unitObj.setType(SKGUnitObject::SECONDARY))
                IFOKDO(err, unitObj.save())

                // Send message
                IFOKDO(
                    err,
                    unitObj.getDocument()->sendMessage(i18nc("An information to the user", "The unit '%1' is now the secondary unit", unitObj.getDisplayName()),
                                                       SKGDocument::Hidden))
            }

            // status bar
            IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Secondary currency defined.")))
            else
            {
                err.addError(ERR_FAIL, i18nc("Error message", "Secondary currency definition failed"));
            }

            // Display error
            SKGMainPanel::displayErrorMessage(err);
        }

        return SKGError();
    }
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgunitplugin_notdownloaded|"))) {
        if (iSolution == 0) {
            SKGMainPanel::getMainPanel()->openPage(QStringLiteral("skg://skrooge_unit_plugin"));
        } else {
            // Get parameters
            QString unit = iAdviceIdentifier.right(iAdviceIdentifier.length() - 28);

            SKGError err;
            SKGUnitObject unitObj(m_currentBankDocument);
            err = unitObj.setName(unit);
            IFOKDO(err, unitObj.load())
            IFOKDO(err, SKGUnitPluginWidget::downloadUnitValue(unitObj, SKGUnitPluginWidget::getDownloadModeFromSettings()))

            // Display error
            SKGMainPanel::displayErrorMessage(err);
        }

        return SKGError();
    }
    if ((m_currentBankDocument != nullptr)
        && (iAdviceIdentifier.startsWith(QStringLiteral("skgunitplugin_veryold|")) || iAdviceIdentifier.startsWith(QStringLiteral("skgunitplugin_toocomplex"))
            || iAdviceIdentifier.startsWith(QStringLiteral("skgunitplugin_amountnotdefined|"))
            || iAdviceIdentifier.startsWith(QStringLiteral("skgunitplugin_too_long_symbol|")))) {
        SKGMainPanel::getMainPanel()->openPage(QStringLiteral("skg://skrooge_unit_plugin"));
        return SKGError();
    }
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgunitplugin_decimalsymbol"))) {
        QProcess::execute(QStringLiteral("kcmshell5"), QStringList() << QStringLiteral("formats"));
        return SKGError();
    }
    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

void SKGUnitPlugin::deleteUnusedUnits() const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (m_currentBankDocument != nullptr) {
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Delete unused units"), err)

        // Modification of payee object
        QString sql = QStringLiteral(
            "DELETE FROM unit WHERE t_type NOT IN ('I', '1', '2') AND NOT EXISTS (SELECT 1 FROM operation WHERE operation.rc_unit_id=unit.id) AND NOT EXISTS "
            "(SELECT 1 FROM unit as unit2 WHERE unit2.rd_unit_id=unit.id)");
        err = m_currentBankDocument->executeSqliteOrder(sql);
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Unused units deleted")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Unused units deletion failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

#include <skgunitplugin.moc>
