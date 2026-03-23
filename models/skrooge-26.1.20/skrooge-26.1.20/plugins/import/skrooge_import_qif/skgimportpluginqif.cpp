/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for QIF import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginqif.h"

#include <klocalizedstring.h>

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
#define OPENINGBALANCE QStringLiteral("Opening Balance")

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginQif, "metadata.json")

SKGImportPluginQif::SKGImportPluginQif(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)

    m_importParameters[QStringLiteral("date_format")] = QString();
    m_exportParameters[QStringLiteral("uuid_of_selected_accounts_or_operations")] = QString();
}

SKGImportPluginQif::~SKGImportPluginQif() = default;

bool SKGImportPluginQif::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return isExportPossible();
}

SKGError SKGImportPluginQif::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }

    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Begin transaction
    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "QIF"), 3);
    IFOK(err)
    {
        // Create account if needed
        QDateTime now = QDateTime::currentDateTime();
        QString postFix = SKGServices::dateToSqlString(now);

        // Step 1 done
        IFOKDO(err, m_importer->getDocument()->stepForward(1))

        // Open file
        QFile file(m_importer->getLocalFileName());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
        } else {
            QTextStream stream(&file);
            if (!m_importer->getCodec().isEmpty()) {
                stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
            }

            // load file in memory
            QStringList lines;
            QStringList dates;
            bool inWrongSection = false;
            bool inPriceSection = false;
            while (!stream.atEnd()) {
                // Read line
                // Check line if line is empty or is a commented
                QString line = stream.readLine().trimmed().toUtf8();
                if (!line.isEmpty() && line[0] != '#') {
                    lines.push_back(line);
                    // Manage !Account section
                    if (line.startsWith(QStringLiteral("!"))) {
                        inWrongSection = false;
                        inPriceSection = false;
                    }

                    if (QString::compare(line, QStringLiteral("!account"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:payee"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:budget"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:cat"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:tag"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:class"), Qt::CaseInsensitive) == 0) {
                        inWrongSection = true;
                    } else if (QString::compare(line, QStringLiteral("!type:prices"), Qt::CaseInsensitive) == 0) {
                        inPriceSection = true;
                    }

                    // We try to find automatically the date format
                    if (!inWrongSection && line[0] == 'D') {
                        dates.push_back(line.right(line.length() - 1));
                    } else if (inPriceSection) {
                        QStringList vals = SKGServices::splitCSVLine(line, ',');
                        if (vals.count() == 3) {
                            dates.push_back(vals.at(2));
                        }
                    }
                }
            }

            // close file
            file.close();

            // Select dateformat
            QString dateFormat = m_importParameters.value(QStringLiteral("date_format"));
            if (dateFormat.isEmpty()) {
                dateFormat = SKGServices::getDateFormat(dates); // Automatic detection
            }
            if (dateFormat.isEmpty()) {
                err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "Date format not supported"));
            }
            IFOKDO(err,
                   m_importer->getDocument()->sendMessage(i18nc("An information message",
                                                                "Import of '%1' with code '%2' and date format '%3'",
                                                                m_importer->getFileName().toDisplayString(),
                                                                m_importer->getCodec(),
                                                                dateFormat)))

            // Step 2 done
            IFOKDO(err, m_importer->getDocument()->stepForward(2))

            // Treat all lines
            IFOK(err)
            {
                SKGAccountObject *account = nullptr;
                SKGOperationObject currentOperation;
                SKGOperationObject payement;
                SKGPayeeObject currentPayee;
                SKGTrackerObject currentTracker;
                SKGUnitObject currentUnit;
                SKGSubOperationObject currentSubOperation;
                QDate currentOperationDate;
                QString lastTransferAccount;
                QList<QString> transferAccount;
                QList<double> transferQuantity;
                bool addNextAmountToTransferQuantity = false;
                QString stringForHash;
                QString currentUnitForInvestment;
                QChar inSection = 'B';
                bool currentOperationInitialized = false;
                bool latestSubCatMustBeRemoved = false;
                bool investmentAccount = false;
                bool div = false;
                bool automaticAccount = true;
                int quantityFactor = 1;
                double currentUnitPrice = 1;
                double checkOperationAmount = 0;
                double checkSuboperationsAmount = 0;
                bool openingbalancecreated = false;

                int nb = lines.size();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    QString line = lines.at(i);
                    QString val;
                    QChar op = line[0];
                    if (line.length() > 1) {
                        val = line.right(line.length() - 1).trimmed();
                    }

                    // Manage !Account section
                    if (QString::compare(line, QStringLiteral("!type:bank"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:payee"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:budget"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:cash"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:ccard"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:oth a"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:oth l"), Qt::CaseInsensitive) == 0
                        || QString::compare(line, QStringLiteral("!type:invst"), Qt::CaseInsensitive) == 0) {
                        inSection = 'B';
                        openingbalancecreated = false;
                        investmentAccount = (QString::compare(val, QStringLiteral("type:invst"), Qt::CaseInsensitive) == 0);

                        // Set type of account
                        if (account == nullptr) {
                            SKGAccountObject defAccount;
                            err = m_importer->getDefaultAccount(defAccount);
                            IFOKDO(err, defAccount.addOperation(currentOperation, true))
                            IFOK(err) account = new SKGAccountObject(defAccount);
                        }

                        if (!err && (account != nullptr) && account->getType() == SKGAccountObject::OTHER) {
                            err = account->setType(QString::compare(line, QStringLiteral("!type:bank"), Qt::CaseInsensitive) == 0
                                                       ? SKGAccountObject::CURRENT
                                                       : (QString::compare(line, QStringLiteral("!type:cash"), Qt::CaseInsensitive) == 0
                                                              ? SKGAccountObject::WALLET
                                                              : (QString::compare(line, QStringLiteral("!type:ccard"), Qt::CaseInsensitive) == 0
                                                                     ? SKGAccountObject::CREDITCARD
                                                                     : (QString::compare(line, QStringLiteral("!type:invst"), Qt::CaseInsensitive) == 0
                                                                            ? SKGAccountObject::INVESTMENT
                                                                            : (QString::compare(line, QStringLiteral("!type:oth a"), Qt::CaseInsensitive) == 0
                                                                                   ? SKGAccountObject::ASSETS
                                                                                   : SKGAccountObject::OTHER)))));
                            IFOKDO(err, account->save())
                        }
                    } else if (QString::compare(line, QStringLiteral("!account"), Qt::CaseInsensitive) == 0) {
                        inSection = 'A';
                        openingbalancecreated = false;
                        automaticAccount = false;
                    } else if (QString::compare(line, QStringLiteral("!type:cat"), Qt::CaseInsensitive) == 0) {
                        inSection = 'C';
                        openingbalancecreated = false;
                        IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Categories found and imported")))
                    } else if (QString::compare(line, QStringLiteral("!type:prices"), Qt::CaseInsensitive) == 0) {
                        inSection = 'U';
                        openingbalancecreated = false;
                        IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Units prices found and imported")))
                    } else if (QString::compare(line, QStringLiteral("!type:security"), Qt::CaseInsensitive) == 0) {
                        inSection = 'S';
                        IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Units found and imported")))
                    } else if (QString::compare(line, QStringLiteral("!type:tag"), Qt::CaseInsensitive) == 0) {
                        inSection = 'T';
                        IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Trackers found and imported")))
                    } else if (line.at(0) == '!') {
                        inSection = '?';
                        openingbalancecreated = false;
                    } else if (inSection == 'U') {
                        // Unit value creation
                        openingbalancecreated = false;
                        QStringList vals = SKGServices::splitCSVLine(line, ',');
                        if (vals.count() == 3 && !vals.at(0).isEmpty()) {
                            err =
                                m_importer->getDocument()->addOrModifyUnitValue(vals.at(0),
                                                                                SKGServices::stringToDate(SKGServices::dateToSqlString(vals.at(2), dateFormat)),
                                                                                SKGServices::stringToDouble(vals.at(1)));
                        }

                    } else if (inSection == 'T') {
                        // Tracker creation
                        if (op == 'N') {
                            IFOKDO(err, SKGTrackerObject::createTracker(m_importer->getDocument(), val, currentTracker))
                        } else if (op == 'D') {
                            IFOKDO(err, currentTracker.setComment(val))
                            IFOKDO(err, currentTracker.save())
                        }
                    } else if (inSection == 'S') {
                        // Unit creation
                        if (op == 'N') {
                            currentUnit = SKGUnitObject(m_importer->getDocument());
                            IFOKDO(err, currentUnit.setName(val))
                            IFOKDO(err, currentUnit.setSymbol(val))
                            IFOKDO(err, currentUnit.setType(SKGUnitObject::CURRENCY))
                            IFOKDO(err, currentUnit.setNumberDecimal(2))
                            IFOKDO(err, currentUnit.save())
                        } else if (op == 'S') {
                            IFOKDO(err, currentUnit.setSymbol(val))
                            IFOKDO(err, currentUnit.save())
                        } else if (op == 'T') {
                            if (QString::compare(val, QStringLiteral("stock"), Qt::CaseInsensitive) == 0) {
                                IFOKDO(err, currentUnit.setType(SKGUnitObject::SHARE))
                                IFOKDO(err, currentUnit.setNumberDecimal(4))
                                IFOKDO(err, currentUnit.save())
                            }
                        }
                    } else if (inSection == 'C') {
                        // Category creation
                        openingbalancecreated = false;
                        if (op == 'N') {
                            SKGCategoryObject Category;
                            val.replace('/', OBJECTSEPARATOR);
                            val.replace(':', OBJECTSEPARATOR);
                            err = SKGCategoryObject::createPathCategory(m_importer->getDocument(), val, Category);
                        }
                    } else if (inSection == 'A') {
                        // Account creation
                        openingbalancecreated = false;
                        if (op == 'N') {
                            // Check if the account already exist
                            SKGAccountObject account2;
                            err = SKGNamedObject::getObjectByName(m_importer->getDocument(), QStringLiteral("account"), val, account2);
                            IFKO(err)
                            {
                                // Create account
                                SKGBankObject bank(m_importer->getDocument());
                                err = bank.setName(i18nc("Noun", "Bank for import %1", postFix));
                                if (!err && bank.load().isFailed()) {
                                    err = bank.save(false);
                                }
                                IFOKDO(err, bank.addAccount(account2))
                                IFOKDO(err, account2.setName(val))
                                if (!err && account2.load().isFailed()) {
                                    err = account2.save(false); // Save only
                                }
                            }

                            IFOK(err)
                            {
                                delete account;
                                account = new SKGAccountObject(account2);
                            }
                        } else if (op == 'D') {
                            if (account != nullptr) {
                                err = account->setNumber(val);
                            }
                        } else if (op == 'T') {
                            if (account != nullptr) {
                                err = account->setType(
                                    val == QStringLiteral("Bank")
                                        ? SKGAccountObject::CURRENT
                                        : (val == QStringLiteral("CCard")
                                               ? SKGAccountObject::CREDITCARD
                                               : (val == QStringLiteral("Invst")
                                                      ? SKGAccountObject::INVESTMENT
                                                      : (val == QStringLiteral("Oth A") ? SKGAccountObject::ASSETS : SKGAccountObject::OTHER))));
                            }
                        } else if (op == '^') {
                            // ^     End of entry
                            // save
                            if (account != nullptr) {
                                err = account->save();
                            }
                        }
                    } else if (inSection == 'B') {
                        // Transaction creation
                        /*
                        >>>> Items for Non-Investment Accounts <<<<
                        DONE    D      Date
                        DONE    T      Amount
                            U      Transaction amount (higher possible value than T)
                        DONE    C      Cleared status
                        DONE    N      Number (check or reference number)
                        DONE    P      Payee/description
                        DONE    M      Memo
                        DONE    A      Address (up to 5 lines; 6th line is an optional message)
                        DONE    L      Category (category/class or transfer/class)
                        DONE    S      Category in split (category/class or transfer/class)
                        DONE    E      Memo in split
                        DONE    $      Dollar amount of split
                            %      Percentage of split if percentages are used
                            F      Reimbursable business expense flag
                            X      Small Business extensions
                        DONE    ^      End of entry

                        >>>> Items for Investment Accounts <<<<
                        DONE    D   Date
                            N   Action
                        DONE    Y   Security
                        DONE    I   Price
                        DONE    Q   Quantity (number of shares or split ratio)
                        DONE    T   Transaction amount
                        DONE    C   Cleared status
                            P   Text in the first line for transfers and reminders
                        DONE    M   Memo
                            O   Commission
                            L   Account for the transfer
                            $   Amount transferred
                            ^   End of entry
                        */
                        stringForHash += line;
                        if (op == 'D') {
                            // D     Date
                            /*
                            Dates in US QIF files are usually in the format MM/DD/YY, although
                            four-digit years are not uncommon.  Dates sometimes occur without the
                            slash separator, or using other separators in place of the slash,
                            commonly '-' and '.'.  US Quicken seems to be using the ' to indicate
                            post-2000 two-digit years (such as 01/01'00 for Jan 1 2000).  Some
                            banks appear to be using a completely undifferentiated numeric QString
                            formateed YYYYMMDD in downloaded QIF files.
                            */
                            // Transaction creation
                            SKGUnitObject unit;
                            IFOK(err)
                            {
                                if (account != nullptr) {
                                    err = account->addOperation(currentOperation, true);
                                    if (!openingbalancecreated) {
                                        double initBalance;
                                        account->getInitialBalance(initBalance, unit);
                                    }
                                } else {
                                    SKGAccountObject defAccount;
                                    err = m_importer->getDefaultAccount(defAccount);
                                    IFOKDO(err, defAccount.addOperation(currentOperation, true))
                                    if (!openingbalancecreated) {
                                        double initBalance;
                                        defAccount.getInitialBalance(initBalance, unit);
                                    }
                                }
                                currentOperationInitialized = true;
                            }

                            // Set date
                            currentOperationDate = SKGServices::stringToDate(SKGServices::dateToSqlString(val, dateFormat));
                            IFOKDO(err, currentOperation.setDate(currentOperationDate))

                            // Set unit
                            IFOK(err)
                            {
                                // Create unit if needed
                                // If an initial balance is existing for the account then we use the unit else we look for the most appropriate unit
                                if (!unit.exist()) {
                                    err = m_importer->getDefaultUnit(unit, &currentOperationDate);
                                }
                                IFOKDO(err, currentOperation.setUnit(unit))
                            }

                            IFOKDO(err, currentOperation.save())

                            // Create suboperation
                            IFOKDO(err, currentOperation.addSubOperation(currentSubOperation))
                        } else if (op == 'Y') {
                            // Y     Security
                            if (!div) {
                                currentUnitForInvestment = std::move(val);

                                SKGUnitObject unit(m_importer->getDocument());
                                if (currentUnitForInvestment.isEmpty()) {
                                    IFOKDO(err, err = m_importer->getDefaultUnit(unit))
                                } else {
                                    IFOKDO(err, unit.setName(currentUnitForInvestment))
                                    IFOKDO(err, unit.setSymbol(currentUnitForInvestment))
                                    if (unit.load().isFailed()) {
                                        IFOKDO(err, unit.setType(investmentAccount ? SKGUnitObject::SHARE : SKGUnitObject::CURRENCY))
                                        IFOKDO(err, unit.save(false))
                                    }
                                }
                                IFOKDO(err, currentOperation.setUnit(unit))
                            } else {
                                // For dividend, if comment is empty, we set the security in comment
                                if (currentOperation.getComment().isEmpty()) {
                                    err = currentOperation.setComment(val);
                                }
                            }
                        } else if (op == 'O') {
                            // O     Commission
                            // Get previous quantity
                            double quantity = SKGServices::stringToDouble(val);
                            SKGObjectBase::SKGListSKGObjectBase subops;
                            payement.getSubOperations(subops);
                            if (!subops.isEmpty()) {
                                SKGSubOperationObject subpayement(subops.at(0));
                                err = subpayement.setQuantity(subpayement.getQuantity() + quantity);
                                IFOKDO(err, subpayement.save())
                            }

                            SKGSubOperationObject subcommission;
                            if (!payement.exist()) {
                                // We have to create a new transaction
                                if (account != nullptr) {
                                    err = account->addOperation(payement, true);
                                } else {
                                    SKGAccountObject defAccount;
                                    err = m_importer->getDefaultAccount(defAccount);
                                    IFOKDO(err, defAccount.addOperation(payement, true))
                                }
                                IFOKDO(err, payement.setDate(currentOperationDate))
                                IFOK(err)
                                {
                                    // If an initial balance is existing for the account then we use the unit else we look for the most appropriate unit
                                    SKGUnitObject unit;
                                    if ((account != nullptr) && !openingbalancecreated) {
                                        double initBalance;
                                        account->getInitialBalance(initBalance, unit);
                                    }
                                    if (!unit.exist()) {
                                        err = m_importer->getDefaultUnit(unit, &currentOperationDate);
                                    }
                                    IFOKDO(err, payement.setUnit(unit))
                                }
                                IFOKDO(err, payement.save())
                            }
                            IFOKDO(err, payement.addSubOperation(subcommission))
                            IFOKDO(err, subcommission.setQuantity(-quantity))
                            IFOKDO(err, subcommission.save(false, false))
                        } else if (op == 'I') {
                            // I     Price
                            currentUnitPrice = SKGServices::stringToDouble(val);
                            if ((currentUnitPrice != 0.0) && !currentUnitForInvestment.isEmpty()) {
                                err = m_importer->getDocument()->addOrModifyUnitValue(currentUnitForInvestment, currentOperationDate, currentUnitPrice);
                            }
                        } else if (op == 'N') {
                            if (investmentAccount) {
                                // N     Action
                                /*
                                QIF N Line    Notes
                                ============  =====
                                Aktab         Same as ShrsOut.
                                AktSplit      Same as StkSplit.
                                Aktzu         Same as ShrsIn.
                                Buy           Buy shares.
                                BuyX          Buy shares. Used with an L line.
                                Cash          Miscellaneous cash transaction. Used with an L line.
                                CGMid         Mid-term capital gains.
                                CGMidX        Mid-term capital gains. For use with an L line.
                                CGLong        Long-term capital gains.
                                CGLongX       Long-term capital gains. For use with an L line.
                                CGShort       Short-term capital gains.
                                CGShortX      Short-term capital gains. For use with an L line.
                                ContribX      Same as XIn. Used for tax-advantaged accounts.
                                CvrShrt       Buy shares to cover a short sale.
                                CvrShrtX      Buy shares to cover a short sale. Used with an L line.
                                Div           Dividend received.
                                DivX          Dividend received. For use with an L line.
                                Errinerg      Same as Reminder.
                                Exercise      Exercise an option.
                                ExercisX      Exercise an option. For use with an L line.
                                Expire        Mark an option as expired. (Uses D, N, Y & M lines)
                                Grant         Receive a grant of stock options.
                                Int           Same as IntInc.
                                IntX          Same as IntIncX.
                                IntInc        Interest received.
                                IntIncX       Interest received. For use with an L line.
                                K.gewsp       Same as CGShort. (German)
                                K.gewspX      Same as CGShortX. (German)2307068
                                Kapgew        Same as CGLong. Kapitalgewinnsteuer.(German)
                                KapgewX       Same as CGLongX. Kapitalgewinnsteuer. (German)
                                Kauf          Same as Buy. (German)
                                KaufX         Same as BuyX. (German)
                                MargInt       Margin interest paid.
                                MargIntX      Margin interest paid. For use with an L line.
                                MiscExp       Miscellaneous expense.
                                MiscExpX      Miscellaneous expense. For use with an L line.
                                MiscInc       Miscellaneous income.
                                MiscIncX      Miscellaneous income. For use with an L line.
                                ReinvDiv      Reinvested dividend.
                                ReinvInt      Reinvested interest.
                                ReinvLG       Reinvested long-term capital gains.
                                Reinvkur      Same as ReinvLG.
                                Reinvksp      Same as ReinvSh.
                                ReinvMd       Reinvested mid-term capital gains.
                                ReinvSG       Same as ReinvSh.
                                ReinvSh       Reinvested short-term capital gains.
                                Reinvzin      Same as ReinvDiv.
                                Reminder      Reminder. (Uses D, N, C & M lines)
                                RtrnCap       Return of capital.
                                RtrnCapX      Return of capital. For use with an L line.
                                Sell          Sell shares.
                                SellX         Sell shares. For use with an L line.
                                ShtSell       Short sale.
                                ShrsIn        Deposit shares.
                                ShrsOut       Withdraw shares.
                                StkSplit      Share split.
                                Verkauf       Same as Sell. (German)
                                VerkaufX      Same as SellX. (German)
                                Vest          Mark options as vested. (Uses N, Y, Q, C & M lines)
                                WithDrwX      Same as XOut. Used for tax-advantaged accounts.
                                XIn           Transfer cash from another account.
                                XOut          Transfer cash to another account.
                                */
                                val = val.toLower();
                                if (val.contains(QStringLiteral("div")) && val != QStringLiteral("reinvdiv")) {
                                    // TODO(Stephane MANKOWSKI) err=currentOperation.setProperty ( "SKG_OP_ORIGINAL_AMOUNT", "" );
                                    div = true;
                                } else if (val.contains(QStringLiteral("sell")) || val.contains(QStringLiteral("verkauf"))
                                           || val.contains(QStringLiteral("miscexp")) || val.contains(QStringLiteral("shrsout"))) {
                                    quantityFactor = -1;
                                }
                                // Correction 214851 vvvv
                                // err=currentOperation.setComment ( val );
                                // if ( !err ) err=currentOperation.setMode ( i18nc ( "Noun, the title of an item","Title" ) );
                                // Correction 214851 ^^^^
                            } else {
                                // N     Num (check or reference number)
                                // Set number
                                bool ok;
                                int number = val.toInt(&ok);
                                if (ok && number != 0) {
                                    err = currentOperation.setNumber(val);
                                } else {
                                    err = currentOperation.setMode(val);
                                }
                            }
                        } else if (op == 'Q') {
                            // Q     Quantity (number of shares or split ratio)
                            // Set value
                            if (!val.isEmpty()) {
                                double previousQuantity = currentSubOperation.getQuantity();
                                if (previousQuantity != 0.0) {
                                    // We have to create a new transaction
                                    if (account != nullptr) {
                                        err = account->addOperation(payement, true);
                                    } else {
                                        SKGAccountObject defAccount;
                                        err = m_importer->getDefaultAccount(defAccount);
                                        IFOKDO(err, defAccount.addOperation(payement, true))
                                    }
                                    IFOKDO(err, payement.setDate(currentOperationDate))
                                    IFOK(err)
                                    {
                                        // Create unit if needed
                                        // If an initial balance is existing for the account then we use the unit else we look for the most appropriate unit
                                        SKGUnitObject unit;
                                        if ((account != nullptr) && !openingbalancecreated) {
                                            double initBalance;
                                            account->getInitialBalance(initBalance, unit);
                                        }
                                        if (!unit.exist()) {
                                            err = m_importer->getDefaultUnit(unit, &currentOperationDate);
                                        }
                                        IFOKDO(err, payement.setUnit(unit))
                                    }
                                    IFOKDO(err, payement.save())
                                    IFOKDO(err, currentOperation.setGroupOperation(payement))

                                    SKGSubOperationObject subpayement;
                                    IFOKDO(err, payement.addSubOperation(subpayement))
                                    IFOKDO(err, subpayement.setQuantity(-previousQuantity))
                                    IFOKDO(err, subpayement.save())
                                }

                                IFOKDO(err, currentSubOperation.setQuantity(quantityFactor * SKGServices::stringToDouble(val)))
                            }
                        } else if (op == 'T') {
                            // T     Amount
                            // Set value
                            checkOperationAmount = SKGServices::stringToDouble(val);
                            err = currentSubOperation.setQuantity(checkOperationAmount / currentUnitPrice);
                            if (!err && investmentAccount) {
                                err = currentOperation.setProperty(QStringLiteral("SKG_OP_ORIGINAL_AMOUNT"), val);
                            }
                        } else if (op == '$') {
                            // Dollar amount of split
                            // Set value
                            if (!investmentAccount) {
                                double vald = SKGServices::stringToDouble(val);
                                checkSuboperationsAmount += vald;
                                if (addNextAmountToTransferQuantity && !lastTransferAccount.isEmpty()) {
                                    transferQuantity[transferAccount.count() - 1] += vald;
                                }
                                addNextAmountToTransferQuantity = false;
                                lastTransferAccount = QString();
                                err = currentSubOperation.setQuantity(vald);

                                // save
                                IFOKDO(err, currentSubOperation.save())

                                // Create suboperation
                                IFOKDO(err, currentOperation.addSubOperation(currentSubOperation))

                                latestSubCatMustBeRemoved = true;
                            }
                        } else if (op == 'P') {
                            // P Payee
                            // Set Payee
                            // Clean QIF coming from bankperfect
                            val.remove(QStringLiteral("[auto]"));

                            err = SKGPayeeObject::createPayee(m_importer->getDocument(), val, currentPayee);
                            IFOKDO(err, currentOperation.setPayee(currentPayee))
                        } else if (op == 'A') {
                            // A      Address (up to 5 lines; 6th line is an optional message)
                            QString add = currentPayee.getAddress();
                            if (!add.isEmpty()) {
                                add += ' ';
                            }
                            add += val;
                            err = currentPayee.setAddress(add);
                            IFOKDO(err, currentPayee.save())
                        } else if (op == 'M') {
                            // M     Memo
                            // Set Memo
                            err = currentOperation.setComment(val);
                        } else if (op == 'E') {
                            // E     Memo in split
                            // Set Memo
                            err = currentSubOperation.setComment(val);
                        } else if (op == 'S' || op == 'L') {
                            // S     Category in split (Category/Transfer/Class)
                            // L     Category (Category/Subcategory/Transfer/Class)
                            // LCategory of transaction
                            // L[Transfer account]
                            // LCategory of transaction/Class of transaction
                            // L[Transfer account]/Class of transaction// Set Category
                            if (!val.isEmpty()) {
                                if (val[0] == '[') {
                                    addNextAmountToTransferQuantity = true;

                                    int pos = val.indexOf(']');
                                    if (pos != -1) {
                                        SKGPayeeObject payeeObj;
                                        currentOperation.getPayee(payeeObj);
                                        bool opening = (payeeObj.getName().compare(OPENINGBALANCE, Qt::CaseInsensitive) == 0);

                                        // If the very first Bank transaction in the file has a payee of "Opening Balance", the L line contains the name of the
                                        // account that the file describes. This is not a transfer
                                        if (op == 'L' && automaticAccount && (account != nullptr) && opening) {
                                            QString accountName = val.mid(1, pos - 1);

                                            SKGAccountObject newAccount(m_importer->getDocument());
                                            err = newAccount.setName(accountName);
                                            IFOK(err)
                                            {
                                                if (newAccount.exist()) {
                                                    // Oups, the real account is existing and it is another one
                                                    err = newAccount.load();

                                                    // We move the transaction in the right account
                                                    IFOKDO(err, currentOperation.setParentAccount(newAccount))
                                                    IFOKDO(err, currentOperation.save())

                                                    // We delete the previous account if empty
                                                    IFOK(err)
                                                    {
                                                        if (account->getNbOperation() == 0) {
                                                            err = account->remove();
                                                        }
                                                        delete account;
                                                        account = new SKGAccountObject(newAccount);
                                                    }
                                                } else {
                                                    err = account->setName(accountName);
                                                    IFOKDO(err, account->save())
                                                }
                                            }
                                        }
                                        //                                            if ( op=='L' && currentOperation.getPayee().compare ( "Opening Balance",
                                        //                                            Qt::CaseInsensitive ) !=0 && !investmentAccount)
                                        if (!opening) {
                                            lastTransferAccount = val.mid(1, pos - 1);
                                            if ((account != nullptr) && lastTransferAccount == account->getName()) {
                                                lastTransferAccount = QString();
                                            }

                                            if (!lastTransferAccount.isEmpty()
                                                && (transferAccount.count() == 0 || transferAccount.at(transferAccount.count() - 1) != lastTransferAccount
                                                    || transferQuantity.at(transferQuantity.count() - 1) != 0.0)) {
                                                transferAccount.append(lastTransferAccount);
                                                transferQuantity.append(0.0);
                                            }
                                        }
                                        val = val.mid(pos + 2);
                                    }
                                }
                                if (!err && !val.isEmpty()) {
                                    SKGCategoryObject Category;
                                    val.replace(':', OBJECTSEPARATOR);
                                    val.replace(',', OBJECTSEPARATOR);
                                    val.replace(';', OBJECTSEPARATOR);
                                    err = SKGCategoryObject::createPathCategory(m_importer->getDocument(), val, Category);
                                    IFOKDO(err, currentSubOperation.setCategory(Category))
                                }
                            }
                        } else if (op == 'C') {
                            // C     Cleared status
                            // Set status
                            err = currentOperation.setStatus(
                                (val == QStringLiteral("C") || val == QStringLiteral("*")
                                     ? SKGOperationObject::MARKED
                                     : (val == QStringLiteral("R") || val == QStringLiteral("X") ? SKGOperationObject::CHECKED : SKGOperationObject::NONE)));
                        } else if (op == '^') {
                            // ^     End of entry
                            // save

                            if (currentOperationInitialized) {
                                QByteArray hash = QCryptographicHash::hash(stringForHash.toUtf8(), QCryptographicHash::Md5);
                                SKGPayeeObject payeeObj;
                                currentOperation.getPayee(payeeObj);
                                bool opening = (payeeObj.getName().compare(OPENINGBALANCE, Qt::CaseInsensitive) == 0);
                                if (!err && opening) {
                                    // Specific values for initial balance
                                    err = currentOperation.setStatus(SKGOperationObject::CHECKED);
                                    IFOKDO(err, currentOperation.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
                                    IFOKDO(err, currentSubOperation.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
                                    openingbalancecreated = true;
                                }

                                IFOKDO(err, currentOperation.setImportID(hash.toHex()))
                                IFOKDO(err, currentOperation.save())
                                if (!latestSubCatMustBeRemoved && !err) {
                                    err = currentSubOperation.save();
                                }

                                // Create transfers if needed
                                // Get origin op
                                SKGOperationObject opOrigin(m_importer->getDocument(), currentOperation.getID());
                                SKGAccountObject accountOrigin;
                                IFOKDO(err, opOrigin.getParentAccount(accountOrigin))
                                int nbTransfers = transferAccount.count();
                                for (int j = 0; !err && j < nbTransfers; ++j) {
                                    bool merged = false;
                                    double tq = transferQuantity.at(j);
                                    const QString &ta = transferAccount.at(j);
                                    // Is the transfert transaction already existing?
                                    double qua = tq == 0.0 && addNextAmountToTransferQuantity
                                        ? SKGServices::stringToDouble(opOrigin.getAttribute(QStringLiteral("f_QUANTITY")))
                                        : tq;
                                    QString wc = "t_ACCOUNT='" % SKGServices::stringToSqlString(ta) %
                                                 "' AND t_TOACCOUNT='" % SKGServices::stringToSqlString(accountOrigin.getName()) %
                                                 "' AND ABS(f_QUANTITY-(" % SKGServices::doubleToString(-qua) % "))<0.0001"
                                                 " AND ABS(julianday(d_date) - julianday('" % SKGServices::dateToSqlString(opOrigin.getDate()) % "'))<1"
                                                 " ORDER BY ABS(julianday(d_date) - julianday('" % SKGServices::dateToSqlString(opOrigin.getDate()) % "')) ASC";
                                    SKGObjectBase::SKGListSKGObjectBase obs;
                                    m_importer->getDocument()->getObjects(QStringLiteral("v_operation_display"), wc, obs);
                                    if (!obs.isEmpty()) {
                                        // We have to merge them and we do not need to create the transfer
                                        SKGOperationObject firstOne(obs.at(0));

                                        // Remove all transaction attached to this transfer
                                        SKGObjectBase::SKGListSKGObjectBase list;
                                        IFOKDO(err, firstOne.getGroupedOperations(list))
                                        for (const auto &o : std::as_const(list)) {
                                            SKGOperationObject op2(o);
                                            if (op2 != firstOne && op2 != currentOperation) {
                                                IFOKDO(err, op2.setStatus(SKGOperationObject::NONE))
                                                IFOKDO(err, op2.remove(false, true))
                                            }
                                        }

                                        // Attach myself
                                        IFOKDO(err, currentOperation.setGroupOperation(firstOne))
                                        IFOKDO(err, currentOperation.save())

                                        merged = true;
                                    } else {
                                        // Is the transaction already created as a transfer of an other one?
                                        QString wc = "t_import_id='QIF TRANSFER-" % SKGServices::stringToSqlString(ta) % "' AND t_ACCOUNT='" % SKGServices::stringToSqlString(accountOrigin.getName()) %
                                                     "' AND (ABS(f_CURRENTAMOUNT-(" % SKGServices::doubleToString(opOrigin.getCurrentAmount()) % "))<0.0001 OR f_QUANTITY=" % SKGServices::doubleToString(qua) % ")"
                                                     " AND ABS(julianday(d_date) - julianday('" % SKGServices::dateToSqlString(opOrigin.getDate()) % "'))<1"
                                                     " ORDER BY ABS(julianday(d_date) - julianday('" % SKGServices::dateToSqlString(opOrigin.getDate()) % "')) ASC";
                                        m_importer->getDocument()->getObjects(QStringLiteral("v_operation_display"), wc, obs);
                                        if (!obs.isEmpty()) {
                                            // We have to merge them and we do not need to create the transfer
                                            SKGOperationObject firstOne(obs.at(0));
                                            err = opOrigin.setStatus(SKGOperationObject::NONE); // To be sure we can delete it
                                            IFOKDO(err, opOrigin.save())
                                            IFOKDO(err, firstOne.mergeAttribute(opOrigin))

                                            SKGObjectBase::SKGListSKGObjectBase list;
                                            IFOKDO(err, currentOperation.getGroupedOperations(list))
                                            for (const auto &o : std::as_const(list)) {
                                                SKGOperationObject op2(o);
                                                IFOKDO(err, op2.setStatus(SKGOperationObject::NONE))
                                                IFOKDO(err, op2.remove(false, true))
                                            }
                                            merged = true;
                                        }
                                    }

                                    if (!merged) {
                                        // Create target account if needed
                                        SKGAccountObject accountTransfer(m_importer->getDocument());
                                        if (m_accountCache.contains(ta)) {
                                            accountTransfer = m_accountCache[ta];
                                        } else {
                                            accountTransfer.setName(ta);
                                            if (!accountTransfer.exist()) {
                                                // The account is created in the same bank by default
                                                SKGBankObject bankOrigin;
                                                IFOKDO(err, accountOrigin.getBank(bankOrigin))
                                                IFOKDO(err, accountTransfer.setBank(bankOrigin))
                                                IFOKDO(err, accountTransfer.save(false, true))
                                            } else {
                                                err = accountTransfer.load();
                                            }

                                            m_accountCache[ta] = accountTransfer;
                                        }

                                        // Create operation
                                        SKGUnitObject unit;
                                        opOrigin.getUnit(unit);

                                        SKGOperationObject opTransfer;
                                        IFOKDO(err, accountTransfer.addOperation(opTransfer, true))
                                        IFOKDO(err, opTransfer.setDate(opOrigin.getDate()))
                                        IFOKDO(err, opTransfer.setComment(opOrigin.getComment()))
                                        SKGPayeeObject payeeObj2;
                                        opTransfer.getPayee(payeeObj2);
                                        IFOKDO(err, opTransfer.setPayee(payeeObj2))
                                        IFOKDO(err, opTransfer.setStatus(opOrigin.getStatus()))
                                        IFOKDO(err, opTransfer.setUnit(unit))
                                        IFOKDO(err, opTransfer.setImportID("QIF TRANSFER-" % accountOrigin.getName()))
                                        IFOKDO(err, opTransfer.save()) // save needed before setGroupOperation
                                        IFOKDO(err, opTransfer.setGroupOperation(opOrigin))
                                        IFOKDO(err, opOrigin.load()) // Must be reload because of setGroupOperation modified it
                                        IFOKDO(err, opTransfer.save())

                                        SKGSubOperationObject subopTransfer;
                                        IFOKDO(err, opTransfer.addSubOperation(subopTransfer))
                                        IFOKDO(err, subopTransfer.setQuantity(-qua))
                                        IFOKDO(err, subopTransfer.save())
                                    }
                                }
                            }

                            // Check Sum($)=T for incident 214462
                            QString checkOperationAmountString = SKGServices::doubleToString(checkOperationAmount);
                            QString checkSuboperationsAmountString = SKGServices::doubleToString(checkSuboperationsAmount);
                            if (!err && checkOperationAmount != 0 && checkSuboperationsAmount != 0
                                && checkOperationAmountString != checkSuboperationsAmountString) {
                                SKGSubOperationObject suboprepair;
                                IFOKDO(err, currentOperation.addSubOperation(suboprepair))
                                IFOKDO(err, suboprepair.setQuantity(checkOperationAmount - checkSuboperationsAmount))
                                IFOKDO(err, suboprepair.setComment(i18nc("An information message", "Auto repaired transaction")))
                                IFOKDO(err, suboprepair.save())

                                IFOKDO(err,
                                       m_importer->getDocument()->sendMessage(i18nc("An information message",
                                                                                    "The total amount of the transaction (%1) was different to the sum of the "
                                                                                    "sub-transactions (%2). The transaction has been repaired.",
                                                                                    checkOperationAmountString,
                                                                                    checkSuboperationsAmountString),
                                                                              SKGDocument::Warning))
                            }

                            // Initialize variables
                            currentOperationInitialized = false;
                            latestSubCatMustBeRemoved = false;
                            currentUnitForInvestment = QString();
                            quantityFactor = 1;
                            currentUnitPrice = 1;
                            stringForHash = QString();
                            checkOperationAmount = 0;
                            checkSuboperationsAmount = 0;
                            lastTransferAccount = QString();
                            transferAccount.clear();
                            transferQuantity.clear();
                            payement = SKGOperationObject();
                        } else {
                            // A    Address (up to five lines; the sixth line is an optional message)
                        }
                    }

                    if (!err && i % 500 == 0) {
                        err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                delete account;
                account = nullptr;
                SKGENDTRANSACTION(m_importer->getDocument(), err)

                // Lines treated
                IFOKDO(err, m_importer->getDocument()->stepForward(3))
            }
        }
    }
    SKGENDTRANSACTION(m_importer->getDocument(), err)

    return err;
}

bool SKGImportPluginQif::isExportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("QIF"));
}

