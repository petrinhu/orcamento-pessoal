/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGDocumentBank.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdocumentbank.h"
#ifdef SKG_DBUS
#include <qdbusconnection.h>
#endif
#include <qicon.h>
#include <qsqldatabase.h>

#include <cmath>

#include "skgaccountobject.h"
#include "skgbankobject.h"
#include "skgerror.h"
#include "skgreportbank.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include "skgunitobject.h"
#include "skgunitvalueobject.h"

SKGDocumentBank::SKGDocumentBank()
    : SKGDocument()
{
    SKGTRACEINFUNC(10)
    connect(this, &SKGDocumentBank::tableModified, this, &SKGDocumentBank::refreshCache);
#ifdef SKG_DBUS
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/skrooge/skgdocumentbank"), this, QDBusConnection::ExportAllContents);
#endif

    // Initialisation of not undoable tables
    SKGListNotUndoable.push_back(QStringLiteral("T.operationbalance"));
    SKGListNotUndoable.push_back(QStringLiteral("T.budgetsuboperation"));
}

SKGDocumentBank::~SKGDocumentBank(){SKGTRACEINFUNC(10)}

SKGError SKGDocumentBank::computeBudgetSuboperationLinks() const
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    // Remove computed values
    err = this->executeSqliteOrder(QStringLiteral("DELETE FROM budgetsuboperation"));

    // Compute values
    IFOKDO(err,
           executeSqliteOrder(
               "INSERT INTO budgetsuboperation (id, id_suboperation, i_priority) "

               // Garbage collector annually
               "SELECT b.id, s.id, 6.0 FROM budget b, operation o, suboperation s WHERE +s.rd_operation_id=o.id AND b.rc_category_id=0 AND b.i_month=0 AND "
               "b.i_year=STRFTIME('%Y', IFNULL(s.d_date, o.d_date))"

               // Garbage collectory monthly
               " UNION SELECT b.id, s.id, 5.0 FROM budget b, operation o, suboperation s WHERE +s.rd_operation_id=o.id AND  b.rc_category_id=0 AND "
               "b.i_month<>0 AND b.i_year=STRFTIME('%Y', IFNULL(s.d_date, o.d_date)) AND b.i_month=STRFTIME('%m', IFNULL(s.d_date, o.d_date))"

               // Garbage categories annually
               " UNION SELECT b.id, s.id, 4.0 - (LENGTH(c2.t_fullname)-LENGTH(REPLACE(c2.t_fullname, '"
               % OBJECTSEPARATOR % "', '')))/(100.0*LENGTH('" % OBJECTSEPARATOR
               % "')) FROM budget b, operation o, v_suboperation_display s, category c2 WHERE +s.rd_operation_id=o.id AND b.rc_category_id<>0 AND b.i_month=0 "
                 "AND b.i_year=STRFTIME('%Y', IFNULL(s.d_date, o.d_date)) AND b.t_including_subcategories='Y' AND s.t_CATEGORY LIKE c2.t_fullname||'"
               % OBJECTSEPARATOR
               % "%' AND c2.id=b.rc_category_id"

                 // Garbage categories monthly
                 " UNION SELECT b.id, s.id, 3.0 - (LENGTH(c2.t_fullname)-LENGTH(REPLACE(c2.t_fullname, '"
               % OBJECTSEPARATOR % "', '')))/(100.0*LENGTH('" % OBJECTSEPARATOR
               % "')) FROM budget b, operation o, v_suboperation_display s, category c2 WHERE +s.rd_operation_id=o.id AND b.rc_category_id<>0 AND b.i_month<>0 "
                 "AND b.i_year=STRFTIME('%Y', IFNULL(s.d_date, o.d_date)) AND b.i_month=STRFTIME('%m', IFNULL(s.d_date, o.d_date)) AND "
                 "b.t_including_subcategories='Y' AND s.t_CATEGORY LIKE c2.t_fullname||'"
               % OBJECTSEPARATOR
               % "%' AND c2.id=b.rc_category_id"

                 // Strict category annually
                 " UNION SELECT b.id, s.id, 2.0 FROM budget b, operation o, v_suboperation_display s WHERE +s.rd_operation_id=o.id AND b.rc_category_id<>0 AND "
                 "b.i_month=0 AND b.i_year=STRFTIME('%Y', IFNULL(s.d_date, o.d_date)) AND b.rc_category_id=s.r_category_id"

                 // Strict category monthly
                 " UNION SELECT b.id, s.id, 1.0 FROM budget b, operation o, v_suboperation_display s WHERE +s.rd_operation_id=o.id AND b.rc_category_id<>0 AND "
                 "b.i_month<>0 AND b.i_year=STRFTIME('%Y', IFNULL(s.d_date, o.d_date)) AND b.i_month=STRFTIME('%m', IFNULL(s.d_date, o.d_date)) AND "
                 "+b.rc_category_id=s.r_category_id"));
    // Remove useless values
    IFOKDO(err,
           executeSqliteOrder(QStringLiteral("DELETE FROM budgetsuboperation WHERE EXISTS (SELECT 1 FROM budgetsuboperation b2 WHERE "
                                             "b2.id_suboperation=budgetsuboperation.id_suboperation AND b2.i_priority<budgetsuboperation.i_priority)")))

    return err;
}

void SKGDocumentBank::setComputeBalances(bool iEnabled)
{
    if (iEnabled != m_computeBalances) {
        m_computeBalances = iEnabled;
        computeBalances();
    }
}

SKGError SKGDocumentBank::computeBalances() const
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    // Remove computed values
    err = this->executeSqliteOrder(QStringLiteral("DELETE FROM operationbalance"));

    if (m_computeBalances) {
        IFOKDO(
            err,
            this->executeSqliteOrder(
                "INSERT INTO operationbalance (r_operation_id,f_balance,f_balance_entered) "
                "SELECT id, SUM(f_CURRENTAMOUNT) OVER (PARTITION BY rd_account_id ORDER BY d_date, id ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) AS "
                "f_balance, "
                "SUM(f_QUANTITY) OVER (PARTITION BY rd_account_id ORDER BY d_date, id ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW) AS f_balance_entered "
                "FROM v_operation WHERE t_template = 'N';"))
    }
    return err;
}

SKGError SKGDocumentBank::endTransaction(bool succeedded)
{
    SKGError err;
    if (succeedded && getDepthTransaction() == 1) {
        if (getCachedValue(QStringLiteral("SKG_REFRESH_VIEW")) == QStringLiteral("Y")) {
            QStringList listModifiedTables;
            err = this->getDistinctValues(QStringLiteral("doctransactionitem"),
                                          QStringLiteral("t_object_table"),
                                          QStringLiteral("rd_doctransaction_id=0"),
                                          listModifiedTables);
            if (!err
                && (listModifiedTables.contains(QStringLiteral("operation")) || listModifiedTables.contains(QStringLiteral("suboperation"))
                    || listModifiedTables.contains(QStringLiteral("unit")) || listModifiedTables.contains(QStringLiteral("unitvalue")))) {
                // Computation of cache
                err = computeBalances();
            }

            if (!err
                && (listModifiedTables.contains(QStringLiteral("operation")) || listModifiedTables.contains(QStringLiteral("suboperation"))
                    || listModifiedTables.contains(QStringLiteral("unit")) || listModifiedTables.contains(QStringLiteral("unitvalue"))
                    || listModifiedTables.contains(QStringLiteral("category")) || listModifiedTables.contains(QStringLiteral("budget")))) {
                // Computation of cache
                err = computeBudgetSuboperationLinks();
            }
        }
        // Clean main variations cache
        m_5mainVariations_cache.clear();
        m_5mainVariationsCat_cache.clear();
        m_5mainVariations_inputs = QString();
    }

    SKGError err2 = SKGDocument::endTransaction(succeedded);
    if (!err && err2) {
        err = err2;
    }
    return err;
}

QString SKGDocumentBank::getViewsIndexesAndTriggersVersion() const
{
    return "2026.01.15_" % getParameter(QStringLiteral("SKG_LANGUAGE"));
}

