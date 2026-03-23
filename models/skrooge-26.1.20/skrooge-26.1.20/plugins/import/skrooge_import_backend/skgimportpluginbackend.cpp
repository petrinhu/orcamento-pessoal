/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for BACKEND import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginbackend.h"

#include <qapplication.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qprocess.h>
#include <qregularexpression.h>
#include <qstandardpaths.h>
#include <qtconcurrentmap.h>
#include <qvariant.h>

#include <kaboutdata.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginBackend, "metadata.json")

SKGImportPluginBackend::SKGImportPluginBackend(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)

    m_listBackends = SKGServices::findDataPlugins(QStringLiteral("skrooge_import_backend"));
    SKGTRACEL(10) << m_listBackends.count() << " plugins found" << Qt::endl;
}

SKGImportPluginBackend::~SKGImportPluginBackend() = default;

const KPluginMetaData *SKGImportPluginBackend::getService() const
{
    for (const auto &service : m_listBackends) {
        const auto &id = service.pluginId().remove(QStringLiteral("org.kde.skrooge-import-backend-")).toUpper();
        if (id == m_importer->getFileNameExtension()) {
            return &service;
        }
    }
    return nullptr;
}

QString SKGImportPluginBackend::getParameter(const QString &iAttribute)
{
    auto service = getService();
    auto output = service ? service->value(iAttribute, QString()) : QString();
    QMap<QString, QString> parameters = this->getImportParameters();

    for (int i = 1; i <= 10; ++i) {
        QString param = "parameter" + SKGServices::intToString(i);
        if (output.contains(QStringLiteral("%") % param)) {
            output = output.replace(QStringLiteral("%") % param, parameters.value(param));
        }
    }

    return output;
}

bool SKGImportPluginBackend::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : getService() != nullptr);
}

struct download {
    download(int iNbToDownload, QString iDate, QString iCmd, QString iPwd, QString iPath)
        : m_nbToDownload(iNbToDownload)
        , m_date(std::move(iDate))
        , m_cmd(std::move(iCmd))
        , m_pwd(std::move(iPwd))
        , m_path(std::move(iPath))
    {
    }

    using result_type = QString;

    QString operator()(const QString &iAccountId)
    {
        QString file = m_path % "/" % iAccountId % ".csv";
        int retry = 0;
        int nb_retry = 5;
        QString cmd;
        QProcess p;
        do {
            // Build cmd
            cmd = m_cmd;
            cmd = cmd.replace(QStringLiteral("%2"), SKGServices::intToString(retry == nb_retry ? 1000 : m_nbToDownload * (nb_retry - retry) / nb_retry))
                      .replace(QStringLiteral("%1"), iAccountId)
                      .replace(QStringLiteral("%3"), m_pwd)
                      .replace(QStringLiteral("%4"), m_date);

            // Execute
            if (cmd.contains(QStringLiteral("%4"))) {
                cmd = cmd.replace(QStringLiteral("%4"), file);
                p.setProcessChannelMode(QProcess::ForwardedChannels);
            } else {
                p.setStandardOutputFile(file);
            }
            cmd = SKGServices::getFullPathCommandLine(cmd);
            SKGTRACEL(1) << "Execute: " << cmd << Qt::endl;

            p.start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << cmd);
            if (p.waitForFinished(1000 * 60 * 2)) {
                if (p.exitCode() == 0) {
                    return iAccountId;
                }
                SKGTRACE << i18nc("A warning message", "WARNING: The command %1 failed with code %2 (Retry %3)", cmd, p.exitCode(), retry + 1) << Qt::endl;

            } else {
                SKGTRACE << i18nc("A warning message", "WARNING: The command %1 failed due to a time out (Retry %2)", cmd, retry + 1) << Qt::endl;
                p.terminate();
                p.kill();
            }
            ++retry;
        } while (retry <= nb_retry);

        QString errorMsg = i18nc("Error message", "The following command line failed with code %2:\n'%1'", cmd, p.exitCode());
        SKGTRACE << errorMsg << Qt::endl;

        return QStringLiteral("ERROR:") + errorMsg;
    }

    int m_nbToDownload;
    QString m_date;
    QString m_cmd;
    QString m_pwd;
    QString m_path;
};

