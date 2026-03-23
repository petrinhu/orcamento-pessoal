/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for OFX import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginofx.h"

#include <qchar.h>
#include <qcryptographichash.h>
#include <qfile.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGError SKGImportPluginOfx::m_ofxError;
QStringList SKGImportPluginOfx::m_ofxInitialBalanceName;
QList<double> SKGImportPluginOfx::m_ofxInitialBalanceAmount;
QList<QDate> SKGImportPluginOfx::m_ofxInitialBalanceDate;
QMap<QString, SKGAccountObject> SKGImportPluginOfx::m_accounts;
/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginOfx, "metadata.json")

SKGImportPluginOfx::SKGImportPluginOfx(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
    SKGImportPluginOfx::m_accounts.clear();
}

SKGImportPluginOfx::~SKGImportPluginOfx() = default;

bool SKGImportPluginOfx::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr
                ? true
                : m_importer->getFileNameExtension() == QStringLiteral("OFX") || m_importer->getFileNameExtension() == QStringLiteral("QFX"));
}

SKGError SKGImportPluginOfx::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }

    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    if (!QFile(m_importer->getLocalFileName()).exists()) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
    }
    IFOKDO(err, m_importer->getDocument()->beginTransaction(QStringLiteral("#INTERNAL#") % i18nc("Import step", "Import OFX file")))
    IFOK(err)
    {
        SKGImportPluginOfx::m_ofxError = SKGError();
        SKGImportPluginOfx::m_ofxInitialBalanceName.clear();
        SKGImportPluginOfx::m_ofxInitialBalanceAmount.clear();
        SKGImportPluginOfx::m_ofxInitialBalanceDate.clear();

        try {
            // Check file type
            // auto type = libofx_detect_file_type( m_importer->getLocalFileName().toUtf8().data());

            LibofxContextPtr ctx = libofx_get_new_context();

            ofx_set_account_cb(ctx, SKGImportPluginOfx::ofxAccountCallback, m_importer);
            ofx_set_statement_cb(ctx, SKGImportPluginOfx::ofxStatementCallback, m_importer);
            ofx_set_transaction_cb(ctx, SKGImportPluginOfx::ofxTransactionCallback, m_importer);
            // ofx_set_security_cb(ctx, ofxSecurityCallback, this);
            libofx_proc_file(ctx, m_importer->getLocalFileName().toUtf8().data(), AUTODETECT);
            IFOKDO(err, SKGImportPluginOfx::m_ofxError)
            {
                // This is an option ==> no error management
                SKGError err2;
                int nb = SKGImportPluginOfx::m_ofxInitialBalanceName.count();
                for (int i = 0; !err2 && i < nb; ++i) {
                    SKGAccountObject act;
                    err2 = m_importer->getDocument()->getObject(QStringLiteral("v_account"),
                                                                QStringLiteral("t_number='") % SKGServices::stringToSqlString(m_ofxInitialBalanceName.at(i))
                                                                    % u'\'',
                                                                act);

                    // Date of balance
                    auto date = m_ofxInitialBalanceDate.at(i);

                    // Get unit
                    SKGUnitObject unit;
                    if (!err2) {
                        err2 = m_importer->getDefaultUnit(unit);
                    }

                    // Current amount
                    double currentAmount = act.getAmount(date);

                    // Update account
                    if (!err2) {
                        err2 = act.setInitialBalance(m_ofxInitialBalanceAmount.at(i) - currentAmount, unit);
                    }
                    if (!err2) {
                        err2 = act.save();
                    }
                    if (!err2) {
                        err2 =
                            m_importer->getDocument()->sendMessage(i18nc("An information message", "The initial balance of '%1' has been set", act.getName()));
                    }
                }
            }
            libofx_free_context(ctx);
        } catch (...) { // NOLINT(whitespace/parens)
            err = SKGError(ERR_HANDLE, i18nc("Error message", "OFX import failed"));
        }

        SKGImportPluginOfx::m_ofxInitialBalanceName.clear();
        SKGImportPluginOfx::m_ofxInitialBalanceAmount.clear();
        SKGImportPluginOfx::m_ofxInitialBalanceDate.clear();

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

QString SKGImportPluginOfx::getAccountName(OfxAccountData *iAccountData)
{
    SKGTRACEINFUNC(3)
    QString accountNumber;
    if (iAccountData != nullptr) {
        accountNumber = QString::fromUtf8(iAccountData->account_id);
        QString bankNumber = QString::fromUtf8(iAccountData->bank_id);
        // Correction BUG 234771 vvvvv
        accountNumber = accountNumber.trimmed();
        bankNumber = bankNumber.trimmed();
        if (accountNumber.isEmpty()) {
            accountNumber = QString::fromUtf8(iAccountData->account_number);
        }
        // Correction BUG 234771 ^^^^^
        if (accountNumber.startsWith(bankNumber % u' ')) {
            accountNumber = accountNumber.right(accountNumber.length() - bankNumber.length() - 1);
            QStringList splitNumbers = accountNumber.split(u' ');
            if (splitNumbers.count() == 2) {
                accountNumber = splitNumbers.at(1);
            }
        }
    }
    SKGTRACEL(3) << "accountNumber=" << accountNumber << Qt::endl;
    return accountNumber;
}

SKGError SKGImportPluginOfx::getAccount(OfxAccountData *iAccountData, SKGDocumentBank *iDoc, SKGAccountObject &oAccount)
{
    SKGError err;
    SKGTRACEINFUNCRC(3, err)
    if ((iAccountData != nullptr) && (iDoc != nullptr)) {
        // Check if account is already existing
        QString name = getAccountName(iAccountData);
        if (m_accounts.contains(name)) {
            SKGTRACEL(3) << "Found in index" << Qt::endl;
            oAccount = m_accounts[name];
        } else {
            SKGTRACEL(3) << "NOT found in index" << Qt::endl;
            QString wc = QStringLiteral("t_number='") % SKGServices::stringToSqlString(name)
                % QStringLiteral("' OR EXISTS(SELECT 1 FROM parameters WHERE t_uuid_parent=v_account.id||'-account' AND t_name='alias' AND t_value= '")
                % SKGServices::stringToSqlString(name) % QStringLiteral("')");
            err = iDoc->getObject(QStringLiteral("v_account"), wc, oAccount);
        }
    }

    return err;
}

int SKGImportPluginOfx::ofxStatementCallback(struct OfxStatementData data, void *pv) // clazy:exclude=function-args-by-ref
{
    if (SKGImportPluginOfx::m_ofxError) {
        return 0;
    }
    SKGTRACEINFUNCRC(5, SKGImportPluginOfx::m_ofxError)

    auto *impotExporter = static_cast<SKGImportExportManager *>(pv);
    if (impotExporter == nullptr) {
        return 0;
    }
    SKGDocumentBank *doc = impotExporter->getDocument();
    if (doc == nullptr) {
        return 0;
    }

    // // Get data
    OfxAccountData *accountData = data.account_ptr;
    if ((accountData != nullptr) && static_cast<bool>(data.ledger_balance_valid)) {
        // Get account
        SKGAccountObject act;
        SKGImportPluginOfx::m_ofxError = getAccount(accountData, doc, act);
        if (!SKGImportPluginOfx::m_ofxError) {
            impotExporter->addAccountToCheck(act, data.ledger_balance);
            if (act.getNbOperation() > 1) {
                SKGImportPluginOfx::m_ofxError =
                    doc->sendMessage(i18nc("An information message",
                                           "The initial balance of '%1' has not been set because some transactions are already existing",
                                           act.getName()));
            } else {
                auto d = QDate::currentDate();
                if (static_cast<bool>(data.ledger_balance_date_valid)) {
                    QDateTime date_time;
                    date_time.setSecsSinceEpoch(data.ledger_balance_date);
                    d = date_time.date();
                }
                m_ofxInitialBalanceName.push_back(getAccountName(accountData));
                m_ofxInitialBalanceDate.push_back(d);
                m_ofxInitialBalanceAmount.push_back(data.ledger_balance);
            }
        }
    }

    return SKGImportPluginOfx::m_ofxError.getReturnCode();
}

int SKGImportPluginOfx::ofxAccountCallback(struct OfxAccountData data, void *pv)
{
    if (SKGImportPluginOfx::m_ofxError) {
        return 0;
    }
    SKGTRACEINFUNCRC(5, SKGImportPluginOfx::m_ofxError)

    auto *impotExporter = static_cast<SKGImportExportManager *>(pv);
    if (impotExporter == nullptr) {
        return 0;
    }
    SKGDocumentBank *doc = impotExporter->getDocument();
    if (doc == nullptr) {
        return 0;
    }

    SKGObjectBase tmp;
    QString agencyNumber = QString::fromUtf8(data.branch_id);
    QString accountNumber = QString::fromUtf8(data.account_id);
    QString bankNumber = QString::fromUtf8(data.bank_id);
    // Correction BUG 234771 vvvvv
    accountNumber = accountNumber.trimmed();
    bankNumber = bankNumber.trimmed();
    // Correction BUG 234771 ^^^^^
    if (accountNumber.isEmpty()) {
        accountNumber = QString::fromUtf8(data.account_number);
    }
    if (accountNumber.startsWith(bankNumber % u' ')) {
        accountNumber = accountNumber.right(accountNumber.length() - bankNumber.length() - 1);
        QStringList splitNumbers = accountNumber.split(u' ');
        if (splitNumbers.count() == 2) {
            agencyNumber = splitNumbers.at(0);
            accountNumber = splitNumbers.at(1);
        }
    }

    // Check if account is already existing
    SKGAccountObject account;
    SKGImportPluginOfx::m_ofxError = getAccount(&data, doc, account);
    if (!SKGImportPluginOfx::m_ofxError) {
        // Already existing
        account = tmp;
        SKGImportPluginOfx::m_ofxError = impotExporter->setDefaultAccount(&account);
    } else {
        // Not existing
        QString bankId = (static_cast<bool>(data.bank_id_valid) ? QString::fromUtf8(data.bank_id) : QString::fromUtf8(data.broker_id));
        if (!bankId.isEmpty()) {
            bankId = i18nc("Adjective, an unknown item", "Unknown");
        }

        // Check if bank is already existing
        SKGBankObject bank;
        SKGImportPluginOfx::m_ofxError =
            doc->getObject(QStringLiteral("v_bank"), QStringLiteral("t_bank_number='") % SKGServices::stringToSqlString(bankId) % u'\'', tmp);
        if (!SKGImportPluginOfx::m_ofxError) {
            // Already existing
            bank = tmp;
        } else {
            // Create new bank
            bank = SKGBankObject(doc);
            SKGImportPluginOfx::m_ofxError = bank.setName(bankId);
            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError = bank.setNumber(QString::fromUtf8(data.bank_id));
            }
            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError = bank.save();
            }
        }

        // Create new account
        QString name = QString::fromUtf8(data.account_name);
        if (name.isEmpty()) {
            name = QString::fromUtf8(data.account_id);
        } else {
            name = name.remove(QStringLiteral("Bank account "));
        }
        if (name.isEmpty()) {
            name = QString::fromUtf8(data.account_id);
        }

        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = bank.addAccount(account);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = account.setName(name);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = account.setNumber(accountNumber);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = account.setAgencyNumber(agencyNumber);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = account.setComment(QString::fromUtf8(data.account_name));
        }
        SKGAccountObject::AccountType type = SKGAccountObject::CURRENT;
        if (static_cast<bool>(data.account_type_valid)) {
            switch (data.account_type) {
            case OfxAccountData::OFX_CHECKING:
            case OfxAccountData::OFX_SAVINGS:
            case OfxAccountData::OFX_CREDITLINE:
            case OfxAccountData::OFX_CMA:
                type = SKGAccountObject::CURRENT;
                break;
            case OfxAccountData::OFX_MONEYMRKT:
            case OfxAccountData::OFX_INVESTMENT:
                type = SKGAccountObject::INVESTMENT;
                break;
            case OfxAccountData::OFX_CREDITCARD:
                type = SKGAccountObject::CREDITCARD;
                break;
            default:
                break;
            }
        }

        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = account.setType(type);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = account.save();
            m_accounts[accountNumber] = account;
            SKGTRACEL(3) << "Add account '" << accountNumber << "' in index" << Qt::endl;
        }

        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = impotExporter->setDefaultAccount(&account);
        }

        SKGTRACEL(10) << "Account [" << name << "] created" << Qt::endl;
    }

    if (static_cast<bool>(data.currency_valid)) {
        // Check if unit is already existing
        SKGUnitObject unit(doc);
        SKGImportPluginOfx::m_ofxError = doc->getObject(QStringLiteral("v_unit"),
                                                        QStringLiteral("t_name='") % SKGServices::stringToSqlString(QString::fromUtf8(data.currency))
                                                            % QStringLiteral("' OR t_name like '%(")
                                                            % SKGServices::stringToSqlString(QString::fromUtf8(data.currency)) % QStringLiteral(")%'"),
                                                        tmp);
        if (!SKGImportPluginOfx::m_ofxError) {
            // Already existing
            unit = tmp;
            SKGImportPluginOfx::m_ofxError = impotExporter->setDefaultUnit(&unit);
        } else {
            // Create new account
            SKGImportPluginOfx::m_ofxError = unit.setName(QString::fromUtf8(data.currency));
            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError = unit.setSymbol(QString::fromUtf8(data.currency));
            }
            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError = unit.save();
            }

            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError = impotExporter->setDefaultUnit(&unit);
            }
        }
    }
    return SKGImportPluginOfx::m_ofxError.getReturnCode();
}