SKGError SKGDocumentBank::refreshViewsIndexesAndTriggers(bool iForce) const
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)

    QString version = getParameter(QStringLiteral("SKG_DB_BANK_VIEWS_VERSION"));
    if (!iForce && version == getViewsIndexesAndTriggersVersion()) {
        return err;
    }

    err = setParameter(QStringLiteral("SKG_DB_BANK_VIEWS_VERSION"), getViewsIndexesAndTriggersVersion());

    QString dateFormatShort = QLocale().dateFormat(QLocale::ShortFormat);

    // WARNING: Do not forget to update getViewVersion when this method is modified
    /**
     * This constant is used to initialized the data model (trigger creation)
     * IF YOU MODIFY THIS METHOD, DO NOT FORGET TO MODIFY getViewsIndexesAndTriggersVersion TOO
     */
    QStringList BankInitialDataModelTrigger;
    BankInitialDataModelTrigger << DELETECASCADEPARAMETER("bank")
                                << DELETECASCADEPARAMETER("account")
                                << DELETECASCADEPARAMETER("unit")
                                << DELETECASCADEPARAMETER("unitvalue")
                                << DELETECASCADEPARAMETER("category")
                                << DELETECASCADEPARAMETER("operation")
                                << DELETECASCADEPARAMETER("interest")
                                << DELETECASCADEPARAMETER("suboperation")
                                << DELETECASCADEPARAMETER("refund")
                                << DELETECASCADEPARAMETER("payee")
                                << DELETECASCADEPARAMETER("recurrentoperation")
                                << DELETECASCADEPARAMETER("rule")
                                << DELETECASCADEPARAMETER("budget")
                                << DELETECASCADEPARAMETER("budgetrule")


                                // Compute fullname
                                << QStringLiteral("DROP TRIGGER IF EXISTS cpt_category_fullname3")
                                /* << "CREATE TRIGGER cpt_category_fullname1 " // This trigger must be the first
                                 "AFTER UPDATE OF t_fullname ON category BEGIN "
                                 "UPDATE category SET t_name=t_name WHERE rd_category_id=new.id;"
                                 "END"*/

                                << QStringLiteral("DROP TRIGGER IF EXISTS cpt_category_fullname1")
                                << "CREATE TRIGGER cpt_category_fullname1 "
                                "AFTER INSERT ON category BEGIN "
                                "UPDATE category SET t_fullname="
                                "CASE WHEN rd_category_id IS NULL OR rd_category_id='' OR rd_category_id=0 THEN new.t_name ELSE (SELECT c.t_fullname FROM category c WHERE c.id=new.rd_category_id)||'" % OBJECTSEPARATOR % "'||new.t_name END "
                                "WHERE id=new.id;"
                                "END"

                                << QStringLiteral("DROP TRIGGER IF EXISTS cpt_category_fullname2")
                                << "CREATE TRIGGER cpt_category_fullname2 "
                                "AFTER UPDATE OF t_name, rd_category_id ON category BEGIN "
                                "UPDATE category SET t_fullname="
                                "CASE WHEN rd_category_id IS NULL OR rd_category_id='' OR rd_category_id=0 THEN new.t_name ELSE (SELECT c.t_fullname FROM category c WHERE c.id=new.rd_category_id)||'" % OBJECTSEPARATOR % "'||new.t_name END "
                                "WHERE id=new.id;"
                                "UPDATE category SET t_name=t_name WHERE rd_category_id=new.id;"
                                "END"

                                // -- Reparent suboperation on parent category when a category is removed
                                << QStringLiteral("DROP TRIGGER IF EXISTS fkdc_category_delete")
                                << "CREATE TRIGGER fkdc_category_delete "
                                "BEFORE DELETE ON category "
                                "FOR EACH ROW BEGIN "
                                "    UPDATE suboperation SET r_category_id=OLD.rd_category_id WHERE r_category_id IN (SELECT c.id FROM category c WHERE c.id=OLD.id OR c.t_fullname LIKE OLD.t_fullname||'" % OBJECTSEPARATOR % "%'); "
                                "    UPDATE payee SET r_category_id=0 WHERE r_category_id IN (SELECT c.id FROM category c WHERE c.id=OLD.id OR c.t_fullname LIKE OLD.t_fullname||'" % OBJECTSEPARATOR % "%'); "
                                "END "

                                << QStringLiteral("DROP TRIGGER IF EXISTS fkdc_category_parent_id_category_id")

                                // Trigger for update on view
                                << QStringLiteral("DROP TRIGGER IF EXISTS trgu_v_operation_prop_i_tmp")
                                << QStringLiteral("CREATE TRIGGER trgu_v_operation_prop_i_tmp "
                                        "INSTEAD OF UPDATE OF i_tmp ON v_operation_prop "
                                        "FOR EACH ROW BEGIN "
                                        "    UPDATE suboperation SET i_tmp=NEW.i_tmp WHERE id=OLD.i_SUBOPID; "
                                        "    UPDATE operation SET i_tmp=NEW.i_tmp WHERE id=OLD.i_OPID; "
                                        "    UPDATE parameters SET i_tmp=NEW.i_tmp WHERE id=OLD.i_PROPPID; "
                                        "END ")

                                << QStringLiteral("DROP TRIGGER IF EXISTS trgu_v_operation_prop_t_realcomment")
                                << QStringLiteral("CREATE TRIGGER trgu_v_operation_prop_t_realcomment "
                                        "INSTEAD OF UPDATE OF t_REALCOMMENT ON v_operation_prop "
                                        "FOR EACH ROW BEGIN "
                                        "    UPDATE suboperation SET t_comment=NEW.t_REALCOMMENT WHERE id=OLD.i_SUBOPID; "
                                        "END ")

                                << QStringLiteral("DROP TRIGGER IF EXISTS trgu_v_operation_prop_t_unit")
                                << QStringLiteral("CREATE TRIGGER trgu_v_operation_prop_t_unit "
                                        "INSTEAD OF UPDATE OF t_UNIT ON v_operation_prop "
                                        "FOR EACH ROW BEGIN "
                                        "    INSERT OR IGNORE INTO unit (t_name, t_symbol) VALUES (NEW.t_UNIT, NEW.t_UNIT); "
                                        "    UPDATE operation set rc_unit_id=(SELECT id FROM unit WHERE t_name=NEW.t_UNIT) WHERE id=OLD.i_OPID; "
                                        "END ")

                                << QStringLiteral("DROP TRIGGER IF EXISTS trgu_v_operation_prop_t_account")
                                << QStringLiteral("CREATE TRIGGER trgu_v_operation_prop_t_account "
                                        "INSTEAD OF UPDATE OF t_ACCOUNT ON v_operation_prop "
                                        "FOR EACH ROW BEGIN "
                                        "    INSERT OR IGNORE INTO account (t_name, rd_bank_id) VALUES (NEW.t_ACCOUNT, (SELECT MIN(id) FROM bank)); "
                                        "    UPDATE operation set rd_account_id=(SELECT id FROM account WHERE t_name=NEW.t_ACCOUNT) WHERE id=OLD.i_OPID; "
                                        "END ")

                                << QStringLiteral("DROP TRIGGER IF EXISTS trgu_v_operation_prop_t_payee")
                                << QStringLiteral("CREATE TRIGGER trgu_v_operation_prop_t_payee "
                                        "INSTEAD OF UPDATE OF t_PAYEE ON v_operation_prop "
                                        "FOR EACH ROW BEGIN "
                                        "    INSERT OR IGNORE INTO payee (t_name) VALUES (NEW.t_PAYEE); "
                                        "    UPDATE operation set r_payee_id=(SELECT id FROM payee WHERE t_name=NEW.t_PAYEE) WHERE id=OLD.i_OPID; "
                                        "END ")

                                << QStringLiteral("DROP TRIGGER IF EXISTS trgu_v_operation_prop_t_realrefund")
                                << QStringLiteral("CREATE TRIGGER trgu_v_operation_prop_t_realrefund "
                                        "INSTEAD OF UPDATE OF t_REALREFUND ON v_operation_prop "
                                        "FOR EACH ROW BEGIN "
                                        "    INSERT OR IGNORE INTO refund (t_name) VALUES (NEW.t_REALREFUND); "
                                        "    UPDATE suboperation set r_refund_id=(SELECT id FROM refund WHERE t_name=NEW.t_REALREFUND) WHERE id=OLD.i_SUBOPID; "
                                        "END ")

                                << QStringLiteral("DROP TRIGGER IF EXISTS trgu_v_operation_prop_d_dateop")
                                << QStringLiteral("CREATE TRIGGER trgu_v_operation_prop_d_dateop "
                                        "INSTEAD OF UPDATE OF d_DATEOP ON v_operation_prop "
                                        "FOR EACH ROW BEGIN "
                                        "    UPDATE suboperation set d_date=date(d_date, '+'||(julianday(NEW.d_DATEOP)-julianday(old.d_DATEOP))||' days') WHERE id=OLD.i_SUBOPID; "
                                        "    UPDATE operation set d_date=NEW.d_DATEOP WHERE id=OLD.i_OPID; "
                                        "END ");

    // Build triggers for normal attribute
    SKGServices::SKGAttributesList attributes;
    getAttributesDescription(QStringLiteral("operation"), attributes);
    int nb = attributes.count();
    for (int i = 0; i < nb; ++i) {
        QString att = attributes.at(i).name;
        if (att == att.toLower() && att != QStringLiteral("i_tmp")) {
            BankInitialDataModelTrigger << QStringLiteral("DROP TRIGGER IF EXISTS trgu_v_operation_prop_") % att
                                        << "CREATE TRIGGER trgu_v_operation_prop_" % att % " "
                                        "INSTEAD OF UPDATE OF " % att % " ON v_operation_prop "
                                        "FOR EACH ROW BEGIN "
                                        "    UPDATE operation SET " % att % "=NEW." % att % " WHERE id=OLD.i_OPID; "
                                        "END ";
        }
    }
    /**
     * This constant is used to initialized the data model (index creation)
     */
    QStringList BankInitialDataModelIndex;
    BankInitialDataModelIndex << QStringLiteral("CREATE UNIQUE INDEX uidx_unit_name ON unit(t_name)")
                              << QStringLiteral("CREATE UNIQUE INDEX uidx_unit_symbol ON unit(t_symbol)")

                              << QStringLiteral("CREATE INDEX idx_unit_unit_id ON unitvalue(rd_unit_id)")
                              << QStringLiteral("CREATE UNIQUE INDEX uidx_unitvalue ON unitvalue(d_date,rd_unit_id)")
                              << QStringLiteral("CREATE UNIQUE INDEX uidx_unitvalue2 ON unitvalue(rd_unit_id, d_date)")

                              << QStringLiteral("CREATE UNIQUE INDEX uidx_bank_name ON bank(t_name)")

                              << QStringLiteral("CREATE UNIQUE INDEX uidx_account_name ON account(t_name)")
                              << QStringLiteral("CREATE INDEX idx_account_bank_id ON account(rd_bank_id)")
                              << QStringLiteral("CREATE INDEX idx_account_r_account_id ON account(r_account_id)")
                              << QStringLiteral("CREATE INDEX idx_account_type ON account(t_type)")

                              << QStringLiteral("CREATE INDEX idx_category_category_id ON category(rd_category_id)")
                              << QStringLiteral("CREATE INDEX idx_category_t_fullname ON category(t_fullname)")

                              << QStringLiteral("CREATE UNIQUE INDEX uidx_category_parent_id_name ON category(t_name,rd_category_id)")

                              << QStringLiteral("CREATE INDEX  idx_operation_tmp1_found_transfert ON operation (rc_unit_id, d_date)")
                              << QStringLiteral("CREATE INDEX  idx_operation_grouped_operation_id ON operation (i_group_id)")
                              // << "CREATE INDEX  idx_operation_t_mode ON operation (t_mode)"
                              // << "CREATE INDEX  idx_operation_t_payee ON operation (t_payee)"
                              << QStringLiteral("CREATE INDEX  idx_operation_t_number ON operation (t_number)")
                              << QStringLiteral("CREATE INDEX  idx_operation_i_tmp ON operation (i_tmp)")
                              << QStringLiteral("CREATE INDEX  idx_operation_rd_account_id ON operation (rd_account_id)")
                              << QStringLiteral("CREATE INDEX  idx_operation_rd_account_id_t_imported ON operation (rd_account_id, t_imported)")
                              << QStringLiteral("CREATE INDEX  idx_operation_rd_account_id_t_number ON operation (rd_account_id, t_number)")
                              << QStringLiteral("CREATE INDEX  idx_operation_rc_unit_id ON operation (rc_unit_id)")
                              << QStringLiteral("CREATE INDEX  idx_operation_t_import_id ON operation (t_import_id)")
                              << QStringLiteral("CREATE INDEX  idx_operation_d_date ON operation (d_date)")

                              << QStringLiteral("CREATE INDEX  idx_operationbalance_operation_id ON operationbalance (r_operation_id)")

                              << QStringLiteral("CREATE INDEX idx_suboperation_operation_id ON suboperation (rd_operation_id)")
                              << QStringLiteral("CREATE INDEX idx_suboperation_operation_id_f_value ON suboperation (rd_operation_id, f_value)")
                              << QStringLiteral("CREATE INDEX idx_suboperation_i_tmp ON suboperation (i_tmp)")
                              << QStringLiteral("CREATE INDEX idx_suboperation_category_id ON suboperation (r_category_id)")
                              << QStringLiteral("CREATE INDEX idx_suboperation_refund_id_id ON suboperation (r_refund_id)")

                              << QStringLiteral("CREATE INDEX  idx_recurrentoperation_rd_operation_id ON recurrentoperation (rd_operation_id)")

                              << QStringLiteral("CREATE UNIQUE INDEX uidx_refund_name ON refund(t_name)")

                              << QStringLiteral("CREATE UNIQUE INDEX uidx_payee_name ON payee(t_name)")

                              << QStringLiteral("CREATE INDEX  idx_interest_account_id ON interest (rd_account_id)")
                              << QStringLiteral("CREATE UNIQUE INDEX uidx_interest ON interest(d_date,rd_account_id)")

                              << QStringLiteral("CREATE INDEX idx_rule_action_type ON rule(t_action_type)")

                              << QStringLiteral("CREATE UNIQUE INDEX uidx_budget ON budget(i_year,i_month, rc_category_id)")
                              << QStringLiteral("CREATE INDEX idx_budget_category_id ON budget(rc_category_id)")
                              << QStringLiteral("CREATE INDEX idx_budgetsuboperation_id ON budgetsuboperation (id)")
                              << QStringLiteral("CREATE INDEX idx_budgetsuboperation_id_suboperation ON budgetsuboperation (id_suboperation)");

    /**
     * This constant is used to initialized the data model (view creation)
     */
    QStringList BankInitialDataModelView;
    BankInitialDataModelView
    // ==================================================================
    // These following views contains only attributes used by corresponding class (better for performances)
    // unit
            << QStringLiteral("CREATE VIEW  v_unit_displayname AS "
                             "SELECT *, t_name||' ('||t_symbol||')' AS t_displayname FROM unit")

            << "CREATE VIEW  v_unit_tmp1 AS "
            "SELECT *,"
            "(SELECT COUNT(1) FROM unitvalue s WHERE s.rd_unit_id=unit.id) AS i_NBVALUES, "
            "(CASE WHEN unit.rd_unit_id=0 THEN '' ELSE (SELECT (CASE WHEN s.t_symbol!='' THEN s.t_symbol ELSE s.t_name END) FROM unit s WHERE s.id=unit.rd_unit_id) END) AS t_UNIT,"
            "(CASE unit.t_type "
            "WHEN '1' THEN '" % SKGServices::stringToSqlString(i18nc("Noun", "Primary currency")) % "' "
            "WHEN '2' THEN '" % SKGServices::stringToSqlString(i18nc("Noun", "Secondary currency")) % "' "
            "WHEN 'C' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, a country's currency", "Currency")) % "' "
            "WHEN 'S' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, a financial share, as in a stock market", "Share")) % "' "
            "WHEN 'I' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, a financial index like the Dow Jones, NASDAQ, CAC40…", "Index")) % "' "
            "ELSE '" % SKGServices::stringToSqlString(i18nc("Noun, a physical object like a house or a car", "Object")) % "' END) AS t_TYPENLS, "
            "(SELECT MIN(s.d_date) FROM  unitvalue s WHERE s.rd_unit_id=unit.id) AS d_MINDATE, "
            "(SELECT MAX(s.d_date) FROM  unitvalue s WHERE s.rd_unit_id=unit.id) AS d_MAXDATE "
            "FROM unit"

            << QStringLiteral("CREATE VIEW  v_unit_tmp2 AS "
                             "SELECT *,"
                             "CASE WHEN v_unit_tmp1.t_type='1' THEN 1 ELSE IFNULL((SELECT s.f_quantity FROM unitvalue s INDEXED BY uidx_unitvalue2 WHERE s.rd_unit_id=v_unit_tmp1.id AND s.d_date=v_unit_tmp1.d_MAXDATE),1) END AS f_LASTVALUE "
                             "FROM v_unit_tmp1")

            << QStringLiteral("CREATE VIEW  v_unit AS "
                             "SELECT *,"
                             "CASE WHEN v_unit_tmp2.t_type='1' THEN 1 ELSE v_unit_tmp2.f_LASTVALUE*IFNULL((SELECT s2.f_LASTVALUE FROM v_unit_tmp2 s2 WHERE s2.id=v_unit_tmp2.rd_unit_id) , 1) END AS f_CURRENTAMOUNT "
                             "FROM v_unit_tmp2")

            // unitvalue
            << "CREATE VIEW  v_unitvalue_displayname AS "
            "SELECT *, (SELECT t_displayname FROM v_unit_displayname WHERE unitvalue.rd_unit_id=v_unit_displayname.id)||' '||IFNULL(TOFORMATTEDDATE(d_date,'" % SKGServices::stringToSqlString(dateFormatShort) % "'),STRFTIME('%Y-%m-%d',d_date)) AS t_displayname FROM unitvalue"

            << QStringLiteral("CREATE VIEW  v_unitvalue AS "
                             "SELECT * "
                             "FROM unitvalue")

            // suboperation
            << QStringLiteral("CREATE VIEW  v_suboperation AS "
                             "SELECT * "
                             "FROM suboperation")

            // operation
            << QStringLiteral("CREATE VIEW  v_operation_numbers AS "
                             "SELECT DISTINCT t_number, rd_account_id FROM operation")

            << QStringLiteral("CREATE VIEW  v_operation_next_numbers AS "
                             "SELECT NEXT(T1.t_number) AS t_number, T1.rd_account_id FROM v_operation_numbers AS T1 LEFT OUTER JOIN v_operation_numbers T2 "
                             "ON T2.rd_account_id=T1.rd_account_id AND T2.t_number=NEXT(T1.t_number) "
                             "WHERE T1.t_number!='' AND (T2.t_number IS NULL) ORDER BY T1.t_number")

            << QStringLiteral("CREATE VIEW  v_operation_tmp1 AS "
                             "SELECT operation.*,"
                             "(CASE WHEN v_unit.t_symbol!='' THEN v_unit.t_symbol ELSE v_unit.t_name END) AS t_UNIT,"
                             "IFNULL((SELECT s.t_name FROM payee s WHERE s.id=operation.r_payee_id), '') AS t_PAYEE,"
                             "v_unit.i_nbdecimal AS i_NBDEC,"
                             "v_unit.t_type AS t_TYPEUNIT,"
                             "v_unit.f_CURRENTAMOUNT AS f_CURRENTAMOUNTUNIT,"
                             "(SELECT TOTAL(s.f_value) FROM suboperation s WHERE s.rd_operation_id=operation.ID) AS f_QUANTITY,"
                             "(SELECT COUNT(1) FROM suboperation s WHERE s.rd_operation_id=operation.ID) AS i_NBSUBOPERATIONS, "
                             "account.t_name AS t_ACCOUNT, "
                             "account.t_type AS t_TYPEACCOUNT, "
                             "(CASE WHEN bank.t_name='' THEN '") % i18nc("Noun", "Wallets") % QStringLiteral("' ELSE bank.t_name END) AS t_BANK "
                             "FROM operation, account, bank, v_unit WHERE +operation.rd_account_id=account.id AND +account.rd_bank_id=bank.id AND +operation.rc_unit_id=v_unit.id")

            << QStringLiteral("CREATE VIEW  v_operation AS "
                             "SELECT *,"
                             "(SELECT s.id FROM suboperation s WHERE s.rd_operation_id=v_operation_tmp1.id AND ABS(s.f_value)=(SELECT MAX(ABS(s2.f_value)) FROM suboperation s2 WHERE s2.rd_operation_id=v_operation_tmp1.id)) AS i_MOSTIMPSUBOP,"
                             "v_operation_tmp1.f_CURRENTAMOUNTUNIT*v_operation_tmp1.f_QUANTITY AS f_CURRENTAMOUNT, "
                             "(CASE WHEN v_operation_tmp1.i_group_id<>0 AND v_operation_tmp1.t_TYPEACCOUNT<>'L' AND v_operation_tmp1.t_TYPEUNIT IN ('1', '2', 'C') AND "
                             "(SELECT COUNT(1) FROM operation WHERE i_group_id=v_operation_tmp1.i_group_id)=2 AND "
                             "EXISTS (SELECT 1 FROM v_operation_tmp1 op2 WHERE op2.i_group_id=v_operation_tmp1.i_group_id "
                             "AND op2.t_TYPEACCOUNT<>'L' AND op2.t_TYPEUNIT IN ('1', '2', 'C') AND op2.f_QUANTITY*v_operation_tmp1.f_QUANTITY<=0) THEN 'Y' ELSE 'N' END) AS t_TRANSFER "
//        "ROUND((SELECT s.f_CURRENTAMOUNT FROM v_unit s WHERE s.id=v_operation_tmp1.rc_unit_id)*v_operation_tmp1.f_QUANTITY, 2) AS f_CURRENTAMOUNT "
                             "FROM v_operation_tmp1")

            << "CREATE VIEW  v_operation_displayname AS "
            "SELECT *, IFNULL(TOFORMATTEDDATE(d_date,'" % SKGServices::stringToSqlString(dateFormatShort) % "'),STRFTIME('%Y-%m-%d',d_date))||' '||IFNULL(t_PAYEE,'')||' '||TOCURRENCY(v_operation.f_quantity, (SELECT (CASE WHEN s.t_symbol!='' THEN s.t_symbol ELSE s.t_name END) FROM unit s WHERE s.id=v_operation.rc_unit_id)) AS t_displayname FROM v_operation"

            << "CREATE VIEW  v_operation_delete AS "
            "SELECT *, (CASE WHEN t_status='Y' THEN '" %
            SKGServices::stringToSqlString(i18nc("Error message",  "You are not authorized to delete this transaction because in \"checked\" status")) %
            "' END) t_delete_message FROM operation"

            // account
            << QStringLiteral("CREATE VIEW  v_account AS "
                             "SELECT "
                             "account.*,"
                             "(CASE t_type "
                             "WHEN 'C' THEN '") % SKGServices::stringToSqlString(i18nc("Adjective, a current account", "Current")) % "' "
            "WHEN 'D' THEN '" % SKGServices::stringToSqlString(i18nc("Noun",  "Credit card")) % "' "
            "WHEN 'A' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, the type of an account", "Assets")) % "' "
            "WHEN 'I' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, a type of account WHERE you invest money", "Investment")) % "' "
            "WHEN 'W' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, a type of account", "Wallet")) % "' "
            "WHEN 'L' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, a type of account", "Loan")) % "' "
            "WHEN 'S' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, a type of account", "Saving")) % "' "
            "WHEN 'P' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, a type of account", "Pension")) % "' "
            "WHEN 'O' THEN '" % SKGServices::stringToSqlString(i18nc("Noun, as in other type of item", "Other")) % "' END) AS t_TYPENLS,"
            "(CASE WHEN bank.t_name='' THEN '" % i18nc("Noun", "Wallets") % QStringLiteral("' ELSE bank.t_name END) AS t_BANK,"
                    "bank.t_bank_number AS t_BANK_NUMBER,"
                    "bank.t_icon AS t_ICON,"
                    "IFNULL((SELECT f_CURRENTAMOUNTUNIT FROM v_operation_tmp1  WHERE d_date='0000-00-00' AND rd_account_id=account.id), 1) AS f_CURRENTAMOUNTUNIT,"
                    "(SELECT MAX(s.d_date) FROM  interest s WHERE s.rd_account_id=account.id) AS d_MAXDATE "
                    "FROM account, bank WHERE +account.rd_bank_id=bank.id")

            << QStringLiteral("CREATE VIEW  v_account_amount AS "
                             "SELECT "
                             "v_account.*,"
                             "(SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_operation s WHERE s.rd_account_id=v_account.id AND s.t_template='N') AS f_CURRENTAMOUNT "
                             "FROM v_account")

            << "CREATE VIEW  v_account_delete AS "
            "SELECT *, (CASE WHEN EXISTS(SELECT 1 FROM operation WHERE rd_account_id=account.id AND d_date<>'0000-00-00' AND t_template='N' AND t_status='Y') THEN '" %
            SKGServices::stringToSqlString(i18nc("Error message",  "You are not authorized to delete this account because it contains some checked transactions")) %
            "' END) t_delete_message FROM account"

            // bank
            << QStringLiteral("CREATE VIEW  v_bank_displayname AS "
                             "SELECT *, t_name AS t_displayname FROM bank")

            << QStringLiteral("CREATE VIEW  v_account_displayname AS "
                             "SELECT *, (SELECT t_displayname FROM v_bank_displayname WHERE account.rd_bank_id=v_bank_displayname.id)||'-'||t_name AS t_displayname FROM account")

            << QStringLiteral("CREATE VIEW  v_bank AS "
                             "SELECT * FROM bank")

            << QStringLiteral("CREATE VIEW  v_bank_amount AS "
                             "SELECT *,"
                             "(SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_account_amount s WHERE s.rd_bank_id=v_bank.id) AS f_CURRENTAMOUNT "
                             "FROM v_bank")

            // category
            << QStringLiteral("CREATE VIEW  v_category_displayname AS "
                             "SELECT *, t_fullname AS t_displayname FROM category")

            << QStringLiteral("CREATE VIEW  v_category AS SELECT * "
                             "FROM category")

            // recurrentoperation
            << "CREATE VIEW  v_recurrentoperation AS "
            "SELECT *,"
            "date(d_date, '-'||((CASE t_period_unit WHEN 'W' THEN 7  ELSE 1 END)*i_period_increment)||' '||(CASE t_period_unit WHEN 'M' THEN 'month' WHEN 'Y' THEN 'year' ELSE 'day' END)) as d_PREVIOUS,"
            "i_period_increment||' '||(CASE t_period_unit "
            "WHEN 'Y' THEN '" % SKGServices::stringToSqlString(i18nc("Noun",  "year(s)")) % "' "
            "WHEN 'M' THEN '" % SKGServices::stringToSqlString(i18nc("Noun",  "month(s)")) % "' "
            "WHEN 'W' THEN '" % SKGServices::stringToSqlString(i18nc("Noun",  "week(s)")) % "' "
            "ELSE '" % SKGServices::stringToSqlString(i18nc("Noun",  "day(s)")) % "' END) AS t_PERIODNLS "
            "FROM recurrentoperation"

            << "CREATE VIEW  v_recurrentoperation_displayname AS "
            "SELECT *, IFNULL(TOFORMATTEDDATE(d_date,'" % SKGServices::stringToSqlString(dateFormatShort) % "'),STRFTIME('%Y-%m-%d',d_date))||' '||(SELECT SUBSTR(t_displayname, INSTR(t_displayname, ' ')+1) FROM v_operation_displayname WHERE v_operation_displayname.id=v_recurrentoperation.rd_operation_id) AS t_displayname FROM v_recurrentoperation"

            // ==================================================================
            // These following views contains all attributes needed for display
            // unitvalue
            << QStringLiteral("CREATE VIEW  v_unitvalue_display AS "
                             "SELECT *,"
                             "unitvalue.f_QUANTITY*(SELECT TOTAL(v_operation.f_QUANTITY) FROM v_operation WHERE v_operation.rc_unit_id=unitvalue.rd_unit_id AND v_operation.d_date<=unitvalue.d_date AND v_operation.t_template='N') AS f_AMOUNTOWNED,"
                             "IFNULL((SELECT (CASE WHEN s.t_symbol!='' THEN s.t_symbol ELSE s.t_name END) FROM unit s WHERE s.id=(SELECT s2.rd_unit_id FROM unit s2 WHERE s2.id=unitvalue.rd_unit_id)),'') AS t_UNIT,"
                             "STRFTIME('%Y-%m',unitvalue.d_date) AS d_DATEMONTH,"
                             "STRFTIME('%Y',unitvalue.d_date) AS d_DATEYEAR "
                             "FROM unitvalue")

            // suboperation
            << QStringLiteral("CREATE VIEW  v_suboperation_display AS "
                             "SELECT *,"
                             "IFNULL((SELECT s.t_fullname FROM category s WHERE s.id=v_suboperation.r_category_id),'') AS t_CATEGORY, "
                             "IFNULL((SELECT s.t_name FROM refund s WHERE s.id=v_suboperation.r_refund_id),'') AS t_REFUND, "
                             "IFNULL((SELECT s.t_name||\" (\"||TOCURRENCY((SELECT TOTAL(s2.f_value) FROM v_suboperation s2 WHERE s2.d_date<=v_suboperation.d_date AND s2.r_refund_id=v_suboperation.r_refund_id), (SELECT t_UNIT FROM v_operation WHERE v_suboperation.rd_operation_id = v_operation.id))||\")\" FROM refund s WHERE s.id=v_suboperation.r_refund_id),'') AS t_REFUNDDISPLAY, "
                             "(CASE WHEN v_suboperation.f_value>=0 THEN v_suboperation.f_value ELSE 0 END) AS f_VALUE_INCOME, "
                             "(CASE WHEN v_suboperation.f_value<=0 THEN v_suboperation.f_value ELSE 0 END) AS f_VALUE_EXPENSE "
                             "FROM v_suboperation")

            << QStringLiteral("CREATE VIEW  v_suboperation_displayname AS "
                             "SELECT *, t_CATEGORY||' : '||f_value AS t_displayname FROM v_suboperation_display")

            // operation
            << "CREATE VIEW  v_operation_display_all AS "
            "SELECT *,"
            // "(SELECT s.t_comment FROM v_suboperation_display s WHERE s.id=v_operation.i_MOSTIMPSUBOP) AS t_COMMENT,"
            "IFNULL((CASE WHEN v_operation.i_group_id=0 THEN '' ELSE (SELECT GROUP_CONCAT(DISTINCT(op2.t_ACCOUNT)) FROM v_operation_tmp1 op2 WHERE op2.i_group_id=v_operation.i_group_id AND op2.id<>v_operation.id) END), '') AS t_TOACCOUNT, "
            "(SELECT s.t_CATEGORY FROM v_suboperation_display s WHERE s.id=v_operation.i_MOSTIMPSUBOP) AS t_CATEGORY,"
            "(SELECT s.t_REFUND FROM v_suboperation_display s WHERE s.id=v_operation.i_MOSTIMPSUBOP) AS t_REFUND,"
            "(SELECT GROUP_CONCAT(s.t_REFUNDDISPLAY) FROM v_suboperation_display s WHERE s.rd_operation_id=v_operation.id AND s.t_REFUNDDISPLAY!='') AS t_REFUNDDISPLAY,"
            "(CASE WHEN v_operation.f_QUANTITY<0 THEN '-' WHEN v_operation.f_QUANTITY=0 THEN '' ELSE '+' END) AS t_TYPEEXPENSE, "
            "(CASE WHEN v_operation.f_QUANTITY<=0 THEN '" % SKGServices::stringToSqlString(i18nc("Noun, financial transactions with a negative amount", "Expenditure")) % "' ELSE '" % SKGServices::stringToSqlString(i18nc("Noun, financial transactions with a positive amount", "Income")) % "' END) AS t_TYPEEXPENSENLS, "
            "TOWEEKYEAR(v_operation.d_date) AS d_DATEWEEK,"
            "STRFTIME('%Y-%m',v_operation.d_date) AS d_DATEMONTH,"
            "STRFTIME('%Y',v_operation.d_date)||'-Q'||(CASE WHEN STRFTIME('%m',v_operation.d_date)<='03' THEN '1' WHEN STRFTIME('%m',v_operation.d_date)<='06' THEN '2' WHEN STRFTIME('%m',v_operation.d_date)<='09' THEN '3' ELSE '4' END) AS d_DATEQUARTER, "
            "STRFTIME('%Y',v_operation.d_date)||'-S'||(CASE WHEN STRFTIME('%m',v_operation.d_date)<='06' THEN '1' ELSE '2' END) AS d_DATESEMESTER, "
            "STRFTIME('%Y',v_operation.d_date) AS d_DATEYEAR, "
            "(SELECT COUNT(1) FROM v_recurrentoperation s WHERE s.rd_operation_id=v_operation.id) AS i_NBRECURRENT,  "
            "(CASE WHEN v_operation.f_QUANTITY>=0 THEN v_operation.f_QUANTITY ELSE 0 END) AS f_QUANTITY_INCOME, "
            "(CASE WHEN v_operation.f_QUANTITY<=0 THEN v_operation.f_QUANTITY ELSE 0 END) AS f_QUANTITY_EXPENSE, "
            "(SELECT o2.f_balance FROM operationbalance o2 WHERE o2.r_operation_id=v_operation.id ) AS f_BALANCE, "
            "(SELECT o2.f_balance_entered FROM operationbalance o2 WHERE o2.r_operation_id=v_operation.id ) AS f_BALANCE_ENTERED, "
            "(CASE WHEN v_operation.f_QUANTITY>=0 THEN v_operation.f_CURRENTAMOUNT ELSE 0 END) AS f_CURRENTAMOUNT_INCOME, "
            "(CASE WHEN v_operation.f_QUANTITY<=0 THEN v_operation.f_CURRENTAMOUNT ELSE 0 END) AS f_CURRENTAMOUNT_EXPENSE "
            "FROM v_operation"

            << QStringLiteral("CREATE VIEW  v_operation_display AS "
                             "SELECT * FROM v_operation_display_all WHERE d_date!='0000-00-00' AND t_template='N'")

            // unit
            << QStringLiteral("CREATE VIEW  v_unit_display AS "
                             "SELECT *,"
                             "i_nbdecimal AS i_NBDEC,"
                             "(SELECT TOTAL(o.f_QUANTITY) FROM v_operation_display_all o WHERE o.rc_unit_id=v_unit.id AND o.t_template='N') AS f_QUANTITYOWNED, "
                             "(SELECT TOTAL(o.f_QUANTITY) FROM v_operation_display_all o WHERE o.rc_unit_id=v_unit.id AND o.t_template='N')*v_unit.f_CURRENTAMOUNT AS f_AMOUNTOWNED "
                             "FROM v_unit")

            // account
            << "CREATE VIEW  v_account_display AS "
            "SELECT "
            "v_account_amount.*,"
            "(v_account_amount.f_CURRENTAMOUNT/(SELECT u.f_CURRENTAMOUNT FROM v_unit u, operation s WHERE u.id=s.rc_unit_id AND s.rd_account_id=v_account_amount.id AND s.d_date='0000-00-00')) AS f_QUANTITY, "
            "(SELECT (CASE WHEN u.t_symbol!='' THEN u.t_symbol ELSE u.t_name END) FROM unit u, operation s WHERE u.id=s.rc_unit_id AND s.rd_account_id=v_account_amount.id AND s.d_date='0000-00-00') AS t_UNIT, "
            "(SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_operation s WHERE s.rd_account_id=v_account_amount.id AND s.t_status='Y' AND s.t_template='N') AS f_CHECKED, "
            "(SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_operation s WHERE s.rd_account_id=v_account_amount.id AND s.t_status!='N' AND s.t_template='N') AS f_CHECKEDANDPOINTED, "
            "(SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_operation s WHERE s.rd_account_id=v_account_amount.id AND s.t_status!='Y' AND s.t_template='N') AS f_COMING_SOON, "
            "(SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_operation s WHERE s.rd_account_id IN (SELECT id FROM account WHERE account.r_account_id=v_account_amount.id) AND s.t_status='N' AND s.t_template='N') AS f_COMING_SOON_FROM_LINKED_ACCOUNT, "
            "(SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_operation s WHERE s.rd_account_id=v_account_amount.id AND s.d_date<=date('now', 'localtime') AND s.t_template='N') AS f_TODAYAMOUNT, "
            "(SELECT COUNT(1) FROM v_operation_display s WHERE s.rd_account_id=v_account_amount.id) AS i_NBOPERATIONS, "
            "IFNULL((SELECT s.f_rate FROM interest s WHERE s.rd_account_id=v_account_amount.id AND s.d_date=v_account_amount.d_MAXDATE),0) AS f_RATE "
            "FROM v_account_amount"

            // transactions
            << "CREATE VIEW  v_suboperation_consolidated AS "
            "SELECT "
            "(SELECT s.t_TYPENLS FROM v_account s WHERE s.id=op.rd_account_id) AS t_ACCOUNTTYPE,"
            "(SELECT s.t_BANK FROM v_account s WHERE s.id=op.rd_account_id) AS t_BANK,"
            "(SELECT u.t_TYPENLS FROM v_unit_tmp1 u WHERE u.id=op.rc_unit_id) AS t_UNITTYPE,"
            "sop.id AS id, "
            "sop.id AS i_SUBOPID, "
            "sop.r_refund_id AS r_refund_id, "
            "(CASE WHEN sop.t_comment='' THEN op.t_comment ELSE sop.t_comment END) AS t_REALCOMMENT, "
            "sop.t_CATEGORY AS t_REALCATEGORY, "
            "sop.t_REFUND AS t_REALREFUND, "
            "sop.r_category_id AS i_IDCATEGORY, "
            "(CASE WHEN sop.f_value<0 THEN '-' WHEN sop.f_value=0 THEN '' ELSE '+' END) AS t_TYPEEXPENSE, "
            "(CASE WHEN sop.f_value<=0 THEN '" % SKGServices::stringToSqlString(i18nc("Noun, financial transactions with a negative amount", "Expenditure")) % "' ELSE '" % SKGServices::stringToSqlString(i18nc("Noun, financial transactions with a positive amount", "Income")) % "' END) AS t_TYPEEXPENSENLS, "
            "sop.f_value AS f_REALQUANTITY, "
            "sop.f_VALUE_INCOME AS f_REALQUANTITY_INCOME, "
            "sop.f_VALUE_EXPENSE AS f_REALQUANTITY_EXPENSE, "
            "((SELECT u.f_CURRENTAMOUNT FROM v_unit u WHERE u.id=op.rc_unit_id)*sop.f_value) AS f_REALCURRENTAMOUNT, "
            "((SELECT u.f_CURRENTAMOUNT FROM v_unit u WHERE u.id=op.rc_unit_id)*sop.f_VALUE_INCOME) AS f_REALCURRENTAMOUNT_INCOME, "
            "((SELECT u.f_CURRENTAMOUNT FROM v_unit u WHERE u.id=op.rc_unit_id)*sop.f_VALUE_EXPENSE) AS f_REALCURRENTAMOUNT_EXPENSE, "
            "TOWEEKYEAR(sop.d_date) AS d_DATEWEEK,"
            "STRFTIME('%Y-%m',sop.d_date) AS d_DATEMONTH,"
            "STRFTIME('%Y',sop.d_date)||'-Q'||(CASE WHEN STRFTIME('%m',sop.d_date)<='03' THEN '1' WHEN STRFTIME('%m',sop.d_date)<='06' THEN '2' WHEN STRFTIME('%m',sop.d_date)<='09' THEN '3' ELSE '4' END) AS d_DATEQUARTER, "
            "STRFTIME('%Y',sop.d_date)||'-S'||(CASE WHEN STRFTIME('%m',sop.d_date)<='06' THEN '1' ELSE '2' END) AS d_DATESEMESTER, "
            "STRFTIME('%Y',sop.d_date) AS d_DATEYEAR, "
            "sop.d_date AS d_date, "
            "op.id AS i_OPID, "
            "op.d_date AS d_DATEOP, "
            "op.*, "
            "sop.* "
            "FROM v_operation_display_all AS op, v_suboperation_display AS sop WHERE +sop.rd_operation_id=op.ID AND op.t_template='N'"

            << QStringLiteral("CREATE VIEW  v_operation_prop AS "
                             "SELECT "
                             "p.id AS i_PROPPID, "
                             "p.t_name AS i_PROPPNAME, "
                             "p.t_value AS i_PROPVALUE, "
                             "op.* "
                             "FROM v_suboperation_consolidated AS op LEFT OUTER JOIN parameters AS p ON (p.t_uuid_parent=op.id||'-suboperation' OR p.t_uuid_parent=op.i_OPID||'-operation')")

            // refund
            << "CREATE VIEW  v_refund_delete AS "
            "SELECT *, (CASE WHEN EXISTS(SELECT 1 FROM v_suboperation_consolidated WHERE r_refund_id=refund.id AND t_status='Y') THEN '" %
            SKGServices::stringToSqlString(i18nc("Error message",  "You are not authorized to delete this tracker because used by some checked transactions")) %
            "' END) t_delete_message FROM refund"

            << QStringLiteral("CREATE VIEW  v_refund AS SELECT * FROM refund")

            << QStringLiteral("CREATE VIEW  v_refund_amount AS "
                             "SELECT *, "
                             "(SELECT TOTAL(o.f_REALCURRENTAMOUNT) FROM v_suboperation_consolidated o WHERE o.r_refund_id=v_refund.id) AS f_CURRENTAMOUNT "
                             "FROM v_refund")

            << QStringLiteral("CREATE VIEW  v_refund_display AS "
                             "SELECT *,"
                             "(SELECT MIN(o.d_date) FROM v_suboperation_consolidated o WHERE o.r_refund_id=v_refund_amount.id) AS d_FIRSTDATE, "
                             "(SELECT MAX(o.d_date) FROM v_suboperation_consolidated o WHERE o.r_refund_id=v_refund_amount.id) AS d_LASTDATE "
                             " FROM v_refund_amount")

            << QStringLiteral("CREATE VIEW  v_refund_displayname AS "
                             "SELECT *, t_name AS t_displayname FROM refund")

            // Payee
            << "CREATE VIEW  v_payee_delete AS "
            "SELECT *, (CASE WHEN EXISTS(SELECT 1 FROM operation WHERE r_payee_id=payee.id AND t_status='Y') THEN '" %
            SKGServices::stringToSqlString(i18nc("Error message",  "You are not authorized to delete this payee because used by some checked transactions")) %
            "' END) t_delete_message FROM payee"

            << QStringLiteral("CREATE VIEW v_payee_amount AS SELECT o.r_payee_id AS r_payee_id, TOTAL(o.f_CURRENTAMOUNT) AS f_CURRENTAMOUNT, COUNT(1) AS i_NBOPERATIONS FROM v_operation o GROUP BY o.r_payee_id")

            << QStringLiteral("CREATE VIEW  v_payee AS SELECT *,"
                             "IFNULL((SELECT s.t_fullname FROM category s WHERE s.id=payee.r_category_id),'') AS t_CATEGORY "
                             "FROM payee")

            << QStringLiteral("CREATE VIEW  v_payee_display AS "
                             "SELECT v_payee.*, "
                             "(CASE WHEN p.f_CURRENTAMOUNT IS NULL THEN 0 ELSE p.f_CURRENTAMOUNT END) AS f_CURRENTAMOUNT, "
                             "(CASE WHEN p.i_NBOPERATIONS IS NULL THEN 0 ELSE p.i_NBOPERATIONS END) AS i_NBOPERATIONS "
                             "FROM v_payee LEFT OUTER JOIN v_payee_amount p ON p.r_payee_id=v_payee.id")

            << QStringLiteral("CREATE VIEW  v_payee_displayname AS "
                             "SELECT *, t_name AS t_displayname FROM payee")

            // category
            << "CREATE VIEW  v_category_delete AS "
            "SELECT *, (CASE WHEN EXISTS(SELECT 1 FROM v_suboperation_consolidated WHERE (t_REALCATEGORY=category.t_fullname OR t_REALCATEGORY like category.t_fullname||'%') AND t_status='Y') THEN '" %
            SKGServices::stringToSqlString(i18nc("Error message",  "You are not authorized to delete this category because used by some checked transactions")) %
            "' END) t_delete_message FROM category"

            << QStringLiteral("CREATE VIEW  v_category_amount AS SELECT o.i_IDCATEGORY AS i_IDCATEGORY, TOTAL(o.f_REALCURRENTAMOUNT) AS f_REALCURRENTAMOUNT FROM v_suboperation_consolidated o GROUP BY o.i_IDCATEGORY")

            << QStringLiteral("CREATE VIEW  v_category_display_tmp AS SELECT v_category.*, "
                             "IFNULL(t.f_REALCURRENTAMOUNT, 0) AS f_REALCURRENTAMOUNT, "
                             "(SELECT COUNT(DISTINCT(so.rd_operation_id)) FROM operation o, suboperation so WHERE +so.rd_operation_id=o.id AND so.r_category_id=v_category.ID AND o.t_template='N') AS i_NBOPERATIONS "
                             "FROM v_category LEFT OUTER JOIN v_category_amount t ON t.i_IDCATEGORY=v_category.ID")

            << QStringLiteral("CREATE VIEW  v_category_used1 AS SELECT v_category.*, "
                             "(CASE WHEN EXISTS(SELECT 1 FROM operation o, suboperation so WHERE +so.rd_operation_id=o.id AND so.r_category_id=v_category.ID AND o.t_template='N') THEN 'Y' ELSE 'N' END) AS t_ISUSED "
                             "FROM v_category")

            << "CREATE VIEW  v_category_used2 AS SELECT v_category_used1.*, "
            "(CASE WHEN v_category_used1.t_ISUSED='Y' THEN 'Y' WHEN EXISTS(SELECT 1 FROM v_category_used1 c WHERE c.t_ISUSED='Y' AND c.t_fullname like v_category_used1.t_fullname||'" % OBJECTSEPARATOR % "%') THEN 'C' ELSE 'N' END) AS t_ISUSEDCASCADE "
            "FROM v_category_used1"

            << "CREATE VIEW  v_category_display AS SELECT *,"
            "v_category_display_tmp.f_REALCURRENTAMOUNT+(SELECT TOTAL(c.f_REALCURRENTAMOUNT) FROM v_category_display_tmp c WHERE c.t_fullname LIKE v_category_display_tmp.t_fullname||'" % OBJECTSEPARATOR % "%') AS f_SUMCURRENTAMOUNT, "
            "v_category_display_tmp.i_NBOPERATIONS+(SELECT CAST(TOTAL(c.i_NBOPERATIONS) AS INTEGER) FROM v_category_display_tmp c WHERE c.t_fullname like v_category_display_tmp.t_fullname||'" % OBJECTSEPARATOR % "%') AS i_SUMNBOPERATIONS, "
            "(CASE WHEN v_category_display_tmp.t_close='Y' THEN 'Y' WHEN EXISTS(SELECT 1 FROM category c WHERE c.t_close='Y' AND c.t_fullname like v_category_display_tmp.t_fullname||'" % OBJECTSEPARATOR % "%') THEN 'Y' ELSE 'N' END) AS t_HASCLOSEDCHILD, "
            "(CASE WHEN v_category_display_tmp.t_close='N' THEN 'Y' WHEN EXISTS(SELECT 1 FROM category c WHERE c.t_close='N' AND c.t_fullname like v_category_display_tmp.t_fullname||'" % OBJECTSEPARATOR % "%') THEN 'Y' ELSE 'N' END) AS t_HASOPENEDCHILD, "
            "(CASE WHEN v_category_display_tmp.t_bookmarked='Y' THEN 'Y' WHEN EXISTS(SELECT 1 FROM category c WHERE c.t_bookmarked='Y' AND c.t_fullname like v_category_display_tmp.t_fullname||'" % OBJECTSEPARATOR % "%') THEN 'C' ELSE 'N' END) AS t_HASBOOKMARKEDCHILD, "
            "(CASE WHEN v_category_display_tmp.f_REALCURRENTAMOUNT<0 THEN '-' WHEN v_category_display_tmp.f_REALCURRENTAMOUNT=0 THEN '' ELSE '+' END) AS t_TYPEEXPENSE,"
            "(CASE WHEN v_category_display_tmp.f_REALCURRENTAMOUNT<0 THEN '" % SKGServices::stringToSqlString(i18nc("Noun, financial transactions with a negative amount", "Expenditure")) % "' WHEN v_category_display_tmp.f_REALCURRENTAMOUNT=0 THEN '' ELSE '" % SKGServices::stringToSqlString(i18nc("Noun, financial transactions with a positive amount", "Income")) % "' END) AS t_TYPEEXPENSENLS "
            "FROM v_category_display_tmp"

            // recurrentoperation
            << QStringLiteral("CREATE VIEW  v_recurrentoperation_display AS "
                             "SELECT rop.*, op.t_ACCOUNT, op.t_number, op.t_mode, op.i_group_id, op.t_TRANSFER, op.t_PAYEE, op.t_comment, op.t_CATEGORY, op.t_status, op.f_CURRENTAMOUNT "
                             "FROM v_recurrentoperation rop, v_operation_display_all AS op WHERE +rop.rd_operation_id=op.ID")

            // rule
            << QStringLiteral("CREATE VIEW v_rule AS SELECT *,"
                             "(SELECT COUNT(1) FROM rule r WHERE r.f_sortorder<=rule.f_sortorder) AS i_ORDER "
                             "FROM rule")
            << QStringLiteral("CREATE VIEW v_rule_displayname AS SELECT *, t_description AS t_displayname FROM rule")

            << QStringLiteral("CREATE VIEW v_rule_display AS SELECT * FROM v_rule")

            // interest
            << QStringLiteral("CREATE VIEW v_interest AS SELECT *,"
                             "(SELECT s.t_name FROM account s WHERE s.id=interest.rd_account_id) AS t_ACCOUNT "
                             " FROM interest")
            << "CREATE VIEW v_interest_displayname AS SELECT *, IFNULL(TOFORMATTEDDATE(d_date,'" % SKGServices::stringToSqlString(dateFormatShort) % "'),STRFTIME('%Y-%m-%d',d_date))||' '||f_rate||'%' AS t_displayname FROM interest"

            // budgetrule
            << "CREATE VIEW  v_budgetrule AS "
            "SELECT *, "
            "(SELECT COUNT(1) FROM budgetrule r WHERE r.f_sortorder<=budgetrule.f_sortorder) AS i_ORDER, "
            "IFNULL((SELECT s.t_fullname FROM category s WHERE s.id=budgetrule.rc_category_id),'') AS t_CATEGORYCONDITION, "
            "IFNULL((SELECT s.t_fullname FROM category s WHERE s.id=budgetrule.rc_category_id_target),'') AS t_CATEGORY, "
            "(CASE "
            "WHEN budgetrule.i_condition=-1 THEN '" % SKGServices::stringToSqlString(i18nc("Noun", "Negative")) % "' "
            "WHEN budgetrule.i_condition=1 THEN '" % SKGServices::stringToSqlString(i18nc("Noun", "Positive")) % "' "
            "WHEN budgetrule.i_condition=0 THEN '" % SKGServices::stringToSqlString(i18nc("Noun", "All")) % "' "
            "END) AS t_WHENNLS, "
            "f_quantity||(CASE WHEN budgetrule.t_absolute='N' THEN '%' ELSE (SELECT t_symbol FROM unit WHERE t_type='1') END) AS t_WHATNLS,"
            "(CASE "
            "WHEN budgetrule.t_rule='N' THEN '" % SKGServices::stringToSqlString(i18nc("Noun", "Next")) % "' "
            "WHEN budgetrule.t_rule='C' THEN '" % SKGServices::stringToSqlString(i18nc("Noun", "Current")) % "' "
            "WHEN budgetrule.t_rule='Y' THEN '" % SKGServices::stringToSqlString(i18nc("Noun", "Year")) % "' "
            "END) AS t_RULENLS "
            "FROM budgetrule"

            << QStringLiteral("CREATE VIEW  v_budgetrule_display AS "
                             "SELECT * "
                             " FROM v_budgetrule")

            << QStringLiteral("CREATE VIEW  v_budgetrule_displayname AS "
                             "SELECT *, t_WHENNLS||' '||t_WHATNLS||' '||t_RULENLS||' '||t_CATEGORY AS t_displayname FROM v_budgetrule")

            // budget
            << QStringLiteral("CREATE VIEW  v_budget_tmp AS "
                             "SELECT *, "
                             "IFNULL((SELECT s.t_fullname FROM category s WHERE s.id=budget.rc_category_id),'') AS t_CATEGORY, "
                             "(budget.i_year||(CASE WHEN budget.i_month=0 THEN '' WHEN budget.i_month<10 THEN '-0'||budget.i_month ELSE '-'||budget.i_month END)) AS t_PERIOD, "
                             "(SELECT TOTAL(o.f_REALCURRENTAMOUNT) FROM v_suboperation_consolidated o WHERE o.t_TYPEACCOUNT<>'L' AND o.i_SUBOPID IN "
                             "(SELECT b2.id_suboperation FROM budgetsuboperation b2 WHERE b2.id=budget.id)"
                             ") AS f_CURRENTAMOUNT, "
                             "(SELECT GROUP_CONCAT(v_budgetrule_displayname.t_displayname,',') FROM v_budgetrule_displayname WHERE "
                             "(v_budgetrule_displayname.t_year_condition='N' OR budget.i_year=v_budgetrule_displayname.i_year) AND "
                             "(v_budgetrule_displayname.t_month_condition='N' OR budget.i_month=v_budgetrule_displayname.i_month) AND "
                             "(v_budgetrule_displayname.t_category_condition='N' OR budget.rc_category_id=v_budgetrule_displayname.rc_category_id) "
                             "ORDER BY v_budgetrule_displayname.t_absolute DESC, v_budgetrule_displayname.id) AS t_RULES "
                             "FROM budget")

            << QStringLiteral("CREATE VIEW  v_budget AS "
                             "SELECT *, "
                             "(f_CURRENTAMOUNT-f_budgeted_modified) AS f_DELTABEFORETRANSFER, "
                             "(f_CURRENTAMOUNT-f_budgeted_modified-f_transferred) AS f_DELTA "
                             "FROM v_budget_tmp")

            << QStringLiteral("CREATE VIEW  v_budget_display AS "
                             "SELECT *, "
                             "(f_CURRENTAMOUNT-f_budgeted_modified) AS f_DELTABEFORETRANSFER, "
                             "(f_CURRENTAMOUNT-f_budgeted_modified-f_transferred) AS f_DELTA "
                             "FROM vm_budget_tmp")

            << QStringLiteral("CREATE VIEW  v_budget_displayname AS "
                             "SELECT *, t_CATEGORY||' '||t_PERIOD||' '||f_budgeted_modified AS t_displayname FROM v_budget")

            << QStringLiteral("CREATE VIEW  v_operation_all_comment AS "
                             "SELECT t_comment FROM operation UNION SELECT t_comment FROM suboperation");

    IFOKDO(err, SKGDocument::refreshViewsIndexesAndTriggers(iForce))
    QStringList tables;
    tables << QStringLiteral("account") << QStringLiteral("unit") << QStringLiteral("unitvalue") << QStringLiteral("bank")
           << QStringLiteral("recurrentoperation") << QStringLiteral("refund") << QStringLiteral("payee") << QStringLiteral("operation")
           << QStringLiteral("operationbalance") << QStringLiteral("interest") << QStringLiteral("rule") << QStringLiteral("suboperation")
           << QStringLiteral("budget") << QStringLiteral("budgetrule") << QStringLiteral("budgetcategory") << QStringLiteral("budgetsuboperation")
           << QStringLiteral("category");
    IFOKDO(err, dropViewsAndIndexes(tables))
    IFOKDO(err, executeSqliteOrders(BankInitialDataModelIndex))
    IFOKDO(err, executeSqliteOrders(BankInitialDataModelView))
    IFOKDO(err, executeSqliteOrders(BankInitialDataModelTrigger))
    IFOKDO(err, executeSqliteOrder(QStringLiteral("ANALYZE")))

    return err;
}