SKGError SKGImportPluginQif::exportFile()
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

        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export %1 file", "QIF"), 3);
        IFOK(err)
        {
            // Export categories
            SKGObjectBase::SKGListSKGObjectBase categories;
            IFOKDO(err,
                   m_importer->getDocument()->getObjects(QStringLiteral("v_category_display_tmp"), QStringLiteral("1=1 ORDER BY t_fullname, id"), categories))
            int nbcat = categories.count();
            if (!err && (nbcat != 0)) {
                stream << "!Type:Cat\n";
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export categories"), nbcat);
                for (int i = 0; !err && i < nbcat; ++i) {
                    SKGCategoryObject cat(categories.at(i));
                    QString catName = cat.getFullName();
                    if (!catName.isEmpty()) {
                        stream << QStringLiteral("N") << catName.replace(OBJECTSEPARATOR, QStringLiteral(":")) << Qt::endl;
                        if (SKGServices::stringToDouble(cat.getAttribute(QStringLiteral("f_REALCURRENTAMOUNT"))) < 0) {
                            stream << "E" << Qt::endl;
                        } else {
                            stream << "I" << Qt::endl;
                        }
                        stream << "^" << Qt::endl;
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            SKGServices::SKGUnitInfo primaryUnit = m_importer->getDocument()->getPrimaryUnit();

            // Get transactions
            QString currentAccountName;
            SKGObjectBase::SKGListSKGObjectBase transactions;
            IFOKDO(err,
                   m_importer->getDocument()->getObjects(QStringLiteral("v_operation_display_all"),
                                                         QStringLiteral("t_template='N' ORDER BY t_ACCOUNT, d_date, id"),
                                                         transactions))
            int nb = transactions.count();
            IFOK(err)
            {
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export transactions"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    SKGOperationObject operation(transactions.at(i));
                    SKGAccountObject a;
                    operation.getParentAccount(a);
                    if ((listOperationsToExport.isEmpty() || listOperationsToExport.contains(operation.getUniqueID()))
                        && (listAccountsToExport.isEmpty() || listAccountsToExport.contains(a.getUniqueID()))) {
                        // Get account name
                        QString accountName = operation.getAttribute(QStringLiteral("t_ACCOUNT"));

                        // In the same account ?
                        if (accountName != currentAccountName) {
                            SKGAccountObject account(m_importer->getDocument());
                            account.setName(accountName);
                            account.load();

                            SKGBankObject bank;
                            account.getBank(bank);

                            // Write header
                            stream << "!Account\n";
                            stream << 'N' << accountName << Qt::endl;
                            QString type =
                                (account.getType() == SKGAccountObject::CURRENT
                                     ? QStringLiteral("Bank")
                                     : (account.getType() == SKGAccountObject::CREDITCARD
                                            ? QStringLiteral("CCard")
                                            : (account.getType() == SKGAccountObject::INVESTMENT
                                                   ? QStringLiteral("Invst")
                                                   : (account.getType() == SKGAccountObject::ASSETS ? QStringLiteral("Oth A") : QStringLiteral("Cash")))));
                            stream << 'T' << type << Qt::endl;
                            QString number = bank.getNumber();
                            QString bnumber = account.getAgencyNumber();
                            QString cnumber = account.getNumber();
                            if (!bnumber.isEmpty()) {
                                if (!number.isEmpty()) {
                                    number += '-';
                                }
                                number += bnumber;
                            }
                            if (!cnumber.isEmpty()) {
                                if (!number.isEmpty()) {
                                    number += '-';
                                }
                                number += cnumber;
                            }
                            stream << 'D' << number << Qt::endl;
                            // stream << "/"      Statement balance date
                            // stream << "$"      Statement balance amount
                            stream << '^' << Qt::endl;
                            currentAccountName = accountName;

                            stream << "!Type:" << type << "\n";
                        }

                        // Write operation
                        /*
                        DONE    D      Date
                        DONE    T      Amount
                        N/A U      Transaction amount (higher possible value than T)
                        DONE    C      Cleared status
                        DONE    N      Number (check or reference number)
                        DONE    P      Payee/description
                        DONE    M      Memo
                        N/A A      Address (up to 5 lines; 6th line is an optional message)
                        DONE    L      Category (category/class or transfer/class)
                        DONE    S      Category in split (category/class or transfer/class)
                        DONE    E      Memo in split
                        DONE    $      Dollar amount of split
                        N/A %      Percentage of split if percentages are used
                        N/A F      Reimbursable business expense flag
                        N/A X      Small Business extensions
                        DONE    Y      Security
                        DONE    I      Price
                        DONE    Q      Quantity (number of shares or split ratio)
                        N/A    O      Commission
                        DONE    ^      End of entry
                        */
                        SKGUnitObject unit;
                        operation.getUnit(unit);
                        bool investment = false;
                        bool unitExported = false;
                        if (unit.getSymbol() != primaryUnit.Symbol && !primaryUnit.Symbol.isEmpty()) {
                            unitExported = true;
                        }
                        if (unit.getType() == SKGUnitObject::SHARE) {
                            unitExported = true;
                            investment = true;
                        }

                        QString date = SKGServices::dateToSqlString(operation.getDate());
                        if (date.isEmpty()) {
                            // This is an opening balance
                            date = QStringLiteral("0000-00-00");
                        }
                        stream << 'D' << date << Qt::endl;
                        if (!unitExported) {
                            stream << 'T' << SKGServices::doubleToString(operation.getCurrentAmount()) << Qt::endl;
                        }

                        if (!investment) {
                            auto number = operation.getNumber();
                            if (!number.isEmpty()) {
                                stream << 'N' << operation.getNumber() << Qt::endl;
                            }
                        } else {
                            stream << 'N' << (operation.getCurrentAmount() > 0 ? "Buy" : "Sell") << Qt::endl;
                        }

                        if (unitExported) {
                            stream << 'Y' << unit.getSymbol() << Qt::endl;
                        }

                        SKGPayeeObject payeeObj;
                        operation.getPayee(payeeObj);
                        QString payee = payeeObj.getName();
                        QString address = payeeObj.getAddress();
                        if (date == QStringLiteral("0000-00-00")) {
                            payee = OPENINGBALANCE;
                        }
                        if (!payee.isEmpty()) {
                            stream << 'P' << payee << Qt::endl;
                        }
                        if (!address.isEmpty()) {
                            stream << 'A' << address << Qt::endl;
                        }

                        QString memo = operation.getMode() % "  " % operation.getComment();
                        memo = memo.trimmed();
                        if (!memo.isEmpty()) {
                            stream << 'M' << memo << Qt::endl;
                        }

                        SKGOperationObject::OperationStatus status = operation.getStatus();
                        stream << 'C' << (status == SKGOperationObject::MARKED ? "C" : (status == SKGOperationObject::CHECKED ? "R" : "")) << Qt::endl;

                        // Get sub transactions
                        SKGObjectBase::SKGListSKGObjectBase subtransactions;
                        err = operation.getSubOperations(subtransactions);
                        IFOK(err)
                        {
                            int nbSubOps = subtransactions.size();
                            QString category;
                            if (nbSubOps == 1) {
                                SKGSubOperationObject suboperation(subtransactions.at(0));
                                // Dump quantity
                                if (unitExported) {
                                    stream << 'Q' << SKGServices::doubleToString(qAbs(suboperation.getQuantity())) << Qt::endl;
                                    stream << 'I' << SKGServices::doubleToString(qAbs(operation.getCurrentAmount() / suboperation.getQuantity())) << Qt::endl;
                                }

                                // Get category of this simple operation
                                SKGCategoryObject cat;
                                suboperation.getCategory(cat);
                                category = cat.getFullName().replace(OBJECTSEPARATOR, QStringLiteral(":"));
                            }

                            // Is it a transfer
                            SKGOperationObject transfer;
                            if (operation.isTransfer(transfer)) {
                                if (!category.isEmpty()) {
                                    category.prepend('/');
                                }

                                SKGAccountObject transferAccount;
                                err = transfer.getParentAccount(transferAccount);
                                IFOK(err) category.prepend(u'[' % transferAccount.getName() % u']');
                            }
                            if (!category.isEmpty()) {
                                stream << 'L' << category << Qt::endl;
                            }

                            if (nbSubOps > 1) {
                                // Split operation
                                for (int k = 0; k < nbSubOps; ++k) {
                                    SKGSubOperationObject suboperation(subtransactions.at(k));
                                    SKGCategoryObject cat;
                                    suboperation.getCategory(cat);

                                    QString category2 = cat.getFullName().replace(OBJECTSEPARATOR, QStringLiteral(":"));
                                    if (!category2.isEmpty()) {
                                        stream << 'S' << category2 << Qt::endl;
                                    }
                                    QString memo2 = suboperation.getComment();
                                    memo2 = memo2.trimmed();
                                    if (!memo2.isEmpty()) {
                                        stream << 'E' << memo2 << Qt::endl;
                                    }
                                    stream << '$' << SKGServices::doubleToString(suboperation.getQuantity()) << Qt::endl;
                                }
                            }
                        }

                        stream << '^' << Qt::endl;
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(2))

            // Export prices
            SKGObjectBase::SKGListSKGObjectBase unitvalues;
            IFOKDO(err,
                   m_importer->getDocument()->getObjects(QStringLiteral("v_unitvalue"),
                                                         QStringLiteral("1=1 ORDER BY (select t_name from unit where v_unitvalue.rd_unit_id=unit.id), d_date"),
                                                         unitvalues))
            nb = unitvalues.count();
            if (!err && (nb != 0)) {
                stream << "!Type:Prices\n";
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export units"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    SKGUnitValueObject unitVal(unitvalues.at(i));
                    SKGUnitObject unit;
                    err = unitVal.getUnit(unit);
                    IFOK(err)
                    {
                        QStringList vals;
                        QString v = unit.getSymbol();
                        if (v.isEmpty()) {
                            v = unit.getName();
                        }
                        vals.push_back(v);
                        vals.push_back(SKGServices::doubleToString(unitVal.getQuantity()));
                        vals.push_back(SKGServices::dateToSqlString(unitVal.getDate()));

                        stream << SKGServices::stringsToCsv(vals) << Qt::endl;
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }
                stream << "^" << Qt::endl;

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(3))
            SKGENDTRANSACTION(m_importer->getDocument(), err)
        }

        // Close file
        file.commit();
    }

    return err;
}

QString SKGImportPluginQif::getMimeTypeFilter() const
{
    return "*.qif|" % i18nc("A file format", "QIF file");
}

#include <skgimportpluginqif.moc>