int SKGImportPluginOfx::ofxTransactionCallback(struct OfxTransactionData data, void *pv) // clazy:exclude=function-args-by-ref
{
    if (SKGImportPluginOfx::m_ofxError) {
        return 0;
    }
    SKGTRACEINFUNCRC(5, SKGImportPluginOfx::m_ofxError)

    auto *impotExporter = static_cast<SKGImportExportManager *>(pv);
    if (impotExporter == nullptr) {
        return 0;
    }
    SKGDocumentBank *doc = impotExporter->getDocument();
    if (doc == nullptr) {
        return 0;
    }

    // Get account
    SKGAccountObject account;
    SKGImportPluginOfx::m_ofxError = getAccount(data.account_ptr, doc, account);
    if (!SKGImportPluginOfx::m_ofxError) {
        // Get transaction date
        QDateTime date_time;
        date_time.setSecsSinceEpoch(static_cast<bool>(data.date_initiated_valid) ? data.date_initiated : data.date_posted);
        QDate date = date_time.date();

        // Get unit
        SKGUnitObject unit;
        SKGImportPluginOfx::m_ofxError = impotExporter->getDefaultUnit(unit);

        // Create id
        QString ID;
        if (static_cast<bool>(data.fi_id_valid)) {
            if (QString::fromUtf8(data.fi_id).count(u'0') != QString::fromUtf8(data.fi_id).size()) {
                ID = QStringLiteral("ID-") % QString::fromUtf8(data.fi_id);
            }
        } else if (static_cast<bool>(data.reference_number_valid)) {
            if (QString::fromUtf8(data.reference_number).count(u'0') != QString::fromUtf8(data.reference_number).size()) {
                ID = QStringLiteral("REF-") % QString::fromUtf8(data.reference_number);
            }
        }

        if (ID.isEmpty()) {
            QByteArray hash = QCryptographicHash::hash(QString(SKGServices::dateToSqlString(date) % SKGServices::doubleToString(data.amount)).toUtf8(),
                                                       QCryptographicHash::Md5);
            ID = QStringLiteral("ID-") % QString::fromLatin1(hash.toHex());
        }

        // Create operation
        SKGOperationObject ope;
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = account.addOperation(ope, true);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = ope.setDate(date);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = ope.setUnit(unit);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = ope.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T"));
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = ope.setImportID(ID);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            QString payeeName;
            if (static_cast<bool>(data.payee_id_valid)) {
                payeeName = QString::fromUtf8(data.payee_id);
            } else if (static_cast<bool>(data.name_valid)) {
                payeeName = QString::fromUtf8(data.name);
            }
            if (!payeeName.isEmpty()) {
                SKGPayeeObject payeeObj;
                SKGImportPluginOfx::m_ofxError = SKGPayeeObject::createPayee(doc, payeeName, payeeObj);
                if (!SKGImportPluginOfx::m_ofxError) {
                    SKGImportPluginOfx::m_ofxError = ope.setPayee(payeeObj);
                }
            }
        }
        if (!SKGImportPluginOfx::m_ofxError && static_cast<bool>(data.memo_valid)) {
            SKGImportPluginOfx::m_ofxError = ope.setComment(QString::fromUtf8(data.memo));
        }
        if (!SKGImportPluginOfx::m_ofxError && static_cast<bool>(data.check_number_valid)) {
            SKGImportPluginOfx::m_ofxError = ope.setNumber(QString::fromUtf8(data.check_number));
        }
        if (!SKGImportPluginOfx::m_ofxError && static_cast<bool>(data.invtransactiontype_valid)) {
            SKGImportPluginOfx::m_ofxError = ope.setMode(i18nc("Noun, the title of an item", "Title"));
        }
        if (!SKGImportPluginOfx::m_ofxError && static_cast<bool>(data.transactiontype_valid)) {
            QString mode;
            if (data.transactiontype == OFX_CREDIT) {
                mode = i18nc("Noun, type of OFX transaction", "Credit");
            } else if (data.transactiontype == OFX_DEBIT) {
                mode = i18nc("Noun, type of OFX transaction", "Debit");
            } else if (data.transactiontype == OFX_INT) {
                mode = i18nc("Noun, type of OFX transaction", "Interest");
            } else if (data.transactiontype == OFX_DIV) {
                mode = i18nc("Noun, type of OFX transaction", "Dividend");
            } else if (data.transactiontype == OFX_FEE) {
                mode = i18nc("Noun, type of OFX transaction", "FI fee");
            } else if (data.transactiontype == OFX_SRVCHG) {
                mode = i18nc("Noun, type of OFX transaction", "Service charge");
            } else if (data.transactiontype == OFX_DEP) {
                mode = i18nc("Noun, type of OFX transaction", "Deposit");
            } else if (data.transactiontype == OFX_ATM) {
                mode = i18nc("Noun, type of OFX transaction", "ATM");
            } else if (data.transactiontype == OFX_POS) {
                mode = i18nc("Noun, type of OFX transaction", "Point of sale");
            } else if (data.transactiontype == OFX_XFER) {
                mode = i18nc("An transaction mode", "Transfer");
            } else if (data.transactiontype == OFX_CHECK) {
                mode = i18nc("An transaction mode", "Check");
            } else if (data.transactiontype == OFX_PAYMENT) {
                mode = i18nc("Noun, type of OFX transaction", "Electronic payment");
            } else if (data.transactiontype == OFX_CASH) {
                mode = i18nc("An transaction mode", "Withdrawal");
            } else if (data.transactiontype == OFX_DIRECTDEP) {
                mode = i18nc("Noun, type of OFX transaction", "Deposit");
            } else if (data.transactiontype == OFX_REPEATPMT) {
                mode = i18nc("Noun, type of OFX transaction", "Repeating payment");
            } else if (data.transactiontype == OFX_DIRECTDEBIT) {
                mode = i18nc("An transaction mode", "Direct debit");
            }

            SKGImportPluginOfx::m_ofxError = ope.setMode(mode);
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = ope.save();
        }

        // Create sub operation
        SKGSubOperationObject subop;
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = ope.addSubOperation(subop);
        }
        if (!SKGImportPluginOfx::m_ofxError && static_cast<bool>(data.amount_valid)) {
            bool mustFlipSign = (data.transactiontype == OFX_DEBIT || data.transactiontype == OFX_FEE || data.transactiontype == OFX_SRVCHG
                                 || data.transactiontype == OFX_PAYMENT || data.transactiontype == OFX_CASH || data.transactiontype == OFX_DIRECTDEBIT
                                 || data.transactiontype == OFX_REPEATPMT)
                && data.amount > 0; // OFX spec version 2.1.1, section 3.2.9.2
            double sign = mustFlipSign ? -1.0 : 1.0;
            double commission = static_cast<bool>(data.commission_valid) ? data.commission : 0;
            SKGImportPluginOfx::m_ofxError = subop.setQuantity(sign * (data.amount + commission));
        }
        if (!SKGImportPluginOfx::m_ofxError && static_cast<bool>(data.memo_valid)) {
            SKGImportPluginOfx::m_ofxError = subop.setComment(QString::fromUtf8(data.memo));
        }
        if (!SKGImportPluginOfx::m_ofxError) {
            SKGImportPluginOfx::m_ofxError = subop.save();
        }

        // Commission
        if (!SKGImportPluginOfx::m_ofxError && static_cast<bool>(data.commission_valid) && static_cast<bool>(data.amount_valid) && data.commission > 0) {
            // Create splitter operation
            SKGSubOperationObject subop2;
            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError = ope.addSubOperation(subop2);
            }
            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError =
                    subop2.setComment(i18nc("Noun, a quantity of money taken by a financial institution to perform an operation", "Commission"));
            }
            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError = subop2.setQuantity(-data.commission);
            }
            if (!SKGImportPluginOfx::m_ofxError) {
                SKGImportPluginOfx::m_ofxError = subop2.save();
            }
        }
    }
    return SKGImportPluginOfx::m_ofxError.getReturnCode();
}

QString SKGImportPluginOfx::getMimeTypeFilter() const
{
    return QStringLiteral("*.ofx *.qfx|") % i18nc("A file format", "OFX file");
}

#include <skgimportpluginofx.moc>