QStringList SKGDocumentBank::getMigationSteps()
{
    SKGTRACEINFUNC(5)
    QStringList migrationSteps;
    migrationSteps.reserve(1000);
    migrationSteps
    // ============
            << QString()
            << QStringLiteral("0.1")
            << QStringLiteral("0.2")
            << QStringLiteral("ALTER TABLE unit ADD COLUMN rc_unit_id INTEGER NOT NULL DEFAULT 0")
            // ============
            << QString()
            << QStringLiteral("0.2")
            << QStringLiteral("0.3")
            << QStringLiteral("DROP TABLE IF EXISTS unitvalue2")
            << QStringLiteral("CREATE TABLE unitvalue2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "rd_unit_id INTEGER NOT NULL,"
                             "d_date DATE NOT NULL,"
                             "f_quantity FLOAT NOT NULL CHECK (f_quantity>=0))")

            << QStringLiteral("INSERT INTO unitvalue2 (id,rd_unit_id,d_date,f_quantity) SELECT id,rd_unit_id,d_date,f_value FROM unitvalue")

            << QStringLiteral("DROP TABLE IF EXISTS unitvalue")
            << QStringLiteral("ALTER TABLE unitvalue2 RENAME TO unitvalue")
            // ============
            << QString()
            << QStringLiteral("0.3")
            << QStringLiteral("0.4")
            << QStringLiteral("ALTER TABLE operation ADD COLUMN t_import_id TEXT DEFAULT ''")
            // ============
            << QString()
            << QStringLiteral("0.4")
            << QStringLiteral("0.5")
            << QStringLiteral("DROP TABLE IF EXISTS recurrentoperation")
            << QStringLiteral("CREATE TABLE recurrentoperation ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                             "rd_operation_id INTEGER NOT NULL,"
                             "i_period_increment INTEGER NOT NULL DEFAULT 1 CHECK (i_period_increment>=0),"
                             "t_period_unit TEXT NOT NULL DEFAULT 'M' CHECK (t_period_unit IN ('D', 'M', 'Y')),"
                             "t_auto_write VARCHAR(1) DEFAULT 'Y' CHECK (t_auto_write IN ('Y', 'N')),"
                             "i_auto_write_days INTEGER NOT NULL DEFAULT 5 CHECK (i_auto_write_days>=0),"
                             "t_warn VARCHAR(1) DEFAULT 'Y' CHECK (t_auto_write IN ('Y', 'N')),"
                             "i_warn_days INTEGER NOT NULL DEFAULT 5 CHECK (i_warn_days>=0)"
                             ")")
            << QStringLiteral("ALTER TABLE operation ADD COLUMN r_recurrentoperation_id INTEGER NOT NULL DEFAULT 0")
            // ============
            << QString()
            << QStringLiteral("0.5")
            << QStringLiteral("0.6")
            << QStringLiteral("ALTER TABLE account ADD COLUMN t_comment TEXT NOT NULL DEFAULT ''")
            // ============
            << QString()
            << QStringLiteral("0.6")
            << QStringLiteral("0.7")
            << QStringLiteral("DROP TABLE IF EXISTS unit2")
            << QStringLiteral("CREATE TABLE unit2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_name TEXT NOT NULL,"
                             "t_symbol TEXT NOT NULL DEFAULT '',"
                             "t_country TEXT NOT NULL DEFAULT '',"
                             "t_type VARCHAR(1) NOT NULL DEFAULT 'C' CHECK (t_type IN ('1', '2', 'C', 'S', 'O')),"
                             // 1=main currency, 2=secondary currency, C=currencies S=share O=object
                             "t_internet_code TEXT NOT NULL DEFAULT '',"
                             "rd_unit_id INTEGER NOT NULL DEFAULT 0)")

            << QStringLiteral("INSERT INTO unit2 (id,t_name,t_symbol,t_country,t_type,t_internet_code,rd_unit_id) SELECT id,t_name,t_symbol,t_country,t_type,t_internet_code,rc_unit_id FROM unit")

            << QStringLiteral("DROP TABLE IF EXISTS unit")
            << QStringLiteral("ALTER TABLE unit2 RENAME TO unit")
            // ============
            << QString()
            << QStringLiteral("0.7")
            << QStringLiteral("0.8")
            << QStringLiteral("DELETE FROM operation WHERE id IN (SELECT id FROM operation op WHERE NOT EXISTS(SELECT 1 FROM suboperation sop WHERE sop.rd_operation_id=op.id))")
            // ============
            << QString()
            << QStringLiteral("0.8")
            << QStringLiteral("0.9")
            << QStringLiteral("UPDATE operation SET i_group_id=0 WHERE i_group_id=''")
            // ============
            << QString()
            << QStringLiteral("0.9")
            << QStringLiteral("1.0")
            << QStringLiteral("DROP TABLE IF EXISTS unit2")
            << QStringLiteral("CREATE TABLE unit2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_name TEXT NOT NULL,"
                             "t_symbol TEXT NOT NULL DEFAULT '',"
                             "t_country TEXT NOT NULL DEFAULT '',"
                             "t_type VARCHAR(1) NOT NULL DEFAULT 'C' CHECK (t_type IN ('1', '2', 'C', 'S', 'I', 'O')),"
                             // 1=main currency, 2=secondary currency, C=currencies S=share, I=index, O=object
                             "t_internet_code TEXT NOT NULL DEFAULT '',"
                             "rd_unit_id INTEGER NOT NULL DEFAULT 0)")

            << QStringLiteral("INSERT INTO unit2 (id,t_name,t_symbol,t_country,t_type,t_internet_code,rd_unit_id) SELECT id,t_name,t_symbol,t_country,t_type,t_internet_code,rd_unit_id FROM unit")

            << QStringLiteral("DROP TABLE IF EXISTS unit")
            << QStringLiteral("ALTER TABLE unit2 RENAME TO unit")
            // ============
            << QString()
            << QStringLiteral("1.0")
            << QStringLiteral("1.1")
            << QStringLiteral("DELETE FROM parameters WHERE t_name LIKE 'SKG_MONTHLY_REPORT_%'")
            // ============
            << QString()
            << QStringLiteral("1.1")
            << QStringLiteral("1.2")
            << QStringLiteral("ALTER TABLE suboperation ADD COLUMN t_comment TEXT NOT NULL DEFAULT ''")
            // ============
            << QString()
            << QStringLiteral("1.2")
            << QStringLiteral("1.3")
            << QStringLiteral("UPDATE node SET f_sortorder=id WHERE f_sortorder IS NULL OR f_sortorder=''")
            // ============
            << QString()
            << QStringLiteral("1.3")
            << QStringLiteral("1.4")
            << QStringLiteral("DROP TABLE IF EXISTS refund")
            << QStringLiteral("CREATE TABLE refund ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_name TEXT NOT NULL DEFAULT '',"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')))")

            << QStringLiteral("ALTER TABLE suboperation ADD COLUMN r_refund_id INTEGER NOT NULL DEFAULT 0")
            // ============
            << QString()
            << QStringLiteral("1.4")
            << QStringLiteral("1.5")
            << QStringLiteral("DELETE FROM parameters WHERE (t_name LIKE 'SKG_DEFAULT_%' AND t_name!='SKG_DEFAULT_PROPERTIES') OR t_name='DBVERSION'")
            // ============
            << QString()
            << QStringLiteral("1.5")
            << QStringLiteral("1.6")
            << QStringLiteral("CREATE TABLE rule ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_description TEXT NOT NULL DEFAULT '',"
                             "t_definition TEXT NOT NULL DEFAULT '',"
                             "f_sortorder FLOAT"
                             ")")
            // ============
            << QString()
            << QStringLiteral("1.6")
            << QStringLiteral("1.7")
            << QStringLiteral("CREATE TABLE action ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "rd_rule_id INTEGER NOT NULL,"
                             "t_description TEXT NOT NULL DEFAULT '',"
                             "t_definition TEXT NOT NULL DEFAULT ''"
                             ")")
            << QStringLiteral("DELETE FROM rule")
            << QStringLiteral("DROP TABLE IF EXISTS budget")
            // ============
            << QString()
            << QStringLiteral("1.7")
            << QStringLiteral("1.8")
            << QStringLiteral("DROP TABLE IF EXISTS action")
            << QStringLiteral("ALTER TABLE rule ADD COLUMN t_action_description TEXT NOT NULL DEFAULT ''")
            << QStringLiteral("ALTER TABLE rule ADD COLUMN t_action_definition TEXT NOT NULL DEFAULT ''")
            << QStringLiteral("DROP TRIGGER IF EXISTS fkdc_rule_action_id_rd_rule_id")
            // ============
            << QString()
            << QStringLiteral("1.8")
            << QStringLiteral("1.9")
            << QStringLiteral("DROP TABLE IF EXISTS operation2")
            << QStringLiteral("CREATE TABLE operation2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "i_group_id INTEGER NOT NULL DEFAULT 0,"
                             "i_number INTEGER DEFAULT 0 CHECK (i_number>=0),"
                             "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                             "rd_account_id INTEGER NOT NULL,"
                             "t_mode TEXT NOT NULL DEFAULT '',"
                             "t_payee TEXT NOT NULL DEFAULT '',"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "rc_unit_id INTEGER NOT NULL,"
                             "t_status VARCHAR(1) DEFAULT 'N' CHECK (t_status IN ('N', 'P', 'Y')),"
                             "t_bookmarked VARCHAR(1) DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "t_imported VARCHAR(1) DEFAULT 'N' CHECK (t_imported IN ('Y', 'N', 'P')),"
                             "t_import_id TEXT DEFAULT '',"
                             "r_recurrentoperation_id INTEGER NOT NULL DEFAULT 0)")

            << QStringLiteral("INSERT INTO operation2 (id,i_group_id,i_number,d_date,rd_account_id,t_mode,t_payee,t_comment,rc_unit_id,"
                             "t_status,t_bookmarked,t_imported,t_import_id,r_recurrentoperation_id) "
                             "SELECT id,i_group_id,i_number,d_date,rd_account_id,t_mode,t_payee,t_comment,rc_unit_id,"
                             "(CASE WHEN t_status='C' THEN 'Y' ELSE t_status END),t_bookmarked,t_imported,t_import_id,r_recurrentoperation_id FROM operation")

            << QStringLiteral("DROP TABLE IF EXISTS operation")
            << QStringLiteral("ALTER TABLE operation2 RENAME TO operation")
            // ============
            << QString()
            << QStringLiteral("1.9")
            << QStringLiteral("2.0")
            << QStringLiteral("ALTER TABLE operation ADD COLUMN i_tmp INTEGER NOT NULL DEFAULT 0")
            << QStringLiteral("ALTER TABLE suboperation ADD COLUMN i_tmp INTEGER NOT NULL DEFAULT 0")
            // ============
            << QString()
            << QStringLiteral("2.0")
            << QStringLiteral("2.1")
            << QStringLiteral("ALTER TABLE operation ADD COLUMN t_template VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_template IN ('Y', 'N'))")
            // ============
            << QString()
            << QStringLiteral("2.1")
            << QStringLiteral("2.2")
            << QStringLiteral("UPDATE recurrentoperation SET d_date=date(d_date,i_period_increment||(CASE WHEN t_period_unit='Y' THEN ' year' ELSE (CASE WHEN t_period_unit='M' THEN ' month' ELSE ' day' END) END))")


            << QStringLiteral("DROP TABLE IF EXISTS recurrentoperation2")
            << QStringLiteral("CREATE TABLE recurrentoperation2 ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                             "rd_operation_id INTEGER NOT NULL,"
                             "i_period_increment INTEGER NOT NULL DEFAULT 1 CHECK (i_period_increment>=0),"
                             "t_period_unit TEXT NOT NULL DEFAULT 'M' CHECK (t_period_unit IN ('D', 'M', 'Y')),"
                             "t_auto_write VARCHAR(1) DEFAULT 'Y' CHECK (t_auto_write IN ('Y', 'N')),"
                             "i_auto_write_days INTEGER NOT NULL DEFAULT 5 CHECK (i_auto_write_days>=0),"
                             "t_warn VARCHAR(1) DEFAULT 'Y' CHECK (t_warn IN ('Y', 'N')),"
                             "i_warn_days INTEGER NOT NULL DEFAULT 5 CHECK (i_warn_days>=0),"
                             "t_times VARCHAR(1) DEFAULT 'N' CHECK (t_times IN ('Y', 'N')),"
                             "i_nb_times INTEGER NOT NULL DEFAULT 1 CHECK (i_nb_times>=0)"
                             ")")

            << QStringLiteral("INSERT INTO recurrentoperation2 (id,d_date,rd_operation_id,i_period_increment,t_period_unit,t_auto_write,i_auto_write_days,t_warn,i_warn_days) "
                             "SELECT id,d_date,rd_operation_id,i_period_increment,t_period_unit,t_auto_write,i_auto_write_days,t_warn,i_warn_days FROM recurrentoperation")

            << QStringLiteral("DROP TABLE IF EXISTS recurrentoperation")
            << QStringLiteral("ALTER TABLE recurrentoperation2 RENAME TO recurrentoperation")
            // ============
            << QString()
            << QStringLiteral("2.2")
            << QStringLiteral("2.3")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'%9','#ATT#')")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'''%1''','''#V1S#''')")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'''%2''','''#V2S#''')")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'''%%1%''','''%#V1S#%''')")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'''%1%''','''#V1S#%''')")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'''%%1''','''%#V1S#''')")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'%1','#V1#')")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'%2','#V2#')")

            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'%9','#ATT#')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'''%1''','''#V1S#''')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'''%2''','''#V2S#''')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'''%%1%''','''%#V1S#%''')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'''%1%''','''#V1S#%''')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'''%%1''','''%#V1S#''')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'%1','#V1#')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'%2','#V2#')")
            // ============
            << QString()
            << QStringLiteral("2.3")
            << QStringLiteral("2.4")
            << QStringLiteral("UPDATE operation SET t_template='N' WHERE t_template NOT IN ('Y', 'N')")
            // ============
            << QString()
            << QStringLiteral("2.4")
            << QStringLiteral("2.5")
            << QStringLiteral("ALTER TABLE rule ADD COLUMN t_action_type VARCHAR(1) DEFAULT 'S' CHECK (t_action_type IN ('S', 'U', 'A'))")
            << QStringLiteral("UPDATE rule SET t_action_type='S' WHERE t_action_type NOT IN ('S', 'U', 'A') AND  t_action_definition=''")
            << QStringLiteral("UPDATE rule SET t_action_type='U' WHERE t_action_type NOT IN ('S', 'U', 'A') AND  t_action_definition!=''")
            // ============
            << QString()
            << QStringLiteral("2.5")
            << QStringLiteral("2.6")
            << QStringLiteral("ALTER TABLE unit ADD COLUMN i_nbdecimal INT NOT NULL DEFAULT 2")
            << QStringLiteral("UPDATE unit SET i_nbdecimal=2")
            // ============
            << QString()
            << QStringLiteral("2.6")
            << QStringLiteral("2.7")
            << QStringLiteral("CREATE TABLE operation2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "i_group_id INTEGER NOT NULL DEFAULT 0,"
                             "i_number INTEGER DEFAULT 0 CHECK (i_number>=0),"
                             "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                             "rd_account_id INTEGER NOT NULL,"
                             "t_mode TEXT NOT NULL DEFAULT '',"
                             "t_payee TEXT NOT NULL DEFAULT '',"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "rc_unit_id INTEGER NOT NULL,"
                             "t_status VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_status IN ('N', 'P', 'Y')),"
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "t_imported VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_imported IN ('Y', 'N', 'P', 'T')),"
                             "t_template VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_template IN ('Y', 'N')),"
                             "t_import_id TEXT NOT NULL DEFAULT '',"
                             "i_tmp INTEGER NOT NULL DEFAULT 0,"
                             "r_recurrentoperation_id INTEGER NOT NULL DEFAULT 0)")

            << QStringLiteral("INSERT INTO operation2 ("
                             "id,i_group_id,i_number,d_date,rd_account_id,t_mode,t_payee,t_comment,rc_unit_id,t_status,t_bookmarked,t_imported,t_template,t_import_id,i_tmp,r_recurrentoperation_id) "
                             "SELECT id,i_group_id,i_number,d_date,rd_account_id,t_mode,t_payee,t_comment,rc_unit_id,t_status,t_bookmarked,t_imported,t_template,t_import_id,i_tmp,r_recurrentoperation_id FROM operation")

            << QStringLiteral("DROP TABLE IF EXISTS operation")
            << QStringLiteral("ALTER TABLE operation2 RENAME TO operation")
            // ============
            << QString()
            << QStringLiteral("2.7")
            << QStringLiteral("2.8")
            << QStringLiteral("UPDATE rule SET t_action_type='U' WHERE t_action_type='S' AND  t_action_definition!=''")
            // ============
            << QString()
            << QStringLiteral("2.8")
            << QStringLiteral("2.9")
            << QStringLiteral("DROP TABLE IF EXISTS interest")
            << QStringLiteral("CREATE TABLE interest("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "rd_account_id INTEGER NOT NULL,"
                             "d_date DATE NOT NULL,"
                             "f_rate FLOAT NOT NULL CHECK (f_rate>=0),"
                             "t_income_value_date_mode VARCHAR(1) NOT NULL DEFAULT 'F' CHECK (t_income_value_date_mode IN ('F', '0', '1', '2', '3', '4', '5')),"
                             "t_expenditure_value_date_mode VARCHAR(1) NOT NULL DEFAULT 'F' CHECK (t_expenditure_value_date_mode IN ('F', '0', '1', '2', '3', '4', '5')),"
                             "t_base VARCHAR(3) NOT NULL DEFAULT '24' CHECK (t_base IN ('24', '360', '365'))"
                             ")")
            // ============
            << QString()
            << QStringLiteral("2.9")
            << QStringLiteral("3.0")
            // Current month
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"1\"\"', 'period=\"\"1\"\" interval=\"\"2\"\" nb_intervals=\"\"1\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Previous month
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"2\"\"', 'period=\"\"2\"\" interval=\"\"2\"\" nb_intervals=\"\"1\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Current year
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"3\"\"', 'period=\"\"1\"\" interval=\"\"3\"\" nb_intervals=\"\"1\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Previous year
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"4\"\"', 'period=\"\"2\"\" interval=\"\"3\"\" nb_intervals=\"\"1\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Last 30 days
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"5\"\"', 'period=\"\"3\"\" interval=\"\"0\"\" nb_intervals=\"\"30\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Last 3 months
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"6\"\"', 'period=\"\"3\"\" interval=\"\"2\"\" nb_intervals=\"\"3\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Last 6 months
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"7\"\"', 'period=\"\"3\"\" interval=\"\"2\"\" nb_intervals=\"\"6\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Last 12 months
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"8\"\"', 'period=\"\"3\"\" interval=\"\"2\"\" nb_intervals=\"\"12\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Last 2 years
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"9\"\"', 'period=\"\"3\"\" interval=\"\"3\"\" nb_intervals=\"\"2\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Last 3 years
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"10\"\"', 'period=\"\"3\"\" interval=\"\"3\"\" nb_intervals=\"\"3\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Last 5 years
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"11\"\"', 'period=\"\"3\"\" interval=\"\"3\"\" nb_intervals=\"\"5\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Custom…
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'period=\"\"12\"\"', 'period=\"\"4\"\"') WHERE t_data like '%Skrooge report plugin%'")

            // All without transfers
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'type=\"\"0\"\"', 'incomes=\"\"Y\"\" expenses=\"\"Y\"\" transfers=\"\"N\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Income without transfers
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'type=\"\"1\"\"', 'incomes=\"\"Y\"\" expenses=\"\"N\"\" transfers=\"\"N\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Expenditure without transfers
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'type=\"\"2\"\"', 'incomes=\"\"N\"\" expenses=\"\"Y\"\" transfers=\"\"N\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // All with transfers
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'type=\"\"3\"\"', 'incomes=\"\"Y\"\" expenses=\"\"Y\"\" transfers=\"\"Y\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Income with transfers
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'type=\"\"4\"\"', 'incomes=\"\"Y\"\" expenses=\"\"N\"\" transfers=\"\"Y\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // Expenditure with transfers
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'type=\"\"5\"\"', 'incomes=\"\"N\"\" expenses=\"\"Y\"\" transfers=\"\"Y\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // ============
            << QString()
            << QStringLiteral("3.0")
            << QStringLiteral("3.1")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'columns=\"\"2\"\"', 'columns=\"\"4\"\"') WHERE t_data like '%Skrooge report plugin%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'columns=\"\"1\"\"', 'columns=\"\"3\"\"') WHERE t_data like '%Skrooge report plugin%'")
            // ============
            << QString()
            << QStringLiteral("3.1")
            << QStringLiteral("3.2")
            << QStringLiteral("UPDATE parameters SET t_name='SKGSEARCH_DEFAULT_PARAMETERS' WHERE t_name='SKGIMPORT_DEFAULT_PARAMETERS'")
            // ============
            << QString()
            << QStringLiteral("3.2")
            << QStringLiteral("3.3")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=&quot;4&quot;', ' columns=&quot;&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=&quot;3&quot;', ' columns=&quot;d_DATEYEAR&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=&quot;2&quot;', ' columns=&quot;d_DATESEMESTER&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=&quot;1&quot;', ' columns=&quot;d_DATEQUARTER&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=&quot;0&quot;', ' columns=&quot;d_DATEMONTH&quot;') WHERE t_data like '%graphicViewState=%'")

            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=\"\"4\"\"', ' columns=\"\"\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=\"\"3\"\"', ' columns=\"\"d_DATEYEAR\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=\"\"2\"\"', ' columns=\"\"d_DATESEMESTER\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=\"\"1\"\"', ' columns=\"\"d_DATEQUARTER\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=\"\"0\"\"', ' columns=\"\"d_DATEMONTH\"\"') WHERE t_data like '%graphicViewState=%'")

            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=&quot;4&quot;', ' columns=&quot;&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=&quot;3&quot;', ' columns=&quot;d_DATEYEAR&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=&quot;2&quot;', ' columns=&quot;d_DATESEMESTER&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=&quot;1&quot;', ' columns=&quot;d_DATEQUARTER&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=&quot;0&quot;', ' columns=&quot;d_DATEMONTH&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")


            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;0&quot;', ' lines=&quot;t_REALCATEGORY&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;1&quot;', ' lines=&quot;t_payee&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;2&quot;', ' lines=&quot;t_mode&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;3&quot;', ' lines=&quot;t_TYPEEXPENSENLS&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;4&quot;', ' lines=&quot;t_status&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;5&quot;', ' lines=&quot;t_ACCOUNTTYPE&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;6&quot;', ' lines=&quot;t_UNITTYPE&quot;') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;7&quot;', ' lines=&quot;t_REALREFUND&quot;') WHERE t_data like '%graphicViewState=%'")

            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"0\"\"', ' lines=\"\"t_REALCATEGORY\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"1\"\"', ' lines=\"\"t_payee\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"2\"\"', ' lines=\"\"t_mode\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"3\"\"', ' lines=\"\"t_TYPEEXPENSENLS\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"4\"\"', ' lines=\"\"t_status\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"5\"\"', ' lines=\"\"t_ACCOUNTTYPE\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"6\"\"', ' lines=\"\"t_UNITTYPE\"\"') WHERE t_data like '%graphicViewState=%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"7\"\"', ' lines=\"\"t_REALREFUND\"\"') WHERE t_data like '%graphicViewState=%'")

            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&quot;0&quot;', ' lines=&quot;t_REALCATEGORY&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&quot;1&quot;', ' lines=&quot;t_payee&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&quot;2&quot;', ' lines=&quot;t_mode&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&quot;3&quot;', ' lines=&quot;t_TYPEEXPENSENLS&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&quot;4&quot;', ' lines=&quot;t_status&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&quot;5&quot;', ' lines=&quot;t_ACCOUNTTYPE&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&quot;6&quot;', ' lines=&quot;t_UNITTYPE&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&quot;7&quot;', ' lines=&quot;t_REALREFUND&quot;') WHERE t_name='SKGDASHBOARD_DEFAULT_PARAMETERS'")
            // ============
            << QString()
            << QStringLiteral("3.3")
            << QStringLiteral("3.4")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=\"4\"', ' columns=\"\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=\"3\"', ' columns=\"d_DATEYEAR\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=\"2\"', ' columns=\"d_DATESEMESTER\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=\"1\"', ' columns=\"d_DATEQUARTER\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=\"0\"', ' columns=\"d_DATEMONTH\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")


            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=\"0\"', ' lines=\"t_REALCATEGORY\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=\"1\"', ' lines=\"t_payee\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=\"2\"', ' lines=\"t_mode\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=\"3\"', ' lines=\"t_TYPEEXPENSENLS\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=\"4\"', ' lines=\"t_status\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=\"5\"', ' lines=\"t_ACCOUNTTYPE\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=\"6\"', ' lines=\"t_UNITTYPE\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=\"7\"', ' lines=\"t_REALREFUND\"') WHERE t_name='SKGREPORT_DEFAULT_PARAMETERS'")
            // ============
            << QString()
            << QStringLiteral("3.4")
            << QStringLiteral("3.5")
            << QStringLiteral("ALTER TABLE account ADD COLUMN t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'))")
            << QStringLiteral("UPDATE account SET t_bookmarked='N'")
            // ============
            << QString()
            << QStringLiteral("3.5")
            << QStringLiteral("3.6")
            << QStringLiteral("ALTER TABLE rule ADD COLUMN t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'))")
            << QStringLiteral("UPDATE rule SET t_bookmarked='N'")
            // ============
            << QString()
            << QStringLiteral("3.6")
            << QStringLiteral("3.7")
            << QStringLiteral("UPDATE suboperation SET r_category_id=0 WHERE r_category_id=(SELECT id FROM category WHERE t_name='')")
            << QStringLiteral("DELETE FROM category WHERE t_name=''")
            // ============
            << QString()
            << QStringLiteral("3.7")
            << QStringLiteral("3.8")
            << QStringLiteral("UPDATE recurrentoperation SET t_times='N' WHERE t_times IS NULL")
            // ============
            << QString()
            << QStringLiteral("3.8")
            << QStringLiteral("3.9")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 'Skrooge dashboard plugin', 'Dashboard plugin') WHERE t_data like '%Skrooge dashboard plugin%'")
            // ============
            << QString()
            << QStringLiteral("3.9")
            << QStringLiteral("4.0")
            << "UPDATE rule SET t_definition=replace(t_definition, '" % SKGServices::stringToSqlString(QStringLiteral("date('now', 'localtime','-1")) % "', '" % SKGServices::stringToSqlString(QStringLiteral("date('now', 'localtime','start of month','-1")) % "')"
            // ============
            << QString()
            << QStringLiteral("4.0")
            << QStringLiteral("4.1")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'t_REFUND','t_REALREFUND')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition,'t_REFUND','t_REALREFUND')")
            // ============
            << QString()
            << QStringLiteral("4.1")
            << QStringLiteral("4.2")
            << QStringLiteral("UPDATE operation SET t_imported='Y' WHERE t_imported='T'")
            << QStringLiteral("UPDATE operation SET t_imported='N' WHERE t_imported!='N' AND t_import_id='';")
            // ============
            << QString()
            << QStringLiteral("4.2")
            << QStringLiteral("4.3")
            <<  QStringLiteral("CREATE TABLE payee ("
                              "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                              "t_name TEXT NOT NULL DEFAULT '',"
                              "t_address TEXT NOT NULL DEFAULT '')")

            << QStringLiteral("INSERT INTO payee (t_name) "
                             "SELECT distinct(operation.t_payee) FROM operation WHERE operation.t_payee<>''")

            << QStringLiteral("CREATE TABLE operation2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "i_group_id INTEGER NOT NULL DEFAULT 0,"
                             "i_number INTEGER DEFAULT 0 CHECK (i_number>=0),"
                             "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                             "rd_account_id INTEGER NOT NULL,"
                             "t_mode TEXT NOT NULL DEFAULT '',"
                             "r_payee_id INTEGER NOT NULL DEFAULT 0,"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "rc_unit_id INTEGER NOT NULL,"
                             "t_status VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_status IN ('N', 'P', 'Y')),"
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "t_imported VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_imported IN ('Y', 'N', 'P', 'T')),"
                             "t_template VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_template IN ('Y', 'N')),"
                             "t_import_id TEXT NOT NULL DEFAULT '',"
                             "i_tmp INTEGER NOT NULL DEFAULT 0,"
                             "r_recurrentoperation_id INTEGER NOT NULL DEFAULT 0)")

            << QStringLiteral("INSERT INTO operation2 ("
                             "id,i_group_id,i_number,d_date,rd_account_id,t_mode,r_payee_id,t_comment,rc_unit_id,t_status,t_bookmarked,t_imported,t_template,t_import_id,i_tmp,r_recurrentoperation_id) "
                             "SELECT id,i_group_id,i_number,d_date,rd_account_id,t_mode,(CASE WHEN (SELECT payee.id FROM payee WHERE payee.t_name=operation.t_payee) IS NULL THEN 0 ELSE (SELECT payee.id FROM payee WHERE payee.t_name=operation.t_payee) END),t_comment,rc_unit_id,t_status,t_bookmarked,t_imported,t_template,t_import_id,i_tmp,r_recurrentoperation_id FROM operation")

            << QStringLiteral("DROP TABLE IF EXISTS operation")
            << QStringLiteral("ALTER TABLE operation2 RENAME TO operation")

            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, 't_payee', 't_PAYEE') WHERE t_name like '%_DEFAULT_PARAMETERS'")
            // ============
            << QString()
            << QStringLiteral("4.3")
            << QStringLiteral("4.4")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition, 't_payee', 't_PAYEE') WHERE t_definition like '%t_payee'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, 't_payee', 't_PAYEE') WHERE t_data like '%t_payee'")
            // ============
            << QString()
            << QStringLiteral("4.4")
            << QStringLiteral("4.5")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition, 't_payee', 't_PAYEE') WHERE t_definition like '%t_payee%'")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition, 't_payee', 't_PAYEE') WHERE t_action_definition like '%t_payee%'")
            // ============
            << QString()
            << QStringLiteral("4.5")
            << QStringLiteral("4.6")
            << QStringLiteral("DELETE FROM suboperation WHERE NOT EXISTS (SELECT 1 FROM operation WHERE operation.id=suboperation.rd_operation_id)")
            // ============
            << QString()
            << QStringLiteral("4.6")
            << QStringLiteral("4.7")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' smoothScrolling=&quot;N&quot;', ' zoomPosition=&quot;0&quot;') WHERE t_data like '% smoothScrolling=&quot;N&quot;%'")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' smoothScrolling=&quot;Y&quot;', ' zoomPosition=&quot;0&quot;') WHERE t_data like '% smoothScrolling=&quot;Y&quot;%'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' smoothScrolling=&quot;N&quot;', ' zoomPosition=&quot;0&quot;') WHERE t_value like '% smoothScrolling=&quot;N&quot;%'")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' smoothScrolling=&quot;Y&quot;', ' zoomPosition=&quot;0&quot;') WHERE t_value like '% smoothScrolling=&quot;Y&quot;%'")
            // ============
            << QString()
            << QStringLiteral("4.7")
            << QStringLiteral("4.8")
            << QStringLiteral("CREATE TABLE operationbalance("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "f_balance FLOAT NOT NULL DEFAULT 0,"
                             "r_operation_id INTEGER NOT NULL)")
            // ============
            << QString()
            << QStringLiteral("4.8")
            << QStringLiteral("4.9")
            <<
            QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;t_ACCOUNTTYPE&quot; nbLevelLines=&quot;0&quot;', ' lines=&quot;&quot; nbLevelLines=&quot;0&quot;')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"t_ACCOUNTTYPE\"\" nbLevelLines=\"\"0\"\"', ' lines=\"\"\"\" nbLevelLines=\"\"0\"\"')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&amp;quot;t_ACCOUNTTYPE&amp;quot; nbLevelLines=&amp;quot;0&amp;quot;', ' lines=&amp;quot;&amp;quot; nbLevelLines=&amp;quot;0&amp;quot;')")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&amp;quot;t_ACCOUNTTYPE&amp;quot; nbLevelLines=&amp;quot;0&amp;quot;', ' lines=&amp;quot;&amp;quot; nbLevelLines=&amp;quot;0&amp;quot;')")

            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;t_UNITTYPE&quot;', ' lines=&quot;t_UNITTYPE&quot; lines2=&quot;t_UNIT&quot;')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"t_UNITTYPE\"\"', ' lines=\"\"t_UNITTYPE\"\"  lines2=\"\"t_UNIT\"\"')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&amp;quot;t_UNITTYPE&amp;quot;', ' lines=&amp;quot;t_UNITTYPE&amp;quot; lines2=&amp;quot;t_UNIT&amp;quot;')")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&amp;quot;t_UNITTYPE&amp;quot;', ' lines=&amp;quot;t_UNITTYPE&amp;quot; lines2=&amp;quot;t_UNIT&amp;quot;')")

            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;t_ACCOUNTTYPE&quot;', ' lines=&quot;t_ACCOUNTTYPE&quot; lines2=&quot;t_ACCOUNT&quot;')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"t_ACCOUNTTYPE\"\"', ' lines=\"\"t_ACCOUNTTYPE\"\"  lines2=\"\"t_ACCOUNT\"\"')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&amp;quot;t_ACCOUNTTYPE&amp;quot;', ' lines=&amp;quot;t_ACCOUNTTYPE&amp;quot; lines2=&amp;quot;t_ACCOUNT&amp;quot;')")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&amp;quot;t_ACCOUNTTYPE&amp;quot;', ' lines=&amp;quot;t_ACCOUNTTYPE&amp;quot; lines2=&amp;quot;t_ACCOUNT&amp;quot;')")

            // ============
            << QString()
            << QStringLiteral("4.9")
            << QStringLiteral("5.0")
            << QStringLiteral("CREATE TABLE budget ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "rc_category_id INTEGER NOT NULL DEFAULT 0,"
                             "f_budgeted FLOAT NOT NULL DEFAULT 0.0,"
                             "i_year INTEGER NOT NULL DEFAULT 2010,"
                             "i_month INTEGER NOT NULL DEFAULT 0 CHECK (i_month>=0 AND i_month<=12)"
                             ")")
            // ============
            << QString()
            << QStringLiteral("5.0")
            << QStringLiteral("5.1")
            << QStringLiteral("CREATE TABLE budgetrule ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "rc_category_id INTEGER NOT NULL DEFAULT 0,"
                             "i_year INTEGER NOT NULL DEFAULT 2010,"
                             "i_month INTEGER NOT NULL DEFAULT 0 CHECK (i_month>=0 AND i_month<=12),"
                             "i_condition INTEGER NOT NULL DEFAULT 0 CHECK (i_condition IN (-1,0,1)),"
                             "f_quantity FLOAT NOT NULL DEFAULT 0.0,"
                             "t_absolute TEXT NOT NULL DEFAULT 'Y' CHECK (t_absolute IN ('Y', 'N')),"
                             "rc_category_id_target INTEGER NOT NULL DEFAULT 0,"
                             "t_rule TEXT NOT NULL DEFAULT 'N' CHECK (t_rule IN ('N', 'C', 'Y'))"
                             ")")
            // ============
            << QString()
            << QStringLiteral("5.1")
            << QStringLiteral("5.2")
            << QStringLiteral("CREATE TABLE budgetcategory("
                             "id INTEGER NOT NULL DEFAULT 0,"
                             "id_category INTEGER NOT NULL DEFAULT 0)")
            // ============
            << QString()
            << QStringLiteral("5.2")
            << QStringLiteral("5.3")
            << QStringLiteral("ALTER TABLE budget ADD COLUMN f_budgeted_modified FLOAT NOT NULL DEFAULT 0.0")
            << QStringLiteral("UPDATE budget SET f_budgeted_modified=f_budgeted")
            // ============
            << QString()
            << QStringLiteral("5.3")
            << QStringLiteral("5.4")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&quot;&quot;', ' lines=&quot;#NOTHING#&quot;')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=\"\"\"\"', ' lines=\"\"#NOTHING#\"\"')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines=&amp;quot;&amp;quot;', ' lines=&amp;quot;#NOTHING#&amp;quot;')")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines=&amp;quot;&amp;quot;', ' lines=&amp;quot;#NOTHING#&amp;quot;')")

            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines2=&quot;&quot;', ' lines2=&quot;#NOTHING#&quot;')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines2=\"\"\"\"', ' lines2=\"\"#NOTHING#\"\"')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' lines2=&amp;quot;&amp;quot;', ' lines2=&amp;quot;#NOTHING#&amp;quot;')")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' lines2=&amp;quot;&amp;quot;', ' lines2=&amp;quot;#NOTHING#&amp;quot;')")

            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=&quot;&quot;', ' columns=&quot;#NOTHING#&quot;')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=\"\"\"\"', ' columns=\"\"#NOTHING#\"\"')")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data, ' columns=&amp;quot;&amp;quot;', ' columns=&amp;quot;#NOTHING#&amp;quot;')")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' columns=&amp;quot;&amp;quot;', ' columns=&amp;quot;#NOTHING#&amp;quot;')")
            // ============
            << QString()
            << QStringLiteral("5.4")
            << QStringLiteral("5.5")
            << QStringLiteral("ALTER TABLE budgetrule ADD COLUMN t_category_condition TEXT NOT NULL DEFAULT 'Y' CHECK (t_category_condition IN ('Y', 'N'))")
            << QStringLiteral("ALTER TABLE budgetrule ADD COLUMN t_year_condition TEXT NOT NULL DEFAULT 'Y' CHECK (t_year_condition IN ('Y', 'N'))")
            << QStringLiteral("ALTER TABLE budgetrule ADD COLUMN t_month_condition TEXT NOT NULL DEFAULT 'Y' CHECK (t_month_condition IN ('Y', 'N'))")

            << QStringLiteral("UPDATE budgetrule SET t_year_condition='Y'")
            << QStringLiteral("UPDATE budgetrule SET t_year_condition='N', i_year=2010 WHERE i_year=0")
            << QStringLiteral("UPDATE budgetrule SET t_month_condition='Y'")
            << QStringLiteral("UPDATE budgetrule SET t_month_condition='N', i_month=1 WHERE i_month=0")
            << QStringLiteral("UPDATE budgetrule SET t_category_condition='Y'")
            << QStringLiteral("UPDATE budgetrule SET t_category_condition='N' WHERE rc_category_id=0")
            // ============
            << QString()
            << QStringLiteral("5.5")
            << QStringLiteral("5.6")
            << QStringLiteral("ALTER TABLE budgetrule ADD COLUMN t_category_target TEXT NOT NULL DEFAULT 'Y' CHECK (t_category_target IN ('Y', 'N'))")
            << QStringLiteral("UPDATE budgetrule SET t_category_target='N'")
            // ============
            << QString()
            << QStringLiteral("5.6")
            << QStringLiteral("5.7")
            << QStringLiteral("ALTER TABLE budget ADD COLUMN f_transferred FLOAT NOT NULL DEFAULT 0.0")
            << QStringLiteral("UPDATE budget SET f_transferred=0")
            // ============
            << QString()
            << QStringLiteral("5.7")
            << QStringLiteral("5.8")
            << QStringLiteral("ALTER TABLE budget ADD COLUMN t_including_subcategories TEXT NOT NULL DEFAULT 'N' CHECK (t_including_subcategories IN ('Y', 'N'));")
            << QStringLiteral("UPDATE budget SET t_including_subcategories='N'")
            // ============
            << QString()
            << QStringLiteral("5.8")
            << QStringLiteral("5.9")
            << QStringLiteral("DELETE FROM parameters WHERE t_uuid_parent='advices';")
            // ============
            << QString()
            << QStringLiteral("5.9")
            << QStringLiteral("6.0")
            << QStringLiteral("UPDATE category SET t_name=t_name;")
            // ============
            << QString()
            << QStringLiteral("6.0")
            << QStringLiteral("6.1")
            << QStringLiteral("UPDATE node SET t_data=replace(t_data,'t_type', 't_TYPENLS')")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, 't_type', 't_TYPENLS') where t_name like '%_DEFAULT_PARAMETERS'")
            // ============
            << QString()
            << QStringLiteral("6.1")
            << QStringLiteral("6.2")
            << QStringLiteral("CREATE TABLE account2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_name TEXT NOT NULL,"
                             "t_number TEXT NOT NULL DEFAULT '',"
                             "t_agency_number TEXT NOT NULL DEFAULT '',"
                             "t_agency_address TEXT NOT NULL DEFAULT '',"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                             "t_type VARCHAR(1) NOT NULL DEFAULT 'C' CHECK (t_type IN ('C', 'D', 'A', 'I', 'O', 'W')),"
                             // C=current D=credit card A=assets (for objects) I=Investment W=Wallet O=other
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "rd_bank_id INTEGER NOT NULL)")
            << QStringLiteral("INSERT INTO account2 (id, t_name, t_number, t_agency_number, t_agency_address, t_comment, t_close, t_type, t_bookmarked, rd_bank_id) "
                             "SELECT id, t_name, t_number, t_agency_number, t_agency_address, t_comment, t_close, t_type, t_bookmarked, rd_bank_id FROM account")
            << QStringLiteral("DROP TABLE IF EXISTS account")
            << QStringLiteral("ALTER TABLE account2 RENAME TO account")
            // ============
            << QString()
            << QStringLiteral("6.2")
            << QStringLiteral("6.3")
            << QStringLiteral("ALTER TABLE suboperation ADD COLUMN t_formula TEXT NOT NULL DEFAULT '';")
            << QStringLiteral("UPDATE suboperation SET t_formula=''")
            // ============
            << QString()
            << QStringLiteral("6.3")
            << QStringLiteral("6.4")
            << QStringLiteral("CREATE TABLE vm_category_display_tmp(  id INT,  t_name TEXT,  t_fullname TEXT,  rd_category_id INT,  i_NBOPERATIONS,  f_REALCURRENTAMOUNT)")
            // ============
            << QString()
            << QStringLiteral("6.4")
            << QStringLiteral("6.5")
            << QStringLiteral("ALTER TABLE category ADD COLUMN t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'));")
            << QStringLiteral("ALTER TABLE payee ADD COLUMN t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'));")
            << QStringLiteral("UPDATE category SET t_bookmarked='N'")
            << QStringLiteral("UPDATE payee SET t_bookmarked='N'")
            // ============
            << QString()
            << QStringLiteral("6.5")
            << QStringLiteral("6.6")
            << QStringLiteral("CREATE TABLE vm_budget_tmp(  id INT,  rc_category_id INT,  f_budgeted REAL,  i_year INT,  i_month INT,  f_budgeted_modified REAL,  f_transferred REAL,  t_including_subcategories TEXT,  t_CATEGORY,  t_PERIOD,  f_CURRENTAMOUNT,  t_RULES)")
            // ============
            << QString()
            << QStringLiteral("6.6")
            << QStringLiteral("6.7")
            << QStringLiteral("DROP TABLE IF EXISTS vm_category_display_tmp")
            << QStringLiteral("CREATE TABLE vm_category_display_tmp(  id INT,  t_name TEXT,  t_fullname TEXT,  rd_category_id INT,  i_NBOPERATIONS,  f_REALCURRENTAMOUNT, t_bookmarked)")
            // ============
            << QString()
            << QStringLiteral("6.7")
            << QStringLiteral("6.8")
            << QStringLiteral("CREATE TABLE recurrentoperation2 ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                             "rd_operation_id INTEGER NOT NULL,"
                             "i_period_increment INTEGER NOT NULL DEFAULT 1 CHECK (i_period_increment>=0),"
                             "t_period_unit TEXT NOT NULL DEFAULT 'M' CHECK (t_period_unit IN ('D', 'W', 'M', 'Y')),"
                             "t_auto_write VARCHAR(1) DEFAULT 'Y' CHECK (t_auto_write IN ('Y', 'N')),"
                             "i_auto_write_days INTEGER NOT NULL DEFAULT 5 CHECK (i_auto_write_days>=0),"
                             "t_warn VARCHAR(1) DEFAULT 'Y' CHECK (t_warn IN ('Y', 'N')),"
                             "i_warn_days INTEGER NOT NULL DEFAULT 5 CHECK (i_warn_days>=0),"
                             "t_times VARCHAR(1) DEFAULT 'N' CHECK (t_times IN ('Y', 'N')),"
                             "i_nb_times INTEGER NOT NULL DEFAULT 1 CHECK (i_nb_times>=0)"
                             ")")

            << QStringLiteral("INSERT INTO recurrentoperation2 (id,d_date,rd_operation_id,i_period_increment,t_period_unit,t_auto_write,i_auto_write_days,t_warn,i_warn_days,t_times,i_nb_times) "
                             "SELECT id,d_date,rd_operation_id,i_period_increment,t_period_unit,t_auto_write,i_auto_write_days,t_warn,i_warn_days,t_times,i_nb_times FROM recurrentoperation")

            << QStringLiteral("DROP TABLE IF EXISTS recurrentoperation")
            << QStringLiteral("ALTER TABLE recurrentoperation2 RENAME TO recurrentoperation")
            // ============
            << QString()
            << QStringLiteral("6.8")
            << QStringLiteral("6.9")
            << "CREATE TABLE category2 ("
            "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
            "t_name TEXT NOT NULL DEFAULT '' CHECK (t_name NOT LIKE '%" % OBJECTSEPARATOR % "%'),"
            "t_fullname TEXT,"
            "rd_category_id INT,"
            "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'))"
            ")"
            << QStringLiteral("INSERT INTO category2 (id, t_name, t_fullname, rd_category_id, t_bookmarked) "
                             "SELECT id, t_name, t_fullname, r_category_id, t_bookmarked FROM category")

            << QStringLiteral("DROP TABLE IF EXISTS category")
            << QStringLiteral("ALTER TABLE category2 RENAME TO category")

            << QStringLiteral("DROP TABLE IF EXISTS vm_category_display_tmp")
            << QStringLiteral("CREATE TABLE vm_category_display_tmp(  id INT,  t_name TEXT,  t_fullname TEXT,  rd_category_id INT,  i_NBOPERATIONS,  f_REALCURRENTAMOUNT, t_bookmarked)")
            // ============
            << QString()
            << QStringLiteral("6.9")
            << QStringLiteral("7.0")
            << QStringLiteral("DELETE FROM parameters WHERE t_name LIKE 'SKG_MONTHLY_REPORT_%'")
            // ============ SKROOGE 1.0.0 ^^^
            << QString()
            << QStringLiteral("7.0")
            << QStringLiteral("7.1")
            << QStringLiteral("ALTER TABLE unit ADD COLUMN t_source TEXT NOT NULL DEFAULT ''")
            << QStringLiteral("UPDATE unit SET t_source=''")
            // ============
            << QString()
            << QStringLiteral("7.1")
            << QStringLiteral("7.2")
            << QStringLiteral("UPDATE unit SET t_source='Yahoo' WHERE t_source='' AND t_internet_code<>''")
            // ============
            << QString()
            << QStringLiteral("7.2")
            << QStringLiteral("7.3")
            << QStringLiteral("CREATE TABLE account2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_name TEXT NOT NULL,"
                             "t_number TEXT NOT NULL DEFAULT '',"
                             "t_agency_number TEXT NOT NULL DEFAULT '',"
                             "t_agency_address TEXT NOT NULL DEFAULT '',"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                             "t_type VARCHAR(1) NOT NULL DEFAULT 'C' CHECK (t_type IN ('C', 'D', 'A', 'I', 'L', 'W', 'O')),"
                             // C=current D=credit card A=assets (for objects) I=Investment W=Wallet L=Loan O=other
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "rd_bank_id INTEGER NOT NULL)")
            << QStringLiteral("INSERT INTO account2 (id, t_name, t_number, t_agency_number, t_agency_address, t_comment, t_close, t_type, t_bookmarked, rd_bank_id) "
                             "SELECT id, t_name, t_number, t_agency_number, t_agency_address, t_comment, t_close, t_type, t_bookmarked, rd_bank_id FROM account")
            << QStringLiteral("DROP TABLE IF EXISTS account")
            << QStringLiteral("ALTER TABLE account2 RENAME TO account")
            // ============ SKROOGE 1.1.0 ^^^
            << QString()
            << QStringLiteral("7.3")
            << QStringLiteral("7.4")
            << QStringLiteral("ALTER TABLE unit ADD COLUMN t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'))")
            << QStringLiteral("UPDATE unit SET t_bookmarked='N'")
            // ============
            << QString()
            << QStringLiteral("7.4")
            << QStringLiteral("7.5")
            << QStringLiteral("DELETE FROM parameters WHERE t_name LIKE 'SKGOPERATION_%'")
            // ============
            << QString()
            << QStringLiteral("7.5")
            << QStringLiteral("7.6")
            << "CREATE TABLE category2 ("
            "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
            "t_name TEXT NOT NULL DEFAULT '' CHECK (t_name NOT LIKE '%" % OBJECTSEPARATOR % "%'),"
            "t_fullname TEXT,"
            "rd_category_id INTEGER NOT NULL DEFAULT 0,"
            "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'))"
            ")"
            << QStringLiteral("INSERT INTO category2 (id, t_name, t_fullname, rd_category_id, t_bookmarked) "
                             "SELECT id, t_name, t_fullname, (CASE WHEN rd_category_id IS NULL OR rd_category_id='' THEN 0 ELSE rd_category_id END), t_bookmarked FROM category")

            << QStringLiteral("DROP TABLE IF EXISTS category")
            << QStringLiteral("ALTER TABLE category2 RENAME TO category")
            // ============
            << QString()
            << QStringLiteral("7.6")
            << QStringLiteral("7.7")
            << QStringLiteral("ALTER TABLE operationbalance ADD COLUMN f_balance_entered FLOAT NOT NULL DEFAULT 0")
            // ============ SKROOGE 1.3.2 ^^^
            << QString()
            << QStringLiteral("7.7")
            << QStringLiteral("7.8")
            << QStringLiteral("CREATE TABLE account2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_name TEXT NOT NULL,"
                             "t_number TEXT NOT NULL DEFAULT '',"
                             "t_agency_number TEXT NOT NULL DEFAULT '',"
                             "t_agency_address TEXT NOT NULL DEFAULT '',"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                             "t_type VARCHAR(1) NOT NULL DEFAULT 'C' CHECK (t_type IN ('C', 'D', 'A', 'I', 'L', 'W', 'S', 'O')),"
                             // C=current D=credit card A=assets (for objects) I=Investment W=Wallet L=Loan S=Saving O=other
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "rd_bank_id INTEGER NOT NULL)")
            << QStringLiteral("INSERT INTO account2 (id, t_name, t_number, t_agency_number, t_agency_address, t_comment, t_close, t_type, t_bookmarked, rd_bank_id) "
                             "SELECT id, t_name, t_number, t_agency_number, t_agency_address, t_comment, t_close, t_type, t_bookmarked, rd_bank_id FROM account")
            << QStringLiteral("DROP TABLE IF EXISTS account")
            << QStringLiteral("ALTER TABLE account2 RENAME TO account")
            // ============
            << QString()
            << QStringLiteral("7.8")
            << QStringLiteral("7.9")
            << QStringLiteral("DROP TABLE IF EXISTS vm_budget_tmp")
            << QStringLiteral("CREATE TABLE vm_budget_tmp(  id INT,  rc_category_id INT,  f_budgeted REAL,  i_year INT,  i_month INT,  f_budgeted_modified REAL,  f_transferred REAL,  t_including_subcategories TEXT,  t_CATEGORY,  t_PERIOD,  f_CURRENTAMOUNT,  t_RULES)")
            << QStringLiteral("DROP TABLE IF EXISTS vm_category_display_tmp")
            << QStringLiteral("CREATE TABLE vm_category_display_tmp(  id INT,  t_name TEXT,  t_fullname TEXT,  rd_category_id INT,  i_NBOPERATIONS,  f_REALCURRENTAMOUNT, t_bookmarked)")
            // ============ SKROOGE 1.3.3 ^^^
            << QString()
            << QStringLiteral("7.9")
            << QStringLiteral("8.0")
            << QStringLiteral("DROP TABLE IF EXISTS operationbalance")
            << QStringLiteral("CREATE TABLE operationbalance("
                             "r_operation_id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "f_balance FLOAT NOT NULL DEFAULT 0,"
                             "f_balance_entered FLOAT NOT NULL DEFAULT 0)")
            << QString()
            << QStringLiteral("8.0")
            << QStringLiteral("8.1")
            << QStringLiteral("DROP TABLE IF EXISTS operationbalance")
            << QStringLiteral("CREATE TABLE operationbalance("
                             "r_operation_id INTEGER NOT NULL,"
                             "f_balance FLOAT NOT NULL DEFAULT 0,"
                             "f_balance_entered FLOAT NOT NULL DEFAULT 0)")
            // ============ SKROOGE 1.4.0 ^^^
            << QString()
            << QStringLiteral("8.1")
            << QStringLiteral("8.2")
            << QStringLiteral("DROP TABLE IF EXISTS budgetcategory")
            << QStringLiteral("CREATE TABLE budgetsuboperation("
                             "id INTEGER NOT NULL DEFAULT 0,"
                             "id_suboperation INTEGER NOT NULL DEFAULT 0,"
                             "i_priority INTEGER NOT NULL DEFAULT 0)")
            << QString()
            << QStringLiteral("8.2")
            << QStringLiteral("8.3")
            << QStringLiteral("DROP TABLE IF EXISTS vm_category_display_tmp")
            << QStringLiteral("DROP TRIGGER IF EXISTS fkdc_category_vm_category_display_tmp_id_rd_category_id")
            // ============ SKROOGE 1.7.4 ^^^
            << QString()
            << QStringLiteral("8.3")
            << QStringLiteral("8.4")
            << QStringLiteral("ALTER TABLE account ADD COLUMN f_maxamount FLOAT NOT NULL DEFAULT 10000.0")
            << QStringLiteral("ALTER TABLE account ADD COLUMN t_maxamount_enabled VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N'))")
            << QStringLiteral("ALTER TABLE account ADD COLUMN f_minamount FLOAT NOT NULL DEFAULT 0.0")
            << QStringLiteral("ALTER TABLE account ADD COLUMN t_minamount_enabled VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N'))")
            << QStringLiteral("UPDATE account SET f_maxamount=10000.0, t_maxamount_enabled='N', f_minamount=0.0, t_minamount_enabled='N'")
            // ============ SKROOGE 1.7.7 ^^^
            << QString()
            << QStringLiteral("8.4")
            << QStringLiteral("8.5")
            << QStringLiteral("ALTER TABLE account ADD COLUMN d_reconciliationdate DATE")
            // ============
            << QString()
            << QStringLiteral("8.5")
            << QStringLiteral("8.6")
            << QStringLiteral("CREATE TABLE account2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_name TEXT NOT NULL,"
                             "t_number TEXT NOT NULL DEFAULT '',"
                             "t_agency_number TEXT NOT NULL DEFAULT '',"
                             "t_agency_address TEXT NOT NULL DEFAULT '',"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                             "t_type VARCHAR(1) NOT NULL DEFAULT 'C' CHECK (t_type IN ('C', 'D', 'A', 'I', 'L', 'W', 'S', 'P', 'O')),"
                             // C=current D=credit card A=assets (for objects) I=Investment W=Wallet L=Loan S=Saving P=Pension O=other
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "f_maxamount FLOAT NOT NULL DEFAULT 10000.0,"
                             "t_maxamount_enabled VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                             "f_minamount FLOAT NOT NULL DEFAULT 0.0,"
                             "t_minamount_enabled VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                             "d_reconciliationdate DATE,"
                             "rd_bank_id INTEGER NOT NULL)")
            << QStringLiteral("INSERT INTO account2 (id, t_name, t_number, t_agency_number, t_agency_address, t_comment, t_close, t_type, t_bookmarked, f_maxamount, t_maxamount_enabled, f_minamount, t_minamount_enabled, d_reconciliationdate, rd_bank_id) "
                             "SELECT id, t_name, t_number, t_agency_number, t_agency_address, t_comment, t_close, t_type, t_bookmarked, f_maxamount, t_maxamount_enabled, f_minamount, t_minamount_enabled, d_reconciliationdate, rd_bank_id FROM account")
            << QStringLiteral("DROP TABLE IF EXISTS account")
            << QStringLiteral("ALTER TABLE account2 RENAME TO account")
            // ============ SKROOGE 1.8.0 ^^^
            << QString()
            << QStringLiteral("8.6")
            << QStringLiteral("8.7")
            << QStringLiteral("ALTER TABLE suboperation ADD COLUMN d_date DATE NOT NULL DEFAULT '0000-00-00'")
            << QStringLiteral("UPDATE suboperation SET d_date=(SELECT d_date FROM operation WHERE suboperation.rd_operation_id=operation.id)")
            << QString()
            << QStringLiteral("8.7")
            << QStringLiteral("8.8")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition, '\"d_date\"', '\"d_DATEOP\"') WHERE t_action_definition like '%\"d_date\"%'")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition, '\"d_date\"', '\"d_DATEOP\"') WHERE t_definition like '%\"d_date\"%'")
            // ============ SKROOGE 1.9.0 ^^^
            << QString()
            << QStringLiteral("8.8")
            << QStringLiteral("8.9")
            << QStringLiteral("ALTER TABLE suboperation ADD COLUMN i_order INTEGER NOT NULL DEFAULT 0")
            << QStringLiteral("UPDATE suboperation SET i_order=id")
            << QString()
            << QStringLiteral("8.9")
            << QStringLiteral("9.0")
            << QStringLiteral("ALTER TABLE account ADD COLUMN r_account_id INTEGER NOT NULL DEFAULT 0")
            << QStringLiteral("UPDATE account SET r_account_id=0")
            << QString()
            << QStringLiteral("9.0")
            << QStringLiteral("9.1")
            << QStringLiteral("CREATE TABLE rule2 ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_description TEXT NOT NULL DEFAULT '',"
                             "t_definition TEXT NOT NULL DEFAULT '',"
                             "t_action_description TEXT NOT NULL DEFAULT '',"
                             "t_action_definition TEXT NOT NULL DEFAULT '',"
                             "t_action_type VARCHAR(1) DEFAULT 'S' CHECK (t_action_type IN ('S', 'U', 'A', 'T')),"
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "f_sortorder FLOAT"
                             ")")
            << QStringLiteral("INSERT INTO rule2 (id, t_description, t_definition, t_action_description, t_action_definition, t_action_type, t_bookmarked,f_sortorder) SELECT id, t_description, t_definition, t_action_description, t_action_definition, t_action_type, t_bookmarked,f_sortorder FROM rule")
            << QStringLiteral("DROP TABLE IF EXISTS rule")
            << QStringLiteral("ALTER TABLE rule2 RENAME TO rule")
            << QString()
            << QStringLiteral("9.1")
            << QStringLiteral("9.2")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' limitVisible=&amp;amp;quot;Y&amp;amp;quot; ', ' limitVisible=&amp;amp;quot;Y&amp;amp;quot; averageVisible=&amp;amp;quot;Y&amp;amp;quot; ')")
            << QStringLiteral("UPDATE parameters SET t_value=replace(t_value, ' limitVisible=&amp;amp;quot;N&amp;amp;quot; ', ' limitVisible=&amp;amp;quot;N&amp;amp;quot; averageVisible=&amp;amp;quot;N&amp;amp;quot; ')")
            << QString()
            << QStringLiteral("9.2")
            << QStringLiteral("9.3")
            << QStringLiteral("CREATE TABLE operation2 ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "i_group_id INTEGER NOT NULL DEFAULT 0,"
                             "i_number INTEGER DEFAULT 0 CHECK (i_number>=0),"
                             "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                             "d_createdate DATE NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                             "rd_account_id INTEGER NOT NULL,"
                             "t_mode TEXT NOT NULL DEFAULT '',"
                             "r_payee_id INTEGER NOT NULL DEFAULT 0,"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "rc_unit_id INTEGER NOT NULL,"
                             "t_status VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_status IN ('N', 'P', 'Y')),"
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "t_imported VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_imported IN ('Y', 'N', 'P', 'T')),"
                             "t_template VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_template IN ('Y', 'N')),"
                             "t_import_id TEXT NOT NULL DEFAULT '',"
                             "i_tmp INTEGER NOT NULL DEFAULT 0,"
                             "r_recurrentoperation_id INTEGER NOT NULL DEFAULT 0)")
            << QStringLiteral("INSERT INTO operation2 (id, i_group_id, i_number, d_date, d_createdate, rd_account_id, t_mode, r_payee_id, t_comment,"
                             "rc_unit_id, t_status, t_bookmarked, t_imported, t_template, t_import_id, i_tmp, r_recurrentoperation_id) "
                             "SELECT id, i_group_id, i_number, d_date, CURRENT_TIMESTAMP, rd_account_id, t_mode, r_payee_id, t_comment,"
                             "rc_unit_id, t_status, t_bookmarked, t_imported, t_template, t_import_id, i_tmp, r_recurrentoperation_id FROM operation")
            << QStringLiteral("DROP TABLE IF EXISTS operation")
            << QStringLiteral("ALTER TABLE operation2 RENAME TO operation")
            << QString()
            << QStringLiteral("9.3")
            << QStringLiteral("9.4")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition,'#ATT#>=#V1# AND #ATT#&lt;=#V2#','((#ATT#>=#V1# AND #ATT#&lt;=#V2#) OR (#ATT#>=#V2# AND #ATT#&lt;=#V1#))')")
            << QString()
            << QStringLiteral("9.4")
            << QStringLiteral("9.5")
            << QStringLiteral("ALTER TABLE account ADD COLUMN f_importbalance FLOAT")
            << QStringLiteral("UPDATE account SET f_importbalance=NULL")
            // ============ SKROOGE 2.4.0 ^^^
            << QString()
            << QStringLiteral("9.5")
            << QStringLiteral("9.6")
            << QStringLiteral("CREATE TABLE IF NOT EXISTS vm_budget_tmp(  id INT,  rc_category_id INT,  f_budgeted REAL,  i_year INT,  i_month INT,  f_budgeted_modified REAL,  f_transferred REAL,  t_including_subcategories TEXT,  t_CATEGORY,  t_PERIOD,  f_CURRENTAMOUNT,  t_RULES)")
            << QStringLiteral("ALTER TABLE budgetrule ADD COLUMN f_sortorder FLOAT")
            << QStringLiteral("UPDATE budgetrule SET f_sortorder=id WHERE f_sortorder IS NULL OR f_sortorder=''")
            << QString()
            << QStringLiteral("9.6")
            << QStringLiteral("9.7")
            << QStringLiteral("ALTER TABLE budget ADD COLUMN t_modification_reasons TEXT NOT NULL DEFAULT ''")
            << QStringLiteral("UPDATE budget SET t_modification_reasons=''")
            // ============ SKROOGE 2.8.1 ^^^
            << QString()
            << QStringLiteral("9.7")
            << QStringLiteral("9.8")
            << QStringLiteral("UPDATE operation SET d_createdate=d_date WHERE d_createdate=''")
            // ============ SKROOGE 2.9.0 ^^^
            << QString()
            << QStringLiteral("9.8")
            << QStringLiteral("9.9")
            << QStringLiteral("ALTER TABLE payee ADD r_category_id INTEGER NOT NULL DEFAULT 0")
            << QStringLiteral("UPDATE payee SET r_category_id=0")
            << QString()
            << QStringLiteral("9.9")
            << QStringLiteral("10.0")
            << QStringLiteral("ALTER TABLE payee ADD t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N'))")
            << QStringLiteral("UPDATE payee SET t_close='N'")
            << QString()
            << QStringLiteral("10.0")
            << QStringLiteral("10.1")
            << QStringLiteral("ALTER TABLE category ADD t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N'))")
            << QStringLiteral("UPDATE category SET t_close='N'")
            // ============ SKROOGE 2.11.0 ^^^
            << QString()
            << QStringLiteral("10.1")
            << QStringLiteral("10.2")
            << QStringLiteral("CREATE TABLE operation2("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "i_group_id INTEGER NOT NULL DEFAULT 0,"
                             "t_number TEXT NOT NULL DEFAULT '',"
                             "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                             "d_createdate DATE NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                             "rd_account_id INTEGER NOT NULL,"
                             "t_mode TEXT NOT NULL DEFAULT '',"
                             "r_payee_id INTEGER NOT NULL DEFAULT 0,"
                             "t_comment TEXT NOT NULL DEFAULT '',"
                             "rc_unit_id INTEGER NOT NULL,"
                             "t_status VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_status IN ('N', 'P', 'Y')),"
                             "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                             "t_imported VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_imported IN ('Y', 'N', 'P', 'T')),"
                             "t_template VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_template IN ('Y', 'N')),"
                             "t_import_id TEXT NOT NULL DEFAULT '',"
                             "i_tmp INTEGER NOT NULL DEFAULT 0,"
                             "r_recurrentoperation_id INTEGER NOT NULL DEFAULT 0)")
            << QStringLiteral("INSERT INTO operation2 (id, i_group_id, t_number, d_date, d_createdate, rd_account_id, t_mode, r_payee_id, t_comment,"
                             "rc_unit_id, t_status, t_bookmarked, t_imported, t_template, t_import_id, i_tmp, r_recurrentoperation_id) "
                             "SELECT id, i_group_id, (CASE WHEN i_number=0 OR i_number IS NULL THEN '' ELSE i_number END), d_date, CURRENT_TIMESTAMP, rd_account_id, t_mode, r_payee_id, t_comment,"
                             "rc_unit_id, t_status, t_bookmarked, t_imported, t_template, t_import_id, i_tmp, r_recurrentoperation_id FROM operation")
            << QStringLiteral("DROP TABLE IF EXISTS operation")
            << QStringLiteral("ALTER TABLE operation2 RENAME TO operation")
            << QString()
            << QStringLiteral("10.2")
            << QStringLiteral("10.3")
            << QStringLiteral("UPDATE rule SET t_definition=replace(t_definition, 'i_number', 't_number')")
            << QStringLiteral("UPDATE rule SET t_action_definition=replace(t_action_definition, 'i_number', 't_number')")
            // ============ SKROOGE 2.11.0 ^^^
            << QString()
            << QStringLiteral("10.3")
            << QStringLiteral("10.4")
            << QStringLiteral("ALTER TABLE account ADD COLUMN f_reconciliationbalance FLOAT")
            << QStringLiteral("UPDATE account SET f_reconciliationbalance=NULL")
            // ============ SKROOGE 2.13.0 ^^^
            << QString()
            << QStringLiteral("10.4")
            << QStringLiteral("10.5")
            << QStringLiteral("ALTER TABLE account ADD COLUMN d_importdate DATE")
            // ============ SKROOGE 2.26.0 ^^^
            << QString()
            << QStringLiteral("10.5")
            << QStringLiteral("10.6")
            << QStringLiteral("DROP TABLE IF EXISTS unitvalue3")
            << QStringLiteral("CREATE TABLE unitvalue3("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "rd_unit_id INTEGER NOT NULL,"
                             "d_date DATE NOT NULL,"
                             "f_quantity FLOAT NOT NULL)")
            << QStringLiteral("INSERT INTO unitvalue3 (id,rd_unit_id,d_date,f_quantity) SELECT id,rd_unit_id,d_date,f_quantity FROM unitvalue")
            << QStringLiteral("DROP TABLE IF EXISTS unitvalue")
            << QStringLiteral("ALTER TABLE unitvalue3 RENAME TO unitvalue")
            // ============ SKROOGE 25.4.0 ^^^
            << QString()
            << QStringLiteral("10.6")
            << QStringLiteral("10.8")
            << QStringLiteral("ALTER TABLE recurrentoperation ADD COLUMN t_adapter TEXT NOT NULL DEFAULT 'NONE'")
            ;

    return migrationSteps;
}

