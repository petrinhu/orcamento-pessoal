/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGImportExportManager.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportexportmanager.h"

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include <qcryptographichash.h>
#include <qfileinfo.h>
#include <qregularexpression.h>
#include <qtemporaryfile.h>

#include <qstandardpaths.h>

#include <algorithm>

#include "skgbankincludes.h"
#include "skgimportplugin.h"
#include "skgobjectbase.h"
#include "skgpayeeobject.h"
#include "skgrecurrentoperationobject.h"
#include "skgruleobject.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGImportExportManager::SKGImportExportManager(SKGDocumentBank *iDocument, const QUrl &iFileName)
    : m_document(iDocument)
    , m_fileName(iFileName)
    , m_defaultAccount(nullptr)
    , m_defaultUnit(nullptr)
    , m_importPlugin(nullptr)
    , m_exportPlugin(nullptr)
{
    SKGTRACEINFUNC(10)
    setAutomaticValidation(true);
    setAutomaticApplyRules(false);
    m_since_last_import = true;
    m_codec = QStringLiteral("UTF-8");
}

SKGImportExportManager::~SKGImportExportManager()
{
    SKGTRACEINFUNC(10)
    setDefaultAccount(nullptr);
    setDefaultUnit(nullptr);
    m_document = nullptr;
    m_importPlugin = nullptr;
    m_exportPlugin = nullptr;

    if (!m_localFileName.isEmpty() && m_localFileName != getFileName().toLocalFile()) {
        QFile(m_localFileName).remove();
    }
}

QString SKGImportExportManager::getFileNameExtension() const
{
    return QFileInfo(getFileName().path()).suffix().toUpper();
}

QUrl SKGImportExportManager::getFileName() const
{
    return m_fileName;
}

QString SKGImportExportManager::getLocalFileName(bool iDownload)
{
    if (m_localFileName.isEmpty()) {
        if (getFileName().isLocalFile()) {
            m_localFileName = getFileName().toLocalFile();
        } else {
            if (iDownload) {
                SKGServices::download(QUrl(getFileName()), m_localFileName);
            } else {
                QTemporaryFile tmpFile;
                tmpFile.setAutoRemove(false);
                if (tmpFile.open())
                    m_localFileName = tmpFile.fileName();
            }
        }
    }
    return m_localFileName;
}

SKGError SKGImportExportManager::setDefaultAccount(SKGAccountObject *iAccount)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    delete m_defaultAccount;
    m_defaultAccount = nullptr;
    if (iAccount != nullptr) {
        m_defaultAccount = new SKGAccountObject(*iAccount);
    }
    return err;
}

SKGError SKGImportExportManager::getDefaultAccount(SKGAccountObject &oAccount)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (m_defaultAccount == nullptr && (m_document != nullptr)) {
        QFileInfo fInfo(getLocalFileName());
        QStringList items = fInfo.completeBaseName().split('@');
        QString bankname;
        QString number;
        QString nameComplete;
        QString name = items.at(0);
        if (items.count() == 2) {
            bankname = items.at(1);

            QStringList items2 = name.split(u'-');
            if (items2.count() == 2) {
                name = items2.at(0);
                number = items2.at(1);
            } else {
                number = name;
            }

            name.replace('_', ' ');
            name.replace('-', ' ');
            nameComplete = name;
        } else {
            bankname = name;

            name.replace('_', ' ');
            name.replace('-', ' ');
            nameComplete = name;
            static QRegularExpression reg(QStringLiteral("(\\d{6,})"));
            auto match = reg.match(name);
            if (match.hasMatch()) {
                number = match.captured(1);
                if (name != number) {
                    name = name.remove(number).trimmed();
                }
            }
        }

        // Searching if an account exist
        QString whereClause = "t_name='" % SKGServices::stringToSqlString(name) % u'\'';
        whereClause += " OR t_number='" % SKGServices::stringToSqlString(name) % "'";
        whereClause += " OR t_agency_number||t_number='" % SKGServices::stringToSqlString(name) % "'";
        whereClause += " OR t_BANK_NUMBER||t_agency_number||t_number='" % SKGServices::stringToSqlString(name) % "'";
        whereClause += " OR (t_number!='' AND '" % SKGServices::stringToSqlString(name) % "' LIKE t_BANK_NUMBER||t_agency_number||t_number||'__')";
        if (!number.isEmpty()) {
            whereClause += " OR t_number='" % SKGServices::stringToSqlString(number) % "'";
            whereClause += " OR t_agency_number||t_number='" % SKGServices::stringToSqlString(number) % "'";
            whereClause += " OR t_BANK_NUMBER||t_agency_number||t_number='" % SKGServices::stringToSqlString(number) % "'";
            whereClause += " OR (t_number!='' AND '" % SKGServices::stringToSqlString(number) % "' LIKE t_BANK_NUMBER||t_agency_number||t_number||'__')";
        }
        whereClause += " OR EXISTS(SELECT 1 FROM parameters WHERE t_uuid_parent=v_account.id||'-account' AND t_name='alias' AND t_value= '"
            % SKGServices::stringToSqlString(name) % "')";

        const auto words = nameComplete.split(u' ');
        for (const auto &val : words) {
            whereClause += " OR t_number='" % SKGServices::stringToSqlString(val) % u'\'';
        }

        // Avoid to take account with another import ongoing
        whereClause = u'(' % whereClause % ") AND NOT EXISTS(SELECT 1 FROM operation WHERE operation.rd_account_id=v_account.id AND operation.t_imported='T')";

        SKGObjectBase::SKGListSKGObjectBase listAccount;
        err = m_document->getObjects(QStringLiteral("v_account"), whereClause % " ORDER BY t_type ASC", listAccount);
        IFOK(err)
        {
            if (!listAccount.isEmpty()) {
                // Yes ! Only one account found
                SKGAccountObject account(listAccount.at(0));
                m_defaultAccount = new SKGAccountObject(account);
                err = m_document->sendMessage(i18nc("An information message", "Using account '%1' for import", account.getName()));
            }
        }

        // If better account not found, then we must create one
        if (m_defaultAccount == nullptr) {
            SKGAccountObject account;
            SKGBankObject bank(m_document);
            IFOKDO(err, bank.setName(bankname))
            if (!err && bank.load().isFailed()) {
                err = bank.save(false); // Save only
            }
            IFOKDO(err, bank.addAccount(account))
            IFOKDO(err, account.setNumber(number))
            int index = 1;
            do {
                IFOKDO(err, account.setName(name % (index > 1 ? SKGServices::intToString(index) : QString())))
                IFOK(err)
                {
                    if (account.exist()) {
                        ++index;
                    } else {
                        err = account.save(false); // Save only
                        index = -1;
                    }
                }
            } while (!err && index > 0);
            IFOK(err)
            {
                m_defaultAccount = new SKGAccountObject(account);
            }
            IFOKDO(err, m_document->sendMessage(i18nc("An information message", "Default account '%1' created for import", name)))
        }
    }

    if (m_defaultAccount != nullptr) {
        oAccount = *m_defaultAccount;
    }

    return err;
}

