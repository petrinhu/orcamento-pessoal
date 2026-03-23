/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for IIF import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginiif.h"

#include <kpluginfactory.h>

#include <qcryptographichash.h>
#include <qfile.h>
#include <qsavefile.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgservices.h"
#include "skgtraces.h"
#include <utility>

/**
 * Opening balance string
 */
#define OPENINGBALANCE "Opening Balance"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginIif, "metadata.json")

SKGImportPluginIif::SKGImportPluginIif(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginIif::~SKGImportPluginIif() = default;

bool SKGImportPluginIif::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return isExportPossible();
}

QString SKGImportPluginIif::getVal(const QStringList &iVals, const QString &iAttribute) const
{
    QString output;

    // Get the corresponding header
    QStringList header = m_headers["!" % iVals[0]];
    if (!header.isEmpty()) {
        // Get index of getAttribute
        int pos = header.indexOf(iAttribute);
        if (pos != -1 && pos < iVals.count()) {
            output = iVals[pos].trimmed();
        }
    }
    return output;
}

SKGError SKGImportPluginIif::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }

    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Create account if needed
    QDateTime now = QDateTime::currentDateTime();
    QString postFix = SKGServices::dateToSqlString(now);

    // Begin transaction
    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "IIF"), 3);
    IFOK(err)
    {
        // Open file
        QFile file(m_importer->getLocalFileName());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
        } else {
            QTextStream stream(&file);
            if (!m_importer->getCodec().isEmpty()) {
                stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
            }

            // Import
            SKGOperationObject operation;
            SKGSubOperationObject suboperation;
            int index = 0;
            while (!stream.atEnd() && !err) {
                // Read line
                QString line = stream.readLine().trimmed();
                if (!line.isEmpty()) {
                    QStringList vals = SKGServices::splitCSVLine(line, '\t');
                    if (vals[0].startsWith(u'!')) {
                        // This is a header
                        m_headers[vals[0]] = vals;
                    } else {
                        // This is an item
                        QString t = vals[0].trimmed();
                        if (t == QStringLiteral("ACCNT")) {
                            // This is an account
                            QString accountType = getVal(vals, QStringLiteral("ACCNTTYPE"));
                            QString name = getVal(vals, QStringLiteral("NAME"));
                            /*
                            AP: Accounts payable            DONE: ACCOUNT
                            AR: Accounts receivable     DONE: ACCOUNT
                            BANK: Checking or savings       DONE: ACCOUNT
                            CCARD: Credit card account      DONE: ACCOUNT
                            COGS: Cost of goods sold        DONE: CATEGORY
                            EQUITY: Capital/Equity      DONE: CATEGORY
                            EXEXP: Other expense        DONE: CATEGORY
                            EXINC: Other income         DONE: CATEGORY
                            EXP: Expense            DONE: CATEGORY
                            FIXASSET: Fixed asset       DONE: ACCOUNT
                            INC: Income             DONE: CATEGORY
                            LTLIAB: Long term liability     DONE: ACCOUNT
                            NONPOSTING: Non-posting account DONE: ACCOUNT
                            OASSET: Other asset         DONE: ACCOUNT
                            OCASSET: Other current asset    DONE: ACCOUNT
                            OCLIAB: Other current liability DONE: ACCOUNT*/
                            if (accountType == QStringLiteral("BANK") || accountType == QStringLiteral("AP") || accountType == QStringLiteral("AR")
                                || accountType == QStringLiteral("OCASSET") || accountType == QStringLiteral("FIXASSET")
                                || accountType == QStringLiteral("OASSET") || accountType == QStringLiteral("OCLIAB") || accountType == QStringLiteral("LTLIAB")
                                || accountType == QStringLiteral("CCARD") || accountType == QStringLiteral("NONPOSTING")) {
                                // Check if the account already exist
                                SKGAccountObject account;
                                err = SKGNamedObject::getObjectByName(m_importer->getDocument(), QStringLiteral("account"), name, account);
                                IFKO(err)
                                {
                                    // Create account
                                    SKGBankObject bank(m_importer->getDocument());
                                    err = bank.setName(i18nc("Noun", "Bank for import %1", postFix));
                                    if (!err && bank.load().isFailed()) {
                                        err = bank.save(false);
                                    }
                                    IFOKDO(err, bank.addAccount(account))
                                    IFOKDO(err, account.setName(name))
                                    if (!err && account.load().isFailed()) {
                                        IFOKDO(err,
                                               account.setType(accountType == QStringLiteral("BANK") || accountType == QStringLiteral("AP")
                                                                       || accountType == QStringLiteral("AR")
                                                                   ? SKGAccountObject::CURRENT
                                                                   : accountType == QStringLiteral("OCASSET") || accountType == QStringLiteral("FIXASSET")
                                                                       || accountType == QStringLiteral("OASSET") || accountType == QStringLiteral("EQUITY")
                                                                   ? SKGAccountObject::ASSETS
                                                                   : accountType == QStringLiteral("OCLIAB") || accountType == QStringLiteral("LTLIAB")
                                                                   ? SKGAccountObject::LOAN
                                                                   : accountType == QStringLiteral("CCARD") ? SKGAccountObject::CREDITCARD
                                                                                                            : SKGAccountObject::OTHER))
                                        IFOKDO(err, account.setAgencyNumber(getVal(vals, QStringLiteral("ACCNUM"))))
                                        IFOKDO(err, account.setComment(getVal(vals, QStringLiteral("DESC"))))
                                        IFOKDO(err, account.save())
                                        m_accounts[name] = account;
                                    }
                                }
                            } else {
                                // This is a category
                                SKGCategoryObject category;
                                QString originalName = name;
                                IFOKDO(err, SKGCategoryObject::createPathCategory(m_importer->getDocument(), name.replace(':', OBJECTSEPARATOR), category))
                                m_categories[originalName] = category;
                            }
                        } else if (t == QStringLiteral("CUST")) {
                            // This is a payee
                            QString name = getVal(vals, QStringLiteral("NAME"));
                            SKGPayeeObject payee;
                            IFOKDO(err, SKGPayeeObject::createPayee(m_importer->getDocument(), name, payee))

                            QString address = getVal(vals, QStringLiteral("BADDR1")) % " " % getVal(vals, QStringLiteral("BADDR2")) % " "
                                % getVal(vals, QStringLiteral("BADDR3")) % " " % getVal(vals, QStringLiteral("BADDR4")) % " "
                                % getVal(vals, QStringLiteral("BADDR5")) % " " % getVal(vals, QStringLiteral("SADDR1")) % " "
                                % getVal(vals, QStringLiteral("SADDR2")) % " " % getVal(vals, QStringLiteral("SADDR3")) % " "
                                % getVal(vals, QStringLiteral("SADDR4")) % " " % getVal(vals, QStringLiteral("SADDR5"));
                            address = address.trimmed();

                            IFOKDO(err, payee.setAddress(address))
                            IFOKDO(err, payee.save())
                            m_payees[name] = payee;
                        } else if (t == QStringLiteral("VEND")) {
                            // This is a payee
                            QString name = getVal(vals, QStringLiteral("NAME"));
                            SKGPayeeObject payee;
                            IFOKDO(err, SKGPayeeObject::createPayee(m_importer->getDocument(), getVal(vals, QStringLiteral("NAME")), payee))

                            QString address = getVal(vals, QStringLiteral("ADDR1")) % " " % getVal(vals, QStringLiteral("ADDR2")) % " "
                                % getVal(vals, QStringLiteral("ADDR3")) % " " % getVal(vals, QStringLiteral("ADDR4")) % " "
                                % getVal(vals, QStringLiteral("ADDR5"));
                            address = address.trimmed();

                            IFOKDO(err, payee.setAddress(address))
                            IFOKDO(err, payee.save())
                            m_payees[name] = payee;
                        } else if (t == QStringLiteral("ENDTRNS")) {
                            operation = SKGOperationObject();
                            suboperation = SKGSubOperationObject();
                        } else if (t == QStringLiteral("TRNS")) {
                            // This is an operation
                            SKGAccountObject account = m_accounts.value(getVal(vals, QStringLiteral("ACCNT")));
                            IFOKDO(err, account.addOperation(operation))

                            QDate date =
                                SKGServices::stringToDate(SKGServices::dateToSqlString(getVal(vals, QStringLiteral("DATE")), QStringLiteral("MM/DD/YYYY")));
                            IFOKDO(err, operation.setDate(date))
                            IFOKDO(err,
                                   operation.setStatus(getVal(vals, QStringLiteral("CLEAR")) == QStringLiteral("Y") ? SKGOperationObject::CHECKED
                                                                                                                    : SKGOperationObject::NONE))

                            QString number = getVal(vals, QStringLiteral("DOCNUM"));
                            if (!number.isEmpty()) {
                                IFOKDO(err, operation.setNumber(number))
                            }

                            // If an initial balance is existing for the account then we use the unit else we look for the most appropriate unit
                            SKGUnitObject unit;
                            double initBalance;
                            account.getInitialBalance(initBalance, unit);
                            if (!unit.exist()) {
                                err = m_importer->getDefaultUnit(unit, &date);
                            }
                            IFOKDO(err, operation.setUnit(unit))

                            IFOKDO(err, operation.setMode(getVal(vals, QStringLiteral("TRNSTYPE"))))
                            QString p = getVal(vals, QStringLiteral("NAME"));
                            if (!p.isEmpty()) {
                                IFOKDO(err, operation.setPayee(m_payees[p]))
                            }
                            IFOKDO(err, operation.setComment(getVal(vals, QStringLiteral("MEMO"))))
                            IFOKDO(err, operation.save())
                        } else if (t == QStringLiteral("SPL")) {
                            // This is a sub operation
                            QString a = getVal(vals, QStringLiteral("ACCNT"));
                            if (m_categories.contains(a)) {
                                // This is a split
                                SKGCategoryObject cat = m_categories.value(a);
                                IFOKDO(err, operation.addSubOperation(suboperation))
                                IFOKDO(err, suboperation.setQuantity(-SKGServices::stringToDouble(getVal(vals, QStringLiteral("AMOUNT")))))
                                IFOKDO(err, suboperation.setCategory(cat))
                                IFOKDO(err, suboperation.setComment(getVal(vals, QStringLiteral("MEMO"))))
                                IFOKDO(err, suboperation.save())
                            } else if (m_accounts.contains(a)) {
                                // This is a transfer
                                SKGAccountObject act = m_accounts.value(a);

                                SKGOperationObject operation2;
                                IFOKDO(err, act.addOperation(operation2))

                                QDate date =
                                    SKGServices::stringToDate(SKGServices::dateToSqlString(getVal(vals, QStringLiteral("DATE")), QStringLiteral("MM/DD/YYYY")));
                                IFOKDO(err, operation2.setDate(date))
                                IFOKDO(err,
                                       operation2.setStatus(getVal(vals, QStringLiteral("CLEAR")) == QStringLiteral("Y") ? SKGOperationObject::CHECKED
                                                                                                                         : SKGOperationObject::NONE))

                                QString number = getVal(vals, QStringLiteral("DOCNUM"));
                                if (!number.isEmpty()) {
                                    IFOKDO(err, operation2.setNumber(number))
                                }

                                SKGUnitObject unit;
                                IFOKDO(err, operation.getUnit(unit))
                                IFOKDO(err, operation2.setUnit(unit))

                                IFOKDO(err, operation2.setMode(getVal(vals, QStringLiteral("TRNSTYPE"))))
                                QString p = getVal(vals, QStringLiteral("NAME"));
                                if (!p.isEmpty()) {
                                    IFOKDO(err, operation2.setPayee(m_payees[p]))
                                }
                                IFOKDO(err, operation2.setComment(getVal(vals, QStringLiteral("MEMO"))))
                                IFOKDO(err, operation2.save())

                                SKGSubOperationObject suboperation2;
                                SKGCategoryObject cat = m_categories.value(a);
                                IFOKDO(err, operation2.addSubOperation(suboperation2))
                                IFOKDO(err, suboperation2.setQuantity(SKGServices::stringToDouble(getVal(vals, QStringLiteral("AMOUNT")))))
                                IFOKDO(err, suboperation2.setCategory(cat))
                                IFOKDO(err, suboperation2.setComment(getVal(vals, QStringLiteral("MEMO"))))
                                IFOKDO(err, suboperation2.save())

                                IFOKDO(err, operation2.setGroupOperation(operation))
                                IFOKDO(err, operation2.save())

                                IFOKDO(err, operation.addSubOperation(suboperation))
                                IFOKDO(err, suboperation.setQuantity(-SKGServices::stringToDouble(getVal(vals, QStringLiteral("AMOUNT")))))
                                IFOKDO(err, suboperation.setCategory(cat))
                                IFOKDO(err, suboperation.setComment(getVal(vals, QStringLiteral("MEMO"))))
                                IFOKDO(err, suboperation.save())
                            }
                        }
                    }

                    ++index;
                    if (!err && index % 500 == 0) {
                        err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                    }
                }

                // Lines treated
                IFOKDO(err, m_importer->getDocument()->stepForward(3))
            }

            // close file
            file.close();
        }
    }
    SKGENDTRANSACTION(m_importer->getDocument(), err)

    return err;
}

