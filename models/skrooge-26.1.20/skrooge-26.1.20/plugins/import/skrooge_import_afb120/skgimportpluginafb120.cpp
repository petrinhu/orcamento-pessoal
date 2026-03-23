/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for AFB120 import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginafb120.h"

#include <kpluginfactory.h>

#include <qcryptographichash.h>
#include <qfile.h>
#include <qmath.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginAFB120, "metadata.json")

SKGImportPluginAFB120::SKGImportPluginAFB120(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginAFB120::~SKGImportPluginAFB120() = default;

bool SKGImportPluginAFB120::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr
                ? true
                : m_importer->getFileNameExtension() == QStringLiteral("AFB120") || m_importer->getFileNameExtension() == QStringLiteral("CFO"));
}

double SKGImportPluginAFB120::toAmount(const QString &iAmount, int iNbDecimal)
{
    QString amount = iAmount;
    QChar lastChar = amount.right(1).at(0);
    int codeAscii = lastChar.toLatin1();
    int sign = (codeAscii > 79 && codeAscii != 123 ? -1 : 1);
    if (codeAscii == 123 || codeAscii == 125) {
        amount[amount.size() - 1] = '0';
    } else {
        bool ok = false;
        amount[amount.size() - 1] =
            QChar(codeAscii + QChar('1').toLatin1() - QString(sign == -1 ? QStringLiteral("0x4A") : QStringLiteral("0x41")).toUInt(&ok, 16));
    }
    return static_cast<double>(sign) * SKGServices::stringToDouble(amount) / qPow(10, iNbDecimal);
}

SKGError SKGImportPluginAFB120::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Begin transaction
    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "AFB120"), 2);
    IFOK(err)
    {
        // Open file
        IFOK(err)
        {
            QFile file(m_importer->getLocalFileName());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
            } else {
                // Read lines
                QStringList lines;
                {
                    QTextStream stream(&file);
                    if (!m_importer->getCodec().isEmpty()) {
                        stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
                    }
                    while (!stream.atEnd()) {
                        // Read line
                        QString line = stream.readLine().trimmed();
                        if (!line.isEmpty()) {
                            lines.push_back(line);
                        }
                    }
                }
                // close file
                file.close();

                // Step 1 done
                IFOKDO(err, m_importer->getDocument()->stepForward(1))

                // Read lines
                SKGAccountObject account;
                SKGUnitObject unit;
                QString bankName = QStringLiteral("AFB120");
                QString inititalAmount;

                int nb = lines.count();
                IFOKDO(err, m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb))
                for (int i = 0; i < nb && !err; ++i) {
                    // Read line
                    const QString &line = lines.at(i);
                    if (!line.isEmpty()) {
                        if (line.startsWith(QStringLiteral("01"))) {
                            // Previous balance
                            QString accountNumber = line.mid(22 - 1, 11);
                            inititalAmount = line.mid(91 - 1, 14);

                            SKGObjectBase::SKGListSKGObjectBase listAccount;
                            err = m_importer->getDocument()->getObjects(QStringLiteral("v_account"), "t_number='" % accountNumber % u'\'', listAccount);
                            IFOK(err)
                            {
                                if (listAccount.count() == 1) {
                                    // Yes ! Only one account found
                                    account = listAccount.at(0);
                                    err = m_importer->getDocument()->sendMessage(
                                        i18nc("An information message", "Using account '%1' for import", account.getName()));
                                } else {
                                    if (listAccount.count() > 1) {
                                        err = m_importer->getDocument()->sendMessage(i18nc("An information message", "More than one possible account found."));
                                    }

                                    SKGBankObject bank(m_importer->getDocument());
                                    IFOKDO(err, bank.setName(bankName))
                                    IFOKDO(err, bank.setNumber(bankName))
                                    if (!err && bank.load().isFailed()) {
                                        err = bank.save();
                                    }
                                    IFOKDO(err, bank.addAccount(account))
                                    IFOKDO(err, account.setName(accountNumber))
                                    IFOKDO(err, account.setNumber(accountNumber))
                                    IFOKDO(err, account.setType(SKGAccountObject::CURRENT))
                                    if (!err && account.load().isFailed()) {
                                        err = account.save();
                                    }
                                    IFOKDO(err,
                                           m_importer->getDocument()->sendMessage(
                                               i18nc("An information message", "Default account '%1' created for import", accountNumber)))
                                }
                            }
                        } else if (line.startsWith(QStringLiteral("04"))) {
                            // Transaction
                            QString unitCode = line.mid(17 - 1, 3);
                            QString nbDecimal = line.mid(20 - 1, 1);
                            // QString codeMode = line.mid(33 - 1, 2);
                            QString dateJJMMAA = line.mid(35 - 1, 6);
                            QString comment = line.mid(49 - 1, 31).trimmed();
                            QString amount = line.mid(91 - 1, 14);

                            // Initialize balance
                            if (unit.getID() == 0) {
                                err = SKGUnitObject::createCurrencyUnit(m_importer->getDocument(), unitCode, unit);
                                if (account.getNbOperation() > 1) {
                                    IFOKDO(err,
                                           m_importer->getDocument()->sendMessage(
                                               i18nc("An information message",
                                                     "The initial balance of '%1' has not been set because some transactions are already existing",
                                                     account.getName()),
                                               SKGDocument::Warning))
                                } else {
                                    // Set initial balance
                                    IFOKDO(err, account.setInitialBalance(toAmount(inititalAmount, SKGServices::stringToInt(nbDecimal)), unit))
                                    IFOKDO(err, account.save())
                                    IFOKDO(err,
                                           m_importer->getDocument()->sendMessage(i18nc("An information message",
                                                                                        "The initial balance of '%1' has been set with AFB120 file content",
                                                                                        account.getName())))
                                }
                            }

                            SKGOperationObject operation;
                            IFOKDO(err, account.addOperation(operation, true))
                            IFOKDO(err, operation.setDate(SKGServices::stringToDate(SKGServices::dateToSqlString(dateJJMMAA, QStringLiteral("DDMMYYYY")))))
                            IFOKDO(err, operation.setUnit(unit))
                            IFOKDO(err, operation.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T")))
                            IFOKDO(err, operation.setComment(comment))
                            QByteArray hash = QCryptographicHash::hash(line.toUtf8(), QCryptographicHash::Md5);
                            IFOKDO(err, operation.setImportID(QStringLiteral("AFB120-") % hash.toHex()))
                            IFOKDO(err, operation.save(false))

                            SKGSubOperationObject subop;
                            IFOKDO(err, operation.addSubOperation(subop))
                            IFOKDO(err, subop.setComment(comment))
                            IFOKDO(err, subop.setQuantity(toAmount(amount, SKGServices::stringToInt(nbDecimal))))
                            IFOKDO(err, subop.save(false, false))
                        }
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }
                SKGENDTRANSACTION(m_importer->getDocument(), err)

                // Step 2 done
                IFOKDO(err, m_importer->getDocument()->stepForward(2))
            }
        }
    }
    SKGENDTRANSACTION(m_importer->getDocument(), err)

    return err;
}

QString SKGImportPluginAFB120::getMimeTypeFilter() const
{
    return "*.afb120 *.cfo|" % i18nc("A file format", "AFB120 file (cfomb)");
}

#include <skgimportpluginafb120.moc>