SKGError SKGImportExportManager::setDefaultUnit(SKGUnitObject *iUnit)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    delete m_defaultUnit;
    m_defaultUnit = nullptr;
    if (iUnit != nullptr) {
        m_defaultUnit = new SKGUnitObject(*iUnit);
    }
    return err;
}

SKGError SKGImportExportManager::getDefaultUnit(SKGUnitObject &oUnit, const QDate *iDate)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if ((m_document != nullptr) && (m_defaultUnit == nullptr || (iDate != nullptr))) {
        if (m_defaultUnit != nullptr) {
            delete m_defaultUnit;
            m_defaultUnit = nullptr;
        }

        // Do we have to found the best unit for a date ?
        QString wc = QStringLiteral("t_type IN ('1', '2', 'C')");
        if (iDate != nullptr) {
            // Yes
            wc += " AND (d_MINDATE<'" % SKGServices::dateToSqlString(*iDate) % "' OR d_MINDATE IS NULL)";
        }

        // Check if a unit exist
        SKGObjectBase::SKGListSKGObjectBase listUnits;
        err = m_document->getObjects(QStringLiteral("v_unit"), wc % " ORDER BY ABS(f_CURRENTAMOUNT-1) ASC", listUnits);
        IFOK(err)
        {
            if (listUnits.isEmpty()) {
                SKGUnitObject unit(m_document);
                QString name = i18nc("Noun", "Unit for import");
                err = unit.setName(name);
                if (unit.load().isFailed()) {
                    IFOKDO(err, unit.setSymbol(name))
                    IFOKDO(err, unit.save(false))

                    SKGUnitValueObject unitval;
                    IFOKDO(err, unit.addUnitValue(unitval))
                    IFOKDO(err, unitval.setQuantity(1))
                    IFOKDO(err, unitval.setDate(QDate(1970, 1, 1)))
                    IFOKDO(err, unitval.save(false, false))

                    IFOKDO(err, m_document->sendMessage(i18nc("An information message", "Default unit '%1' created for import", name)))
                }

                IFOK(err) m_defaultUnit = new SKGUnitObject(unit);
            } else {
                // Found, we can use it
                m_defaultUnit = new SKGUnitObject(listUnits.at(0));
            }
        }
    }

    if (m_defaultUnit != nullptr) {
        oUnit = *m_defaultUnit;
    }

    return err;
}

void SKGImportExportManager::setAutomaticValidation(bool iValidation)
{
    m_automaticValidationOfImportedOperation = iValidation;
}

bool SKGImportExportManager::automaticValidation() const
{
    return m_automaticValidationOfImportedOperation;
}

void SKGImportExportManager::setAutomaticApplyRules(bool iApply)
{
    m_automaticApplyRulesOfImportedOperation = iApply;
}

bool SKGImportExportManager::automaticApplyRules() const
{
    return m_automaticApplyRulesOfImportedOperation;
}

void SKGImportExportManager::setSinceLastImportDate(bool iSinceLast)
{
    m_since_last_import = iSinceLast;
}

bool SKGImportExportManager::sinceLastImportDate() const
{
    return m_since_last_import;
}

void SKGImportExportManager::addAccountToCheck(const SKGAccountObject &iAccount, double iBalance)
{
    m_AccountToCheck.append(QPair<SKGAccountObject, double>(iAccount, iBalance));
}

QList<QPair<SKGAccountObject, double>> SKGImportExportManager::getAccountsToCheck()
{
    return m_AccountToCheck;
}