bool SKGImportPluginIif::isExportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("IIF"));
}

SKGError SKGImportPluginIif::exportFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Read parameters
    auto listUUIDs = SKGServices::splitCSVLine(m_exportParameters.value(QStringLiteral("uuid_of_selected_accounts_or_operations")));

    QStringList listOperationsToExport;
    listOperationsToExport.reserve(listUUIDs.count());
    QStringList listAccountsToExport;
    listAccountsToExport.reserve(listUUIDs.count());
    for (const auto &uuid : std::as_const(listUUIDs)) {
        if (uuid.endsWith(QStringLiteral("-operation"))) {
            listOperationsToExport.push_back(uuid);
        } else if (uuid.endsWith(QStringLiteral("-account"))) {
            listAccountsToExport.push_back(uuid);
        }
    }

    if ((listAccountsToExport.count() != 0) || (listOperationsToExport.count() != 0)) {
        IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Only selected accounts and transactions have been exported")))
    }

    // Open file
    QSaveFile file(m_importer->getLocalFileName(false));
    if (!file.open(QIODevice::WriteOnly)) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", m_importer->getFileName().toDisplayString()));
    } else {
        QTextStream stream(&file);
        if (!m_importer->getCodec().isEmpty()) {
            stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
        }

        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export %1 file", "IIF"), 4);
        IFOK(err)
        {
            // Export accounts
            stream << "!ACCNT\tNAME\tACCNTTYPE\tDESC\tACCNUM\tEXTRA\n";
            SKGObjectBase::SKGListSKGObjectBase accounts;
            IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_account_display"), QStringLiteral("1=1 ORDER BY t_name, id"), accounts))
            int nbaccount = accounts.count();
            if (!err && (nbaccount != 0)) {
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export accounts"), nbaccount);
                for (int i = 0; !err && i < nbaccount; ++i) {
                    SKGAccountObject act(accounts.at(i));
                    if (listAccountsToExport.isEmpty() || listAccountsToExport.contains(act.getUniqueID())) {
                        SKGAccountObject::AccountType type = act.getType();
                        stream << "ACCNT" << '\t' << act.getName() << '\t'
                               << (type == SKGAccountObject::ASSETS ? "FIXASSET"
                                                                    : (type == SKGAccountObject::LOAN             ? "OCLIAB"
                                                                           : type == SKGAccountObject::CREDITCARD ? "CCARD"
                                                                                                                  : "BANK"))
                               << '\t' << act.getComment() << '\t' << act.getAgencyNumber() << '\t' << Qt::endl;
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            // Export categories
            SKGObjectBase::SKGListSKGObjectBase categories;
            IFOKDO(err,
                   m_importer->getDocument()->getObjects(QStringLiteral("v_category_display_tmp"), QStringLiteral("1=1 ORDER BY t_fullname, id"), categories))
            int nbcat = categories.count();
            if (!err && (nbcat != 0)) {
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export categories"), nbcat);
                for (int i = 0; !err && i < nbcat; ++i) {
                    SKGCategoryObject cat(categories.at(i));
                    QString catName = cat.getFullName();
                    if (!catName.isEmpty()) {
                        stream << "ACCNT" << '\t' << catName.replace(OBJECTSEPARATOR, QStringLiteral(":")) << '\t'
                               << (SKGServices::stringToDouble(cat.getAttribute(QStringLiteral("f_REALCURRENTAMOUNT"))) < 0 ? "EXP" : "INC") << '\t' << '\t'
                               << '\t' << Qt::endl;
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(2))

            // Export payees
            stream << "!CUST\tNAME\tBADDR1\n";
            SKGObjectBase::SKGListSKGObjectBase payees;
            IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_payee_display"), QStringLiteral("1=1 ORDER BY t_name, id"), payees))
            int nbpayee = payees.count();
            if (!err && (nbpayee != 0)) {
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export payees"), nbpayee);
                for (int i = 0; !err && i < nbpayee; ++i) {
                    SKGPayeeObject payee(payees.at(i));
                    stream << "CUST" << '\t' << payee.getName() << '\t' << payee.getAddress() << Qt::endl;
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(3))

            // Export transactions
            stream << "!TRNS\tTRNSID\tTRNSTYPE\tDATE\tACCNT\tNAME\tCLASS\tAMOUNT\tDOCNUM\tCLEAR\tMEMO\n";
            stream << "!SPL\tSPLID\tTRNSTYPE\tDATE\tACCNT\tNAME\tCLASS\tAMOUNT\tDOCNUM\tCLEAR\tMEMO\n";
            stream << "!ENDTRNS\n";
            SKGObjectBase::SKGListSKGObjectBase transactions;
            IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_operation_display"), QStringLiteral("1=1 ORDER BY d_date, id"), transactions))
            int nbop = transactions.count();
            if (!err && (nbop != 0)) {
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export transactions"), nbop);
                for (int i = 0; !err && i < nbop; ++i) {
                    SKGOperationObject op(transactions.at(i));
                    SKGAccountObject a;
                    op.getParentAccount(a);
                    if ((listOperationsToExport.isEmpty() || listOperationsToExport.contains(op.getUniqueID()))
                        && (listAccountsToExport.isEmpty() || listAccountsToExport.contains(a.getUniqueID()))) {
                        stream << "TRNS" << '\t' << SKGServices::intToString(op.getID()) << '\t' << "PAYMENT" << '\t'
                               << op.getDate().toString(QStringLiteral("M/d/yyyy")) << '\t' << op.getAttribute(QStringLiteral("t_ACCOUNT")) << '\t'
                               << op.getAttribute(QStringLiteral("t_PAYEE")) << '\t'
                               << op.getAttribute(QStringLiteral("t_CATEGORY")).replace(OBJECTSEPARATOR, QStringLiteral(":")) << '\t'
                               << SKGServices::doubleToString(op.getAmount(QDate::currentDate())) << '\t' << op.getNumber() << '\t'
                               << (op.getStatus() == SKGOperationObject::CHECKED ? QStringLiteral("Y") : QStringLiteral("N")) << '\t' << op.getComment()
                               << Qt::endl;

                        // Export sub transactions
                        SKGObjectBase::SKGListSKGObjectBase sops;
                        err = op.getSubOperations(sops);
                        int nbsops = sops.count();
                        for (int j = 0; !err && j < nbsops; ++j) {
                            SKGSubOperationObject sop(sops.at(j));
                            SKGCategoryObject cat;
                            sop.getCategory(cat);
                            stream << "SPL" << '\t' << SKGServices::intToString(sop.getID()) << '\t' << "PAYMENT" << '\t'
                                   << op.getDate().toString(QStringLiteral("M/d/yyyy")) << '\t'
                                   << cat.getFullName().replace(OBJECTSEPARATOR, QStringLiteral(":")) << '\t' << op.getAttribute(QStringLiteral("t_PAYEE"))
                                   << '\t' << '\t' << SKGServices::doubleToString(-sop.getQuantity()) << '\t' << op.getNumber() << '\t'
                                   << (op.getStatus() == SKGOperationObject::CHECKED ? QStringLiteral("Y") : QStringLiteral("N")) << '\t' << sop.getComment()
                                   << Qt::endl;
                        }
                    }
                    stream << "ENDTRNS" << Qt::endl;
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(4))

            SKGENDTRANSACTION(m_importer->getDocument(), err)
        }
    }

    // Close file
    file.commit();
    return err;
}

QString SKGImportPluginIif::getMimeTypeFilter() const
{
    return "*.iif|" % i18nc("A file format", "Intuit Interchange Format file");
}

#include "skgimportpluginiif.moc"