SKGError SKGDocumentBank::migrate(bool &oMigrationDone)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    oMigrationDone = false;
    QStringList migrationSteps = getMigationSteps();

    {
        int nbSteps = migrationSteps.count();
        SKGBEGINPROGRESSTRANSACTION(*this, "#INTERNAL#" % i18nc("Progression step", "Migrate document"), err, 5)
        IFOK(err)
        {
            QString version = getParameter(QStringLiteral("SKG_DB_BANK_VERSION"));
            QString initialversion = version;
            QString lastversion = QStringLiteral("10.8");
            if (version.isEmpty()) {
                SKGTRACEL(10) << "Initial creation" << Qt::endl;
                /**
                 * This constant is used to initialized the data model.
                 * Rules for attribute name:
                 *    t_xxx for TEXT and VARCHAR
                 *    d_xxx for DATE
                 *    f_xxx for FLOAT
                 *    i_xxx for INTEGER
                 *    r_xxx for a link without constraint
                 *    rc_pointed_table_pointed_attribute_xxx for link on other an object in named "pointed_table" with "pointed_attribute"=id of pointing object
                 *                                       a constraint will be created without DELETE CASCADE
                 *    rd_pointed_table_pointed_attribute_xxx for link on other an object in named "pointed_table" with "pointed_attribute"=id of pointing object
                 *                                       a constraint will be created with DELETE CASCADE
                 *    xxx must be in lower case for R/W attributes and in upper case for R/O attributes
                 * Rules for table name:
                 *    v_yyy for views
                 */
                QStringList BankInitialDataModel;
                BankInitialDataModel
                // ==================================================================
                // Table unit
                        << QStringLiteral("CREATE TABLE unit("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "t_name TEXT NOT NULL,"
                                         "t_symbol TEXT NOT NULL DEFAULT '',"
                                         "t_country TEXT NOT NULL DEFAULT '',"
                                         "t_type VARCHAR(1) NOT NULL DEFAULT 'C' CHECK (t_type IN ('1', '2', 'C', 'S', 'I', 'O')),"
                                         // 1=main currency, 2=secondary currency, C=currencies S=share, I=index, O=object
                                         "t_internet_code TEXT NOT NULL DEFAULT '',"
                                         "i_nbdecimal INT NOT NULL DEFAULT 2,"
                                         "rd_unit_id INTEGER NOT NULL DEFAULT 0,"
                                         "t_source TEXT NOT NULL DEFAULT '',"
                                         "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'))"
                                         ")")

                        // ==================================================================
                        // Table unitvalue
                        << QStringLiteral("CREATE TABLE unitvalue("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "rd_unit_id INTEGER NOT NULL,"
                                         "d_date DATE NOT NULL,"
                                         "f_quantity FLOAT NOT NULL)")

                        // ==================================================================
                        // Table bank
                        << QStringLiteral("CREATE TABLE bank ("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "t_name TEXT NOT NULL DEFAULT '',"
                                         "t_bank_number TEXT NOT NULL DEFAULT '',"
                                         "t_icon TEXT NOT NULL DEFAULT '')")

                        // ==================================================================
                        // Table account
                        << QStringLiteral("CREATE TABLE account("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "t_name TEXT NOT NULL,"
                                         "t_number TEXT NOT NULL DEFAULT '',"
                                         "t_agency_number TEXT NOT NULL DEFAULT '',"
                                         "t_agency_address TEXT NOT NULL DEFAULT '',"
                                         "t_comment TEXT NOT NULL DEFAULT '',"
                                         "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                                         "t_type VARCHAR(1) NOT NULL DEFAULT 'C' CHECK (t_type IN ('C', 'D', 'A', 'I', 'L', 'W', 'S', 'P', 'O')),"
                                         // C=current D=credit card A=assets (for objects) I=Investment W=Wallet L=Loan S=Saving P=Pension O=other
                                         "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                                         "f_maxamount FLOAT NOT NULL DEFAULT 10000.0,"
                                         "t_maxamount_enabled VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                                         "f_minamount FLOAT NOT NULL DEFAULT 0.0,"
                                         "t_minamount_enabled VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                                         "d_importdate DATE,"
                                         "f_importbalance FLOAT,"
                                         "d_reconciliationdate DATE,"
                                         "f_reconciliationbalance FLOAT,"
                                         "r_account_id INTEGER NOT NULL DEFAULT 0,"
                                         "rd_bank_id INTEGER NOT NULL)")

                        // ==================================================================
                        // Table interest
                        << QStringLiteral("CREATE TABLE interest("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "rd_account_id INTEGER NOT NULL,"
                                         "d_date DATE NOT NULL,"
                                         "f_rate FLOAT NOT NULL CHECK (f_rate>=0),"
                                         "t_income_value_date_mode VARCHAR(1) NOT NULL DEFAULT 'F' CHECK (t_income_value_date_mode IN ('F', '0', '1', '2', '3', '4', '5')),"
                                         "t_expenditure_value_date_mode VARCHAR(1) NOT NULL DEFAULT 'F' CHECK (t_expenditure_value_date_mode IN ('F', '0', '1', '2', '3', '4', '5')),"
                                         "t_base VARCHAR(3) NOT NULL DEFAULT '24' CHECK (t_base IN ('24', '360', '365'))"
                                         ")")

                        // ==================================================================
                        // Table category
                        << "CREATE TABLE category ("
                        "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                        "t_name TEXT NOT NULL DEFAULT '' CHECK (t_name NOT LIKE '%" % OBJECTSEPARATOR % "%'),"
                        "t_fullname TEXT,"
                        "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                        "rd_category_id INTEGER NOT NULL DEFAULT 0,"
                        "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N'))"
                        ")"

                        // ==================================================================
                        // Table operation
                        << QStringLiteral("CREATE TABLE operation("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "i_group_id INTEGER NOT NULL DEFAULT 0,"
                                         "t_number TEXT NOT NULL DEFAULT '',"
                                         "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                                         "d_createdate DATE NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                                         "rd_account_id INTEGER NOT NULL,"
                                         "t_mode TEXT NOT NULL DEFAULT '',"
                                         "r_payee_id INTEGER NOT NULL DEFAULT 0,"
                                         "t_comment TEXT NOT NULL DEFAULT '',"
                                         "rc_unit_id INTEGER NOT NULL,"
                                         "t_status VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_status IN ('N', 'P', 'Y')),"
                                         "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                                         "t_imported VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_imported IN ('Y', 'N', 'P', 'T')),"
                                         "t_template VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_template IN ('Y', 'N')),"
                                         "t_import_id TEXT NOT NULL DEFAULT '',"
                                         "i_tmp INTEGER NOT NULL DEFAULT 0,"
                                         "r_recurrentoperation_id INTEGER NOT NULL DEFAULT 0)")

                        << QStringLiteral("CREATE TABLE operationbalance("
                                         "r_operation_id INTEGER NOT NULL,"
                                         "f_balance FLOAT NOT NULL DEFAULT 0,"
                                         "f_balance_entered FLOAT NOT NULL DEFAULT 0)")

                        // ==================================================================
                        // Table refund
                        << QStringLiteral("CREATE TABLE refund ("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "t_name TEXT NOT NULL DEFAULT '',"
                                         "t_comment TEXT NOT NULL DEFAULT '',"
                                         "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')))")

                        // ==================================================================
                        // Table payee
                        << QStringLiteral("CREATE TABLE payee ("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "t_name TEXT NOT NULL DEFAULT '',"
                                         "t_address TEXT NOT NULL DEFAULT '',"
                                         "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                                         "t_close VARCHAR(1) DEFAULT 'N' CHECK (t_close IN ('Y', 'N')),"
                                         "r_category_id INTEGER NOT NULL DEFAULT 0"
                                         ")")

                        // ==================================================================
                        // Table suboperation
                        << QStringLiteral("CREATE TABLE suboperation("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                                         "t_comment TEXT NOT NULL DEFAULT '',"
                                         "rd_operation_id INTEGER NOT NULL,"
                                         "r_category_id INTEGER NOT NULL DEFAULT 0,"
                                         "f_value FLOAT NOT NULL DEFAULT 0.0,"
                                         "t_formula TEXT NOT NULL DEFAULT '',"
                                         "i_tmp INTEGER NOT NULL DEFAULT 0,"
                                         "r_refund_id INTEGER NOT NULL DEFAULT 0,"
                                         "i_order INTEGER NOT NULL DEFAULT 0"
                                         ")")

                        // ==================================================================
                        // Table recurrentoperation
                        << QStringLiteral("CREATE TABLE recurrentoperation ("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "d_date DATE NOT NULL DEFAULT '0000-00-00',"
                                         "rd_operation_id INTEGER NOT NULL,"
                                         "i_period_increment INTEGER NOT NULL DEFAULT 1 CHECK (i_period_increment>=0),"
                                         "t_period_unit TEXT NOT NULL DEFAULT 'M' CHECK (t_period_unit IN ('D', 'W', 'M', 'Y')),"
                                         "t_auto_write VARCHAR(1) DEFAULT 'Y' CHECK (t_auto_write IN ('Y', 'N')),"
                                         "t_adapter TEXT NOT NULL DEFAULT 'NONE',"
                                         "i_auto_write_days INTEGER NOT NULL DEFAULT 5 CHECK (i_auto_write_days>=0),"
                                         "t_warn VARCHAR(1) DEFAULT 'Y' CHECK (t_warn IN ('Y', 'N')),"
                                         "i_warn_days INTEGER NOT NULL DEFAULT 5 CHECK (i_warn_days>=0),"
                                         "t_times VARCHAR(1) DEFAULT 'N' CHECK (t_times IN ('Y', 'N')),"
                                         "i_nb_times INTEGER NOT NULL DEFAULT 1 CHECK (i_nb_times>=0)"
                                         ")")

                        // ==================================================================
                        // Table rule
                        << QStringLiteral("CREATE TABLE rule ("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "t_description TEXT NOT NULL DEFAULT '',"
                                         "t_definition TEXT NOT NULL DEFAULT '',"
                                         "t_action_description TEXT NOT NULL DEFAULT '',"
                                         "t_action_definition TEXT NOT NULL DEFAULT '',"
                                         "t_action_type VARCHAR(1) DEFAULT 'S' CHECK (t_action_type IN ('S', 'U', 'A', 'T')),"
                                         "t_bookmarked VARCHAR(1) NOT NULL DEFAULT 'N' CHECK (t_bookmarked IN ('Y', 'N')),"
                                         "f_sortorder FLOAT"
                                         ")")

                        // ==================================================================
                        // Table budget
                        << QStringLiteral("CREATE TABLE budget ("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "rc_category_id INTEGER NOT NULL DEFAULT 0,"
                                         "t_including_subcategories TEXT NOT NULL DEFAULT 'N' CHECK (t_including_subcategories IN ('Y', 'N')),"
                                         "f_budgeted FLOAT NOT NULL DEFAULT 0.0,"
                                         "f_budgeted_modified FLOAT NOT NULL DEFAULT 0.0,"
                                         "t_modification_reasons TEXT NOT NULL DEFAULT '',"
                                         "f_transferred FLOAT NOT NULL DEFAULT 0.0,"
                                         "i_year INTEGER NOT NULL DEFAULT 2010,"
                                         "i_month INTEGER NOT NULL DEFAULT 0 CHECK (i_month>=0 AND i_month<=12)"
                                         ")")

                        << QStringLiteral("CREATE TABLE budgetsuboperation("
                                         "id INTEGER NOT NULL DEFAULT 0,"
                                         "id_suboperation INTEGER NOT NULL DEFAULT 0,"
                                         "i_priority INTEGER NOT NULL DEFAULT 0)")

                        << QStringLiteral("CREATE TABLE budgetrule ("
                                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                         "rc_category_id INTEGER NOT NULL DEFAULT 0,"
                                         "t_category_condition TEXT NOT NULL DEFAULT 'Y' CHECK (t_category_condition IN ('Y', 'N')),"
                                         "t_year_condition TEXT NOT NULL DEFAULT 'Y' CHECK (t_year_condition IN ('Y', 'N')),"
                                         "i_year INTEGER NOT NULL DEFAULT 2010,"
                                         "i_month INTEGER NOT NULL DEFAULT 0 CHECK (i_month>=0 AND i_month<=12),"
                                         "t_month_condition TEXT NOT NULL DEFAULT 'Y' CHECK (t_month_condition IN ('Y', 'N')),"
                                         "i_condition INTEGER NOT NULL DEFAULT 0 CHECK (i_condition IN (-1,0,1)),"
                                         "f_quantity FLOAT NOT NULL DEFAULT 0.0,"
                                         "t_absolute TEXT NOT NULL DEFAULT 'Y' CHECK (t_absolute IN ('Y', 'N')),"
                                         "rc_category_id_target INTEGER NOT NULL DEFAULT 0,"
                                         "t_category_target TEXT NOT NULL DEFAULT 'Y' CHECK (t_category_target IN ('Y', 'N')),"
                                         "t_rule TEXT NOT NULL DEFAULT 'N' CHECK (t_rule IN ('N', 'C', 'Y')),"
                                         "f_sortorder FLOAT"
                                         ")")

                        << QStringLiteral("CREATE TABLE vm_budget_tmp("
                                         "id INT,"
                                         "rc_category_id INT,"
                                         "f_budgeted REAL,"
                                         "i_year INT,"
                                         "i_month INT,"
                                         "f_budgeted_modified REAL,"
                                         "t_modification_reasons TEXT,"
                                         "f_transferred REAL,"
                                         "t_including_subcategories TEXT,"
                                         "t_CATEGORY TEXT,"
                                         "t_PERIOD TEXT,"
                                         "f_CURRENTAMOUNT REAL,"
                                         "t_RULES TEXT)");

                IFOKDO(err, this->executeSqliteOrders(BankInitialDataModel))

                // Set new version
                version = lastversion;
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_BANK_VERSION"), version))
            }

            if (!err && SKGServices::stringToDouble(version) > SKGServices::stringToDouble(lastversion)) {
                err = SKGError(ERR_ABORT, i18nc("Error message", "Impossible to load a document generated by a more recent version"));
            }

            IFOK(err)
            {
                QString v1;
                QString v2;
                bool computeCaches = false;
                for (int i = 0; !err && i < nbSteps; ++i) {
                    if (migrationSteps.at(i).isEmpty()) {
                        ++i;
                        v1 = migrationSteps.at(i);
                        ++i;
                        v2 = migrationSteps.at(i);
                        if (version == v1) {
                            SKGTRACEL(10) << "Migration from " << v1 << " to " << v2 << Qt::endl;
                            for (int j = i + 1; !err && j < nbSteps; ++j) {
                                const QString &sql = migrationSteps.at(j);
                                if (!sql.isEmpty()) {
                                    ++i;
                                    IFOKDO(err, this->executeSqliteOrder(sql))
                                } else {
                                    break;
                                }
                            }

                            if (v1 == QStringLiteral("4.7") || v1 == QStringLiteral("5.1") || v1 == QStringLiteral("5.7") || v1 == QStringLiteral("5.9")
                                || v1 == QStringLiteral("8.0") || v1 == QStringLiteral("8.1") || v1 == QStringLiteral("10.0") || v1 == QStringLiteral("10.5")) {
                                computeCaches = true;
                            }

                            // Set new version
                            version = v2;
                            IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_BANK_VERSION"), version))
                            oMigrationDone = true;
                        }
                    }
                }
                IFOKDO(err, stepForward(1, i18nc("Progression step", "Refresh views")))

                if (!err && computeCaches) {
                    err = refreshViewsIndexesAndTriggers(v1 == QStringLiteral("10.5"));
                    IFOKDO(err, stepForward(2, i18nc("Progression step", "Computation of balances")))
                    IFOKDO(err, computeBalances())
                    IFOKDO(err, stepForward(3, i18nc("Progression step", "Computation of budgets")))
                    IFOKDO(err, computeBudgetSuboperationLinks())
                    IFOKDO(err, stepForward(4))
                }

                IFOK(err)
                {
                    bool mig = false;
                    err = SKGDocument::migrate(mig);
                    oMigrationDone = oMigrationDone || mig;
                }
                else
                {
                    err.addError(ERR_FAIL,
                                 i18nc("Error message: Could not perform database migration",
                                       "Database migration from version %1 to version %2 failed",
                                       initialversion,
                                       version));
                }
                IFOKDO(err, stepForward(5))
            }
        }
    }

    return err;
}

