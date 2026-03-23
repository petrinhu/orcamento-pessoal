/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGDocument.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "qregularexpression.h"

#include "skgdocument.h"

SKGError SKGDocument::refreshViewsIndexesAndTriggers(bool iForce) const
{
    Q_UNUSED(iForce)
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    /**
     * This constant is used to initialized the data model (trigger creation)
     */
    QString rd_node_id;
    QStringList attsOfNodes;
    err = getAttributesList(QStringLiteral("node"), attsOfNodes);
    rd_node_id = (attsOfNodes.contains(QStringLiteral("rd_node_id")) ? QStringLiteral("rd_node_id") : QStringLiteral("r_node_id"));

    QStringList InitialDataModelTrigger;
    InitialDataModelTrigger
            << DELETECASCADEPARAMETER("parameters")
            << DELETECASCADEPARAMETER("node")

            // Compute fullname
            << QStringLiteral("DROP TRIGGER IF EXISTS cpt_node_fullname3")
            /*<< "CREATE TRIGGER cpt_node_fullname1 " // This trigger must be the first
            "AFTER UPDATE OF t_fullname ON node BEGIN "
            "UPDATE node SET t_name=t_name WHERE rd_node_id=new.id;"
            "END"*/

            << QStringLiteral("DROP TRIGGER IF EXISTS cpt_node_fullname1")
            << "CREATE TRIGGER cpt_node_fullname1 "
            "AFTER INSERT ON node BEGIN "
            "UPDATE node SET t_fullname="
            "CASE WHEN new.rd_node_id IS NULL OR new." % rd_node_id % "='' OR new." % rd_node_id % "=0 THEN new.t_name ELSE (SELECT c.t_fullname from node c where c.id=new." % rd_node_id % ")||'" % OBJECTSEPARATOR % "'||new.t_name END "
            "WHERE id=new.id;"
            "END"

            << QStringLiteral("DROP TRIGGER IF EXISTS cpt_node_fullname2")
            << "CREATE TRIGGER cpt_node_fullname2 "
            "AFTER UPDATE OF t_name, " % rd_node_id % " ON node BEGIN "
            "UPDATE node SET t_fullname="
            "CASE WHEN new." % rd_node_id % " IS NULL OR new." % rd_node_id % "='' OR new." % rd_node_id % "=0 THEN new.t_name ELSE (SELECT c.t_fullname from node c where c.id=new." % rd_node_id % ")||'" % OBJECTSEPARATOR % "'||new.t_name END "
            "WHERE id=new.id;"
            "UPDATE node SET t_name=t_name WHERE " % rd_node_id % "=new.id;"
            "END"

            << QStringLiteral("DROP TRIGGER IF EXISTS fkdc_node_parent_id_node_id");

    /**
     * This constant is used to initialized the data model (index creation)
     */
    QStringList InitialDataModelIndex;
    InitialDataModelIndex << QStringLiteral("CREATE UNIQUE INDEX uidx_parameters_uuid_parent_name ON parameters (t_uuid_parent, t_name)")

                          << "CREATE UNIQUE INDEX uidx_node_parent_id_name ON node(t_name," % rd_node_id % ")"
                          << QStringLiteral("CREATE INDEX idx_node_fullname ON node(t_fullname)")

                          << QStringLiteral("CREATE INDEX idx_doctransaction_parent ON doctransaction (i_parent)")
                          << QStringLiteral("CREATE INDEX idx_doctransactionitem_i_object_id ON doctransactionitem (i_object_id)")
                          << QStringLiteral("CREATE INDEX idx_doctransactionitem_t_object_table ON doctransactionitem (t_object_table)")
                          << QStringLiteral("CREATE INDEX idx_doctransactionitem_t_action ON doctransactionitem (t_action)")
                          << QStringLiteral("CREATE INDEX idx_doctransactionitem_rd_doctransaction_id ON doctransactionitem (rd_doctransaction_id)")
                          << QStringLiteral(
                                 "CREATE INDEX idx_doctransactionitem_optimization ON doctransactionitem (rd_doctransaction_id, i_object_id, t_object_table, "
                                 "t_action, t_sqlorder, id)");

    /**
     * This constant is used to initialized the data model (view creation)
     */
    QStringList InitialDataModelView;
    InitialDataModelView << QStringLiteral("CREATE VIEW  v_node AS SELECT * from node")
                         << QStringLiteral("CREATE VIEW v_node_displayname AS SELECT *, t_fullname AS t_displayname from node")
                         << QStringLiteral("CREATE VIEW v_parameters_displayname AS SELECT *, t_name AS t_displayname from parameters")
                         << QStringLiteral("CREATE VIEW v_doctransaction_displayname AS SELECT *, t_name AS t_displayname from doctransaction");

    IFOKDO(err,
           dropViewsAndIndexes(QStringList() << QStringLiteral("node") << QStringLiteral("parameters") << QStringLiteral("doctransactionitem")
                                             << QStringLiteral("doctransaction")))
    IFOKDO(err, executeSqliteOrders(InitialDataModelIndex))
    IFOKDO(err, executeSqliteOrders(InitialDataModelView))
    IFOKDO(err, executeSqliteOrders(InitialDataModelTrigger))
    IFOK(err)
    {
        // Refresh dynamic triggers
        static QRegularExpression rx_rd(QStringLiteral("rd_([^_]+)_([^_]+).*"));
        static QRegularExpression rx_rc(QStringLiteral("rc_([^_]+)_([^_]+).*"));
        static QRegularExpression rx_r(QStringLiteral("r_([^_]+)_([^_]+).*"));
        static QStringList dbTables;
        err = this->getDistinctValues(QStringLiteral("sqlite_master"), QStringLiteral("name"), QStringLiteral("type='table'"), dbTables);
        int nb = dbTables.count();
        for (int i = 0; !err && i < nb; ++i) {
            const QString &table = dbTables.at(i);
            SKGStringListList attributes;
            err = executeSelectSqliteOrder("PRAGMA table_info(" % table % ");", attributes);
            int nb2 = attributes.count();
            for (int j = 1; !err && j < nb2; ++j) { // Header is ignored
                auto att = attributes.at(j).at(1);
                auto rx_rd_match = rx_rd.match(att);
                if (rx_rd_match.hasMatch()) {
                    // Get parameters
                    QString tab2 = rx_rd_match.captured(1);
                    QString att2 = rx_rd_match.captured(2);
                    QStringList sqlOrders;
                    sqlOrders << FOREIGNCONSTRAINTCASCADE(tab2, att2, table, att);
                    err = executeSqliteOrders(sqlOrders);
                } else {
                    auto rx_rc_match = rx_rc.match(att);
                    if (rx_rc_match.hasMatch()) {
                        // Get parameters
                        QString tab2 = rx_rc_match.captured(1);
                        QString att2 = rx_rc_match.captured(2);
                        QStringList sqlOrders;
                        sqlOrders << FOREIGNCONSTRAINT(tab2, att2, table, att);
                        err = executeSqliteOrders(sqlOrders);
                    } else {
                        auto rx_r_match = rx_r.match(att);
                        if (rx_r_match.hasMatch()) {
                            // Get parameters
                            QString tab2 = rx_r_match.captured(1);
                            QString att2 = rx_r_match.captured(2);
                            QStringList sqlOrders;
                            sqlOrders << FOREIGNCONSTRAINTUPDATE(tab2, att2, table, att);
                            err = executeSqliteOrders(sqlOrders);
                            // The following sql order error is not caught because this repair order is not mandatory
                            executeSqliteOrder("UPDATE " % table % " SET " % att % "=0 WHERE " % att % "!=0 AND " % att % " NOT IN (SELECT DISTINCT(" % att2
                                               % ") FROM " % tab2 % u')');
                        }
                    }
                }
            }
        }
    }
    IFOKDO(err, executeSqliteOrder(QStringLiteral("ANALYZE")))

    return err;
}