SKGError SKGImportExportManager::finalizeImportation()
{
    SKGError err;
    if (!getImportParameters().contains(QStringLiteral("donotfinalize"))) {
        SKGTRACEINFUNCRC(2, err)
        if (m_document != nullptr) {
            // Count the number of transactions imported but already existing
            QString wc =
                "v_operation.t_imported='T' AND exists (SELECT 1 from v_operation op2 WHERE op2.t_imported!='T' AND "
                "op2.rd_account_id=v_operation.rd_account_id AND op2.t_import_id=v_operation.t_import_id AND "
                "ABS(op2.f_CURRENTAMOUNT-v_operation.f_CURRENTAMOUNT)<"
                % SKGServices::doubleToString(EPSILON) % u')';
            SKGObjectBase::SKGListSKGObjectBase objects;
            err = m_document->getObjects(QStringLiteral("v_operation"), wc, objects);
            int nbOperations = objects.count();
            if (!err && (nbOperations != 0)) {
                err = m_document->sendMessage(
                    i18np("One transaction not imported because it already exists", "%1 transactions not imported because they already exist", nbOperations),
                    SKGDocument::Warning);

                SKGTRACEL(1) << "### LIST OF OPERATIONS DELETED BY IMPORT_ID ###" << Qt::endl;
                for (int i = 0; i < nbOperations; ++i) {
                    SKGTRACEL(1) << objects.at(i).getDisplayName() << " " << objects.at(i).getAttribute(QStringLiteral("t_import_id")) << Qt::endl;
                }

                // Remove transactions imported for nothing
                IFOKDO(err, m_document->executeSqliteOrder("DELETE from operation WHERE id IN (SELECT id from v_operation WHERE " % wc % QLatin1Char(')')))
            }

            // Delete transactions before the last import
            if (sinceLastImportDate() && !err) {
                SKGTRACEINRC(2, "SKGImportExportManager::finalizeImportation-since last date", err)

                // Count the number of transactions before the last import
                QString wc2 = QStringLiteral(
                    "operation.t_imported='T' AND "
                    "EXISTS(SELECT 1 FROM operation o INDEXED BY idx_operation_rd_account_id_t_imported WHERE o.rd_account_id=operation.rd_account_id AND "
                    "o.t_imported IN ('Y', 'P') AND date(o.d_date,'-4 day')>operation.d_date)");
                err = m_document->getObjects(QStringLiteral("operation"), wc2, objects);
                int nbOperations2 = objects.count();
                if (!err && (nbOperations2 != 0)) {
                    err = m_document->sendMessage(
                        i18np(
                            "One transaction was not imported because it was dated before the last imported one, you can uncheck the option to avoid this.",
                            "%1 transactions were not imported because they were dated before the last imported one, you can uncheck the option to avoid this.",
                            nbOperations2),
                        SKGDocument::Warning);

                    SKGTRACEL(1) << "### LIST OF OPERATIONS DELETED BECAUSE AFTER THE LAST ONE ###" << Qt::endl;
                    for (int i = 0; i < nbOperations2; ++i) {
                        SKGTRACEL(1) << objects.at(i).getDisplayName() << Qt::endl;
                    }

                    // Remove transactions imported for nothing
                    IFOKDO(err, m_document->executeSqliteOrder("DELETE from operation WHERE " % wc2))
                }
            }

            // For performances
            IFOKDO(err, m_document->executeSqliteOrder(QStringLiteral("ANALYZE")))

            // Apply rules
            if (!err && m_automaticApplyRulesOfImportedOperation) {
                SKGTRACEINRC(2, "SKGImportExportManager::finalizeImportation-apply rules", err)
                // Get rules
                SKGObjectBase::SKGListSKGObjectBase rules;
                IFOKDO(err, m_document->getObjects(QStringLiteral("v_rule"), QStringLiteral("1=1 ORDER BY f_sortorder"), rules))

                int nbRules = rules.count();
                IFOKDO(err, m_document->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Finalize import"), nbRules))
                for (int i = 0; !err && i < nbRules; ++i) {
                    SKGRuleObject rule(rules.at(i));
                    err = rule.execute(SKGRuleObject::IMPORTING);
                    IFOKDO(err, m_document->stepForward(i + 1))
                }
                SKGENDTRANSACTION(m_document, err)

                // A failure in rule will not block the transaction.
                // A warning message is sent
                IFKO(err)
                err = m_document->sendMessage(i18nc("Warning message", "Error during execution of rules:\n%1", err.getFullMessageWithHistorical()),
                                              SKGDocument::Error);
            }

            // Change imported status
            IFOK(err)
            {
                SKGTRACEINRC(2, "SKGImportExportManager::finalizeImportation-change imported status", err)
                int nbopimported = 0;
                err = m_document->getNbObjects(QStringLiteral("operation"), QStringLiteral("t_imported='T'"), nbopimported);
                IFOKDO(err, m_document->sendMessage(i18np("%1 transaction imported", "%1 transactions imported", nbopimported), SKGDocument::Positive))
                IFOKDO(err,
                       m_document->executeSqliteOrder(QStringLiteral("UPDATE operation SET t_imported='")
                                                      % (m_automaticValidationOfImportedOperation ? QStringLiteral("Y") : QStringLiteral("P"))
                                                      % "' WHERE t_imported='T'"));
            }

            // Check balances of accounts
            int nb = m_AccountToCheck.count();
            for (int i = 0; !err && i < nb; ++i) {
                // Get the account to check
                auto act = m_AccountToCheck.at(i).first;
                auto targetBalance = m_AccountToCheck.at(i).second;
                err = act.setAttribute(QStringLiteral("f_importbalance"), SKGServices::doubleToString(targetBalance));
                IFOKDO(err, act.setAttribute(QStringLiteral("d_importdate"), SKGServices::dateToSqlString(QDate::currentDate())))
                IFOKDO(err, act.save())

                auto soluces = act.getPossibleReconciliations(targetBalance, false);
                if (soluces.isEmpty()) {
                    IFOKDO(err,
                           m_document->sendMessage(
                               i18nc("Information message",
                                     "The balance of account '%1' is not aligned with import balance %2",
                                     act.getDisplayName(),
                                     m_document->formatMoney(targetBalance, m_document->getPrimaryUnit())),
                               SKGDocument::Warning,
                               QString("skg://skrooge_operation_plugin/?title_icon=quickopen&title="
                                       % SKGServices::encodeForUrl(
                                           i18nc("Noun, a list of items", "Transactions of account \"%1\" used for auto reconciliation", act.getDisplayName()))
                                       % "&operationWhereClause="
                                       % SKGServices::encodeForUrl("rd_account_id=" + SKGServices::intToString(act.getID())
                                                                   + " AND t_template='N' AND ((t_status='N' AND t_imported IN ('Y','P')) OR t_status='Y')"))));
                } else {
                    IFOKDO(err,
                           m_document->sendMessage(i18nc("Information message",
                                                         "The balance of account '%1' is aligned with import balance %2",
                                                         act.getDisplayName(),
                                                         m_document->formatMoney(targetBalance, m_document->getPrimaryUnit())),
                                                   SKGDocument::Positive))
                }
            }
        }
    }
    return err;
}

void SKGImportExportManager::setExportParameters(const QMap<QString, QString> &iParameters)
{
    SKGImportPlugin *plugin = getExportPlugin();
    if (plugin != nullptr) {
        plugin->setExportParameters(iParameters);
    }
}

QMap<QString, QString> SKGImportExportManager::getExportParameters()
{
    QMap<QString, QString> output;
    SKGImportPlugin *plugin = getExportPlugin();
    if (plugin != nullptr) {
        output = plugin->getExportParameters();
    }
    return output;
}

void SKGImportExportManager::setImportParameters(const QMap<QString, QString> &iParameters)
{
    SKGImportPlugin *plugin = getImportPlugin();
    if (plugin != nullptr) {
        plugin->setImportParameters(iParameters);
    }
}

void SKGImportExportManager::loadPlugins()
{
    if (m_plugins.isEmpty()) {
        const auto plugins = KPluginMetaData::findPlugins(QStringLiteral("skrooge_import"));
        int nb = plugins.count();
        for (int i = 0; i < nb; ++i) {
            const auto factory = KPluginFactory::loadFactory(plugins.at(i));
            const auto pluginInterface = factory.plugin ? factory.plugin->create<SKGImportPlugin>(this) : nullptr;
            if (pluginInterface != nullptr) {
                m_plugins.append(pluginInterface);
            }
        }
    }
}

QString SKGImportExportManager::getParameterDefaultValue(const QString &iParameter)
{
    QString output;

    loadPlugins();

    int nb = m_plugins.count();
    for (int i = 0; i < nb && output.isEmpty(); ++i) {
        auto pluginInterface = m_plugins.at(i);
        auto val = pluginInterface->getImportParameters().value(iParameter);
        if (!val.isEmpty()) {
            output = std::move(val);
        } else {
            output = pluginInterface->getExportParameters().value(iParameter);
        }
    }
    return output;
}

QMap<QString, QString> SKGImportExportManager::getImportParameters()
{
    QMap<QString, QString> output;
    SKGImportPlugin *plugin = getImportPlugin();
    if (plugin != nullptr) {
        output = plugin->getImportParameters();
    }
    return output;
}

QString SKGImportExportManager::getImportMimeTypeFilter(bool iIncludingAll)
{
    loadPlugins();

    QMap<QString, QString> tmp;
    int nb = m_plugins.count();
    for (int i = 0; i < nb; ++i) {
        auto pluginInterface = m_plugins.at(i);
        if (pluginInterface->isImportPossible()) {
            QString mime = pluginInterface->getMimeTypeFilter();
            if (!mime.isEmpty()) {
                QStringList lines = SKGServices::splitCSVLine(mime, '\n');
                int nblines = lines.count();
                for (int l = 0; l < nblines; ++l) {
                    QStringList items = SKGServices::splitCSVLine(lines.at(l), '|');
                    tmp[items.at(1)] = items.at(0);
                }
            }
        }
    }

    QStringList descriptions = tmp.keys();
    std::sort(descriptions.begin(), descriptions.end());
    QStringList regexps = tmp.values();

    QString output;
    if (iIncludingAll) {
        output = regexps.join(QStringLiteral(" ")) % u'|' % i18nc("A file format", "All supported formats");
    }
    nb = descriptions.count();
    for (int i = 0; i < nb; ++i) {
        if (!output.isEmpty()) {
            output += '\n';
        }
        output += tmp[descriptions.at(i)] % u'|' % descriptions.at(i);
    }

    return output;
}

QString SKGImportExportManager::getExportMimeTypeFilter(bool iIncludingAll)
{
    loadPlugins();

    QMap<QString, QString> tmp;
    int nb = m_plugins.count();
    for (int i = 0; i < nb; ++i) {
        auto pluginInterface = m_plugins.at(i);
        if (pluginInterface->isExportPossible()) {
            QString mime = pluginInterface->getMimeTypeFilter();
            if (!mime.isEmpty()) {
                QStringList lines = SKGServices::splitCSVLine(mime, '\n');
                int nblines = lines.count();
                for (int l = 0; l < nblines; ++l) {
                    QStringList items = SKGServices::splitCSVLine(lines.at(l), '|');
                    tmp[items.at(1)] = items.at(0);
                }
            }
        }
    }

    QStringList descriptions = tmp.keys();
    std::sort(descriptions.begin(), descriptions.end());
    QStringList regexps = tmp.values();

    QString output;
    if (iIncludingAll) {
        output = regexps.join(QStringLiteral(" ")) % u'|' % i18nc("A file format", "All supported formats");
    }
    nb = descriptions.count();
    for (int i = 0; i < nb; ++i) {
        if (!output.isEmpty()) {
            output += '\n';
        }
        output += tmp[descriptions.at(i)] % u'|' % descriptions.at(i);
    }

    return output;
}

SKGImportPlugin *SKGImportExportManager::getImportPlugin()
{
    if (m_importPlugin == nullptr) {
        loadPlugins();

        int nb = m_plugins.count();
        for (int i = 0; i < nb && m_importPlugin == nullptr; ++i) {
            auto pluginInterface = m_plugins.at(i);
            if (pluginInterface->isImportPossible()) {
                m_importPlugin = pluginInterface;
            }
        }
    }
    return m_importPlugin;
}

SKGImportPlugin *SKGImportExportManager::getExportPlugin()
{
    if (m_exportPlugin == nullptr) {
        loadPlugins();

        int nb = m_plugins.count();
        for (int i = 0; i < nb && m_exportPlugin == nullptr; ++i) {
            auto pluginInterface = m_plugins.at(i);
            if (pluginInterface->isExportPossible()) {
                m_exportPlugin = pluginInterface;
            }
        }
    }
    return m_exportPlugin;
}

SKGError SKGImportExportManager::importFile()
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)
    if (m_document != nullptr) {
        SKGBEGINPROGRESSTRANSACTION(*m_document, i18nc("Noun, name of the user action", "Import with codec %1", m_codec), err, 4)

        err = m_document->executeSqliteOrder(QStringLiteral("ANALYZE"));

        IFOKDO(err, m_document->stepForward(1))

        // Get list of banks
        SKGStringListList listBanks;
        IFOKDO(err, m_document->executeSelectSqliteOrder(QStringLiteral("SELECT group_concat(id) FROM bank"), listBanks));

        // Get list of accounts
        SKGStringListList listAccounts;
        IFOKDO(err, m_document->executeSelectSqliteOrder(QStringLiteral("SELECT group_concat(id) FROM account"), listAccounts));

        IFOK(err)
        {
            // Search plugins
            bool fileTreated = false;
            SKGImportPlugin *pluginInterface = getImportPlugin();
            if (pluginInterface != nullptr) {
                // Import
                fileTreated = true;
                m_AccountToCheck.clear();
                SKGTRACEL(2) << "Input filename=" << m_fileName.toDisplayString() << Qt::endl;
                IFOKDO(err, pluginInterface->importFile());
            }

            if (!err && !fileTreated) {
                err.setReturnCode(ERR_NOTIMPL).setMessage(i18nc("Error message", "The import mode %1 is not yet implemented", getFileNameExtension()));
            }
        }
        IFOKDO(err, m_document->stepForward(2))

        IFOKDO(err, finalizeImportation())
        IFOKDO(err, m_document->stepForward(3))

        // Delete empty new accounts
        if (listAccounts.length() == 2 && listAccounts.at(1).at(0).length() > 0) {
            IFOKDO(err,
                   m_document->executeSqliteOrder(QStringLiteral("DELETE FROM account where id in (select id from v_account_amount where id not in (")
                                                  % listAccounts.at(1).at(0) % QStringLiteral(") and f_CURRENTAMOUNT=0)")));
        }

        // Delete empty new banks
        if (listBanks.length() == 2 && listBanks.at(1).at(0).length() > 0) {
            IFOKDO(err,
                   m_document->executeSqliteOrder(QStringLiteral("DELETE FROM bank where id in (select id from v_bank_amount where id not in (")
                                                  % listBanks.at(1).at(0) % QStringLiteral(") and f_CURRENTAMOUNT=0)")));
        }

        IFOKDO(err, m_document->stepForward(4))
    }

    return err;
}