SKGError SKGDocumentBank::dump(int iMode) const
{
    SKGError err;
    if (Q_LIKELY(getMainDatabase())) {
        // dump parameters
        SKGTRACE << "=== START DUMP BANK DOCUMENT ===" << Qt::endl;
        err = SKGDocument::dump(iMode);

        if ((iMode & DUMPUNIT) != 0) {
            SKGTRACE << "=== DUMPUNIT (UNITS))===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_unit_display ORDER BY id")));

            SKGTRACE << "=== DUMPUNIT (VALUES) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_unitvalue_display ORDER BY rd_unit_id, d_date")));
        }

        if ((iMode & DUMPACCOUNT) != 0) {
            SKGTRACE << "=== DUMPACCOUNT (BANKS) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_bank_amount ORDER BY id")));

            SKGTRACE << "=== DUMPACCOUNT (ACCOUNTS) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_account_display ORDER BY rd_bank_id, id")));
        }

        if ((iMode & DUMPOPERATION) != 0) {
            SKGTRACE << "=== DUMPOPERATION (OPERATIONS) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_operation_display_all ORDER BY id")));

            SKGTRACE << "=== DUMPOPERATION (SUBOPERATIONS) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_suboperation_display ORDER BY rd_operation_id, id")));

            SKGTRACE << "=== DUMPOPERATION (RECURRENT) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_recurrentoperation ORDER BY rd_operation_id, id")));

            SKGTRACE << "=== DUMPOPERATION (TRACKER) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_refund ORDER BY id")));
        }

        if ((iMode & DUMPPAYEE) != 0) {
            SKGTRACE << "=== DUMPOPERATION (PAYEE) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_payee ORDER BY id")));
        }

        if ((iMode & DUMPCATEGORY) != 0) {
            SKGTRACE << "=== DUMPCATEGORY ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_category_display ORDER BY rd_category_id, id")));
        }

        if ((iMode & DUMPBUDGET) != 0) {
            SKGTRACE << "=== DUMPBUDGET (BUDGET) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_budget_display ORDER BY t_PERIOD")));

            SKGTRACE << "=== DUMPBUDGET (RULES) ===" << Qt::endl;
            err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM v_budgetrule_display ORDER BY t_absolute DESC, id")));
        }

        SKGTRACE << "=== END DUMP BANK DOCUMENT ===" << Qt::endl;
    }

    return err;
}

