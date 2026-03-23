/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGUNITCOMBOBOX_H
#define SKGUNITCOMBOBOX_H
/** @file
 * This file defines classes SKGUnitComboBox.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbankgui_export.h"
#include "skgcombobox.h"
#include "skgunitobject.h"

class SKGDocumentBank;

/**
 * This class is a unit
 */
class SKGBANKGUI_EXPORT SKGUnitComboBox : public SKGComboBox
{
    Q_OBJECT
    /**
     * the current unit
     */
    Q_PROPERTY(SKGUnitObject unit READ getUnit WRITE setUnit NOTIFY unitChanged)

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGUnitComboBox(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGUnitComboBox() override;

    /**
     * Set the current document
     * @param iDocument the current document
     */
    virtual void setDocument(SKGDocumentBank *iDocument);

    /**
     * Set the condition to selection the list of units to display
     * @param iCondition the condition (Default= "t_type!='I'")
     */
    virtual void setWhereClauseCondition(const QString &iCondition);

    /**
     * Get the current unit
     * The unit will be created if not existing
     * @return the current unit
     */
    virtual SKGUnitObject getUnit();

    /**
     * Set the current unit
     * @param iUnit the current unit
     */
    virtual void setUnit(const SKGUnitObject &iUnit);

public Q_SLOTS:
    /**
     * To refresh the list of the combo
     */
    virtual void refershList();

Q_SIGNALS:
    /**
     * Emitted when the unit is changed
     */
    void unitChanged();

private Q_SLOTS:
    void dataModified(const QString &iTableName, int iIdTransaction);

private:
    SKGDocumentBank *m_document;
    QString m_fillWhereClause;
};

#endif // SKGUNITCOMBOBOX_H
