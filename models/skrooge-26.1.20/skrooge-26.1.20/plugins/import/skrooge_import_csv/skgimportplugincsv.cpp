/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for CSV import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugincsv.h"

#include <klocalizedstring.h>

#include <kpluginfactory.h>

#include <qcryptographichash.h>
#include <qfileinfo.h>
#include <qregularexpression.h>
#include <qsavefile.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgservices.h"
#include "skgtraces.h"
#include <utility>

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginCsv, "metadata.json")

SKGImportPluginCsv::SKGImportPluginCsv(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
    , m_csvSeparator(QChar())
    , m_csvHeaderIndex(-1)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)

    m_importParameters[QStringLiteral("mapping_date")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and translate the word", "^date");
    m_importParameters[QStringLiteral("mapping_account")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and translate the word", "^account");
    m_importParameters[QStringLiteral("mapping_number")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and | and translate the words", "^number|^num?ro");
    m_importParameters[QStringLiteral("mapping_mode")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and | and translate the words", "^mode|^type");
    m_importParameters[QStringLiteral("mapping_payee")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and | and translate the words", "^payee|^tiers");
    m_importParameters[QStringLiteral("mapping_comment")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and | and translate the words",
              "^comment|^libell?|^d?tail|^info");
    m_importParameters[QStringLiteral("mapping_status")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and | and translate the words", "^status|^pointage");
    m_importParameters[QStringLiteral("mapping_bookmarked")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and translate the word", "^bookmarked");
    m_importParameters[QStringLiteral("mapping_category")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and translate the word", "^cat\\w*gor\\w*");
    m_importParameters[QStringLiteral("mapping_amount")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and | and translate the words",
              "^value|^amount|^valeur|^montant|^credit|^debit");
    m_importParameters[QStringLiteral("mapping_quantity")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and translate the word", "^quantity");
    m_importParameters[QStringLiteral("mapping_unit")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and translate the word", "^unit");
    m_importParameters[QStringLiteral("mapping_sign")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and | and translate the words", "^sign|^sens");
    m_importParameters[QStringLiteral("mapping_debit")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and | and translate the words", "^-|^debit|^withdrawal");
    m_importParameters[QStringLiteral("mapping_idgroup")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and translate the word", "^idgroup");
    m_importParameters[QStringLiteral("mapping_idtransaction")] =
        i18nc("This is a regular expression to find the column in a csv file. You should keep the ^ and translate the word", "^idtransaction");
    m_importParameters[QStringLiteral("mapping_property")] = QString();

    m_importParameters[QStringLiteral("automatic_search_header")] = 'Y';
    m_importParameters[QStringLiteral("header_position")] = '1';
    m_importParameters[QStringLiteral("automatic_search_columns")] = 'Y';
    m_importParameters[QStringLiteral("columns_positions")] = QString();

    m_importParameters[QStringLiteral("mode_csv_unit")] = 'N';
    m_importParameters[QStringLiteral("mode_csv_rule")] = 'N';

    m_importParameters[QStringLiteral("date_format")] = QString();
}

SKGImportPluginCsv::~SKGImportPluginCsv() = default;

void SKGImportPluginCsv::setImportParameters(const QMap<QString, QString> &iParameters)
{
    SKGImportPlugin::setImportParameters(iParameters);

    if (m_importParameters.value(QStringLiteral("automatic_search_header")) == QStringLiteral("N")) {
        int header_position = SKGServices::stringToInt(m_importParameters.value(QStringLiteral("header_position")));
        setCSVHeaderIndex(header_position);
    }

    if (m_importParameters.value(QStringLiteral("automatic_search_columns")) == QStringLiteral("N")) {
        QStringList columns_positions = m_importParameters.value(QStringLiteral("columns_positions")).split('|');
        if (m_csvHeaderIndex == -1) {
            m_csvHeaderIndex = 1;
        }
        setCSVMapping(&columns_positions);
    }
}

bool SKGImportPluginCsv::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return isExportPossible();
}

QStringList SKGImportPluginCsv::getCSVMappingFromLine(const QString &iLine)
{
    QStringList output;
    QString line = iLine.trimmed();

    // Split first line
    QStringList csvAttributes = SKGServices::splitCSVLine(line, getCSVSeparator(iLine), true);
    int nb = csvAttributes.count();

    for (int i = 0; i < nb; ++i) {
        QString att = csvAttributes.at(i).toLower();

        // Search if this csv column is mapping a std attribute
        QMapIterator<QString, QString> csvMapperIterator(m_importParameters);
        bool found = false;
        while (!found && csvMapperIterator.hasNext()) {
            csvMapperIterator.next();

            QString key = csvMapperIterator.key();
            if (key.startsWith(QStringLiteral("mapping_"))) {
                key = key.right(key.length() - 8);
                if (key != QStringLiteral("debit") && key != QStringLiteral("property") && !csvMapperIterator.value().isEmpty()
                    && QRegularExpression(csvMapperIterator.value(), QRegularExpression::CaseInsensitiveOption).match(att).hasMatch()
                    && (!output.contains(key) || key == QStringLiteral("comment") || key == QStringLiteral("category") || key == QStringLiteral("amount"))) {
                    output.push_back(key);
                    found = true;
                }
            }
        }

        // Search if this csv column must be added as a property
        if (!found && !m_importParameters.value(QStringLiteral("mapping_property")).isEmpty()
            && QRegularExpression(m_importParameters.value(QStringLiteral("mapping_property")), QRegularExpression::CaseInsensitiveOption).match(att).hasMatch()
            && !output.contains(att)) {
            output.push_back(att);
            found = true;
        }

        if (!found) {
            output.push_back(QString()); // To ignore this column
        }
    }
    return output;
}

SKGError SKGImportPluginCsv::setCSVMapping(const QStringList *iCSVMapping)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    m_csvMapping.clear();

    if (iCSVMapping == nullptr) {
        // Automatic build
        // Open file
        QFile file(m_importer->getLocalFileName());
        if (Q_UNLIKELY(!file.open(QIODevice::ReadOnly))) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
        } else {
            QTextStream stream(&file);
            if (!m_importer->getCodec().isEmpty()) {
                stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
            }

            // Ignore useless lines
            int headerIndex = getCSVHeaderIndex();
            for (int i = 1; i < headerIndex; ++i) {
                stream.readLine();
            }

            // Get mapping
            if (!stream.atEnd()) {
                m_csvMapping = getCSVMappingFromLine(stream.readLine());
            } else {
                err.setReturnCode(ERR_INVALIDARG);
            }

            // close file
            file.close();
        }
    } else {
        // Manual build
        m_csvMapping = *iCSVMapping;
    }

    IFOK(err)
    {
        // Check if mandatory attributes have been found
        if (m_importParameters.value(QStringLiteral("mode_csv_rule")) == QStringLiteral("Y")) {
            if (!m_csvMapping.contains(QStringLiteral("payee")) || !m_csvMapping.contains(QStringLiteral("category"))) {
                err = SKGError(ERR_FAIL,
                               i18nc("Error message",
                                     "Columns payee and category not found. Set import parameters in settings (Configure Skrooge… / Import/Export / CSV / Edit "
                                     "regular expressions…)."));
            }
        } else {
            if (!m_csvMapping.contains(QStringLiteral("date")) || !m_csvMapping.contains(QStringLiteral("amount"))) {
                err = SKGError(ERR_FAIL,
                               i18nc("Error message",
                                     "Columns date and amount not found. Set import parameters in settings (Configure Skrooge… / Import/Export / CSV / Edit "
                                     "regular expressions…)."));
            }
        }
    }

    return err;
}

SKGError SKGImportPluginCsv::setCSVHeaderIndex(int iIndex)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    if (iIndex == -1) {
        // Automatic build
        // Open file
        QFile file(m_importer->getLocalFileName());
        if (Q_UNLIKELY(!file.open(QIODevice::ReadOnly))) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
        } else {
            QTextStream stream(&file);
            if (!m_importer->getCodec().isEmpty()) {
                stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
            }

            int i = 1;
            m_csvHeaderIndex = -1;
            while (!stream.atEnd() && m_csvHeaderIndex == -1) {
                // Read line
                QStringList map = getCSVMappingFromLine(stream.readLine());
                if (m_importParameters.value(QStringLiteral("mode_csv_rule")) == QStringLiteral("Y")) {
                    if (map.contains(QStringLiteral("payee")) && map.contains(QStringLiteral("category"))) {
                        m_csvHeaderIndex = i;
                    }
                } else {
                    if (map.contains(QStringLiteral("date")) && map.contains(QStringLiteral("amount"))) {
                        m_csvHeaderIndex = i;
                    }
                }

                ++i;
            }

            // close file
            file.close();
        }
    } else {
        // Manual build
        m_csvHeaderIndex = iIndex;
    }

    return err;
}

int SKGImportPluginCsv::getCSVHeaderIndex()
{
    SKGTRACEINFUNC(10)
    if (m_csvHeaderIndex == -1) {
        setCSVHeaderIndex(-1);
    }
    return m_csvHeaderIndex;
}

QChar SKGImportPluginCsv::getCSVSeparator(const QString &iLine)
{
    SKGTRACEINFUNC(10)
    if (m_csvSeparator.isNull()) {
        QStringList csvAttributes = SKGServices::splitCSVLine(iLine, ';', true, &m_csvSeparator);
        int nb = csvAttributes.count();

        // If the split fails, we try with another separator
        if (nb == 1) {
            csvAttributes = SKGServices::splitCSVLine(iLine, ',', true, &m_csvSeparator);
            nb = csvAttributes.count();

            // If the split fails, we try with another separator
            if (nb == 1) {
                csvAttributes = SKGServices::splitCSVLine(iLine, '\t', true, &m_csvSeparator);
            }
        }
    }
    return m_csvSeparator;
}

SKGError SKGImportPluginCsv::importFile()
{
    if (m_importParameters.value(QStringLiteral("mode_csv_rule")) == QStringLiteral("Y")) {
        return importCSVRule();
    }
    if (m_importParameters.value(QStringLiteral("mode_csv_unit")) == QStringLiteral("Y")) {
        return importCSVUnit();
    }
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }

    SKGError err;
    SKGTRACEINFUNCRC(2, err)
    // Begin transaction
    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "CSV"), 3);
    IFOK(err)
    {
        // Initialize some variables
        QDateTime now = QDateTime::currentDateTime();
        QString postFix = SKGServices::dateToSqlString(now);

        // Default mapping
        if (m_csvMapping.isEmpty()) {
            err = setCSVMapping(nullptr);
            IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Use automatic search of the columns")))
        }
        IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Mapping used: %1", m_csvMapping.join(QStringLiteral("|")))))

        // Step 1 done
        IFOKDO(err, m_importer->getDocument()->stepForward(1))

        // Open file
        IFOK(err)
        {
            QFile file(m_importer->getLocalFileName());
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
            } else {
                QTextStream stream(&file);
                if (!m_importer->getCodec().isEmpty()) {
                    stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
                }

                // Ignore useless lines
                int headerIndex = getCSVHeaderIndex();
                if (headerIndex == -1) {
                    err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "Header not found in CSV file"));
                }

                for (int i = 1; i <= headerIndex; ++i) {
                    stream.readLine();
                }

                QList<int> amountIndexes;
                int nb = m_csvMapping.count();
                for (int i = 0; i < nb; ++i) {
                    if (m_csvMapping.at(i) == QStringLiteral("amount")) {
                        amountIndexes.push_back(i);
                    }
                }
                int nbAmount = amountIndexes.count();

                // Get data column
                QStringList dates;
                QStringList lines;
                int posdate = -1;
                bool modeAutoCreditDebit = (!m_csvMapping.contains(QStringLiteral("sign")) && nbAmount == 2);
                IFOK(err)
                {
                    posdate = m_csvMapping.indexOf(QStringLiteral("date"));
                    if (posdate == -1) {
                        posdate = m_csvMapping.indexOf(QStringLiteral("date1"));
                    }
                    if (posdate == -1) {
                        posdate = m_csvMapping.indexOf(QStringLiteral("date2"));
                    }
                    if (posdate != -1) {
                        QString currentLine;
                        while (!stream.atEnd()) {
                            // Read line
                            QString line = stream.readLine().trimmed();
                            if (!line.isEmpty()) {
                                if (line.startsWith(QStringLiteral("\""))) {
                                    currentLine.clear();
                                }
                                currentLine += line;

                                // Get date
                                QStringList field = SKGServices::splitCSVLine(currentLine, getCSVSeparator(currentLine));
                                if (field.isEmpty()) {
                                    // This is a multi line csv filename
                                } else {
                                    lines.push_back(currentLine);
                                    currentLine.clear();

                                    if (field.count() >= m_csvMapping.count()) {
                                        if (posdate < field.count()) {
                                            dates.push_back(field.value(posdate).remove(QStringLiteral(" 00:00:00")).trimmed());
                                        }

                                        // Check if all amounts are positive
                                        if (modeAutoCreditDebit) {
                                            for (int i = 0; modeAutoCreditDebit && i < nbAmount; ++i) {
                                                QString s = field.at(amountIndexes.at(i)).trimmed();
                                                if (!s.isEmpty() && SKGServices::stringToDouble(s, true) < 0) {
                                                    modeAutoCreditDebit = false;
                                                }
                                            }
                                        }
                                    }
                                }
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
                if (!err) {
                    int posdate2 = m_csvMapping.indexOf(QStringLiteral("date2"));
                    if (dateFormat.isEmpty()) {
                        // Check if another "date" attribute exists
                        if (posdate2 != -1) {
                            m_csvMapping[posdate] = QString();

                            err = importFile();

                            SKGENDTRANSACTION(m_importer->getDocument(), err)
                            return err;
                        }
                        err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "Date format not supported"));
                    } else {
                        // Disable other date columns
                        if (posdate2 != -1 && posdate2 != posdate) {
                            m_csvMapping[posdate2] = QString();
                        }
                    }
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
                    int nb2 = lines.size();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb2);

                    // Save last mapping used in a settings
                    QString mappingDesc;
                    int nbMap = m_csvMapping.count();
                    for (int i = 0; i < nbMap; ++i) {
                        if (i != 0) {
                            mappingDesc += '|';
                        }
                        mappingDesc += m_csvMapping.at(i);
                    }
                    IFOKDO(err, m_importer->getDocument()->setParameter(QStringLiteral("SKG_LAST_CSV_MAPPING_USED"), mappingDesc))

                    SKGUnitObject defUnit;
                    SKGAccountObject defAccount;
                    QMap<int, SKGOperationObject> mapGroup;
                    QMap<int, SKGOperationObject> mapOperation;
                    bool noUnitColumn = (m_csvMapping.indexOf(QStringLiteral("unit")) == -1);
                    bool noAccountColumn = (m_csvMapping.indexOf(QStringLiteral("account")) == -1);
                    bool emptyAccount = false;

                    for (int i = 0; !err && i < nb2; ++i) {
                        QString currentCategory;
                        SKGOperationObject currentOperation(m_importer->getDocument());
                        SKGSubOperationObject currentSubOperation(m_importer->getDocument());

                        // Valuate mandatory attribute with default value
                        if (noUnitColumn && i == 0) {
                            err = m_importer->getDefaultUnit(defUnit);
                        }
                        IFOKDO(err, currentOperation.setUnit(defUnit))

                        if (!err && noAccountColumn) {
                            err = m_importer->getDefaultAccount(defAccount);
                            if (i == 0) {
                                emptyAccount = (defAccount.getNbOperation() == 0);
                            }
                            IFOKDO(err, currentOperation.setParentAccount(defAccount))
                        }

                        const QString &line = lines.at(i);
                        QString skg_op_original_amount;
                        QStringList atts = SKGServices::splitCSVLine(line, getCSVSeparator(line));
                        int nbcol = m_csvMapping.count();
                        if (atts.count() < nbcol) {
                            if (i == nb2 - 1) {
                                // This is a footer (see 408284)
                                break;
                            }
                            err = SKGError(ERR_INVALIDARG,
                                           i18nc("Error message",
                                                 "Invalid number of columns in line %1. Expected %2. Found %3.",
                                                 headerIndex + i + 1,
                                                 nbcol,
                                                 atts.count()));
                        }
                        QString linecleaned;
                        for (int z = 0; !err && z < nbcol; ++z) {
                            if (!m_csvMapping[z].isEmpty()) {
                                if (!linecleaned.isEmpty()) {
                                    linecleaned += getCSVSeparator();
                                }
                                linecleaned += atts[z];
                            }
                        }

                        QByteArray hash = QCryptographicHash::hash(linecleaned.toUtf8(), QCryptographicHash::Md5);

                        int initialBalance = 0;
                        int idgroup = 0;
                        int idtransaction = 0;
                        QStringList propertiesAtt;
                        QStringList propertiesVal;
                        double sign = (modeAutoCreditDebit ? -1.0 : 1.0);
                        bool amountSet = false;
                        for (int c = 0; !err && c < nbcol; ++c) {
                            QString col = m_csvMapping[c];
                            if (!col.isEmpty()) {
                                QString val;
                                if (c >= 0 && c < atts.count()) {
                                    val = atts.at(c).trimmed();
                                }
                                if (col == QStringLiteral("date") || col == QStringLiteral("date1") || col == QStringLiteral("date2")) {
                                    QDate d = SKGServices::stringToDate(SKGServices::dateToSqlString(val.remove(QStringLiteral(" 00:00:00")), dateFormat));
                                    err = currentOperation.setDate(d);
                                    IFOKDO(err, currentSubOperation.setDate(d))
                                    if (val == QStringLiteral("0000-00-00")) {
                                        initialBalance = 1;
                                    }
                                } else if (col == QStringLiteral("number")) {
                                    if (!val.isEmpty()) {
                                        err = currentOperation.setNumber(val);
                                    }
                                } else if (col == QStringLiteral("mode")) {
                                    if (val == QStringLiteral("1")) {
                                        val = i18nc("An transaction mode", "Transfer");
                                    } else if (val == QStringLiteral("2")) {
                                        val = i18nc("An transaction mode", "Direct debit");
                                    } else if (val == QStringLiteral("3")) {
                                        val = i18nc("An transaction mode", "Check");
                                    } else if (val == QStringLiteral("4")) {
                                        val = i18nc("An transaction mode", "Deposit");
                                    } else if (val == QStringLiteral("5")) {
                                        val = i18nc("An transaction mode", "Payback");
                                    } else if (val == QStringLiteral("6")) {
                                        val = i18nc("An transaction mode", "Withdrawal");
                                    } else if (val == QStringLiteral("7")) {
                                        val = i18nc("An transaction mode", "Card");
                                    } else if (val == QStringLiteral("8")) {
                                        val = i18nc("An transaction mode", "Loan payment");
                                    } else if (val == QStringLiteral("9")) {
                                        val = i18nc("An transaction mode", "Subscription");
                                    } else if (val == QStringLiteral("0")) {
                                        val = QString();
                                    } else if (val == QStringLiteral("10")) {
                                        val = i18nc("An transaction mode", "Cash deposit");
                                    } else if (val == QStringLiteral("11")) {
                                        val = i18nc("An transaction mode", "Card summary");
                                    } else if (val == QStringLiteral("12")) {
                                        val = i18nc("An transaction mode", "Deferred card");
                                    }
                                    err = currentOperation.setMode(val);
                                } else if (col == QStringLiteral("payee")) {
                                    SKGPayeeObject payeeObj;
                                    err = SKGPayeeObject::createPayee(m_importer->getDocument(), val, payeeObj);
                                    IFOKDO(err, currentOperation.setPayee(payeeObj))
                                } else if (col == QStringLiteral("comment")) {
                                    QString comment = currentOperation.getComment();
                                    if (!comment.isEmpty()) {
                                        comment += ' ';
                                    }
                                    comment += val;
                                    err = currentOperation.setComment(comment);
                                    IFOKDO(err, currentSubOperation.setComment(comment))
                                } else if (col == QStringLiteral("status")) {
                                    err = currentOperation.setStatus(val == QStringLiteral("C") || val == QStringLiteral("Y") ? SKGOperationObject::CHECKED
                                                                         : val == QStringLiteral("P")                         ? SKGOperationObject::MARKED
                                                                                                                              : SKGOperationObject::NONE);
                                } else if (col == QStringLiteral("bookmarked")) {
                                    err = currentOperation.bookmark(val == QStringLiteral("Y"));
                                } else if (col == QStringLiteral("idgroup")) {
                                    idgroup = SKGServices::stringToInt(val);
                                } else if (col == QStringLiteral("idtransaction")) {
                                    idtransaction = SKGServices::stringToInt(val);
                                } else if (col == QStringLiteral("amount")) {
                                    if (!val.isEmpty() && SKGServices::stringToDouble(val, true) != 0.0 && !amountSet) {
                                        amountSet = true;
                                        if (m_csvMapping.contains(QStringLiteral("quantity"))) {
                                            // 209705 vvvv
                                            skg_op_original_amount = std::move(val);
                                            // 209705 ^^^^
                                        } else {
                                            err = currentSubOperation.setQuantity(sign * SKGServices::stringToDouble(val, true));
                                        }
                                    } else if (modeAutoCreditDebit) {
                                        sign = 1.0; // Next one will be considered as a credit
                                    }
                                } else if (col == QStringLiteral("quantity")) {
                                    err = currentSubOperation.setQuantity(SKGServices::stringToDouble(val, true));
                                } else if (col == QStringLiteral("sign")) {
                                    if (QRegularExpression(m_importParameters.value(QStringLiteral("mapping_debit")), QRegularExpression::CaseInsensitiveOption)
                                            .match(val)
                                            .hasMatch()) {
                                        sign = -1;
                                        double cval = currentSubOperation.getQuantity();
                                        if (cval > 0) {
                                            err = currentSubOperation.setQuantity(-cval);
                                        }
                                    }
                                } else if (col == QStringLiteral("unit")) {
                                    // Looking for unit
                                    SKGUnitObject unit(m_importer->getDocument());
                                    if (val != defUnit.getName()) { // For performance
                                        err = unit.setName(val);
                                        IFOKDO(err, unit.setSymbol(val))
                                        if (!err && unit.load().isFailed()) {
                                            err = unit.save(false); // Save only
                                        }

                                        // This unit is now the default one, it is better for performance
                                        defUnit = unit;
                                    } else {
                                        unit = defUnit;
                                    }

                                    SKGUnitValueObject unitval;
                                    IFOKDO(err, unit.addUnitValue(unitval))
                                    IFOK(err)
                                    {
                                        int posAmount = m_csvMapping.indexOf(QStringLiteral("amount"));
                                        int posQuantity = m_csvMapping.indexOf(QStringLiteral("quantity"));
                                        if (posAmount != -1 && posQuantity != -1) {
                                            err = unitval.setQuantity(SKGServices::stringToDouble(atts.at(posAmount), true)
                                                                      / SKGServices::stringToDouble(atts.at(posQuantity), true));
                                        } else {
                                            err = unitval.setQuantity(1);
                                        }
                                    }
                                    IFOKDO(err, unitval.setDate(now.date()))
                                    IFOKDO(err, unitval.save())
                                    IFOKDO(err, currentOperation.setUnit(unit))
                                } else if (col == QStringLiteral("account")) {
                                    // Looking for account
                                    if (val.isEmpty()) {
                                        err = m_importer->getDefaultAccount(defAccount);
                                        if (i == 0) {
                                            emptyAccount = (defAccount.getNbOperation() == 0);
                                        }
                                    } else if (val != defAccount.getName()) { // For performance
                                        SKGAccountObject account(m_importer->getDocument());
                                        account.setName(val);
                                        err = account.load();
                                        IFKO(err)
                                        {
                                            // Not found, we have to create one
                                            SKGBankObject bank(m_importer->getDocument());
                                            QString name = i18nc("Noun", "Bank for import %1", postFix);
                                            err = bank.setName(name);
                                            if (!err && bank.load().isFailed()) {
                                                err = bank.save(false); // Save only
                                                IFOKDO(err,
                                                       m_importer->getDocument()->sendMessage(
                                                           i18nc("An information message", "Default bank '%1' created for import", name)))
                                            }
                                            IFOKDO(err, bank.addAccount(account))
                                            IFOKDO(err, account.setName(val))
                                            if (!err && account.load().isFailed()) {
                                                err = account.save(false); // Save only
                                            }
                                        }

                                        // This account is now the default one, it is better for performance
                                        defAccount = account;
                                    }
                                    IFOKDO(err, currentOperation.setParentAccount(defAccount))
                                } else if (col == QStringLiteral("category")) {
                                    // Set Category
                                    if (!val.isEmpty()) {
                                        // Prepare val
                                        val.replace('/', OBJECTSEPARATOR);
                                        val.replace(':', OBJECTSEPARATOR);
                                        val.replace(',', OBJECTSEPARATOR);
                                        val.replace(';', OBJECTSEPARATOR);
                                        // Get previous category
                                        if (!currentCategory.isEmpty()) {
                                            val = currentCategory % OBJECTSEPARATOR % val;
                                        }
                                        currentCategory = val;

                                        // Create and set category
                                        SKGCategoryObject Category;
                                        err = SKGCategoryObject::createPathCategory(m_importer->getDocument(), val, Category);
                                        IFOKDO(err, currentSubOperation.setCategory(Category))

                                        IFOK(err)
                                        {
                                            if (m_csvMapping.indexOf(QStringLiteral("payee")) == -1) {
                                                SKGPayeeObject payeeObj;
                                                QString p = m_importer->getDocument()->getCategoryForPayee(val, false);
                                                err = SKGPayeeObject::createPayee(m_importer->getDocument(), p, payeeObj);
                                                IFOKDO(err, currentOperation.setPayee(payeeObj))
                                            }
                                        }
                                    }
                                } else {
                                    // A property
                                    propertiesAtt.push_back(col);
                                    propertiesVal.push_back(val);
                                }
                            }
                        }

                        if (!err && (initialBalance != 0)) {
                            // Specific values for initial balance
                            err = currentOperation.setStatus(SKGOperationObject::CHECKED);
                            IFOKDO(err, currentOperation.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
                            IFOKDO(err, currentSubOperation.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
                        }
                        IFOKDO(err, currentOperation.setImportID(hash.toHex()))
                        IFOK(err)
                        {
                            if (idtransaction != 0) {
                                if (mapOperation.contains(idtransaction)) {
                                    currentOperation = mapOperation[idtransaction];
                                    skg_op_original_amount = QString();
                                } else {
                                    err = currentOperation.save();
                                    mapOperation[idtransaction] = currentOperation;
                                }
                            } else {
                                err = currentOperation.save(false); // Save only
                            }
                        }
                        if (!err && idgroup != 0) {
                            if (mapGroup.contains(idgroup) && currentOperation != mapGroup[idgroup]) {
                                err = currentOperation.setGroupOperation(mapGroup[idgroup]);
                                IFOKDO(err, currentOperation.save())
                            }
                            mapGroup[idgroup] = currentOperation;
                        }

                        IFOKDO(err, currentSubOperation.setParentOperation(currentOperation))
                        IFOKDO(err, currentSubOperation.save(false, false))

                        // 209705 vvvv
                        if (!err && !skg_op_original_amount.isEmpty()) {
                            err = currentOperation.setProperty(QStringLiteral("SKG_OP_ORIGINAL_AMOUNT"), skg_op_original_amount);
                        }
                        // 209705 ^^^^

                        // Add properties
                        int nbp = propertiesAtt.count();
                        for (int p = 0; !err && p < nbp; ++p) {
                            err = currentOperation.setProperty(propertiesAtt.at(p), propertiesVal.at(p));
                        }

                        if (!err && i % 500 == 0) {
                            err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                        }
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    IFOK(err)
                    {
                        QString balance = m_importParameters.value(QStringLiteral("balance"));
                        if (!balance.isEmpty()) {
                            if (emptyAccount) {
                                // Current amount
                                double currentAmount = defAccount.getAmount(QDate::currentDate());

                                // Update account
                                IFOKDO(err, defAccount.setInitialBalance(SKGServices::stringToDouble(balance, true) - currentAmount, defUnit))
                                IFOKDO(err, defAccount.save())
                                IFOKDO(err,
                                       m_importer->getDocument()->sendMessage(
                                           i18nc("An information message", "The initial balance of '%1' has been set", defAccount.getName()),
                                           SKGDocument::Warning))

                            } else {
                                m_importer->getDocument()->sendMessage(
                                    i18nc("An information message",
                                          "The initial balance of '%1' has not been set because some transactions are already existing",
                                          defAccount.getName()));
                            }
                        }
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)

                    // Lines treated
                    IFOKDO(err, m_importer->getDocument()->stepForward(3))
                }
            }
        }
    }
    SKGENDTRANSACTION(m_importer->getDocument(), err)

    return err;
}

bool SKGImportPluginCsv::isExportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("CSV"));
}

SKGError SKGImportPluginCsv::exportFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    auto listUUIDs = SKGServices::splitCSVLine(m_exportParameters.value(QStringLiteral("uuid_of_selected_accounts_or_operations")));

    QString wc;
    for (const auto &uuid : std::as_const(listUUIDs)) {
        auto items = SKGServices::splitCSVLine(uuid, '-');
        if (items.at(1) == QStringLiteral("operation")) {
            if (!wc.isEmpty()) {
                wc += QStringLiteral(" OR ");
            }
            wc += " i_OPID=" + items.at(0);
        } else if (items.at(1) == QStringLiteral("account")) {
            if (!wc.isEmpty()) {
                wc += QStringLiteral(" OR ");
            }
            wc += " rd_account_id=" + items.at(0);
        }
    }
    if (wc.isEmpty()) {
        wc = QStringLiteral("1=1");
    } else {
        IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Only selected accounts and transactions have been exported")))
    }

    // Open file
    QSaveFile file(m_importer->getLocalFileName(false));
    if (!file.open(QIODevice::WriteOnly)) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", m_importer->getFileName().toDisplayString()));
    } else {
        QTextStream out(&file);
        if (!m_importer->getCodec().isEmpty()) {
            out.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
        }
        err = m_importer->getDocument()->dumpSelectSqliteOrder(
            QStringLiteral("SELECT v.d_date as date, v.t_BANK as bank, v.t_ACCOUNT as account, v.t_number as number, v.t_mode as mode, "
                           "v.t_PAYEE as payee, v.t_REALCOMMENT as comment, PRINTF('%.'||u.i_nbdecimal||'f', v.f_REALQUANTITY) as quantity, "
                           "v.t_UNIT as unit, PRINTF('%.'||u.i_nbdecimal||'f', v.f_REALCURRENTAMOUNT) as amount, v.t_TYPEEXPENSE as sign, v.t_REALCATEGORY as "
                           "category, v.t_status as status, "
                           "v.t_REALREFUND as tracker, v.t_bookmarked as bookmarked, v.i_SUBOPID id, v.i_OPID idtransaction, v.i_group_id idgroup "
                           "FROM v_suboperation_consolidated as v, unit as u WHERE v.rc_unit_id=u.id AND (")
                % wc % ") ORDER BY v.d_date, v.i_OPID, v.i_SUBOPID",
            &out,
            SKGServices::DUMP_CSV);

        // Close file
        file.commit();
    }

    return err;
}

QString SKGImportPluginCsv::getMimeTypeFilter() const
{
    return "*.csv|" % i18nc("A file format", "CSV file");
}

SKGError SKGImportPluginCsv::importCSVUnit()
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    if (m_importer->getDocument() != nullptr) {
        // Begin transaction
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), 3);
        IFOK(err)
        {
            // File name is the name of the unit
            QFileInfo fInfo(m_importer->getFileName().path());
            QString unitName = fInfo.baseName();

            // Default mapping
            if (m_csvMapping.isEmpty()) {
                err = setCSVMapping(nullptr);
                IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Use automatic search of the columns")))
            }

            // Step 1 done
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            // Open file
            IFOK(err)
            {
                QFile file(m_importer->getLocalFileName());
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
                } else {
                    QTextStream stream(&file);
                    if (!m_importer->getCodec().isEmpty()) {
                        stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
                    }

                    // Ignore useless lines
                    int headerIndex = getCSVHeaderIndex();
                    for (int i = 1; i <= headerIndex; ++i) {
                        stream.readLine();
                    }

                    // Get data column
                    QStringList dates;
                    QStringList lines;
                    int posdate = m_csvMapping.indexOf(QStringLiteral("date"));
                    if (posdate != -1) {
                        while (!stream.atEnd()) {
                            // Read line
                            QString line = stream.readLine().trimmed();
                            if (!line.isEmpty()) {
                                lines.push_back(line);

                                // Get date
                                QStringList field = SKGServices::splitCSVLine(line, getCSVSeparator(line));
                                if (posdate < field.count()) {
                                    dates.push_back(field.at(posdate));
                                }
                            }
                        }
                    }

                    // close file
                    file.close();

                    // Select dateformat
                    QString dateFormat = SKGServices::getDateFormat(dates);
                    if (dateFormat.isEmpty()) {
                        err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "Date format not supported"));
                    }
                    IFOKDO(err,
                           m_importer->getDocument()->sendMessage(i18nc("An information message",
                                                                        "Import of '%1' with codec '%2' and date format '%3'",
                                                                        m_importer->getFileName().toDisplayString(),
                                                                        m_importer->getCodec(),
                                                                        dateFormat)))

                    // Step 2 done
                    IFOKDO(err, m_importer->getDocument()->stepForward(2))

                    // Treat all lines
                    IFOK(err)
                    {
                        int nb = lines.size();
                        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nb);

                        // Save last mapping used in a settings
                        QString mappingDesc;
                        int nbMap = m_csvMapping.count();
                        for (int i = 0; i < nbMap; ++i) {
                            if (i != 0) {
                                mappingDesc += '|';
                            }
                            mappingDesc += m_csvMapping.at(i);
                        }
                        IFOKDO(err, m_importer->getDocument()->setParameter(QStringLiteral("SKG_LAST_CSV_UNIT_MAPPING_USED"), mappingDesc))

                        int posdate2 = m_csvMapping.indexOf(QStringLiteral("date"));
                        int posvalue = m_csvMapping.indexOf(QStringLiteral("amount"));
                        if (posdate2 != -1 && posvalue != -1) {
                            for (int i = 0; !err && i < nb; ++i) {
                                QStringList atts = SKGServices::splitCSVLine(lines.at(i), getCSVSeparator(lines.at(i)));
                                err = m_importer->getDocument()->addOrModifyUnitValue(
                                    unitName,
                                    SKGServices::stringToDate(SKGServices::dateToSqlString(atts.at(posdate2), dateFormat)),
                                    SKGServices::stringToDouble(atts.at(posvalue), true));

                                IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                            }
                        }

                        SKGENDTRANSACTION(m_importer->getDocument(), err)

                        // Lines treated
                        IFOKDO(err, m_importer->getDocument()->stepForward(3))
                    }
                }
            }
        }
        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

