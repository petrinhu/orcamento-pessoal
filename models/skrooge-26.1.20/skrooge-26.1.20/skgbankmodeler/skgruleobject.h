/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGRULEOBJECT_H
#define SKGRULEOBJECT_H
/** @file
 * This file defines classes SKGRuleObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgdocument.h"
#include "skgerror.h"
#include "skgobjectbase.h"

/**
 * This class allows to define rules
 */
class SKGBANKMODELER_EXPORT SKGRuleObject final : public SKGObjectBase
{
public:
    /**
     * Alarm
     */
    struct SKGAlarmInfo {
        /** To know if the alarm is raised */
        bool Raised{};
        /** The message to display */
        QString Message;
        /** The amount in absolute value */
        double Amount{};
        /** The limit */
        double Limit{};
    };

    /**
     * Process mode
     */
    enum ProcessMode {
        ALL,
        NOTCHECKED,
        IMPORTED,
        IMPORTEDNOTVALIDATE,
        IMPORTING
    };
    /**
     * Process mode
     */
    Q_ENUM(ProcessMode)

    /**
     * Action type
     */
    enum ActionType {
        SEARCH,
        UPDATE,
        ALARM,
        APPLYTEMPLATE
    };
    /**
     * Action type
     */
    Q_ENUM(ActionType)

    /**
     * Default constructor
     */
    explicit SKGRuleObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier of the object
     */
    explicit SKGRuleObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGRuleObject(const SKGRuleObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGRuleObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGRuleObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGRuleObject &operator=(const SKGRuleObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGRuleObject();

    /**
     * Return the name of the object for the display
     * @return name of the object
     */
    QString getDisplayName() const override;

    /**
     * Set the XML for the search definition
     * @param iXml the XML search definition
     * @code example:
     * <!DOCTYPE SKGML>
     *   <element> <!--OR-->
     *     <element>  <!--AND-->
     *       <element attribute="d_date" operator="STRFTIME('%Y',#ATT#)=STRFTIME('%Y',date('now', 'localtime'))" />
     *     </element>
     *   </element>
     * @endcode
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setXMLSearchDefinition(const QString &iXml);

    /**
     * Get the XML for the search definition
     * @return the XML
     */
    QString getXMLSearchDefinition() const;

    /**
     * Set the XML for the action definition
     * @param iXml the XML action definition
     * @code example for an update action:
     * <!DOCTYPE SKGML>
     *   <element> <!--OR-->
     *     <element>  <!--AND-->
     *       <element attribute="t_number" att2="t_PAYEE" value2="10000000" operator="#ATT#=substr(#ATT2#,'#V1#','#V2#')" att2s="Payee" value="11" />
     *     </element>
     *   </element>
     * @endcode
     * @code example for an alarm action:
     * <!DOCTYPE SKGML>
     *   <element> <!--OR-->
     *     <element>  <!--AND-->
     *       <element attribute="f_REALCURRENTAMOUNT" operator="ABS(TOTAL(#ATT#))#OP##V1#,ABS(TOTAL(#ATT#)), #V1#, '#V2S#'" value="1000" value2="Take care!"
     * operator2=">="/>
     *     </element>
     *   </element>
     * @endcode
     * @code example to apply a template:
     * <!DOCTYPE SKGML>
     *   <element> <!--OR-->
     *     <element>  <!--AND-->
     *       <element attribute="id" operator="APPLYTEMPLATE(#V1#)" value="123" value2="The template name"/>
     *     </element>
     *   </element>
     * @endcode*
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setXMLActionDefinition(const QString &iXml);

    /**
     * Get the XML for the action definition
     * @return the XML
     */
    QString getXMLActionDefinition() const;

    /**
     * Set the search description
     * @param iDescription the search description
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setSearchDescription(const QString &iDescription);

    /**
     * Get the search description
     * @return the description
     */
    QString getSearchDescription() const;

    /**
     * Set the action description
     * @param iDescription the action description
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setActionDescription(const QString &iDescription);

    /**
     * Get the action description
     * @return the description
     */
    QString getActionDescription() const;

    /**
     * Set the action type
     * @param iType the action type
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setActionType(SKGRuleObject::ActionType iType);

    /**
     * Get the action type
     * @return the type
     */
    SKGRuleObject::ActionType getActionType() const;

    /**
     * Set the order for the rule
     * @param iOrder the order. (-1 means "at the end")
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setOrder(double iOrder);

    /**
     * Get the order for the rule
     * @return the order
     */
    double getOrder() const;

    /**
     * To bookmark or not an account
     * @param iBookmark the bookmark: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError bookmark(bool iBookmark);

    /**
     * To know if the account is bookmarked
     * @return an object managing the error
     *   @see SKGError
     */
    bool isBookmarked() const;

    /**
     * save the object into the database
     * @param iInsertOrUpdate the save mode.
     *    true: try an insert, if the insert failed then try an update.
     *    false: try an insert, if the insert failed then return an error.
     * @param iReloadAfterSave to reload the object after save. Set false if you are sure that you will not use this object after save
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError save(bool iInsertOrUpdate = true, bool iReloadAfterSave = true) override;

    /**
     * Execute actions
     * @param iMode mode
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError execute(ProcessMode iMode = SKGRuleObject::ALL);

    /**
     * Get where clause corresponding to search condition
     * @param iAdditionalCondition additional select condition
     * @return the where clause
     */
    QString getSelectSqlOrder(const QString &iAdditionalCondition = QString()) const;

    /**
     * Get alarm info
     * @return alarm info
     */
    SKGRuleObject::SKGAlarmInfo getAlarmInfo() const;

    /**
     * Create a rule to update a category on transactions having a specific payee
     * @param iDocument the document containing the object
     * @param iPayee the payee
     * @param iCategory the category
     * @param oRule the created rule
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError createPayeeCategoryRule(SKGDocument *iDocument, const QString &iPayee, const QString &iCategory, SKGRuleObject &oRule);

    /**
     * Get the list of supported operators
     * @param iAttributeType type of attribute
     * @param  iType mode
     * @return list of supported operators
     */
    static QStringList getListOfOperators(SKGServices::AttributeType iAttributeType, SKGRuleObject::ActionType iType = SEARCH);

    /**
     * Get the NLS display of an operator
     * @param iOperator the operator (see @see getListOfOperators)
     * @param iParam1 parameter
     * @param iParam2 parameter
     * @param iAtt2 attribute number 2
     * @return the NLS display
     */
    static QString getDisplayForOperator(const QString &iOperator, const QString &iParam1, const QString &iParam2, const QString &iAtt2);

    /**
     * Get the NLS tooltip of an operator
     * @param iOperator the operator (see @see getListOfOperators)
     * @return the NLS display
     */
    static QString getToolTipForOperator(const QString &iOperator);

protected:
    /**
     * Get the description of an XML definition
     * @param iDocument the document
     * @param iXML the XML
     * @param iSQL to define if you want the TXT or SQL description
     * @param iType mode
     * @return the description
     */
    static QString getDescriptionFromXML(SKGDocument *iDocument, const QString &iXML, bool iSQL = false, SKGRuleObject::ActionType iType = SEARCH);

private:
    static QStringList
    getFromXML(SKGDocument *iDocument, const QString &iXML, bool iSQL = false, SKGRuleObject::ActionType iType = SEARCH, bool iFullUpdate = false);
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGRuleObject, Q_MOVABLE_TYPE);
#endif