SKGError SKGDocumentBank::addOrModifyUnitValue(const QString &iUnitName, QDate iDate, double iValue, SKGUnitValueObject *oValue) const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Creation or update of the unit
    bool insertOrUpdate = true;
    SKGUnitObject unit(const_cast<SKGDocumentBank *>(this));
    err = unit.setName(iUnitName);
    IFOKDO(err, unit.setSymbol(iUnitName))
    if (!unit.exist()) {
        insertOrUpdate = false;
        IFOKDO(err, unit.save(insertOrUpdate))
    } else {
        err = unit.load();
    }

    // Creation or update of the value
    SKGUnitValueObject value;
    IFOKDO(err, unit.addUnitValue(value))
    IFOKDO(err, value.setDate(iDate))
    IFOKDO(err, value.setQuantity(iValue))
    IFOKDO(err, value.save(insertOrUpdate))

    if (oValue != nullptr) {
        *oValue = value;
    }

    // Add error if needed
    IFKO(err)
    err.addError(ERR_FAIL,
                 i18nc("Error message",
                       "Transaction '%1' on '%2' failed",
                       QStringLiteral("SKGDocumentBank::addOrModifyUnitValue"),
                       iUnitName % " / " % SKGServices::dateToSqlString(iDate) % " / " % SKGServices::doubleToString(iValue)));
    return err;
}

SKGServices::SKGUnitInfo SKGDocumentBank::getPrimaryUnit() const
{
    SKGServices::SKGUnitInfo output;

    output.Name = getCachedValue(QStringLiteral("primaryUnitCache"));
    if (output.Name.isEmpty()) {
        this->refreshCache(QStringLiteral("unit"));
        output.Name = getCachedValue(QStringLiteral("primaryUnitCache"));
    }
    output.Value = 1;
    output.Symbol = getCachedValue(QStringLiteral("primaryUnitSymbolCache"));
    output.NbDecimal = SKGServices::stringToInt(getCachedValue(QStringLiteral("primaryUnitDecimalCache")));

    return output;
}

SKGServices::SKGUnitInfo SKGDocumentBank::getSecondaryUnit() const
{
    SKGServices::SKGUnitInfo output;

    output.Name = getCachedValue(QStringLiteral("secondaryUnitCache"));
    if (output.Name.isEmpty()) {
        this->refreshCache(QStringLiteral("unit"));
        output.Name = getCachedValue(QStringLiteral("secondaryUnitCache"));
    }
    output.Symbol = getCachedValue(QStringLiteral("secondaryUnitSymbolCache"));
    output.Value = SKGServices::stringToDouble(getCachedValue(QStringLiteral("secondaryUnitValueCache")));
    output.NbDecimal = SKGServices::stringToInt(getCachedValue(QStringLiteral("secondaryUnitDecimalCache")));

    return output;
}

QString SKGDocumentBank::formatPrimaryMoney(double iValue, int iForcedNbOfDigit) const
{
    auto unit = getPrimaryUnit();
    if (iForcedNbOfDigit != -1) {
        unit.NbDecimal = iForcedNbOfDigit;
    }
    return formatMoney(iValue, unit, false);
}

QString SKGDocumentBank::formatSecondaryMoney(double iValue, int iForcedNbOfDigit) const
{
    auto unit = getSecondaryUnit();
    if (iForcedNbOfDigit != -1) {
        unit.NbDecimal = iForcedNbOfDigit;
    }
    return formatMoney(iValue, unit, false);
}

QString SKGDocumentBank::getCategoryForPayee(const QString &iPayee, bool iComputeAllPayees) const
{
    SKGTRACEINFUNC(10)
    QString output;
    QString key = "categoryForPayee-" + iPayee;
    output = getCachedValue(key);
    if (output.isEmpty()) {
        QString sql =
            "SELECT * FROM (SELECT 9999, t_name, t_CATEGORY FROM v_payee WHERE t_CATEGORY!='' UNION "
            "SELECT COUNT(1), t_name, t_CATEGORY from (SELECT payee.t_name, t_CATEGORY FROM payee, v_suboperation_display sop, v_operation op WHERE "
            "r_payee_id=payee.id AND sop.rd_operation_id=op.ID) GROUP BY t_name, t_CATEGORY) ORDER BY 2, 1 DESC";
        if (!iComputeAllPayees) {
            sql = "SELECT 9999, t_name, t_CATEGORY FROM v_payee WHERE t_name='" % SKGServices::stringToSqlString(iPayee) % "' AND t_CATEGORY!='' "
                  "UNION ALL SELECT COUNT(1),t_PAYEE, t_REALCATEGORY FROM (SELECT t_PAYEE, t_REALCATEGORY, d_date FROM v_suboperation_consolidated "
                  "WHERE t_PAYEE='" % SKGServices::stringToSqlString(iPayee) % "' ORDER BY d_date DESC LIMIT 50) GROUP BY t_REALCATEGORY ORDER BY COUNT(1) DESC";
        }
        SKGStringListList result;
        executeSelectSqliteOrder(sql, result);
        int nb = result.count();
        if (nb >= 1) {
            QString currentComputeKey;
            QString currentCat;
            int currentCount = 0;
            int sum = 0;
            for (int i = 1; i < nb; ++i) {
                int count = SKGServices::stringToInt(result.at(i).at(0));
                QString newComputeKey = "categoryForPayee-" + result.at(i).at(1);
                if (newComputeKey != currentComputeKey) {
                    // The computed key change
                    if (!currentComputeKey.isEmpty()) {
                        // Store the automatic category of the key
                        if (sum > 0 && 100 * currentCount / sum > 70) {
                            addValueInCache(currentComputeKey, currentCat);
                            if (currentComputeKey == key) {
                                output = currentCat;
                            }
                        }
                    }

                    // Start to compute the new sum and keep this category
                    currentCount = count;
                    currentCat = result.at(i).at(2);
                    currentComputeKey = std::move(newComputeKey);

                    sum = count;
                } else {
                    // Continue to compute the sum
                    sum += count;
                }
            }

            // Compute the last
            if (!currentComputeKey.isEmpty()) {
                // Store the automatic category of the key
                if (sum > 0 && 100 * currentCount / sum > 70) {
                    addValueInCache(currentComputeKey, currentCat);
                    if (currentComputeKey == key) {
                        output = std::move(currentCat);
                    }
                }
            }
        }
    }

    return output;
}