SKGError SKGImportPluginCsv::importCSVRule()
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    if (m_importer->getDocument() != nullptr) {
        // Begin transaction
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import rules"), 3);
        IFOK(err)
        {
            // Default mapping
            if (m_csvMapping.isEmpty()) {
                err = setCSVMapping(nullptr);
                IFOKDO(err, m_importer->getDocument()->sendMessage(i18nc("An information message", "Use automatic search of the columns")))
            }

            // Step 1 done
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            // Open file
            IFOK(err)
            {
                QFile file(m_importer->getLocalFileName());
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
                } else {
                    QTextStream stream(&file);
                    if (!m_importer->getCodec().isEmpty()) {
                        stream.setEncoding(QStringConverter::encodingForName(m_importer->getCodec().toLatin1().constData()).value_or(QStringConverter::Utf8));
                    }

                    // Ignore useless lines
                    int headerIndex = getCSVHeaderIndex();
                    for (int i = 1; i <= headerIndex; ++i) {
                        stream.readLine();
                    }

                    // Get data column
                    QStringList lines;
                    while (!stream.atEnd()) {
                        // Read line
                        QString line = stream.readLine().trimmed();
                        if (!line.isEmpty()) {
                            lines.push_back(line);
                        }
                    }

                    // close file
                    file.close();

                    IFOKDO(err,
                           m_importer->getDocument()->sendMessage(i18nc("An information message",
                                                                        "Import of '%1' with codec '%2'",
                                                                        m_importer->getFileName().toDisplayString(),
                                                                        m_importer->getCodec())))

                    // Step 2 done
                    IFOKDO(err, m_importer->getDocument()->stepForward(2))

                    // Treat all lines
                    IFOK(err)
                    {
                        int nb = lines.size();
                        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import rules"), nb);

                        // Save last mapping used in a settings
                        QString mappingDesc;
                        int nbMap = m_csvMapping.count();
                        for (int i = 0; i < nbMap; ++i) {
                            if (i != 0) {
                                mappingDesc += '|';
                            }
                            mappingDesc += m_csvMapping.at(i);
                        }
                        IFOKDO(err, m_importer->getDocument()->setParameter(QStringLiteral("SKG_LAST_CSV_RULE_MAPPING_USED"), mappingDesc))

                        int pospayee = m_csvMapping.indexOf(QStringLiteral("payee"));
                        int poscategory = m_csvMapping.indexOf(QStringLiteral("category"));
                        if (pospayee != -1 && poscategory != -1) {
                            for (int i = 0; !err && i < nb; ++i) {
                                QStringList atts = SKGServices::splitCSVLine(lines.at(i), getCSVSeparator(lines.at(i)));

                                SKGRuleObject oRule;
                                err = SKGRuleObject::createPayeeCategoryRule(m_importer->getDocument(), atts.at(pospayee), atts.at(poscategory), oRule);

                                IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                            }
                        }

                        SKGENDTRANSACTION(m_importer->getDocument(), err)

                        // Lines treated
                        IFOKDO(err, m_importer->getDocument()->stepForward(3))
                    }
                }
            }
        }
        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

#include <skgimportplugincsv.moc>
