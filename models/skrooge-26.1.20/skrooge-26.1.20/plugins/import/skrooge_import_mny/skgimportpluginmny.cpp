/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for KMY import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginmny.h"

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include <qdir.h>
#include <qjsondocument.h>
#include <qprocess.h>
#include <qstandardpaths.h>
#include <quuid.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgobjectbase.h"
#include "skgpayeeobject.h"
#include "skgservices.h"
#include "skgtraces.h"

QMap<QString, SKGUnitObject> SKGImportPluginMny::m_mapIdSecurity;
QMap<QString, SKGAccountObject> SKGImportPluginMny::m_mapIdAccount;
QMap<QString, SKGCategoryObject> SKGImportPluginMny::m_mapIdCategory;
QMap<QString, SKGTrackerObject> SKGImportPluginMny::m_mapIdTracker;
QMap<QString, SKGPayeeObject> SKGImportPluginMny::m_mapIdPayee;

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginMny, "metadata.json")

SKGImportPluginMny::SKGImportPluginMny(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)

    m_importParameters[QStringLiteral("password")] = QString();
    m_importParameters[QStringLiteral("install_sunriise")] = 'N';
}

SKGImportPluginMny::~SKGImportPluginMny() = default;

bool SKGImportPluginMny::removeDir(const QString &dirName)
{
    bool result = false;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        const auto list = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
        for (const auto &info : list) {
            if (info.isDir()) {
                result = SKGImportPluginMny::removeDir(info.absoluteFilePath());
            } else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }
    return result;
}

bool SKGImportPluginMny::isImportPossible()
{
    SKGTRACEINFUNC(10)
    if (m_importer->getDocument() == nullptr) {
        return true;
    }
    QString extension = m_importer->getFileNameExtension();
    return (extension == QStringLiteral("MNY"));
}

SKGError SKGImportPluginMny::readJsonFile(const QString &iFileName, QVariant &oVariant)
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)
    QFile file(iFileName);
    if (Q_UNLIKELY(!file.open(QIODevice::ReadOnly))) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", iFileName));
    } else {
        QByteArray json = file.readAll();
        file.close();
        QJsonParseError ok{};
        oVariant = QJsonDocument::fromJson(json, &ok).toVariant();
        if (ok.error != QJsonParseError::NoError || json.isEmpty()) {
            err.setReturnCode(ERR_FAIL)
                .setMessage(ok.errorString())
                .addError(ERR_FAIL, i18nc("Error message", "Error during parsing of '%1'", file.fileName()));
        }
    }
    return err;
}

