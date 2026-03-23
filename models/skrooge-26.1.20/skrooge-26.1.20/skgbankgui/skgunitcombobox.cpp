/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGUnitComboBox.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgunitcombobox.h"

#include <klocalizedstring.h>

#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgunitvalueobject.h"

SKGUnitComboBox::SKGUnitComboBox(QWidget *iParent)
    : SKGComboBox(iParent)
    , m_document(nullptr)
    , m_fillWhereClause(QStringLiteral("t_type!='I'"))
{
}

SKGUnitComboBox::~SKGUnitComboBox()
{
    m_document = nullptr;
}

void SKGUnitComboBox::setDocument(SKGDocumentBank *iDocument)
{
    m_document = iDocument;
    connect(m_document, &SKGDocument::tableModified, this, &SKGUnitComboBox::dataModified);
    dataModified(QString(), 0);
}

void SKGUnitComboBox::setWhereClauseCondition(const QString &iCondition)
{
    m_fillWhereClause = iCondition;
    dataModified(QString(), 0);
}

SKGUnitObject SKGUnitComboBox::getUnit()
{
    SKGUnitObject unit(m_document);
    QString unitName = text();
    if ((m_document != nullptr) && !unitName.isEmpty()) {
        SKGError err;
        err = unit.setSymbol(unitName);
        if (!unit.exist()) {
            IFOKDO(err, unit.setName(unitName))
            IFOKDO(err, unit.save())

            SKGUnitValueObject unitVal;
            IFOKDO(err, unit.addUnitValue(unitVal))
            IFOKDO(err, unitVal.setDate(QDate::currentDate()))
            IFOKDO(err, unitVal.setQuantity(1))
            IFOKDO(err, unitVal.save())

            IFOK(err) m_document->sendMessage(i18nc("An information message", "Unit '%1' has been created", text()), SKGDocument::Positive);
        } else {
            err = unit.load();
        }
    }
    return unit;
}

void SKGUnitComboBox::setUnit(const SKGUnitObject &iUnit)
{
    if (text() != iUnit.getSymbol()) {
        setText(iUnit.getSymbol());

        Q_EMIT unitChanged();
    }
}

void SKGUnitComboBox::refershList()
{
    // Fill comboboxes
    if (m_document != nullptr) {
        SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << this,
                                            m_document,
                                            QStringLiteral("unit"),
                                            QStringLiteral("ifnull(t_symbol,t_name)"),
                                            m_fillWhereClause);
        SKGServices::SKGUnitInfo primary = m_document->getPrimaryUnit();
        if (!primary.Symbol.isEmpty()) {
            this->setCurrentIndex(this->findText(primary.Symbol));
        }
    }
}

void SKGUnitComboBox::dataModified(const QString &iTableName, int iIdTransaction)
{
    Q_UNUSED(iIdTransaction)

    // Refresh widgets
    if (m_document != nullptr) {
        QSqlDatabase *db = m_document->getMainDatabase();
        setEnabled(db != nullptr);
        if (db != nullptr && (iTableName == QStringLiteral("unit") || iTableName.isEmpty())) {
            refershList();
        }
    }
}