SKGError SKGImportExportManager::exportFile()
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)
    if (m_document != nullptr) {
        err = m_document->executeSqliteOrder(QStringLiteral("ANALYZE"));
        IFOK(err)
        {
            // Search plugins
            bool fileTreated = false;
            SKGImportPlugin *pluginInterface = getExportPlugin();
            if (pluginInterface != nullptr) {
                // Import
                fileTreated = true;
                SKGTRACEL(2) << "Input filename=" << m_fileName.toDisplayString() << Qt::endl;
                err = pluginInterface->exportFile();
                IFOKDO(err, SKGServices::upload(QUrl::fromLocalFile(getLocalFileName(false)), m_fileName))
            }

            if (!err && !fileTreated) {
                err.setReturnCode(ERR_NOTIMPL).setMessage(i18nc("Error message", "This export mode is not yet implemented"));
            }
        }
    }

    return err;
}

SKGError SKGImportExportManager::cleanBankImport()
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Begin transaction
    if (m_document != nullptr) {
        err = m_document->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Clean import"), 3);
        IFOK(err)
        {
            // Step 1 Clean transactions without mode and with comment with double space
            SKGObjectBase::SKGListSKGObjectBase transactions;
            IFOKDO(err,
                   m_document->getObjects(QStringLiteral("operation"), QStringLiteral("t_imported!='N' and t_mode='' and t_comment like '%  %'"), transactions))

            int nb = transactions.count();
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(transactions.at(i));

                // Comment is like this: <TYPE>  <INFO>
                // Example: RETRAIT DAB             20/01/08 11H44 013330 LCL GAILLAC 000497
                QString comment = op.getComment();
                static QRegularExpression reg(QStringLiteral("(.+) {2,}(.+)"));
                auto match = reg.match(comment);
                if (match.hasMatch()) {
                    // Get parameters
                    QString mode = match.captured(1);
                    QString info = match.captured(2);

                    // Modify
                    err = op.setComment(info.trimmed());
                    IFOKDO(err, op.setMode(mode.trimmed()))
                    IFOKDO(err, op.save(true, false)) // No reload
                }
            }

            // Step 1 done
            IFOKDO(err, m_document->stepForward(1))

            // Step 2 Clean transactions without mode and with comment
            IFOKDO(err, m_document->getObjects(QStringLiteral("operation"), QStringLiteral("t_imported!='N' and t_mode='' and t_comment!=''"), transactions))

            nb = transactions.count();
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(transactions.at(i));

                // Comment is like this: <TYPE> <INFO>
                // Example: RETRAIT DAB 14-05-16607-482390
                QString comment = op.getComment();
                static QRegularExpression reg(QStringLiteral("(\\S+) +(.+)"));
                auto match = reg.match(comment);
                if (match.hasMatch()) {
                    // Get parameters
                    QString mode = match.captured(1);
                    QString info = match.captured(2);

                    // Modify
                    err = op.setComment(info.trimmed());
                    IFOKDO(err, op.setMode(mode.trimmed()))
                    IFOKDO(err, op.save(true, false)) // No reload
                }
            }

            // Step 2 done
            IFOKDO(err, m_document->stepForward(2))

            // Step 3 Clean cheque without number
            IFOKDO(
                err,
                m_document->getObjects(QStringLiteral("operation"), QStringLiteral("t_imported!='N' and t_number='' and lower(t_mode)='cheque'"), transactions))

            nb = transactions.count();
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(transactions.at(i));

                // Comment is like this: <TYPE>  <INFO>
                // Example: RETRAIT DAB             20/01/08 11H44 013330 LCL GAILLAC 000497
                QString comment = op.getComment();
                static QRegularExpression reg(QStringLiteral("(\\d+)"));
                auto match = reg.match(comment);
                if (match.hasMatch()) {
                    // Get parameters
                    auto number = match.captured(1);

                    // Modify
                    err = op.setNumber(number);
                    IFOKDO(err, op.save(true, false)) // No reload
                }
            }

            // Step 3 done
            IFOKDO(err, m_document->stepForward(3))
        }

        SKGENDTRANSACTION(m_document, err)
    }

    return err;
}

