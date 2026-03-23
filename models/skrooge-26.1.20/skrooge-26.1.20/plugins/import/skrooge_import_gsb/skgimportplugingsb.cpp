/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for GSB import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugingsb.h"

#include <kcompressiondevice.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgobjectbase.h"
#include "skgpayeeobject.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginGsb, "metadata.json")

SKGImportPluginGsb::SKGImportPluginGsb(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginGsb::~SKGImportPluginGsb() = default;

bool SKGImportPluginGsb::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("GSB"));
}

SKGError SKGImportPluginGsb::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Initialisation
    // Open file
    KCompressionDevice file(m_importer->getLocalFileName(), KCompressionDevice::GZip);
    if (!file.open(QIODevice::ReadOnly)) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
    } else {
        QDomDocument doc;

        // Set the file without uncompression
        QString errorMsg;
        int errorLine = 0;
        int errorCol = 0;
        bool contentOK = false;
        const auto &content = QString::fromUtf8(file.readAll());
        const auto &result = doc.setContent(content);

        contentOK = !!result;
        errorLine = result.errorLine;
        errorCol = result.errorColumn;
        errorMsg = result.errorMessage;
        file.close();

        // Get root
        QDomElement docElem = doc.documentElement();

        if (!contentOK) {
            err.setReturnCode(ERR_ABORT).setMessage(i18nc("Error message", "%1-%2: '%3'", errorLine, errorCol, errorMsg));
        } else {
            // Check version
            QDomElement general = docElem.firstChildElement(QStringLiteral("General"));
            if (general.isNull()) {
                err.setReturnCode(ERR_ABORT).setMessage(i18nc("Error message", "Bad version of Grisbi document. Version must be >= 0.6.0"));
                contentOK = false;
            }
        }

        if (!contentOK) {
            err.addError(ERR_INVALIDARG, i18nc("Error message", "Invalid XML content in file '%1'", m_importer->getFileName().toDisplayString()));
        } else {
            err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "GSB"), 10);

            QMap<QString, SKGUnitObject> mapIdUnit;
            QMap<QString, SKGBankObject> mapIdBank;
            QMap<QString, SKGAccountObject> mapIdAccount;
            QMap<QString, SKGCategoryObject> mapIdCategory;
            QMap<QString, SKGOperationObject> mapIdOperation;
            QMap<QString, SKGPayeeObject> mapIdPayee;
            QMap<QString, QString> mapIdMode;
            QMap<QString, QString> mapIdBudgetCat;

            // Step 1-Create units
            IFOK(err)
            {
                QDomNodeList currencyList = docElem.elementsByTagName(QStringLiteral("Currency"));
                int nb = currencyList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement currency = currencyList.at(i).toElement();

                    // Creation of the units
                    SKGUnitObject unitObj(m_importer->getDocument());
                    IFOKDO(err, unitObj.setName(getAttribute(currency, QStringLiteral("Na"))))
                    IFOKDO(err, unitObj.setSymbol(getAttribute(currency, QStringLiteral("Co"))))
                    IFOKDO(err, unitObj.setNumberDecimal(SKGServices::stringToInt(getAttribute(currency, QStringLiteral("Fl")))))
                    IFOKDO(err, unitObj.save())

                    mapIdUnit[getAttribute(currency, QStringLiteral("Nb"))] = unitObj;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            // Step 2-Create banks
            IFOK(err)
            {
                QDomNodeList bankList = docElem.elementsByTagName(QStringLiteral("Bank"));
                int nb = bankList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import banks"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement bank = bankList.at(i).toElement();

                    // Creation of the banks
                    SKGBankObject bankObj(m_importer->getDocument());
                    IFOKDO(err, bankObj.setName(getAttribute(bank, QStringLiteral("Na"))))
                    IFOKDO(err, bankObj.setNumber(getAttribute(bank, QStringLiteral("BIC"))))
                    IFOKDO(err, bankObj.save())

                    mapIdBank[getAttribute(bank, QStringLiteral("Nb"))] = bankObj;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(2))

            // Step 3-Create accounts
            SKGBankObject bankDefault(m_importer->getDocument());
            IFOKDO(err, bankDefault.setName(QStringLiteral("GRISBI")))
            IFOKDO(err, bankDefault.save())
            IFOK(err)
            {
                QDomNodeList accountList = docElem.elementsByTagName(QStringLiteral("Account"));
                int nb = accountList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import accounts"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement account = accountList.at(i).toElement();

                    // Creation of the account
                    SKGAccountObject accountObj;
                    SKGBankObject bank = mapIdBank[getAttribute(account, QStringLiteral("Bank"))];
                    if (!bank.exist()) {
                        bank = bankDefault;
                    }
                    IFOKDO(err, bank.addAccount(accountObj))
                    IFOKDO(err, accountObj.setName(getAttribute(account, QStringLiteral("Name"))))
                    IFOKDO(err, accountObj.setNumber(getAttribute(account, QStringLiteral("Bank_account_number"))))
                    IFOKDO(err, accountObj.setComment(getAttribute(account, QStringLiteral("Comment"))))
                    IFOKDO(err, accountObj.setAgencyAddress(getAttribute(account, QStringLiteral("Owner_address"))))
                    IFOKDO(err, accountObj.setMinLimitAmount(SKGServices::stringToDouble(getAttribute(account, QStringLiteral("Minimum_authorised_balance")))))
                    IFOKDO(err, accountObj.minLimitAmountEnabled(accountObj.getMinLimitAmount() != 0.0))
                    IFOKDO(err, accountObj.setClosed(getAttribute(account, QStringLiteral("Closed_account")) != QStringLiteral("0")))
                    IFOKDO(err, accountObj.save())
                    IFOKDO(err,
                           accountObj.setInitialBalance(SKGServices::stringToDouble(getAttribute(account, QStringLiteral("Initial_balance"))),
                                                        mapIdUnit[getAttribute(account, QStringLiteral("Currency"))]))

                    mapIdAccount[getAttribute(account, QStringLiteral("Number"))] = accountObj;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(3))

            // Step 4-Get payees
            IFOK(err)
            {
                QDomNodeList partyList = docElem.elementsByTagName(QStringLiteral("Party"));
                int nb = partyList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import payees"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get payee object
                    QDomElement party = partyList.at(i).toElement();
                    SKGPayeeObject payeeObject;
                    err = SKGPayeeObject::createPayee(m_importer->getDocument(), getAttribute(party, QStringLiteral("Na")), payeeObject);
                    mapIdPayee[getAttribute(party, QStringLiteral("Nb"))] = payeeObject;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(4))

            // Step 5-Get payment mode
            IFOK(err)
            {
                QDomNodeList paymentList = docElem.elementsByTagName(QStringLiteral("Payment"));
                int nb = paymentList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import payment mode"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get mode object
                    QDomElement payment = paymentList.at(i).toElement();
                    mapIdMode[getAttribute(payment, QStringLiteral("Number"))] = getAttribute(payment, QStringLiteral("Name"));

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(5))

            // Step 6-Create categories
            IFOK(err)
            {
                QDomNodeList categoryList = docElem.elementsByTagName(QStringLiteral("Category"));
                int nb = categoryList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import categories"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement category = categoryList.at(i).toElement();

                    // Creation of the categories
                    SKGCategoryObject catObj(m_importer->getDocument());
                    IFOKDO(err, catObj.setName(getAttribute(category, QStringLiteral("Na"))))
                    IFOKDO(err, catObj.save())

                    QString id = SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(category, QStringLiteral("Nb"))));
                    mapIdCategory[id] = catObj;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(6))

            // Step 7-Create subcategories
            IFOK(err)
            {
                QDomNodeList categoryList = docElem.elementsByTagName(QStringLiteral("Sub_category"));
                int nb = categoryList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import categories"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement category = categoryList.at(i).toElement();

                    // Creation of the subcategories
                    SKGCategoryObject catParentObj =
                        mapIdCategory[SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(category, QStringLiteral("Nbc"))))];
                    SKGCategoryObject catObj;
                    IFOKDO(err, catParentObj.addCategory(catObj))
                    IFOKDO(err, catObj.setName(getAttribute(category, QStringLiteral("Na"))))
                    IFOKDO(err, catObj.save())

                    QString id = SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(category, QStringLiteral("Nbc")))
                                                          + SKGServices::stringToInt(getAttribute(category, QStringLiteral("Nb"))));
                    mapIdCategory[id] = catObj;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(7))

            // Step 8-Index of budget categories
            IFOK(err)
            {
                QDomNodeList budgetaryList = docElem.elementsByTagName(QStringLiteral("Budgetary"));
                int nb = budgetaryList.count();
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement budgetary = budgetaryList.at(i).toElement();
                    QString id = SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(budgetary, QStringLiteral("Nb"))));
                    mapIdBudgetCat[id] = getAttribute(budgetary, QStringLiteral("Na"));
                }
            }
            IFOK(err)
            {
                QDomNodeList budgetaryList = docElem.elementsByTagName(QStringLiteral("Sub_budgetary"));
                int nb = budgetaryList.count();
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement budgetary = budgetaryList.at(i).toElement();
                    QString id1 = SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(budgetary, QStringLiteral("Nb"))));
                    QString id = SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(budgetary, QStringLiteral("Nbb")))
                                                          + SKGServices::stringToInt(getAttribute(budgetary, QStringLiteral("Nb"))));
                    mapIdBudgetCat[id] = mapIdBudgetCat[id1] % OBJECTSEPARATOR % getAttribute(budgetary, QStringLiteral("Na"));
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(8))

            // Step 9-Create transaction
            IFOK(err)
            {
                QDomNodeList transactionList = docElem.elementsByTagName(QStringLiteral("Transaction"));
                int nb = transactionList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement transaction = transactionList.at(i).toElement();

                    // Creation of the transaction
                    SKGOperationObject opObj;
                    QString parentOpId = getAttribute(transaction, QStringLiteral("Mo"));
                    if (parentOpId == QStringLiteral("0")) {
                        SKGAccountObject account = mapIdAccount[getAttribute(transaction, QStringLiteral("Ac"))];
                        IFOKDO(err, account.addOperation(opObj, true))
                        IFOKDO(err, opObj.setDate(QDate::fromString(getAttribute(transaction, QStringLiteral("Dt")), QStringLiteral("MM/dd/yyyy"))))
                        IFOKDO(err, opObj.setUnit(mapIdUnit[getAttribute(transaction, QStringLiteral("Cu"))]))
                        IFOKDO(err, opObj.setPayee(mapIdPayee[getAttribute(transaction, QStringLiteral("Pa"))]))
                        IFOKDO(err, opObj.setMode(mapIdMode[getAttribute(transaction, QStringLiteral("Pn"))]))
                        IFOKDO(err, opObj.setNumber(getAttribute(transaction, QStringLiteral("Pc"))))
                        IFOKDO(err, opObj.setComment(getAttribute(transaction, QStringLiteral("No"))))
                        IFOKDO(err, opObj.setImported(true))
                        IFOKDO(err, opObj.setImportID("GSB-" % getAttribute(transaction, QStringLiteral("Nb"))))
                        IFOKDO(err,
                               opObj.setStatus(getAttribute(transaction, QStringLiteral("Re")) == QStringLiteral("0") ? SKGOperationObject::NONE
                                                                                                                      : SKGOperationObject::CHECKED))
                        IFOKDO(err, opObj.setGroupOperation(mapIdOperation[getAttribute(transaction, QStringLiteral("Trt"))]))
                        IFOKDO(err, opObj.save())
                    } else {
                        opObj = mapIdOperation[parentOpId];
                    }

                    // Budgetary allocation
                    IFOK(err)
                    {
                        int sbu = SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("Sbu")));
                        QString id = SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("Bu"))) + qMax(sbu, 0));
                        QString buCat = mapIdBudgetCat[id];
                        if (!buCat.isEmpty()) {
                            err = opObj.setProperty(i18nc("Noun", "Budgetary allocation"), buCat);
                            IFOKDO(err, opObj.save())
                        }
                    }

                    if (getAttribute(transaction, QStringLiteral("Br")) == QStringLiteral("0")) {
                        SKGSubOperationObject subObj;
                        IFOKDO(err, opObj.addSubOperation(subObj))
                        QString id = SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("Ca")))
                                                              + SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("Sca"))));
                        IFOKDO(err, subObj.setCategory(mapIdCategory[id]))
                        IFOKDO(err, subObj.setComment(getAttribute(transaction, QStringLiteral("No"))))
                        IFOKDO(err, subObj.setQuantity(SKGServices::stringToDouble(getAttribute(transaction, QStringLiteral("Am")))))
                        IFOKDO(err, subObj.save())
                    }

                    // Fiscal year
                    IFOK(err)
                    {
                        QString fiscalYear = getAttribute(transaction, QStringLiteral("Vo"));
                        if (!fiscalYear.isEmpty()) {
                            err = opObj.setProperty(i18nc("Noun", "Fiscal year"), fiscalYear);
                            IFOKDO(err, opObj.save())
                        }
                    }

                    if (parentOpId == QStringLiteral("0")) {
                        mapIdOperation[getAttribute(transaction, QStringLiteral("Nb"))] = opObj;
                    }

                    if (!err && i % 500 == 0) {
                        err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(9))

            // Step 10-Create scheduled transaction
            IFOK(err)
            {
                QDomNodeList scheduledList = docElem.elementsByTagName(QStringLiteral("Scheduled"));
                int nb = scheduledList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import scheduled transactions"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement transaction = scheduledList.at(i).toElement();

                    // Creation of the transaction
                    SKGAccountObject account = mapIdAccount[getAttribute(transaction, QStringLiteral("Ac"))];
                    SKGOperationObject opObj;
                    IFOKDO(err, account.addOperation(opObj, true))
                    QDate firstDate = QDate::fromString(getAttribute(transaction, QStringLiteral("Dt")), QStringLiteral("MM/dd/yyyy"));
                    IFOKDO(err, opObj.setDate(firstDate))
                    IFOKDO(err, opObj.setUnit(mapIdUnit[getAttribute(transaction, QStringLiteral("Cu"))]))
                    IFOKDO(err, opObj.setPayee(mapIdPayee[getAttribute(transaction, QStringLiteral("Pa"))]))
                    IFOKDO(err, opObj.setMode(mapIdMode[getAttribute(transaction, QStringLiteral("Pn"))]))
                    IFOKDO(err, opObj.setNumber(getAttribute(transaction, QStringLiteral("Pc"))))
                    IFOKDO(err, opObj.setComment(getAttribute(transaction, QStringLiteral("No"))))
                    IFOKDO(err, opObj.setImported(true))
                    IFOKDO(err, opObj.setImportID("GSB-" % getAttribute(transaction, QStringLiteral("Nb"))))
                    IFOKDO(err, opObj.setTemplate(true))
                    IFOKDO(err, opObj.save())

                    SKGSubOperationObject subObj;
                    IFOKDO(err, opObj.addSubOperation(subObj))
                    QString id = SKGServices::intToString(10000 * SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("Ca")))
                                                          + SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("Sca"))));
                    IFOKDO(err, subObj.setCategory(mapIdCategory[id]))
                    IFOKDO(err, subObj.setComment(getAttribute(transaction, QStringLiteral("No"))))
                    IFOKDO(err, subObj.setQuantity(SKGServices::stringToDouble(getAttribute(transaction, QStringLiteral("Am")))))
                    IFOKDO(err, subObj.save())

                    QString Tra = getAttribute(transaction, QStringLiteral("Tra"));
                    if (Tra != QStringLiteral("0")) {
                        // This is a transfer
                        SKGOperationObject opObj2;
                        IFOKDO(err, opObj.duplicate(opObj2, opObj.getDate(), true))
                        IFOKDO(err, opObj2.setImported(true))
                        IFOKDO(err, opObj2.setImportID("GSB-" % getAttribute(transaction, QStringLiteral("Nb")) % "_TR"))
                        IFOKDO(err, opObj2.save())

                        SKGObjectBase::SKGListSKGObjectBase subObjs2;
                        IFOKDO(err, opObj2.getSubOperations(subObjs2))
                        if (!err && !subObjs2.isEmpty()) {
                            SKGSubOperationObject subObj2(subObjs2.at(0));
                            err = subObj2.setQuantity(-subObj.getQuantity());
                            IFOKDO(err, subObj2.save())
                        }

                        // Group transactions
                        IFOKDO(err, opObj.setGroupOperation(opObj2))
                        IFOKDO(err, opObj.save())
                    }

                    // Create the schedule
                    SKGRecurrentOperationObject recuObj;
                    IFOKDO(err, opObj.addRecurrentOperation(recuObj))
                    IFOKDO(err, recuObj.setAutoWriteDays(0))
                    IFOKDO(err, recuObj.autoWriteEnabled(getAttribute(transaction, QStringLiteral("Au")) != QStringLiteral("0")))
                    IFOK(err)
                    {
                        // text_frequency [] = { _("Once"), _("Weekly"), _("Monthly"), _("two months"), ("trimester"), _("Yearly"), _("Custom"), nullptr };
                        int occu = 1;
                        SKGRecurrentOperationObject::PeriodUnit period = SKGRecurrentOperationObject::DAY;
                        QString Pe = getAttribute(transaction, QStringLiteral("Pe"));
                        if (Pe == QStringLiteral("0")) {
                            period = SKGRecurrentOperationObject::MONTH;
                            IFOKDO(err, recuObj.timeLimit(true))
                            IFOKDO(err, recuObj.setTimeLimit(1))

                        } else if (Pe == QStringLiteral("1")) {
                            period = SKGRecurrentOperationObject::WEEK;
                        } else if (Pe == QStringLiteral("2")) {
                            period = SKGRecurrentOperationObject::MONTH;
                        } else if (Pe == QStringLiteral("3")) {
                            occu = 2;
                            period = SKGRecurrentOperationObject::MONTH;
                        } else if (Pe == QStringLiteral("4")) {
                            occu = 3;
                            period = SKGRecurrentOperationObject::MONTH;
                        } else if (Pe == QStringLiteral("5")) {
                            period = SKGRecurrentOperationObject::YEAR;
                        } else if (Pe == QStringLiteral("6")) {
                            // text_frequency_user [] = { _("Days"), _("Weeks"), _("Months"), _("Years"), nullptr };
                            occu = SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("Pep")));
                            QString Pei = getAttribute(transaction, QStringLiteral("Pei"));
                            if (Pei == QStringLiteral("0")) {
                                period = SKGRecurrentOperationObject::DAY;
                            } else if (Pei == QStringLiteral("1")) {
                                period = SKGRecurrentOperationObject::WEEK;
                            } else if (Pei == QStringLiteral("2")) {
                                period = SKGRecurrentOperationObject::MONTH;
                            } else {
                                period = SKGRecurrentOperationObject::YEAR;
                            }
                        }

                        IFOKDO(err, recuObj.setPeriodUnit(period))
                        IFOKDO(err, recuObj.setPeriodIncrement(occu))

                        QString Dtl = getAttribute(transaction, QStringLiteral("Dtl"));
                        if (!err && !Dtl.isEmpty()) {
                            IFOKDO(err, recuObj.timeLimit(true))
                            IFOKDO(err, recuObj.setTimeLimit(QDate::fromString(Dtl, QStringLiteral("MM/dd/yyyy"))))
                        }
                    }
                    IFOKDO(err, recuObj.save())

                    if (!err && i % 500 == 0) {
                        err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(10))

            SKGENDTRANSACTION(m_importer->getDocument(), err)

            IFOKDO(err, m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE")))
        }
    }

    return err;
}

QString SKGImportPluginGsb::getAttribute(const QDomElement &iElement, const QString &iAttribute)
{
    QString val = iElement.attribute(iAttribute);
    if (val == QStringLiteral("(null)")) {
        val = QString();
    }
    return val;
}
QString SKGImportPluginGsb::getMimeTypeFilter() const
{
    return "*.gsb|" % i18nc("A file format", "Grisbi file");
}

#include <skgimportplugingsb.moc>
