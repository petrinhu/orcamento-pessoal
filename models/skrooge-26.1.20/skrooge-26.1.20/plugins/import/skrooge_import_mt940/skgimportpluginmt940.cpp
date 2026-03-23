/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for MT940 import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginmt940.h"

#include <qfile.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgobjectbase.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginMT940, "metadata.json")

SKGImportPluginMT940::SKGImportPluginMT940(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginMT940::~SKGImportPluginMT940() = default;

bool SKGImportPluginMT940::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr
                ? true
                : m_importer->getFileNameExtension() == QStringLiteral("MT940") || m_importer->getFileNameExtension() == QStringLiteral("STA"));
}

SKGError SKGImportPluginMT940::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Begin transaction
    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "MT940"), 2);
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
                bool importActivated = false;
                bool in86 = false;
                SKGAccountObject account;
                SKGOperationObject operation;
                SKGUnitObject unit;
                QString bankName = QStringLiteral("MT940");

                int nb = lines.count();
                IFOKDO(err, m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb))
                for (int i = 0; i < nb && !err; ++i) {
                    // Read line
                    const QString &line = lines.at(i);
                    if (!line.isEmpty()) {
                        if (line.startsWith(QStringLiteral(":20:"))) {
                            importActivated = true;
                            in86 = false;
                            bankName = line.right(line.size() - 4);
                        } else if (importActivated) {
                            if (line.startsWith(QStringLiteral(":25:"))) {
                                // Account
                                QStringList vals = SKGServices::splitCSVLine(line.right(line.size() - 4), '/');
                                QString name = QStringLiteral("MT940");
                                QString number = name;
                                if (vals.count() == 2) {
                                    bankName = vals.at(0);
                                    number = vals.at(1);
                                    name = number;
                                } else if (vals.count() == 1) {
                                    number = vals.at(0);
                                    name = number;
                                }

                                // Search if account is already existing
                                SKGObjectBase::SKGListSKGObjectBase listAccount;
                                err = m_importer->getDocument()->getObjects(QStringLiteral("v_account"), "t_number='" % number % u'\'', listAccount);
                                IFOK(err)
                                {
                                    if (listAccount.count() == 1) {
                                        // Yes ! Only one account found
                                        account = listAccount.at(0);
                                        err = m_importer->getDocument()->sendMessage(
                                            i18nc("An information message", "Using account '%1' for import", account.getName()));
                                    } else {
                                        if (listAccount.count() > 1) {
                                            err = m_importer->getDocument()->sendMessage(
                                                i18nc("An information message", "More than one possible account found."));
                                        }

                                        SKGBankObject bank(m_importer->getDocument());
                                        IFOKDO(err, bank.setName(bankName))
                                        IFOKDO(err, bank.setNumber(bankName))
                                        if (!err && bank.load().isFailed()) {
                                            err = bank.save();
                                        }
                                        IFOKDO(err, bank.addAccount(account))
                                        IFOKDO(err, account.setName(name))
                                        IFOKDO(err, account.setNumber(number))
                                        IFOKDO(err, account.setType(SKGAccountObject::CURRENT))
                                        if (!err && account.load().isFailed()) {
                                            err = account.save();
                                        }
                                        IFOKDO(err,
                                               m_importer->getDocument()->sendMessage(
                                                   i18nc("An information message", "Default account '%1' created for import", name)))
                                    }
                                }

                                in86 = false;
                            } else if (line.startsWith(QStringLiteral(":28C:"))) {
                                in86 = false;
                            } else if (line.startsWith(QStringLiteral(":60F:")) || line.startsWith(QStringLiteral(":60M:"))) {
                                QString val = line.right(line.size() - 5);
                                err = SKGUnitObject::createCurrencyUnit(m_importer->getDocument(), val.mid(7, 3), unit);
                                // Example C040802EUR16,40
                                if (account.getNbOperation() > 1) {
                                    IFOKDO(err,
                                           m_importer->getDocument()->sendMessage(
                                               i18nc("An information message",
                                                     "The initial balance of '%1' has not been set because some transactions are already existing",
                                                     account.getName()),
                                               SKGDocument::Warning))
                                } else {
                                    // Set initial balance
                                    IFOKDO(
                                        err,
                                        account.setInitialBalance((val[0] == 'C' ? 1.0 : -1.0) * SKGServices::stringToDouble(val.right(val.size() - 10)), unit))
                                    IFOKDO(err, account.save())
                                    IFOKDO(err,
                                           m_importer->getDocument()->sendMessage(i18nc("An information message",
                                                                                        "The initial balance of '%1' has been set with MT940 file content",
                                                                                        account.getName())))
                                }

                                in86 = false;
                            } else if (line.startsWith(QStringLiteral(":62F:")) || line.startsWith(QStringLiteral(":62M:"))) {
                                in86 = false;
                            } else if (line.startsWith(QStringLiteral(":61:"))) {
                                // Example :61:0712280103D000000000200,00FMSCNONREF
                                QString val = line.right(line.size() - 4);
                                QDate date = QDate::fromString(val.left(6), QStringLiteral("yyMMdd"));
                                if (date.year() < 1970) {
                                    date = date.addYears(100);
                                }

                                int index = (val[10] == 'R' ? 11 : 10);
                                double sign = (val[index] == 'C' ? 1.0 : -1.0);
                                ++index;

                                if (val[index] == 'R') {
                                    ++index;
                                }

                                QString amountString;
                                while (true) {
                                    if ((val[index] >= '0' && val[index] <= '9') || val[index] == '-' || val[index] == ',') {
                                        amountString += val[index];
                                        ++index;
                                    } else {
                                        break;
                                    }
                                }

                                err = account.addOperation(operation, true);
                                IFOKDO(err, operation.setDate(date))
                                IFOKDO(err, operation.setUnit(unit))
                                IFOKDO(err, operation.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T")))
                                // if(!err) err = operation.setImportID("MT940");
                                IFOKDO(err, operation.save())

                                SKGSubOperationObject subop;
                                err = operation.addSubOperation(subop);
                                IFOKDO(err, subop.setQuantity(sign * SKGServices::stringToDouble(amountString)))
                                IFOKDO(err, subop.save())

                                in86 = false;
                            } else if (line.startsWith(QStringLiteral(":NS:")) && !operation.exist()) {
                                QString val = line.right(line.size() - 4);
                                QString comment = account.getComment();
                                comment += (comment.isEmpty() || val.isEmpty() ? "" : " ") % val;
                                IFOKDO(err, account.setComment(comment))
                                IFOKDO(err, account.save())
                            } else if (line.startsWith(QStringLiteral(":86:")) || line.startsWith(QStringLiteral(":NS:"))) {
                                QString val = line.right(line.size() - 4);
                                QString comment = operation.getComment();
                                comment += (comment.isEmpty() || val.isEmpty() ? "" : " ") % val;
                                IFOKDO(err, operation.setComment(comment))
                                IFOKDO(err, operation.save())

                                in86 = true;
                            } else if (in86) {
                                const QString &val = line;
                                QString comment = operation.getComment();
                                comment += (comment.isEmpty() || val.isEmpty() ? "" : " ") % val;
                                IFOKDO(err, operation.setComment(comment))
                                IFOKDO(err, operation.save())
                            }
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

QString SKGImportPluginMT940::getMimeTypeFilter() const
{
    return "*.sta *.mt940|" % i18nc("A file format", "MT940 file");
}

#include <skgimportpluginmt940.moc>