SKGError SKGImportExportManager::anonymize(const QString &iKey)
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)
    if (m_document != nullptr) {
        if (m_document->isFileModified()) {
            err = SKGError(ERR_ABORT, i18nc("An information message", "The document must be saved to be anonymized."), QStringLiteral("skg://file_save"));
        } else {
            {
                // Remove password
                m_document->changePassword(QString());

                // Anonymize data
                QStringList sqlOrders;
                if (iKey.isEmpty()) {
                    // Not reversible
                    sqlOrders << QStringLiteral("UPDATE bank SET t_bank_number='', t_name='bank_'||id")
                              << QStringLiteral("UPDATE account SET t_number='', t_agency_number='', t_agency_address='', t_comment='', t_name='account_'||id")
                              << QStringLiteral("UPDATE category SET t_name='category_'||id")
                              << QStringLiteral("UPDATE payee SET t_address='', t_name='payee_'||id")
                              << QStringLiteral("UPDATE refund SET t_comment='', t_name='tracker_'||id") << QStringLiteral("UPDATE operation SET t_comment=''")
                              << QStringLiteral("UPDATE suboperation SET t_comment='', f_value=f_value%1234.56")
                              << QStringLiteral("DELETE FROM parameters WHERE t_name NOT LIKE 'SKG_%' OR t_name='SKG_PASSWORD'");
                } else {
                    // Reversible
                    sqlOrders << QStringLiteral("UPDATE bank SET t_bank_number=XOR(t_bank_number,'") + SKGServices::stringToSqlString(iKey)
                            + "'), t_name=XOR(t_name,'" + SKGServices::stringToSqlString(iKey) + "')"
                              << QStringLiteral("UPDATE account SET t_number=XOR(t_number,'") + SKGServices::stringToSqlString(iKey)
                            + "'), t_agency_number=XOR(t_agency_number,'" + SKGServices::stringToSqlString(iKey) + "'), t_agency_address=XOR(t_agency_address,'"
                            + SKGServices::stringToSqlString(iKey) + "'), t_comment=XOR(t_comment,'" + SKGServices::stringToSqlString(iKey)
                            + "'), t_name=XOR(t_name,'" + SKGServices::stringToSqlString(iKey) + "')"
                              << QStringLiteral("UPDATE category SET t_name=XOR(t_name,'") + SKGServices::stringToSqlString(iKey) + "')"
                              << QStringLiteral("UPDATE payee SET t_address=XOR(t_address,'") + SKGServices::stringToSqlString(iKey) + "'), t_name=XOR(t_name,'"
                            + SKGServices::stringToSqlString(iKey) + "')"
                              << QStringLiteral("UPDATE refund SET t_comment=XOR(t_comment,'") + SKGServices::stringToSqlString(iKey)
                            + "'), t_name=XOR(t_name,'" + SKGServices::stringToSqlString(iKey) + "')"
                              << QStringLiteral("UPDATE operation SET t_comment=XOR(t_comment,'") + SKGServices::stringToSqlString(iKey) + "')"
                              << QStringLiteral("UPDATE suboperation SET t_comment=XOR(t_comment,'") + SKGServices::stringToSqlString(iKey)
                            + "'), f_value=XORD(f_value,'" + SKGServices::stringToSqlString(iKey) + "')"
                              << QStringLiteral("UPDATE parameters SET t_name=XOR(t_name,'") + SKGServices::stringToSqlString(iKey)
                            + "'), t_value=XOR(t_value,'" + SKGServices::stringToSqlString(iKey) + "'), b_blob=XOR(b_blob,'"
                            + SKGServices::stringToSqlString(iKey) + "') WHERE t_name NOT LIKE 'SKG_%'"
                              << QStringLiteral("DELETE FROM parameters WHERE t_name='SKG_PASSWORD'");
                }

                int nb = sqlOrders.count();
                SKGBEGINPROGRESSTRANSACTION(*m_document, "##INTERNAL##" % i18nc("Progression step", "Anonymize"), err, nb)
                for (int i = 0; !err && i < nb; ++i) {
                    err = m_document->executeSqliteOrder(sqlOrders.at(i));
                    IFOKDO(err, m_document->stepForward(i + 1))
                }

                if (iKey.isEmpty()) {
                    m_document->sendMessage(i18nc("An information message", "The document has been made anonymous with an irreversible mode."),
                                            SKGDocument::Positive);
                } else {
                    m_document->sendMessage(
                        i18nc("An information message",
                              "The document has been made anonymous with a reversible mode. Don't forget the key if you want to reverse it."),
                        SKGDocument::Information);
                }
            }

            // Remove transactions
            IFOKDO(err, m_document->removeAllTransactions())

            // Save new file
            QString newName = m_document->getCurrentFileName().replace(QStringLiteral(".skg"), QStringLiteral("-anonymized.skg"));
            if (newName == m_document->getCurrentFileName()) {
                newName = m_document->getCurrentFileName() % "-anonymized.skg";
            }
            IFOKDO(err, m_document->saveAs(newName, true))
        }
    }
    return err;
}