void SKGDocumentBank::refreshCache(const QString &iTable) const
{
    if (iTable == QStringLiteral("unit") || iTable.isEmpty()) {
        SKGTRACEINFUNC(10)
        SKGStringListList result;
        executeSelectSqliteOrder(QStringLiteral("SELECT t_name, t_symbol, i_nbdecimal FROM unit WHERE t_type='1'"), result);
        if (result.size() == 2) {
            addValueInCache(QStringLiteral("primaryUnitCache"), result.at(1).at(0));
            addValueInCache(QStringLiteral("primaryUnitSymbolCache"), result.at(1).at(1));
            addValueInCache(QStringLiteral("primaryUnitDecimalCache"), result.at(1).at(2));
        } else {
            addValueInCache(QStringLiteral("primaryUnitCache"), QString());
            addValueInCache(QStringLiteral("primaryUnitSymbolCache"), QString());
            addValueInCache(QStringLiteral("primaryUnitDecimalCache"), QStringLiteral("2"));
        }

        executeSelectSqliteOrder(QStringLiteral("SELECT t_name, t_symbol, f_CURRENTAMOUNT, i_nbdecimal FROM v_unit WHERE t_type='2'"), result);
        if (result.size() == 2) {
            addValueInCache(QStringLiteral("secondaryUnitCache"), result.at(1).at(0));
            addValueInCache(QStringLiteral("secondaryUnitSymbolCache"), result.at(1).at(1));
            addValueInCache(QStringLiteral("secondaryUnitValueCache"), result.at(1).at(2));
            addValueInCache(QStringLiteral("secondaryUnitDecimalCache"), result.at(1).at(3));
        } else {
            addValueInCache(QStringLiteral("secondaryUnitCache"), QString());
            addValueInCache(QStringLiteral("secondaryUnitSymbolCache"), QString());
            addValueInCache(QStringLiteral("secondaryUnitValueCache"), QStringLiteral("1"));
            addValueInCache(QStringLiteral("secondaryUnitDecimalCache"), QStringLiteral("2"));
        }
    }
    SKGDocument::refreshCache(iTable);
}

SKGError SKGDocumentBank::addOrModifyAccount(const QString &iName, const QString &iNumber, const QString &iBankName) const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Creation or update of the bank

    SKGBankObject bank(const_cast<SKGDocumentBank *>(this));
    err = bank.setName(iBankName);
    IFOKDO(err, bank.save())

    // Creation or update of the account
    SKGAccountObject account;
    IFOKDO(err, bank.addAccount(account))
    IFOKDO(err, account.setAttribute(QStringLiteral("rd_bank_id"), SKGServices::intToString(bank.getID())))
    IFOKDO(err, account.setName(iName))
    IFOKDO(err, account.setAttribute(QStringLiteral("t_number"), iNumber))
    IFOKDO(err, account.save())

    IFKO(err) err.addError(ERR_FAIL, i18nc("Error message", "Transaction '%1' on '%2' failed", QStringLiteral("SKGDocumentBank::addOrModifyAccount"), iName));
    return err;
}

QString SKGDocumentBank::getFileExtension() const
{
    return QStringLiteral("skg");
}

QString SKGDocumentBank::getDocumentHeader() const
{
    return QStringLiteral("SKROOGE");
}

SKGDocument::SKGModelTemplateList SKGDocumentBank::getDisplaySchemas(const QString &iRealTable) const
{
    SKGModelTemplateList listSchema;
    listSchema.reserve(10);

    // Get properties
    QStringList properties;
    QString tableForProperties = iRealTable;
    if (tableForProperties == QStringLiteral("suboperation")) {
        tableForProperties = QStringLiteral("operation");
    }
    this->getDistinctValues(QStringLiteral("parameters"),
                            QStringLiteral("t_name"),
                            "(t_uuid_parent like '%-" % tableForProperties % "' OR t_uuid_parent like '%-sub" % tableForProperties
                                % "') AND t_name NOT LIKE 'SKG_%'",
                            properties);

    // Build property schema
    QString propSchema;
    int nb = properties.count();
    for (int i = 0; i < nb; ++i) {
        propSchema += ";p_" % properties.at(i) % "|N";
    }

    // Build schemas
    if (iRealTable == QStringLiteral("operation") || iRealTable == QStringLiteral("suboperation")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema =
            "d_date;d_DATEWEEK|N;d_DATEMONTH|N;d_DATEQUARTER|N;d_DATESEMESTER|N;d_DATEYEAR|N;i_NBRECURRENT;t_bookmarked;t_ACCOUNT;t_TOACCOUNT|N;t_number;t_"
            "mode;t_PAYEE;t_comment;t_REALCOMMENT;t_CATEGORY;t_REALCATEGORY;t_status;"
            "f_REALCURRENTAMOUNT;f_REALCURRENTAMOUNT_EXPENSE|N;f_REALCURRENTAMOUNT_INCOME|N;"
            "f_CURRENTAMOUNT;f_CURRENTAMOUNT_EXPENSE|N;f_CURRENTAMOUNT_INCOME|N;"
            "f_QUANTITY|N;f_QUANTITY_EXPENSE|N;f_QUANTITY_INCOME|N;f_REALQUANTITY|N;f_REALQUANTITY_EXPENSE|N;f_REALQUANTITY_INCOME|N;t_UNIT|N;"
            "t_imported|N;t_REALREFUND|N;t_REFUND|N;t_REFUNDDISPLAY|N"
            ";f_BALANCE|N;f_BALANCE_ENTERED|N;d_createdate|N;i_OPID|N"
            % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema =
            "d_date;d_DATEWEEK|N;d_DATEMONTH|N;d_DATEQUARTER|N;d_DATESEMESTER|N;d_DATEYEAR|N;i_NBRECURRENT|N;t_bookmarked|N;t_ACCOUNT;t_TOACCOUNT|N;t_number|N;"
            "t_mode|N;t_PAYEE|N;t_comment|N;t_REALCOMMENT|N;t_CATEGORY|N;t_REALCATEGORY|N;t_status;"
            "f_REALCURRENTAMOUNT;f_REALCURRENTAMOUNT_EXPENSE|N;f_REALCURRENTAMOUNT_INCOME|N;"
            "f_CURRENTAMOUNT;f_CURRENTAMOUNT_EXPENSE|N;f_CURRENTAMOUNT_INCOME|N;"
            "f_QUANTITY|N;f_QUANTITY_EXPENSE|N;f_QUANTITY_INCOME|N;f_REALQUANTITY|N;f_REALQUANTITY_EXPENSE|N;f_REALQUANTITY_INCOME|N;t_UNIT|N;"
            "t_imported|N;t_REALREFUND|N;t_REFUND|N;t_REFUNDDISPLAY|N"
            ";f_BALANCE|N;f_BALANCE_ENTERED|N;d_createdate|N;i_OPID|N"
            % propSchema;
        listSchema.push_back(minimum);

        SKGModelTemplate doubleColumn;
        doubleColumn.id = QStringLiteral("doublecolumn");
        doubleColumn.name = i18nc("Noun", "Amount in 2 columns");
        doubleColumn.icon = QString();
        doubleColumn.schema =
            "d_date;d_DATEWEEK|N;d_DATEMONTH|N;d_DATEQUARTER|N;d_DATESEMESTER|N;d_DATEYEAR|N;i_NBRECURRENT;t_bookmarked;t_ACCOUNT;t_TOACCOUNT|N;t_number;t_"
            "mode;t_PAYEE;t_comment;t_REALCOMMENT;t_CATEGORY;t_REALCATEGORY;t_status;"
            "f_REALCURRENTAMOUNT|N;f_REALCURRENTAMOUNT_EXPENSE|Y;f_REALCURRENTAMOUNT_INCOME|Y;"
            "f_CURRENTAMOUNT|N;f_CURRENTAMOUNT_EXPENSE|Y;f_CURRENTAMOUNT_INCOME|Y;"
            "f_QUANTITY|N;f_QUANTITY_EXPENSE|N;f_QUANTITY_INCOME|N;f_REALQUANTITY|N;f_REALQUANTITY_EXPENSE|N;f_REALQUANTITY_INCOME|N;t_UNIT|N;"
            "t_imported|N;t_REALREFUND|N;t_REFUND|N;t_REFUNDDISPLAY|N"
            ";f_BALANCE|N;f_BALANCE_ENTERED|N;d_createdate|N;i_OPID|N"
            % propSchema;
        listSchema.push_back(doubleColumn);

        SKGModelTemplate amountEntered;
        amountEntered.id = QStringLiteral("amountentered");
        amountEntered.name = i18nc("Noun", "Amount entered");
        amountEntered.icon = QString();
        amountEntered.schema =
            "d_date;d_DATEWEEK|N;d_DATEMONTH|N;d_DATEQUARTER|N;d_DATESEMESTER|N;d_DATEYEAR|N;i_NBRECURRENT;t_bookmarked;t_ACCOUNT;t_TOACCOUNT|N;t_number;t_"
            "mode;t_PAYEE;t_comment;t_REALCOMMENT;t_CATEGORY;t_REALCATEGORY;t_status;"
            "f_REALCURRENTAMOUNT|N;f_REALCURRENTAMOUNT_EXPENSE|N;f_REALCURRENTAMOUNT_INCOME|N;"
            "f_CURRENTAMOUNT|N;f_CURRENTAMOUNT_EXPENSE|N;f_CURRENTAMOUNT_INCOME|N;"
            "f_QUANTITY|Y;f_QUANTITY_EXPENSE|N;f_QUANTITY_INCOME|N;f_REALQUANTITY|Y;f_REALQUANTITY_EXPENSE|N;f_REALQUANTITY_INCOME|N;t_UNIT|N;"
            "t_imported|N;t_REALREFUND|N;t_REFUND|N;t_REFUNDDISPLAY|N"
            ";f_BALANCE|N;f_BALANCE_ENTERED|N;d_createdate|N;i_OPID|N"
            % propSchema;
        listSchema.push_back(amountEntered);

        SKGModelTemplate doubleColumnEntered;
        doubleColumnEntered.id = QStringLiteral("doublecolumnentered");
        doubleColumnEntered.name = i18nc("Noun", "Amount entered in 2 columns");
        doubleColumnEntered.icon = QString();
        doubleColumnEntered.schema =
            "d_date;d_DATEWEEK|N;d_DATEMONTH|N;d_DATEQUARTER|N;d_DATESEMESTER|N;d_DATEYEAR|N;i_NBRECURRENT;t_bookmarked;t_ACCOUNT;t_TOACCOUNT|N;t_number;t_"
            "mode;t_PAYEE;t_comment;t_REALCOMMENT;t_CATEGORY;t_REALCATEGORY;t_status;"
            "f_REALCURRENTAMOUNT|N;f_REALCURRENTAMOUNT_EXPENSE|N;f_REALCURRENTAMOUNT_INCOME|N;"
            "f_CURRENTAMOUNT|N;f_CURRENTAMOUNT_EXPENSE|N;f_CURRENTAMOUNT_INCOME|N;"
            "f_QUANTITY|N;f_QUANTITY_EXPENSE|Y;f_QUANTITY_INCOME|Y;f_REALQUANTITY|N;f_REALQUANTITY_EXPENSE|Y;f_REALQUANTITY_INCOME|Y;t_UNIT|N;"
            "t_imported|N;t_REALREFUND|N;t_REFUND|N;t_REFUNDDISPLAY|N"
            ";f_BALANCE|N;f_BALANCE_ENTERED|N;d_createdate|N;i_OPID|N"
            % propSchema;
        listSchema.push_back(doubleColumnEntered);
    } else if (iRealTable == QStringLiteral("recurrentoperation")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema =
            "d_date;t_PERIODNLS;i_nb_times;i_auto_write_days;i_warn_days;t_ACCOUNT;t_number;t_mode;t_PAYEE;t_comment;t_CATEGORY;"
            "t_status;f_CURRENTAMOUNT"
            % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema =
            "d_date;t_PERIODNLS;i_nb_times;i_auto_write_days;i_warn_days;t_ACCOUNT;t_number|N;t_mode|N;t_PAYEE;t_comment|N;t_CATEGORY|N;"
            "t_status;f_CURRENTAMOUNT"
            % propSchema;
        listSchema.push_back(minimum);
    } else if (iRealTable == QStringLiteral("account")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema =
            "t_BANK;t_close;t_bookmarked;t_name;t_TYPENLS;t_BANK_NUMBER;t_agency_number;t_number;t_agency_address;t_comment;f_CURRENTAMOUNT;f_QUANTITY|N;f_"
            "TODAYAMOUNT|N;f_CHECKED;f_COMING_SOON;f_importbalance|N;d_importdate|N;f_reconciliationbalance|N;d_reconciliationdate|N;i_NBOPERATIONS;f_RATE|N"
            % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema =
            "t_BANK;t_close;t_bookmarked|N;t_name;t_TYPENLS|N;t_BANK_NUMBER|N;t_agency_number|N;t_number|N;t_agency_address|N;t_comment|N;f_CURRENTAMOUNT|N;f_"
            "QUANTITY|N;f_TODAYAMOUNT|N;f_CHECKED|N;f_COMING_SOON|N;f_importbalance|N;d_importdate|N;f_reconciliationbalance|N;d_reconciliationdate|N;i_"
            "NBOPERATIONS|N;f_RATE|N"
            % propSchema;
        listSchema.push_back(minimum);

        SKGModelTemplate intermediate;
        intermediate.id = QStringLiteral("intermediate");
        intermediate.name = i18nc("Noun, an intermediate value between two extremums", "Intermediate");
        intermediate.icon = QString();
        intermediate.schema =
            "t_BANK;t_close;t_bookmarked;t_name;t_TYPENLS|N;t_BANK_NUMBER|N;t_agency_number|N;t_number|N;t_agency_address|N;t_comment|N;f_CURRENTAMOUNT;f_"
            "QUANTITY|N;f_TODAYAMOUNT|N,f_CHECKED;f_COMING_SOON;f_importbalance|N;d_importdate|N;f_reconciliationbalance|N;d_reconciliationdate|N;i_"
            "NBOPERATIONS|N;f_RATE|N"
            % propSchema;
        listSchema.push_back(intermediate);
    } else if (iRealTable == QStringLiteral("category")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = "t_name;t_close;t_bookmarked;i_NBOPERATIONS;f_REALCURRENTAMOUNT;i_SUMNBOPERATIONS;f_SUMCURRENTAMOUNT" % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema = "t_name;t_close|N;t_bookmarked;i_NBOPERATIONS|N;f_REALCURRENTAMOUNT|N;i_SUMNBOPERATIONS|N;f_SUMCURRENTAMOUNT|N" % propSchema;
        listSchema.push_back(minimum);

        SKGModelTemplate op;
        op.id = QStringLiteral("with_operations");
        op.name = i18nc("Noun", "With transactions");
        op.icon = QString();
        op.schema = "t_name;t_close;t_bookmarked;i_NBOPERATIONS;f_REALCURRENTAMOUNT;i_SUMNBOPERATIONS|N;f_SUMCURRENTAMOUNT|N" % propSchema;
        listSchema.push_back(op);

        SKGModelTemplate op2;
        op2.id = QStringLiteral("with_cumulative_operations");
        op2.name = i18nc("Noun", "With cumulative transactions");
        op2.icon = QString();
        op2.schema = "t_name;t_close;t_bookmarked;i_NBOPERATIONS|N;f_REALCURRENTAMOUNT|N;i_SUMNBOPERATIONS;f_SUMCURRENTAMOUNT" % propSchema;
        listSchema.push_back(op2);
    } else if (iRealTable == QStringLiteral("unit")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema =
            "t_name;t_symbol;t_bookmarked;t_country;t_TYPENLS;t_source;t_internet_code;f_CURRENTAMOUNT;f_QUANTITYOWNED;f_AMOUNTOWNED;i_nbdecimal;t_UNIT;d_"
            "MAXDATE|N"
            % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema =
            "t_name;t_symbol;t_bookmarked|N;t_country|N;t_TYPENLS;t_source|N;t_internet_code|N;f_CURRENTAMOUNT|N;f_QUANTITYOWNED|N;f_AMOUNTOWNED|N;i_nbdecimal|"
            "N;t_UNIT|N;d_MAXDATE|N"
            % propSchema;
        listSchema.push_back(minimum);
    } else if (iRealTable == QStringLiteral("unitvalue")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = "d_date;f_quantity;t_UNIT|N;f_AMOUNTOWNED|N" % propSchema;
        listSchema.push_back(def);
    } else if (iRealTable == QStringLiteral("refund")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = "t_name;t_comment;t_close;d_FIRSTDATE;d_LASTDATE;f_CURRENTAMOUNT" % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema = "t_name;t_comment|N;t_close;d_FIRSTDATE|N;d_LASTDATE|N;f_CURRENTAMOUNT" % propSchema;
        listSchema.push_back(minimum);
    } else if (iRealTable == QStringLiteral("payee")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = "t_name;t_close;t_bookmarked;t_address;i_NBOPERATIONS|N;f_CURRENTAMOUNT;t_CATEGORY|N" % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema = "t_name;t_close|N;t_bookmarked;t_address|N;i_NBOPERATIONS|N;f_CURRENTAMOUNT;t_CATEGORY|N" % propSchema;
        listSchema.push_back(minimum);
    } else if (iRealTable == QStringLiteral("rule")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = "i_ORDER;t_bookmarked;t_action_type;t_description;t_action_description" % propSchema;
        listSchema.push_back(def);
    } else if (iRealTable == QStringLiteral("interest")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = "d_date;f_rate;t_income_value_date_mode;t_expenditure_value_date_mode;t_base" % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema = "d_date;f_rate;t_income_value_date_mode|N;t_expenditure_value_date_mode|N;t_base|N" % propSchema;
        listSchema.push_back(minimum);
    } else if (iRealTable == QStringLiteral("interest_result")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = "d_date;d_valuedate;t_comment;f_currentamount;f_coef;f_rate;f_annual_interest;f_accrued_interest" % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema = "d_date;d_valuedate|N;t_comment|N;f_currentamount|N;f_coef|N;f_rate;f_annual_interest;f_accrued_interest|N" % propSchema;
        listSchema.push_back(minimum);
    } else if (iRealTable == QStringLiteral("budget")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema =
            "t_CATEGORY;t_PERIOD;i_year|N;i_month|N;f_budgeted;f_budgeted_modified;f_CURRENTAMOUNT;f_DELTABEFORETRANSFER|N;t_RULES;f_DELTA" % propSchema;
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema =
            "t_CATEGORY;t_PERIOD;i_year|N;i_month|N;f_budgeted|N;f_budgeted_modified;f_CURRENTAMOUNT;f_DELTABEFORETRANSFER;t_RULES|N;f_DELTA|N" % propSchema;
        listSchema.push_back(minimum);
    } else if (iRealTable == QStringLiteral("budgetrule")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = "i_ORDER;t_CATEGORYCONDITION;i_year;i_month;t_WHENNLS;t_WHATNLS;t_RULENLS;t_CATEGORY" % propSchema;
        listSchema.push_back(def);
    } else {
        listSchema = SKGDocument::getDisplaySchemas(iRealTable);
    }

    return listSchema;
}

QString SKGDocumentBank::getIconName(const QString &iString) const
{
    QString att = iString.toLower();

    if (att.endsWith(QStringLiteral("t_bookmarked"))) {
        return QStringLiteral("bookmarks");
    }
    if (att.endsWith(QStringLiteral("f_balance")) || att.endsWith(QStringLiteral("f_balance_entered"))
        || att.endsWith(QStringLiteral("f_reconciliationbalance"))) {
        return QStringLiteral("office-chart-line");
    }
    if (att.endsWith(QStringLiteral("i_nbrecurrent"))) {
        return QStringLiteral("view-calendar-day");
    }
    if (att.endsWith(QStringLiteral("t_status")) || att.endsWith(QStringLiteral("f_checked")) || att.endsWith(QStringLiteral("f_coming_soon"))
        || att.endsWith(QStringLiteral("d_reconciliationdate"))) {
        return QStringLiteral("dialog-ok");
    }
    if (att.endsWith(QStringLiteral("t_close"))) {
        return QStringLiteral("window-close");
    }
    if (att.endsWith(QStringLiteral("t_categorycondition")) || att.endsWith(QStringLiteral("t_category")) || att.endsWith(QStringLiteral("t_realcategory"))) {
        return QStringLiteral("applications-all-symbolic");
    }
    if (att.endsWith(QStringLiteral("t_symbol"))) {
        return QStringLiteral("taxes-finances");
    }
    if (att.endsWith(QStringLiteral("t_typeexpensenls"))) {
        return QStringLiteral("skrooge_type");
    }
    if (att.endsWith(QStringLiteral("t_typenls"))) {
        if (att.contains(QStringLiteral("v_unit"))) {
            return QStringLiteral("view-financial-account-savings");
        }
        if (att.contains(QStringLiteral("v_account"))) {
            return QStringLiteral("skrooge_credit_card");
        }
    }
    if (att.endsWith(QStringLiteral("t_unit")) || att.endsWith(QStringLiteral("t_unittype"))) {
        return QStringLiteral("taxes-finances");
    }
    if (att.endsWith(QStringLiteral("f_value")) || att.endsWith(QStringLiteral("f_currentamount")) || att.endsWith(QStringLiteral("f_todayamount"))
        || att.endsWith(QStringLiteral("f_sumcurrentamount")) || att.endsWith(QStringLiteral("quantity"))
        || att.endsWith(QStringLiteral("f_realcurrentamount"))) {
        return QStringLiteral("skrooge_type");
    }
    if (att.endsWith(QStringLiteral("_expense"))) {
        return QStringLiteral("list-remove");
    }
    if (att.endsWith(QStringLiteral("_income")) || att.endsWith(QStringLiteral("f_annual_interest")) || att.endsWith(QStringLiteral("f_accrued_interest"))) {
        return QStringLiteral("list-add");
    }
    if (att.endsWith(QStringLiteral("t_description"))) {
        return QStringLiteral("system-search");
    }
    if (att.endsWith(QStringLiteral("t_action_description"))) {
        return QStringLiteral("system-run");
    }
    if (att.endsWith(QStringLiteral("t_imported")) || att.endsWith(QStringLiteral("f_importbalance")) || att.endsWith(QStringLiteral("d_importdate"))) {
        return QStringLiteral("utilities-file-archiver");
    }
    if (att.endsWith(QStringLiteral("t_refund")) || att.endsWith(QStringLiteral("t_refunddisplay")) || att.endsWith(QStringLiteral("t_realrefund"))) {
        return QStringLiteral("crosshairs");
    }
    if (att.endsWith(QStringLiteral("t_mode"))) {
        return QStringLiteral("skrooge_credit_card");
    }
    if (att.endsWith(QStringLiteral("t_account")) || att.endsWith(QStringLiteral("t_toaccount")) || att.endsWith(QStringLiteral("t_accounttype"))) {
        return QStringLiteral("view-bank");
    }
    if (att.endsWith(QStringLiteral("t_payee"))) {
        return QStringLiteral("user-group-properties");
    }
    if (att.endsWith(QStringLiteral("t_comment")) || att.endsWith(QStringLiteral("t_realcomment"))) {
        return QStringLiteral("draw-freehand");
    }
    if (att.endsWith(QStringLiteral("t_warn")) || att.endsWith(QStringLiteral("i_warn_days"))) {
        return QStringLiteral("dialog-information");
    }
    if (att.endsWith(QStringLiteral("t_name"))) {
        if (att.contains(QStringLiteral("v_account"))) {
            return QStringLiteral("view-bank");
        }
        if (att.contains(QStringLiteral("v_category"))) {
            return QStringLiteral("applications-all-symbolic");
        }
        if (att.contains(QStringLiteral("v_refund"))) {
            return QStringLiteral("crosshairs");
        }
        if (att.contains(QStringLiteral("v_unit"))) {
            return QStringLiteral("taxes-finances");
        }
        if (att.contains(QStringLiteral("v_payee"))) {
            return QStringLiteral("user-group-properties");
        }
    }
    if (att.endsWith(QStringLiteral("f_rate"))) {
        return QStringLiteral("skrooge_more");
    }
    if (att.endsWith(QStringLiteral("t_internet_code")) || att.endsWith(QStringLiteral("t_source")) || att.endsWith(QStringLiteral("d_maxdate"))) {
        return QStringLiteral("download");
    }
    if (att.contains(QStringLiteral(".d_")) || att.startsWith(QStringLiteral("d_"))) {
        return QStringLiteral("view-calendar-day");
    }
    if (att.endsWith(QStringLiteral("i_year")) || att.endsWith(QStringLiteral("i_month")) || att.endsWith(QStringLiteral("t_period"))) {
        return QStringLiteral("view-calendar-day");
    }
    if (att.endsWith(QStringLiteral("f_delta"))) {
        return QStringLiteral("security-high");
    }
    if (att.endsWith(QStringLiteral("f_deltabeforetransfer"))) {
        return QStringLiteral("security-medium");
    }
    if (att.endsWith(QStringLiteral("f_budgeted")) || att.endsWith(QStringLiteral("f_budgeted_modified"))) {
        return QStringLiteral("view-calendar-whatsnext");
    }
    if (att.endsWith(QStringLiteral("t_rules"))) {
        return QStringLiteral("system-run");
    }
    if (att.endsWith(QStringLiteral("t_whennls"))) {
        return QStringLiteral("view-calendar-day");
    }
    if (att.endsWith(QStringLiteral("t_whatnls"))) {
        return QStringLiteral("skrooge_type");
    }
    if (att.endsWith(QStringLiteral("t_rulenls"))) {
        return QStringLiteral("view-calendar-whatsnext");
    }
    if (att.endsWith(QStringLiteral("t_bank"))) {
        return QStringLiteral("view-bank");
    }
    if (att.endsWith(QStringLiteral("t_transfer"))) {
        return QStringLiteral("exchange-positions");
    }
    if (att.endsWith(QStringLiteral("_number"))) {
        return QStringLiteral("dialog-information");
    }
    if (att.endsWith(QStringLiteral("i_auto_write_days"))) {
        return QStringLiteral("insert-text");
    }
    if (att.endsWith(QStringLiteral("_address"))) {
        return QStringLiteral("address-book-new");
    }
    if (att.endsWith(QStringLiteral("i_order"))) {
        return QStringLiteral("view-sort-ascending");
    }
    if (att.endsWith(QStringLiteral("t_periodnls"))) {
        return QStringLiteral("smallclock");
    }
    if (att.endsWith(QStringLiteral("i_nbsuboperations"))) {
        return QStringLiteral("exchange-positions");
    }
    return SKGDocument::getIconName(iString);
}