SKGError SKGImportPluginMny::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }

    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Check read access file
    if (!QFileInfo(m_importer->getLocalFileName()).isReadable()) {
        err.setReturnCode(ERR_READACCESS).setMessage(i18nc("Error message", "The file %1 does not have read access rights", m_importer->getLocalFileName()));
        return err;
    }

    // Check install
    if (QStandardPaths::findExecutable(QStringLiteral("java")).isEmpty()) {
        err.setReturnCode(ERR_ABORT).setMessage(i18nc("Error message", "The java application is not installed. You must manually install it."));
        return err;
    }

    QString sunriise_jar = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) % "/sunriise.jar";
    int expected_sunriise_version = 1; // This number must be incremented at each new version of sunriise
    auto sunriise_version = SKGServices::stringToInt(m_importer->getDocument()->getParameter(QStringLiteral("SKG_SUNRIISE_VERSION"), QStringLiteral("0")));
    auto sunriize_installation_requested = m_importParameters.value(QStringLiteral("install_sunriise")) == QStringLiteral("Y");
    auto sunriise_already_installed = QFile(sunriise_jar).exists();
    if (!sunriise_already_installed || sunriise_version != expected_sunriise_version) {
        if (sunriise_already_installed || sunriize_installation_requested) {
            QDir().mkpath(QFileInfo(sunriise_jar).dir().path());
            err = SKGServices::upload(QUrl(QStringLiteral("https://skrooge.org/files/sunriise.jar")), QUrl::fromLocalFile(sunriise_jar));
            IFKO(err) return err;

            m_importer->getDocument()->setParameter(QStringLiteral("SKG_SUNRIISE_VERSION"), QString::number(expected_sunriise_version));
        } else {
            err.setReturnCode(ERR_INSTALL)
                .setMessage(i18nc("Error message", "The sunriise application is needed for Microsoft Money import but is not installed in '%1'", sunriise_jar));
            err.setProperty(QStringLiteral("sunriise"));
            return err;
        }
    }

    // Initialisation
    m_mapIdSecurity.clear();
    m_mapIdAccount.clear();
    m_mapIdCategory.clear();
    m_mapIdTracker.clear();
    m_mapIdPayee.clear();

    // Execute sunriise
    QString uniqueId = QUuid::createUuid().toString();
    QString temporaryPath = QDir::tempPath() % "/" % uniqueId;
    removeDir(temporaryPath);
    QDir::temp().mkdir(uniqueId);
    QDir temporaryDir(temporaryPath);

    QString cmd = "java -cp \"" % sunriise_jar % "\" com/le/sunriise/export/ExportToJSON \"" % m_importer->getLocalFileName() % "\" \""
        % m_importParameters.value(QStringLiteral("password")) % "\" " % temporaryPath;
    SKGTRACEL(10) << "Execution of :" << cmd << Qt::endl;
    QProcess p;
    p.start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << cmd);
    if (p.waitForFinished(1000 * 60 * 5) && p.exitCode() == 0) {
        // Import
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "MNY"), 6);
        // Step 1-Import categories
        IFOK(err)
        {
            /*[ {
            "id" : 137,
            "parentId" : 130,
            "name" : "Other Income",
            "classificationId" : 0,
            "level" : 1
            },*/
            QVariant var;
            err = readJsonFile(temporaryPath % "/categories.json", var);
            QVariantList list = var.toList();
            IFOK(err)
            {
                SKGTRACEINRC(10, "SKGImportPluginMny::importFile-categories", err)
                int nb = list.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import categories"), nb);
                // Create categories
                int index = 0;
                for (int t = 0; !err && t < 20 && index < nb; ++t) {
                    for (int i = 0; !err && i < nb && index < nb; ++i) {
                        QVariantMap category = list.at(i).toMap();

                        int classificationId = category[QStringLiteral("classificationId")].toInt();
                        if (classificationId == 0) {
                            // This is a category
                            QString parentId = category[QStringLiteral("parentId")].toString();
                            QString id = category[QStringLiteral("id")].toString();
                            QString name = category[QStringLiteral("name")].toString();
                            int level = category[QStringLiteral("level")].toInt();

                            if (level == t) {
                                SKGCategoryObject catObject(m_importer->getDocument());
                                err = catObject.setName(name);
                                if (!err && !parentId.isEmpty()) {
                                    err = catObject.setParentCategory(m_mapIdCategory[parentId]);
                                }
                                IFOKDO(err, catObject.save())

                                m_mapIdCategory[id] = catObject;

                                ++index;
                                IFOKDO(err, m_importer->getDocument()->stepForward(index))
                            }
                        } else if (classificationId > 0) {
                            // This is a tracker
                            QString id = category[QStringLiteral("id")].toString();
                            QString name = category[QStringLiteral("name")].toString();
                            QString parentId = category[QStringLiteral("parentId")].toString();
                            if (!parentId.isEmpty()) {
                                name = m_mapIdTracker[parentId].getName() % OBJECTSEPARATOR % name;
                            }

                            SKGTrackerObject trackObject(m_importer->getDocument());
                            err = trackObject.setName(name);
                            IFOKDO(err, trackObject.save())

                            m_mapIdTracker[id] = trackObject;

                            ++index;
                            IFOKDO(err, m_importer->getDocument()->stepForward(index))
                        }
                    }
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(1))
        }

        // Step 2-Import payees
        IFOK(err)
        {
            QVariant var;
            err = readJsonFile(temporaryPath % "/payees.json", var);
            QVariantList list = var.toList();
            IFOK(err)
            {
                SKGTRACEINRC(10, "SKGImportPluginMny::importFile-payees", err)
                int nb = list.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import payees"), nb);
                // Create categories
                for (int i = 0; !err && i < nb; ++i) {
                    QVariantMap payee = list.at(i).toMap();

                    QString id = payee[QStringLiteral("id")].toString();
                    QString name = payee[QStringLiteral("name")].toString();

                    // Is already created?
                    SKGPayeeObject payeeObject(m_importer->getDocument());
                    err = payeeObject.setName(name);
                    IFOKDO(err, payeeObject.save())

                    m_mapIdPayee[id] = payeeObject;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(2))
        }

        // Step 3-Import securities
        IFOK(err)
        {
            QVariant var;
            err = readJsonFile(temporaryPath % "/securities.json", var);
            QVariantList list = var.toList();
            IFOK(err)
            {
                SKGTRACEINRC(10, "SKGImportPluginMny::importFile-securities", err)
                int nb = list.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nb);
                // Create categories
                for (int i = 0; !err && i < nb; ++i) {
                    QVariantMap unit = list.at(i).toMap();

                    QString id = unit[QStringLiteral("id")].toString();
                    QString name = unit[QStringLiteral("name")].toString();
                    QString symbol = unit[QStringLiteral("symbol")].toString();
                    QString internetCode;

                    // BUG: 507235 vvv
                    auto symbol_items = SKGServices::splitCSVLine(symbol, ':');
                    if (symbol_items.count() == 2) {
                        symbol = symbol_items.at(1);
                        internetCode = symbol;
                    }
                    // BUG: 507235 ^^^
                    if (symbol.isEmpty()) {
                        symbol = name;
                    }

                    // Is already created?
                    SKGUnitObject unitobject(m_importer->getDocument());
                    err = unitobject.setName(name);
                    IFOKDO(err, unitobject.setSymbol(symbol))
                    IFOKDO(err, unitobject.setType(SKGUnitObject::SHARE))
                    IFOKDO(err, unitobject.setInternetCode(internetCode))
                    // The unit is not saved because it will be saved when used

                    m_mapIdSecurity[id] = unitobject;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(3))
        }

        // Step 4-Import accounts
        IFOK(err)
        {
            /*
            {
                "id" : 1,
                "name" : "Investments to Watch",
                "relatedToAccountId" : null,
                "relatedToAccount" : null,
                "type" : 5,
                "accountType" : "INVESTMENT",
                "closed" : false,
                "startingBalance" : 0.0000,
                "currentBalance" : 0,
                "currencyId" : 18,
                "currencyCode" : "GBP",
                "retirement" : false,
                "investmentSubType" : -1,
                "securityHoldings" : [ ],
                "amountLimit" : null,
                "creditCard" : false,
                "401k403b" : false
            }*/
            // List directories
            QStringList list = temporaryDir.entryList(QStringList() << QStringLiteral("*.d"), QDir::Dirs);
            SKGTRACEINFUNCRC(10, err)
            int nb = list.count();
            err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import accounts"), nb);

            SKGBankObject bank(m_importer->getDocument());
            IFOKDO(err, bank.setName(QStringLiteral("Microsoft Money")))
            IFOKDO(err, bank.save())

            // Create accounts
            for (int i = 0; !err && i < nb; ++i) {
                const QString &accountDir = list.at(i);

                QVariant var;
                err = readJsonFile(temporaryPath % "/" % accountDir % "/account.json", var);
                QVariantMap account = var.toMap();
                IFOK(err)
                {
                    // Create currency
                    SKGUnitObject unitObj;
                    err = SKGUnitObject::createCurrencyUnit(m_importer->getDocument(), account[QStringLiteral("currencyCode")].toString(), unitObj);

                    // Create account
                    SKGAccountObject accountObj;
                    IFOKDO(err, bank.addAccount(accountObj))
                    IFOKDO(err, accountObj.setName(account[QStringLiteral("name")].toString()))
                    int type = account[QStringLiteral("type")].toInt();
                    IFOKDO(err,
                           accountObj.setType(type == 0       ? SKGAccountObject::CURRENT
                                                  : type == 1 ? SKGAccountObject::CREDITCARD
                                                  : type == 3 ? SKGAccountObject::ASSETS
                                                  : type == 5 ? SKGAccountObject::INVESTMENT
                                                  : type == 6 ? SKGAccountObject::LOAN
                                                              : SKGAccountObject::OTHER)); // TODO(Stephane MANKOWSKI)
                    IFOKDO(err, accountObj.save())

                    // Update initial balance
                    IFOKDO(err, accountObj.setInitialBalance(account[QStringLiteral("startingBalance")].toDouble(), unitObj))

                    IFOKDO(err, accountObj.setClosed(account[QStringLiteral("closed")].toBool()))
                    IFOKDO(err, accountObj.save())

                    m_mapIdAccount[account[QStringLiteral("id")].toString()] = accountObj;
                }

                IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
            }

            SKGENDTRANSACTION(m_importer->getDocument(), err)
        }
        IFOKDO(err, m_importer->getDocument()->stepForward(4))

        // Step 5-Import operation
        IFOK(err)
        {
            // List directories
            QStringList list = temporaryDir.entryList(QStringList() << QStringLiteral("*.d"), QDir::Dirs);
            SKGTRACEINFUNCRC(10, err)
            int nb = list.count();
            err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb);

            // Create accounts
            int index = 0;
            for (int i = 0; !err && i < nb; ++i) {
                const QString &accountDir = list.at(i);

                QVariant var;
                err = readJsonFile(temporaryPath % "/" % accountDir % "/transactions.json", var);
                QVariantList transactions = var.toList();
                IFOK(err)
                {
                    int nbo = transactions.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nbo);
                    for (int j = 0; !err && j < nbo; ++j) {
                        QVariantMap operation = transactions.at(j).toMap();
                        if (!operation[QStringLiteral("void")].toBool()) {
                            SKGAccountObject accountObj = m_mapIdAccount.value(operation[QStringLiteral("accountId")].toString());
                            QString securityId = operation[QStringLiteral("securityId")].toString();

                            SKGUnitObject unitObj;
                            if (securityId.isEmpty()) {
                                IFOKDO(err, accountObj.getUnit(unitObj))
                            } else {
                                unitObj = m_mapIdSecurity[securityId];
                                if (unitObj.getID() == 0) {
                                    err = unitObj.save();
                                    m_mapIdSecurity[securityId] = unitObj;
                                }
                            }

                            SKGOperationObject operationObj;
                            IFOKDO(err, accountObj.addOperation(operationObj, true))
                            IFOKDO(err, operationObj.setUnit(unitObj))
                            IFOKDO(err, operationObj.setDate(QDateTime::fromMSecsSinceEpoch(operation[QStringLiteral("date")].toULongLong()).date()))
                            IFOKDO(err, operationObj.setComment(operation[QStringLiteral("memo")].toString()))
                            IFOK(err)
                            {
                                // The number is something like "0     4220725" or "1string"
                                QString number = operation[QStringLiteral("number")].toString();
                                if (!number.isEmpty()) {
                                    if (number.startsWith(u'1')) {
                                        err = operationObj.setMode(number.right(number.size() - 1));
                                    } else {
                                        QStringList ln = SKGServices::splitCSVLine(number, ' ');
                                        err = operationObj.setNumber(ln.at(ln.count() - 1));
                                    }
                                }
                            }
                            IFOKDO(err, operationObj.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T")))
                            IFOKDO(err, operationObj.setImportID("MNY-" % operation[QStringLiteral("id")].toString()))
                            QString payId = operation[QStringLiteral("payeeId")].toString();
                            if (!payId.isEmpty() && !err) {
                                err = operationObj.setPayee(m_mapIdPayee[payId]);
                            }
                            IFOKDO(err,
                                   operationObj.setStatus(operation[QStringLiteral("reconciled")].toBool()    ? SKGOperationObject::CHECKED
                                                              : operation[QStringLiteral("cleared")].toBool() ? SKGOperationObject::MARKED
                                                                                                              : SKGOperationObject::NONE));
                            if (operation[QStringLiteral("transfer")].toBool()) {
                                IFOKDO(err, operationObj.setComment("#MNYTRANSFER#" % operationObj.getComment()))
                            }
                            IFOKDO(err, operationObj.save(false))

                            double amount = operation[QStringLiteral("amount")].toDouble();

                            // BUG: 507235 vvv
                            if (operation[QStringLiteral("investment")].toBool()) {
                                amount = operation[QStringLiteral("quantity")].toDouble();

                                if (operation[QStringLiteral("price")].toDouble() != 0.0) {
                                    SKGUnitValueObject unitval;
                                    IFOKDO(err, unitObj.addUnitValue(unitval))
                                    IFOKDO(err, unitval.setQuantity(operation[QStringLiteral("price")].toDouble()))
                                    IFOKDO(err, unitval.setDate(operationObj.getDate()))
                                    IFOKDO(err, unitval.save())
                                }
                            }
                            // BUG: 507235 ^^^

                            // Is it a split?
                            QVariantList splits = operation[QStringLiteral("splits")].toList();
                            int nbs = splits.count();
                            if (nbs != 0) {
                                // Yes
                                for (int k = 0; !err && k < nbs; ++k) {
                                    QVariantMap split = splits[k].toMap();
                                    QVariantMap transaction = split[QStringLiteral("transaction")].toMap();

                                    SKGSubOperationObject subOperationObj;
                                    IFOKDO(err, operationObj.addSubOperation(subOperationObj))
                                    QString catId = transaction[QStringLiteral("categoryId")].toString();
                                    if (!catId.isEmpty() && !err) {
                                        err = subOperationObj.setCategory(m_mapIdCategory[catId]);
                                    }
                                    QString trackId = transaction[QStringLiteral("class2")].toString();
                                    if (trackId.isEmpty() || trackId == QStringLiteral("-1")) {
                                        trackId = transaction[QStringLiteral("class1")].toString();
                                    }
                                    if (!trackId.isEmpty() && trackId != QStringLiteral("-1") && !err) {
                                        err = subOperationObj.setTracker(m_mapIdTracker[trackId]);
                                    }
                                    double splitAmount = transaction[QStringLiteral("amount")].toDouble();
                                    IFOKDO(err, subOperationObj.setQuantity(splitAmount))
                                    IFOKDO(err, subOperationObj.setComment(transaction[QStringLiteral("memo")].toString()))
                                    IFOKDO(err, subOperationObj.save(false, false))
                                    amount -= splitAmount;
                                }

                                // Is the amount equal to the sum of split?
                                if (qAbs(amount) > 0.00001) {
                                    // Create one more sub transaction to align amounts
                                    SKGSubOperationObject subOperationObj;
                                    IFOKDO(err, operationObj.addSubOperation(subOperationObj))
                                    IFOKDO(err, subOperationObj.setQuantity(amount))
                                    IFOKDO(err, subOperationObj.setComment(operation[QStringLiteral("memo")].toString()))
                                    IFOKDO(err, subOperationObj.save(false, false))

                                    IFOKDO(err,
                                           m_importer->getDocument()->sendMessage(i18nc("Warning message",
                                                                                        "The transaction '%1' has been repaired because its amount was not "
                                                                                        "equal to the sum of the amounts of its splits",
                                                                                        operationObj.getDisplayName()),
                                                                                  SKGDocument::Warning))
                                }
                            } else {
                                // No
                                SKGSubOperationObject subOperationObj;
                                IFOKDO(err, operationObj.addSubOperation(subOperationObj))
                                QString catId = operation[QStringLiteral("categoryId")].toString();
                                if (!catId.isEmpty() && !err) {
                                    err = subOperationObj.setCategory(m_mapIdCategory[catId]);
                                }
                                QString trackId = operation[QStringLiteral("class2")].toString();
                                if (trackId.isEmpty() || trackId == QStringLiteral("-1")) {
                                    trackId = operation[QStringLiteral("class1")].toString();
                                }
                                if (!trackId.isEmpty() && trackId != QStringLiteral("-1") && !err) {
                                    err = subOperationObj.setTracker(m_mapIdTracker[trackId]);
                                }
                                IFOKDO(err, subOperationObj.setQuantity(amount))
                                IFOKDO(err, subOperationObj.setComment(operation[QStringLiteral("memo")].toString()))
                                IFOKDO(err, subOperationObj.save(false, false))
                            }
                        }

                        if (!err && index % 500 == 0) {
                            err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                        }
                        ++index;

                        IFOKDO(err, m_importer->getDocument()->stepForward(j + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }

                IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
            }

            SKGENDTRANSACTION(m_importer->getDocument(), err)
        }

        IFOKDO(err, m_importer->getDocument()->stepForward(5))

        // Step 6-Group operation
        int nbg = 0;
        IFOKDO(err, m_importer->findAndGroupTransfers(nbg, QStringLiteral("A.t_comment LIKE '#MNYTRANSFER#%' AND B.t_comment LIKE '#MNYTRANSFER#%'")))
        IFOKDO(err,
               m_importer->getDocument()->executeSqliteOrder(
                   QStringLiteral("UPDATE operation SET t_comment=SUBSTR(t_comment, 14) WHERE t_comment LIKE '#MNYTRANSFER#%'")))

        IFOKDO(err, m_importer->getDocument()->stepForward(6))

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    } else {
        if (p.exitCode() == 1) {
            err.setReturnCode(ERR_ENCRYPTION).setMessage(i18nc("Error message", "Invalid password"));
        } else {
            err.setReturnCode(ERR_FAIL)
                .setMessage(i18nc("Error message", "The execution of '%1' failed", cmd))
                .addError(ERR_FAIL,
                          i18nc("Error message", "The extraction from the Microsoft Money document '%1' failed", m_importer->getFileName().toDisplayString()));
        }
    }

    removeDir(temporaryPath);

    // Clean
    m_mapIdSecurity.clear();
    m_mapIdAccount.clear();
    m_mapIdCategory.clear();
    m_mapIdPayee.clear();

    return err;
}

QString SKGImportPluginMny::getMimeTypeFilter() const
{
    return "*.mny|" % i18nc("A file format", "Microsoft Money document");
}

#include <skgimportpluginmny.moc>