SKGError SKGImportExportManager::findAndGroupTransfers(int &oNbOperationsMerged, const QString &iAdditionnalCondition)
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    oNbOperationsMerged = 0;

    // Begin transaction
    if (m_document != nullptr) {
        err = m_document->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Find and group transfers"), 4);
        IFOK(err)
        {
            IFOKDO(err, m_document->executeSqliteOrder(QStringLiteral("ANALYZE")))
            // Look for transactions with
            //  Same units
            //  Same dates
            //  Null i_group_id
            //  Different accounts
            //  Opposite amounts
            SKGStringListList listTmp;
            {
                SKGTRACEIN(2, "SKGImportExportManager::findAndGroupTransfers-step 1")
                IFOKDO(
                    err,
                    m_document->executeSelectSqliteOrder("SELECT ID1, ID2 FROM (SELECT A.id as ID1, B.id as ID2, (SELECT TOTAL(s.f_value) FROM suboperation s "
                                                         "WHERE s.rd_operation_id=A.ID) AS quantity1, (SELECT TOTAL(s.f_value) FROM suboperation s "
                                                         "WHERE s.rd_operation_id=B.ID) AS quantity2 FROM operation  A, operation B "
                                                         "WHERE +A.d_date=B.d_date AND A.rc_unit_id=B.rc_unit_id AND A.id<=B.id AND "
                                                         "A.rd_account_id!=B.rd_account_id AND A.i_group_id=0 AND B.i_group_id=0"
                                                             % (iAdditionnalCondition.isEmpty() ? QString() : QStringLiteral(" AND ") % iAdditionnalCondition)
                                                             % ") "
                                                               "WHERE ABS(quantity1+quantity2)<1e-5 AND quantity1!=0",
                                                         listTmp));
                // Step 1 done
                IFOKDO(err, m_document->stepForward(1))
            }

            SKGStringListList listTmp2;
            {
                SKGTRACEIN(2, "SKGImportExportManager::findAndGroupTransfers-step 2")
                // +A.i_group_id=0 AND +B.i_group_id=0 is for avoiding to use bad index
                IFOKDO(err,
                       m_document->executeSelectSqliteOrder(
                           "SELECT A.id, B.id FROM v_operation A, operation B, parameters P "
                           "WHERE +P.t_name='SKG_OP_ORIGINAL_AMOUNT' AND +P.t_uuid_parent=B.id||'-operation' AND A.rc_unit_id!=B.rc_unit_id AND "
                           "A.d_date=B.d_date AND A.rd_account_id!=B.rd_account_id AND ABS(A.f_CURRENTAMOUNT+CAST(P.t_value AS REAL))<"
                               % SKGServices::doubleToString(EPSILON) % " AND +A.i_group_id=0 AND +B.i_group_id=0 AND A.f_CURRENTAMOUNT!=0"
                               % (iAdditionnalCondition.isEmpty() ? QString() : QStringLiteral(" AND ") % iAdditionnalCondition),
                           listTmp2));

                // Step 2 done
                IFOKDO(err, m_document->stepForward(2))
                listTmp2.removeAt(0); // Remove header
                listTmp += listTmp2;
            }

            {
                SKGTRACEIN(2, "SKGImportExportManager::findAndGroupTransfers-step 3")
                // +A.i_group_id=0 AND +B.i_group_id=0 is for avoiding to use bad index
                IFOKDO(
                    err,
                    m_document->executeSelectSqliteOrder("SELECT A.id, B.id FROM v_operation A, operation B "
                                                         "WHERE A.d_date=B.d_date AND A.rd_account_id!=B.rd_account_id AND A.rc_unit_id!=B.rc_unit_id AND "
                                                         "A.t_comment!='' AND A.t_comment=B.t_comment AND "
                                                         "+A.i_group_id=0 AND +B.i_group_id=0 AND A.f_CURRENTAMOUNT!=0"
                                                             % (iAdditionnalCondition.isEmpty() ? QString() : QStringLiteral(" AND ") % iAdditionnalCondition),
                                                         listTmp2));

                // Step 3 done
                IFOKDO(err, m_document->stepForward(3))
                listTmp2.removeAt(0); // Remove header
                listTmp += listTmp2;
            }

            // Group
            {
                SKGTRACEIN(2, "SKGImportExportManager::findAndGroupTransfers-group")
                oNbOperationsMerged = listTmp.count();
                IFOKDO(err, m_document->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Find and group transfers"), oNbOperationsMerged - 1))
                for (int i = 1; !err && i < oNbOperationsMerged; ++i) { // First line ignored because of its header
                    SKGOperationObject op1(m_document, SKGServices::stringToInt(listTmp.at(i).at(0)));
                    SKGOperationObject op2(m_document, SKGServices::stringToInt(listTmp.at(i).at(1)));

                    if (!op1.isInGroup() && !op2.isInGroup()) {
                        err = op2.setGroupOperation(op1);
                        IFOKDO(err, op2.save(true, false))
                    }
                    IFOKDO(err, m_document->stepForward(i))
                }
                SKGENDTRANSACTION(m_document, err)
            }
            oNbOperationsMerged = (oNbOperationsMerged - 1) * 2;

            // Step 3 done
            IFOKDO(err, m_document->stepForward(4))
        }
        SKGENDTRANSACTION(m_document, err)
    }

    return err;
}