QString SKGDocumentBank::getDisplay(const QString &iString) const
{
    QString output = iString.toLower();

    // Internationallization
    if (output.endsWith(QStringLiteral("account.t_name")) || output.endsWith(QStringLiteral("t_account"))) {
        return i18nc("Noun, an account as in a bank account", "Account");
    }
    if (output.endsWith(QStringLiteral("t_accounttype"))) {
        return i18nc("Noun, an account as in a bank account", "Account's type");
    }
    if (output.endsWith(QStringLiteral("t_operationname"))) {
        return i18nc("Noun, a financial operation", "Transaction");
    }
    if (output.endsWith(QStringLiteral("t_name"))) {
        return i18nc("Noun, the name of an item", "Name");
    }
    if (output.endsWith(QStringLiteral("account.f_value")) || output.endsWith(QStringLiteral("f_balance"))) {
        return i18nc("Noun, as in commercial balance", "Balance");
    }
    if (output.endsWith(QStringLiteral("f_balance_entered"))) {
        return i18nc("Noun, as in commercial balance", "Balance entered");
    }
    if (output.endsWith(QStringLiteral("f_value"))) {
        return i18nc("Name, the numerical amount of a financial operation", "Amount");
    }
    if (output.endsWith(QStringLiteral("f_currentamount")) || output.endsWith(QStringLiteral("f_realcurrentamount"))) {
        return i18nc("Name, the numerical amount of a financial operation", "Amount");
    }
    if (output.endsWith(QStringLiteral("f_todayamount"))) {
        return i18nc("Name, the numerical amount of a financial operation", "Today amount");
    }
    if (output.endsWith(QStringLiteral("f_currentamount_income")) || output.endsWith(QStringLiteral("f_realcurrentamount_income"))) {
        return i18nc("Noun, financial transactions with a positive amount", "Income");
    }
    if (output.endsWith(QStringLiteral("f_currentamount_expense")) || output.endsWith(QStringLiteral("f_realcurrentamount_expense"))) {
        return i18nc("Noun, financial transactions with a negative amount", "Expenditure");
    }
    if (output.endsWith(QStringLiteral("f_quantity_income")) || output.endsWith(QStringLiteral("f_realquantity_income"))) {
        return i18nc("Noun", "Income entered");
    }
    if (output.endsWith(QStringLiteral("f_quantity_expense")) || output.endsWith(QStringLiteral("f_realquantity_expense"))) {
        return i18nc("Noun", "Expenditure entered");
    }
    if (output.endsWith(QStringLiteral("f_quantityowned"))) {
        return i18nc("Noun", "Quantity owned");
    }
    if (output.endsWith(QStringLiteral("f_amountowned"))) {
        return i18nc("Noun", "Amount owned");
    }
    if (output.endsWith(QStringLiteral("quantity"))) {
        return i18nc("Noun", "Amount entered");
    }
    if (output.endsWith(QStringLiteral("account.t_number"))) {
        return i18nc("Noun", "Account number");
    }
    if (output.endsWith(QStringLiteral("t_number"))) {
        return i18nc("Noun, a number identifying an item", "Number");
    }
    if (output.endsWith(QStringLiteral("t_bank_number"))) {
        return i18nc("Noun", "Bank number");
    }
    if (output.endsWith(QStringLiteral("t_agency_number"))) {
        return i18nc("Noun", "Agency number");
    }
    if (output.endsWith(QStringLiteral("t_agency_address"))) {
        return i18nc("Noun", "Agency address");
    }
    if (output.endsWith(QStringLiteral("t_address"))) {
        return i18nc("Noun", "Address");
    }
    if (output.endsWith(QStringLiteral("t_payee"))) {
        return i18nc("A person or institution receiving a payment, or paying the operation", "Payee");
    }
    if (output.endsWith(QStringLiteral("t_comment"))) {
        return i18nc("Noun, a user comment on an item", "Comment");
    }
    if (output.endsWith(QStringLiteral("t_realcomment"))) {
        return i18nc("Noun, a user comment on an item", "Sub comment");
    }
    if (output.endsWith(QStringLiteral("t_mode"))) {
        return i18nc("Noun, the mode used for payment of the transaction (Credit Card, Cheque, Transfer…)", "Mode");
    }
    if (output.contains(QStringLiteral("recurrentoperation")) && output.endsWith(QStringLiteral("d_date"))) {
        return i18nc("Noun", "Next occurrence");
    }
    if (output.endsWith(QStringLiteral("d_date")) || output.endsWith(QStringLiteral("d_dateop"))) {
        return i18nc("Noun, the date of an item", "Date");
    }
    if (output.endsWith(QStringLiteral("d_createdate"))) {
        return i18nc("Noun, the date of creation of an item", "Creation date");
    }
    if (output.endsWith(QStringLiteral("d_dateweek"))) {
        return i18nc("Noun, 7 days", "Week");
    }
    if (output.endsWith(QStringLiteral("d_datemonth"))) {
        return i18nc("Noun, the months in a year", "Month");
    }
    if (output.endsWith(QStringLiteral("d_datequarter"))) {
        return i18nc("Noun, 3 months", "Quarter");
    }
    if (output.endsWith(QStringLiteral("d_datesemester"))) {
        return i18nc("Noun, 6 months", "Semester");
    }
    if (output.endsWith(QStringLiteral("d_dateyear"))) {
        return i18nc("Noun, the years in a century", "Year");
    }
    if (output.endsWith(QStringLiteral("d_firstdate"))) {
        return i18nc("Noun, the date of an item", "First date");
    }
    if (output.endsWith(QStringLiteral("d_lastdate"))) {
        return i18nc("Noun, the date of an item", "Last date");
    }
    if (output.endsWith(QStringLiteral("d_maxdate"))) {
        return i18nc("Noun, the date of the last download", "Download date");
    }
    if (output.endsWith(QStringLiteral("d_reconciliationdate"))) {
        return i18nc("Noun, the date of the last reconciliation", "Reconciliation date");
    }
    if (output.endsWith(QStringLiteral("t_categorycondition")) || output.endsWith(QStringLiteral("t_category"))
        || output.endsWith(QStringLiteral("t_realcategory"))) {
        return i18nc("Noun, the category of an item", "Category");
    }
    if (output.endsWith(QStringLiteral("t_bank"))) {
        return i18nc("Noun, a financial institution", "Bank");
    }
    if (output.endsWith(QStringLiteral("t_unit"))) {
        return i18nc("Noun, the unit of an operation, usually a currency or a share", "Unit");
    }
    if (output.endsWith(QStringLiteral("t_unittype"))) {
        return i18nc("Noun, the unit of an operation, usually a currency or a share", "Unit's type");
    }
    if (output.endsWith(QStringLiteral("f_checked"))) {
        return i18nc("Adjective, has an item been checked or not", "Checked");
    }
    if (output.endsWith(QStringLiteral("f_coming_soon"))) {
        return i18nc("Adjective, a foreseen value", "To be Checked");
    }
    if (output.endsWith(QStringLiteral("t_symbol"))) {
        return i18nc("Noun, ahe unit symbol, something in the line of $, €, £…", "Symbol");
    }
    if (output.endsWith(QStringLiteral("t_country"))) {
        return i18nc("Noun, a country in the world (France, China…)", "Country");
    }
    if (output.endsWith(QStringLiteral("t_type")) || output.endsWith(QStringLiteral("t_typenls"))) {
        return i18nc("Noun, the type of an item", "Type");
    }
    if (output.endsWith(QStringLiteral("t_typeexpensenls"))) {
        return i18nc("Noun, the type of an item", "Type");
    }
    if (output.endsWith(QStringLiteral("t_internet_code"))) {
        return i18nc("Noun", "Internet code");
    }
    if (output.endsWith(QStringLiteral("i_nboperations"))) {
        return i18nc("Noun", "Number of transactions");
    }
    if (output.endsWith(QStringLiteral("t_periodnls"))) {
        return i18nc("Noun, how frequently something occurs", "Periodicity");
    }
    if (output.endsWith(QStringLiteral("i_auto_write_days"))) {
        return i18nc("Automatically write something", "Auto write");
    }
    if (output.endsWith(QStringLiteral("i_nb_times"))) {
        return i18nc("Noun", "Nb of occurrences");
    }
    if (output.endsWith(QStringLiteral("i_warn_days"))) {
        return i18nc("Verb, warn the user about an event", "Warn");
    }
    if (output.endsWith(QStringLiteral("t_close"))) {
        return i18nc("Adjective, a closed item", "Closed");
    }
    if (output.endsWith(QStringLiteral("t_bookmarked"))) {
        return i18nc("Adjective, an highlighted item", "Highlighted");
    }
    if (output.endsWith(QStringLiteral("t_status"))) {
        return i18nc("Noun, the status of an item", "Status");
    }
    if (output.endsWith(QStringLiteral("i_nbrecurrent"))) {
        return i18nc("Adjective, an item scheduled to happen on a regular basis", "Scheduled");
    }
    if (output.endsWith(QStringLiteral("i_sumnboperations"))) {
        return i18nc("Noun", "Number of transactions (cumulative)");
    }
    if (output.endsWith(QStringLiteral("f_sumcurrentamount"))) {
        return i18nc("Noun", "Amount (cumulative)");
    }
    if (output.endsWith(QStringLiteral("t_description"))) {
        return i18nc("Noun", "Search description");
    }
    if (output.endsWith(QStringLiteral("t_action_description"))) {
        return i18nc("Noun", "Process description");
    }
    if (output.endsWith(QStringLiteral("t_action_type"))) {
        return i18nc("Noun, the type of action", "Action type");
    }
    if (output.endsWith(QStringLiteral("t_refund")) || output.endsWith(QStringLiteral("t_realrefund"))) {
        return i18nc("Noun, something that is used to track items", "Tracker");
    }
    if (output.endsWith(QStringLiteral("t_refunddisplay"))) {
        return i18nc("Noun, something that is used to track items", "Trackers");
    }
    if (output.endsWith(QStringLiteral("t_imported"))) {
        return i18nc("Noun", "Import status");
    }
    if (output.endsWith(QStringLiteral("i_nbdecimal"))) {
        return i18nc("Noun, after the dot", "Nb decimal");
    }
    if (output.endsWith(QStringLiteral("f_rate"))) {
        return i18nc("Noun, for a share", "Rate");
    }
    if (output.endsWith(QStringLiteral("d_valuedate"))) {
        return i18nc("Noun", "Value date");
    }
    if (output.endsWith(QStringLiteral("f_coef"))) {
        return i18nc("Noun", "Coef");
    }
    if (output.endsWith(QStringLiteral("f_annual_interest"))) {
        return i18nc("Noun", "Annual Interest");
    }
    if (output.endsWith(QStringLiteral("f_accrued_interest"))) {
        return i18nc("Noun", "Accrued Interest");
    }
    if (output.endsWith(QStringLiteral("t_income_value_date_mode"))) {
        return i18nc("Noun", "Value date for credit");
    }
    if (output.endsWith(QStringLiteral("t_expenditure_value_date_mode"))) {
        return i18nc("Noun", "Value date for debit");
    }
    if (output.endsWith(QStringLiteral("t_base"))) {
        return i18nc("Noun", "Base computation");
    }
    if (output.endsWith(QStringLiteral("i_year"))) {
        return i18nc("Noun", "Year");
    }
    if (output.endsWith(QStringLiteral("i_month"))) {
        return i18nc("Noun", "Month");
    }
    if (output.endsWith(QStringLiteral("t_period"))) {
        return i18nc("Noun", "Period");
    }
    if (output.endsWith(QStringLiteral("i_order"))) {
        return i18nc("Noun, sort order", "Order");
    }
    if (output.endsWith(QStringLiteral("t_whennls"))) {
        return i18nc("Noun", "When");
    }
    if (output.endsWith(QStringLiteral("t_whatnls"))) {
        return i18nc("Noun", "What");
    }
    if (output.endsWith(QStringLiteral("t_rulenls"))) {
        return i18nc("Noun", "Impacted budget");
    }
    if (output.endsWith(QStringLiteral("t_rules"))) {
        return i18nc("Noun", "Rules");
    }
    if (output.endsWith(QStringLiteral("f_budgeted"))) {
        return i18nc("Noun", "Entered Budget");
    }
    if (output.endsWith(QStringLiteral("f_budgeted_modified"))) {
        return i18nc("Noun", "Corrected budget");
    }
    if (output.endsWith(QStringLiteral("f_delta"))) {
        return i18nc("Noun", "Delta after rules");
    }
    if (output.endsWith(QStringLiteral("f_deltabeforetransfer"))) {
        return i18nc("Noun", "Delta");
    }
    if (output.endsWith(QStringLiteral("t_source"))) {
        return i18nc("Noun", "Download source");
    }
    if (output.endsWith(QStringLiteral("t_transfer"))) {
        return i18nc("Noun", "Transfer");
    }
    if (output.endsWith(QStringLiteral("t_toaccount"))) {
        return i18nc("Noun, a target account of a transfer", "To account");
    }
    if (output.endsWith(QStringLiteral("f_maxamount"))) {
        return i18nc("Noun, a maximum limit", "Maximum limit");
    }
    if (output.endsWith(QStringLiteral("f_minamount"))) {
        return i18nc("Noun, a minimum limit", "Minimum limit");
    }
    if (output.endsWith(QStringLiteral("i_opid"))) {
        return i18nc("Noun, the id of an operation", "Transaction id");
    }
    if (output.endsWith(QStringLiteral("#nothing#"))) {
        return i18nc("Noun, the absence of anything", "-- Nothing --");
    }
    if (output.endsWith(QStringLiteral("f_importbalance"))) {
        return i18nc("Noun", "Balance import");
    }
    if (output.endsWith(QStringLiteral("f_reconciliationbalance"))) {
        return i18nc("Noun", "Balance reconciliation");
    }
    if (output.endsWith(QStringLiteral("d_importdate"))) {
        return i18nc("Noun, the date of the last import", "Import date");
    }
    if (output.endsWith(QStringLiteral("i_nbsuboperations"))) {
        return i18nc("Noun", "Number of split");
    }

    return SKGDocument::getDisplay(iString);
}

QString SKGDocumentBank::getRealAttribute(const QString &iString) const
{
    if (iString.endsWith(QStringLiteral("t_BANK"))) {
        return QStringLiteral("bank.rd_bank_id.t_name");
    }
    if (iString.endsWith(QStringLiteral("t_BANK_NUMBER"))) {
        return QStringLiteral("bank.rd_bank_id.t_bank_number");
    }
    return SKGDocument::getRealAttribute(iString);
}

SKGServices::AttributeType SKGDocumentBank::getAttributeType(const QString &iAttributeName) const
{
    SKGServices::AttributeType output = SKGServices::TEXT;
    if (iAttributeName == QStringLiteral("t_status") || iAttributeName == QStringLiteral("t_imported")) {
        return SKGServices::TRISTATE;
    }
    if (iAttributeName == QStringLiteral("t_close") || iAttributeName == QStringLiteral("t_bookmarked") || iAttributeName == QStringLiteral("t_auto_write")
        || iAttributeName == QStringLiteral("t_warn") || iAttributeName == QStringLiteral("t_TRANSFER") || iAttributeName == QStringLiteral("t_template")
        || iAttributeName == QStringLiteral("t_times") || iAttributeName == QStringLiteral("t_absolute")
        || iAttributeName == QStringLiteral("t_category_condition") || iAttributeName == QStringLiteral("t_month_condition")
        || iAttributeName == QStringLiteral("t_year_condition") || iAttributeName == QStringLiteral("t_including_subcategories")) {
        return SKGServices::BOOL;
    }
    output = SKGDocument::getAttributeType(iAttributeName);

    return output;
}

QVariantList SKGDocumentBank::getBudget(const QString &iMonth) const
{
    SKGTRACEINFUNC(10)
    QVariantList table;
    SKGStringListList listTmp;
    SKGError err = executeSelectSqliteOrder(
        "SELECT t_CATEGORY, f_budgeted, f_CURRENTAMOUNT, f_DELTABEFORETRANSFER, f_budgeted_modified  FROM v_budget "
        "where t_PERIOD='"
            % iMonth % "' ORDER BY t_CATEGORY;",
        listTmp);
    int nbval = listTmp.count();
    if (!err && nbval > 1) {
        table.reserve(nbval + 1);
        table.push_back(QVariantList() << "sum" << getDisplay(QStringLiteral("t_CATEGORY")) << getDisplay(QStringLiteral("f_budgeted_modified"))
                                       << getDisplay(QStringLiteral("f_CURRENTAMOUNT")) << getDisplay(QStringLiteral("f_DELTA")));
        double sum1 = 0;
        double sum2 = 0;
        double sum3 = 0;
        double sum4 = 0;
        for (int i = 1; i < nbval; ++i) { // Ignore header
            double v1 = SKGServices::stringToDouble(listTmp.at(i).at(1));
            double v2 = SKGServices::stringToDouble(listTmp.at(i).at(2));
            double v3 = SKGServices::stringToDouble(listTmp.at(i).at(3));
            double v4 = SKGServices::stringToDouble(listTmp.at(i).at(4));
            table.push_back(QVariantList() << false << listTmp.at(i).at(0) << v1 << v2 << v3 << v4);

            sum1 += v1;
            sum2 += v2;
            sum3 += v3;
            sum4 += v4;
        }
        table.push_back(QVariantList() << true << i18nc("Noun, the numerical total of a sum of values", "Total") << sum1 << sum2 << sum3 << sum4);
    }
    return table;
}

QVariantList SKGDocumentBank::getMainCategories(const QString &iPeriod, int iNb)
{
    SKGTRACEINFUNC(10)
    QVariantList table;
    SKGServices::SKGUnitInfo primary = getPrimaryUnit();

    QString wc = "t_TRANSFER='N' AND t_TYPEEXPENSE='-' AND " + SKGServices::getPeriodWhereClause(iPeriod);

    SKGStringListList listTmp;
    SKGError err = executeSelectSqliteOrder(
        "WITH filtered AS (SELECT  t_REALCATEGORY, f_REALCURRENTAMOUNT, SUM(f_REALCURRENTAMOUNT) OVER () AS total_all "
        "FROM v_suboperation_consolidated WHERE " % wc % ") "
        "SELECT t_REALCATEGORY, SUM(f_REALCURRENTAMOUNT) AS total_cat, 100.0 * SUM(f_REALCURRENTAMOUNT) / MAX(total_all) AS pct "
        "FROM filtered GROUP BY t_REALCATEGORY ORDER BY total_cat ASC",
        listTmp);
    int nbval = listTmp.count();
    if (!err && (nbval != 0)) {
        table.reserve(nbval);
        table.push_back(QVariantList() << "sum" << getDisplay(QStringLiteral("t_REALCATEGORY")) << iPeriod << "url" << "percent");

        // Add X main categories
        for (int i = 1; i < nbval && i <= iNb; ++i) { // Ignore header
            QString cat = listTmp.at(i).at(0);
            double v = qAbs(SKGServices::stringToDouble(listTmp.at(i).at(1)));
            double p = qAbs(SKGServices::stringToDouble(listTmp.at(i).at(2)));
            table.push_back(QVariantList() << false << cat << v << QString(wc % " AND t_REALCATEGORY='" % SKGServices::stringToSqlString(cat) % "'") << p);
        }

        // Build "Other" category
        QStringList listCat;
        listCat.reserve(nbval);
        double sum = 0.0;
        double sumPercent = 0.0;
        for (int i = iNb + 1; i < nbval; ++i) {
            listCat.push_back(SKGServices::stringToSqlString(listTmp.at(i).at(0)));
            sum += qAbs(SKGServices::stringToDouble(listTmp.at(i).at(1)));
            sumPercent += qAbs(SKGServices::stringToDouble(listTmp.at(i).at(2)));
        }
        if (listCat.count() != 0) {
            table.push_back(QVariantList() << false << i18nc("an other category", "Others") << sum
                                           << QString(wc % " AND t_REALCATEGORY IN ('" % listCat.join(QStringLiteral("','")) % "')") << sumPercent);
        }
    }
    return table;
}

QStringList
SKGDocumentBank::get5MainCategoriesVariationList(const QString &iPeriod, const QString &iPreviousPeriod, bool iOnlyIssues, QStringList *oCategoryList)
{
    SKGTRACEINFUNC(10)
    // Compute input string
    QString inputString = iPeriod % iPreviousPeriod % (iOnlyIssues ? 'Y' : 'N') % (oCategoryList != nullptr ? 'Y' : 'N');

    // Use cache or not
    QStringList output;
    if (inputString == m_5mainVariations_inputs) {
        // Yes
        output = m_5mainVariations_cache;
        if (oCategoryList != nullptr) {
            *oCategoryList = m_5mainVariationsCat_cache;
        }
    }
    m_5mainVariations_inputs = std::move(inputString);

    if (output.isEmpty()) {
        SKGServices::SKGUnitInfo primary = getPrimaryUnit();

        SKGStringListList listTmp;
        SKGError err = executeSelectSqliteOrder("select *, 100*(A2-A1)/ABS(A1) as 'V' from "
                                                "(SELECT t_REALCATEGORY as 'C1', TOTAL(f_REALCURRENTAMOUNT) as 'A1' FROM v_suboperation_consolidated where "
                                                "t_TRANSFER='N' AND " + SKGServices::getPeriodWhereClause(iPreviousPeriod) + " AND t_TYPEEXPENSE='-' group by t_REALCATEGORY) A,"
                                                "(SELECT t_REALCATEGORY as 'C2', TOTAL(f_REALCURRENTAMOUNT) as 'A2' FROM v_suboperation_consolidated where "
                                                "t_TRANSFER='N' AND " + SKGServices::getPeriodWhereClause(iPeriod) + " AND t_TYPEEXPENSE='-' group by t_REALCATEGORY) B "
                                                "WHERE A.C1=B.C2 AND ABS(A2-A1)/ABS(A1)>0.1 " +
                                                (iOnlyIssues ? "AND ((A1<0 AND A2<0 AND A2<A1) OR (A1>0 AND A2>0 AND A2<A1))" : "") +
                                                " ORDER BY ABS(A2-A1) DESC LIMIT 5;",
                                                listTmp);
        IFOK(err)
        {
            // Clear the list
            m_5mainVariations_cache.clear();
            m_5mainVariationsCat_cache.clear();

            // Fill list
            int nbval = listTmp.count();
            for (int i = 1; i < nbval; ++i) { // Ignore header
                /*Example of sentences:
                  Expenses in category "Food > Grocery" in November decreased by 14% for a total  of 220,48€.
                  Expenses in category "Food" (no subcategory) in November increased by 24% for a total of 70,20€.
                  Expenses in category "Automotive > Fuel" in November increased by 24% for a total of 122,48€.
                  Expenses in category "Misc" in November decreased by 30% for a total of 36,52€.
                  This month, you spent 75,00€ in Category "Bills > Subscriptions". No expense in that category was found in previous month
                  Expenses with mode "withdrawal" reprensented 60,00€ of your expenses in current month*/
                QString c1 = listTmp.at(i).at(0);
                double a1 = SKGServices::stringToDouble(listTmp.at(i).at(1));
                double a2 = SKGServices::stringToDouble(listTmp.at(i).at(3));
                double v = SKGServices::stringToDouble(listTmp.at(i).at(4));

                QString a2f = formatMoney(qAbs(a2), primary);
                if (a1 < 0 && a2 < 0) {
                    QString vf = formatPercentage(qAbs(v), v < 0);
                    if (v < 0) {
                        m_5mainVariations_cache.push_back(
                            i18n("Expenses in category <b>'%1'</b> increased by <b>%2</b> for a total of <b>%3</b>.", c1, vf, a2f));
                        m_5mainVariationsCat_cache.push_back(c1);
                    } else {
                        if (!iOnlyIssues) {
                            m_5mainVariations_cache.push_back(
                                i18n("Expenses in category <b>'%1'</b> decreased by <b>%2</b> for a total of <b>%3</b>.", c1, vf, a2f));
                            m_5mainVariationsCat_cache.push_back(c1);
                        }
                    }
                } else if (a1 > 0 && a2 > 0) {
                    QString vf = formatPercentage(qAbs(v));
                    if (v > 0) {
                        if (!iOnlyIssues) {
                            m_5mainVariations_cache.push_back(
                                i18n("Incomes in category <b>'%1'</b> increased by <b>%2</b> for a total of <b>%3</b>.", c1, vf, a2f));
                            m_5mainVariationsCat_cache.push_back(c1);
                        }
                    } else {
                        m_5mainVariations_cache.push_back(
                            i18n("Incomes in category <b>'%1'</b> decreased by <b>%2</b> for a total of <b>%3</b>.", c1, vf, a2f));
                        m_5mainVariationsCat_cache.push_back(c1);
                    }
                }
            }
        }
        output = m_5mainVariations_cache;
        if (oCategoryList != nullptr) {
            *oCategoryList = m_5mainVariationsCat_cache;
        }
    }
    return output;
}

SKGReport *SKGDocumentBank::getReport() const
{
    return new SKGReportBank(const_cast<SKGDocumentBank *>(this));
}