SKGError SKGImportPluginBackend::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    SKGBEGINPROGRESSTRANSACTION(*m_importer->getDocument(), i18nc("Noun, name of the user action", "Import with %1", "Backend"), err, 3)
    QString bankendName = m_importer->getFileNameExtension().toLower();

    // Get parameters
    QMap<QString, QString> parameters = this->getImportParameters();
    QString pwd = parameters[QStringLiteral("password")];

    // Get list of accounts
    QStringList backendAccounts;
    QMap<QString, QString> backendAccountsBalance;
    QMap<QString, QString> backendAccountsName;
    QString csvfile = m_tempDir.path() % "/skrooge_backend.csv";
    QString cmd = getParameter(QStringLiteral("X-SKROOGE-getaccounts")).replace(QStringLiteral("%3"), pwd);
    QProcess p;
    if (cmd.contains(QStringLiteral("%4"))) {
        cmd = cmd.replace(QStringLiteral("%4"), csvfile);
        p.setProcessChannelMode(QProcess::ForwardedChannels);
    } else {
        p.setStandardOutputFile(csvfile);
    }
    cmd = SKGServices::getFullPathCommandLine(cmd);
    SKGTRACEL(1) << "Execute: " << cmd << Qt::endl;
    p.start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << cmd);
    if (!p.waitForFinished(1000 * 60 * 2) || p.exitCode() != 0) {
        err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "The following command line failed with code %2:\n'%1'", cmd, p.exitCode()));
    } else {
        QFile file(csvfile);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", csvfile));
        } else {
            QRegularExpression reggetaccounts(getParameter(QStringLiteral("X-SKROOGE-getaccountid")));
            QRegularExpression reggetaccountbalance(getParameter(QStringLiteral("X-SKROOGE-getaccountbalance")));
            QRegularExpression reggetaccountname(getParameter(QStringLiteral("X-SKROOGE-getaccountname")));

            QTextStream stream(&file);
            stream.readLine(); // To avoid header
            QStringList backendAccountsUniqueId;
            while (!stream.atEnd()) {
                // Read line
                QString line = stream.readLine().trimmed();
                SKGTRACEL(10) << "Read line: " << line << Qt::endl;

                // Get account id
                auto match = reggetaccounts.match(line);
                if (match.hasMatch()) {
                    QString accountid = match.captured(1);
                    QString uniqueid = SKGServices::splitCSVLine(accountid, u'@').at(0);

                    if (!backendAccounts.contains(accountid) && !backendAccountsUniqueId.contains(uniqueid)) {
                        backendAccounts.push_back(accountid);
                        backendAccountsUniqueId.push_back(uniqueid);

                        // Get account balance
                        match = reggetaccountbalance.match(line);
                        if (match.hasMatch()) {
                            backendAccountsBalance[accountid] = match.captured(1);
                        } else {
                            backendAccountsBalance[accountid] = '0';
                        }

                        // Get account name
                        match = reggetaccountname.match(line);
                        if (match.hasMatch()) {
                            backendAccountsName[accountid] = match.captured(1);
                        } else {
                            backendAccountsName[accountid] = QString();
                        }
                    }
                } else {
                    // This is an error
                    err.setReturnCode(ERR_FAIL).setMessage(line).addError(
                        ERR_FAIL,
                        i18nc("Error message",
                              "Impossible to find the account id with the regular expression '%1' in line '%2'",
                              getParameter(QStringLiteral("X-SKROOGE-getaccountid")),
                              line));
                    break;
                }
            }

            // close file
            file.close();
            file.remove();
        }
    }

    // Download transactions
    IFOKDO(err, m_importer->getDocument()->stepForward(1, i18nc("Progress message", "Download transactions")))
    IFOK(err)
    {
        if (backendAccounts.isEmpty()) {
            err.setReturnCode(ERR_FAIL).setMessage(
                i18nc("Error message", "Your backend '%1' seems to be not well configure because no account has been found.", bankendName));
        } else {
            // Compute the begin date for download
            QDate lastDownload = SKGServices::stringToDate(m_importer->getDocument()->getParameter("SKG_LAST_" % bankendName.toUpper() % "_IMPORT_DATE"));
            QString lastList = m_importer->getDocument()->getParameter("SKG_LAST_" % bankendName.toUpper() % "_IMPORT_LIST");
            QString currentList = backendAccounts.join(QStringLiteral(";"));

            int nbToDownload = 0;
            QString fromDate;
            if (currentList != lastList || !lastDownload.isValid()) {
                nbToDownload = 99999;
                fromDate = QStringLiteral("2000-01-01");
            } else {
                nbToDownload = qMax(lastDownload.daysTo(QDate::currentDate()) * 10, qint64(20));
                fromDate = SKGServices::dateToSqlString(lastDownload.addDays(-4));
            }

            // Download
            QStringList listDownloadedId;
            QString bulk = getParameter(QStringLiteral("X-SKROOGE-getbulk"));
            QString cmddownload;
            if (!bulk.isEmpty()) {
                // mode bulk
                SKGTRACEL(10) << "Mode getbulk" << Qt::endl;
                QProcess pbulk;
                QString cmd = bulk.replace(QStringLiteral("%1"), m_tempDir.path());
                cmd = SKGServices::getFullPathCommandLine(cmd);
                cmddownload = cmd;
                SKGTRACEL(1) << "Execute: " << cmd << Qt::endl;
                pbulk.start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << cmd);
                if (!pbulk.waitForFinished(1000 * 60 * 2) || pbulk.exitCode() != 0) {
                    err.setReturnCode(ERR_FAIL).setMessage(
                        i18nc("Error message", "The following command line failed with code %2:\n'%1'", cmd, pbulk.exitCode()));
                } else {
                    SKGTRACEL(10) << "Searching csv files " << Qt::endl;
                    QDirIterator it(m_tempDir.path(), QStringList() << QStringLiteral("*.csv"));
                    while (it.hasNext()) {
                        auto id = QFileInfo(it.next()).baseName();
                        listDownloadedId.push_back(id);

                        SKGTRACEL(10) << "Find id: " << id << Qt::endl;
                    }
                }
            } else {
                // mode getoperations
                SKGTRACEL(10) << "Mode getoperations" << Qt::endl;
                cmddownload = getParameter(QStringLiteral("X-SKROOGE-getoperations"));
                QFuture<QString> f = QtConcurrent::mapped(backendAccounts, download(nbToDownload, fromDate, cmddownload, pwd, m_tempDir.path()));
                f.waitForFinished();
                listDownloadedId = f.results();
            }
            listDownloadedId.removeAll(QString());
            // Build list of errors
            QStringList errors;
            int nb = listDownloadedId.count();
            errors.reserve(nb);
            for (int i = nb - 1; i >= 0; --i) {
                auto item = listDownloadedId.value(i);
                if (item.startsWith(QStringLiteral("ERROR:"))) {
                    listDownloadedId.removeAt(i);
                    errors.push_back(item.right(item.length() - 6));
                }
            }

            // Check
            IFOK(err)
            {
                bool checkOK = true;
                int nb = listDownloadedId.count();
                if (errors.count() != 0) {
                    // Some accounts have not been downloaded
                    if (nb == 0) {
                        err = SKGError(
                            ERR_FAIL,
                            i18nc("Error message", "No accounts downloaded with the following command:\n%1\nCheck your backend installation.", cmddownload));
                    } else {
                        // Warning
                        m_importer->getDocument()->sendMessage(
                            i18nc("Warning message", "Some accounts have not been downloaded. %1", errors.join(QStringLiteral(". "))),
                            SKGDocument::Warning);
                    }
                    SKGTRACEL(10) << errors.count() << " accounts not imported => checkOK=false" << Qt::endl;
                    checkOK = false;
                }

                // import
                IFOKDO(err, m_importer->getDocument()->stepForward(2, i18nc("Progress message", "Import")))
                if (!err && (nb != 0)) {
                    // import
                    SKGBEGINPROGRESSTRANSACTION(*m_importer->getDocument(),
                                                "#INTERNAL#" % i18nc("Noun, name of the user action", "Import one account with %1", "Backend"),
                                                err,
                                                nb)

                    // Get all messages
                    SKGDocument::SKGMessageList messages;
                    IFOKDO(err, m_importer->getDocument()->getMessages(m_importer->getDocument()->getCurrentTransaction(), messages, true))

                    auto extension = getParameter(QStringLiteral("X-SKROOGE-extension")).toLower();
                    if (extension.length() == 0) {
                        extension = QStringLiteral("csv");
                    }

                    // Import all files
                    for (int i = 0; !err && i < nb; ++i) {
                        // Rename the imported name
                        QString file = m_tempDir.path() % "/" % listDownloadedId.at(i) % ".csv";
                        if (!listDownloadedId.at(i).contains(QStringLiteral("-")) && !backendAccountsName[listDownloadedId.at(i)].isEmpty()) {
                            QString newFileName =
                                m_tempDir.path() % "/" % backendAccountsName[listDownloadedId.at(i)] % u'-' % listDownloadedId.at(i) % "." % extension;
                            if (QFile::rename(file, newFileName)) {
                                file = std::move(newFileName);
                            }
                        }

                        // Import
                        SKGImportExportManager imp1(m_importer->getDocument(), QUrl::fromLocalFile(file));
                        imp1.setAutomaticValidation(m_importer->automaticValidation());
                        imp1.setAutomaticApplyRules(m_importer->automaticApplyRules());
                        // This option is not used with backend import
                        imp1.setSinceLastImportDate(false);
                        imp1.setCodec(m_importer->getCodec());

                        if (extension == "csv") {
                            QMap<QString, QString> newParameters = imp1.getImportParameters();
                            newParameters[QStringLiteral("automatic_search_header")] = 'N';
                            newParameters[QStringLiteral("header_position")] = '1';
                            newParameters[QStringLiteral("automatic_search_columns")] = 'N';
                            newParameters[QStringLiteral("columns_positions")] = getParameter(QStringLiteral("X-SKROOGE-csvcolumns"));
                            newParameters[QStringLiteral("mode_csv_unit")] = 'N';
                            newParameters[QStringLiteral("mode_csv_rule")] = 'N';
                            newParameters[QStringLiteral("balance")] = backendAccountsBalance[listDownloadedId.at(i)];
                            newParameters[QStringLiteral("donotfinalize")] = 'Y';
                            imp1.setImportParameters(newParameters);
                        }
                        IFOKDO(err, imp1.importFile())

                        if (!backendAccountsBalance[listDownloadedId.at(i)].isEmpty()) {
                            SKGAccountObject act;
                            IFOKDO(err, imp1.getDefaultAccount(act))
                            m_importer->addAccountToCheck(act, SKGServices::stringToDouble(backendAccountsBalance[listDownloadedId.at(i)]));
                        }
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    // Remove all temporary files
                    for (int i = 0; i < nb; ++i) {
                        QString file = m_tempDir.path() % "/" % listDownloadedId.at(i) % ".csv";
                        QFile::remove(file);
                    }

                    // Reset message
                    IFOKDO(err, m_importer->getDocument()->removeMessages(m_importer->getDocument()->getCurrentTransaction()))
                    int nbm = messages.count();
                    for (int j = 0; j < nbm; ++j) {
                        SKGDocument::SKGMessage msg = messages.at(j);
                        m_importer->getDocument()->sendMessage(msg.Text, msg.Type, msg.Action);
                    }

                    // Finalize import
                    IFOKDO(err, m_importer->finalizeImportation())

                    // Disable std finalisation
                    QMap<QString, QString> parameters = m_importer->getImportParameters();
                    parameters[QStringLiteral("donotfinalize")] = 'Y';
                    m_importer->setImportParameters(parameters);

                    // Check balances of accounts
                    auto accountsToCheck = m_importer->getAccountsToCheck();
                    int nb = accountsToCheck.count();
                    for (int i = 0; !err && i < nb; ++i) {
                        // Get the account to check
                        auto act = accountsToCheck[i].first;
                        auto targetBalance = accountsToCheck[i].second;
                        auto soluces = act.getPossibleReconciliations(targetBalance, false);
                        if (soluces.isEmpty()) {
                            SKGTRACEL(10) << "Account " << listDownloadedId.at(i) << " not reconciliable => checkOK=false" << Qt::endl;
                            checkOK = false;
                        }
                    }

                    if (checkOK) {
                        // Last import is memorized only in case of 100% success
                        IFOKDO(err,
                               m_importer->getDocument()->setParameter("SKG_LAST_" % bankendName.toUpper() % "_IMPORT_DATE",
                                                                       SKGServices::dateToSqlString(QDateTime::currentDateTime())))
                        IFOKDO(err, m_importer->getDocument()->setParameter("SKG_LAST_" % bankendName.toUpper() % "_IMPORT_LIST", currentList))
                    } else {
                        // Remove last import for next import
                        IFOKDO(err, m_importer->getDocument()->setParameter("SKG_LAST_" % bankendName.toUpper() % "_IMPORT_DATE", QString()))
                        IFOKDO(err, m_importer->getDocument()->setParameter("SKG_LAST_" % bankendName.toUpper() % "_IMPORT_LIST", QString()))
                    }
                }
                IFOKDO(err, m_importer->getDocument()->stepForward(3))
            }
        }
    }

    return err;
}

QString SKGImportPluginBackend::getMimeTypeFilter() const
{
    return QString();
}

#include <skgimportpluginbackend.moc>