SKGError SKGImportExportManager::findAndGroupTransfers(int &oNbOperationsMerged, bool iOnCurrentlyImport)
{
    return findAndGroupTransfers(oNbOperationsMerged, iOnCurrentlyImport ? QStringLiteral("A.t_imported='T' AND B.t_imported='T'") : QString());
}

SKGError SKGImportExportManager::importItems(QDomElement &docElem)
{
    SKGError err;
    if (docElem.isNull()) {
        return err;
    }
    const auto type = docElem.tagName().remove(QStringLiteral("list_"));
    QDomNodeList items = docElem.elementsByTagName(type);
    int nb = items.count();
    if (nb > 0) {
        SKGTRACEINRC(10, "SKGImportExportManager::importItems-" + type, err)
        err = getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1", type), nb);
        for (int j = 0; !err && j < nb; ++j) {
            QDomNode node = items.at(j);
            QDomElement item = node.toElement();
            SKGObjectBase *obj;
            if (type == "category") {
                obj = new SKGCategoryObject(getDocument());
            } else if (type == "unit") {
                obj = new SKGUnitObject(getDocument());
            } else if (type == "unitvalue") {
                obj = new SKGUnitValueObject(getDocument());
            } else if (type == "refund") {
                obj = new SKGTrackerObject(getDocument());
            } else if (type == "bank") {
                obj = new SKGBankObject(getDocument());
            } else if (type == "rule") {
                obj = new SKGRuleObject(getDocument());
            } else if (type == "account") {
                obj = new SKGAccountObject(getDocument());
            } else if (type == "payee") {
                obj = new SKGPayeeObject(getDocument());
            } else if (type == "operation") {
                obj = new SKGOperationObject(getDocument());
            } else if (type == "suboperation") {
                obj = new SKGSubOperationObject(getDocument());
            } else if (type == "budget") {
                obj = new SKGBudgetObject(getDocument());
            } else if (type == "budgetrule") {
                obj = new SKGBudgetRuleObject(getDocument());
            } else if (type == "interest") {
                obj = new SKGInterestObject(getDocument());
            } else if (type == "recurrentoperation") {
                obj = new SKGRecurrentOperationObject(getDocument());
            } else if (type == "rule") {
                obj = new SKGRuleObject(getDocument());
            } else {
                obj = new SKGObjectBase(getDocument(), type);
            }

            auto attributes = item.attributes();
            int nb2 = attributes.count();
            for (int i = 0; !err && i < nb2; ++i) {
                auto att = attributes.item(i);

                auto name = att.nodeName();
                if (name != "id" && name != "r_recurrentoperation_id") {
                    auto val = att.nodeValue();
                    if (name.startsWith(QStringLiteral("rd_")) || name.startsWith(QStringLiteral("rc_")) || name.startsWith(QStringLiteral("r_"))) {
                        auto tmp = name.split(QStringLiteral("_"));
                        if (m_mapIdObject.contains(tmp[1] + "-" + val)) {
                            val = SKGServices::intToString(m_mapIdObject[tmp[1] + "-" + val].getID());
                        } else {
                            val = QString();
                        }
                    }
                    IFOKDO(err, obj->setAttribute(name, val))
                }
            }
            if (type == "operation") {
                QString str;
                QTextStream stream(&str);
                node.save(stream, 0);
                QByteArray hash = QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5);
                IFOKDO(err, static_cast<SKGOperationObject *>(obj)->setImportID("XML-" + QString(hash.toHex())))
            }
            if (obj->exist()) {
                IFOKDO(err, obj->load())
            } else {
                IFOKDO(err, obj->save(true, true))
            }
            m_mapIdObject[type + "-" + item.attribute(QStringLiteral("id"))] = *obj;

            delete obj;

            IFOKDO(err, getDocument()->stepForward(j + 1))
        }
        SKGENDTRANSACTION(getDocument(), err)
    }
    return err;
}
