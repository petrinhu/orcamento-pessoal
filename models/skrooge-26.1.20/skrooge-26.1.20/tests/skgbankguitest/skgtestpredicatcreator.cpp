/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGPredicatCreator component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestpredicatcreator.h"
#include "skgdocumentbank.h"
#include "skgpredicatcreator.h"
#include "skgquerycreator.h"

void SKGTESTPredicatCreator::Test()
{
    KLocalizedString::setApplicationDomain("skrooge");

    SKGDocumentBank doc;
    QStringList attributeForQuery;
    attributeForQuery << QStringLiteral("d_date") << QStringLiteral("t_number") << QStringLiteral("t_mode") << QStringLiteral("t_PAYEE")
                      << QStringLiteral("p_prop") << QStringLiteral("t_status");

    SKGPredicatCreator d_col_s(nullptr, &doc, QStringLiteral("d_date"), false, attributeForQuery);
    d_col_s.setXmlDescription(d_col_s.xmlDescription());
    d_col_s.text();

    SKGPredicatCreator d_col_u(nullptr, &doc, QStringLiteral("d_date"), true, attributeForQuery);
    d_col_u.setXmlDescription(d_col_u.xmlDescription());
    d_col_u.text();

    SKGPredicatCreator i_col_s(nullptr, &doc, QStringLiteral("t_number"), false, attributeForQuery);
    i_col_s.setXmlDescription(i_col_s.xmlDescription());
    i_col_s.text();

    SKGPredicatCreator i_col_u(nullptr, &doc, QStringLiteral("t_number"), true, attributeForQuery);
    i_col_u.setXmlDescription(i_col_u.xmlDescription());
    i_col_u.text();

    SKGPredicatCreator t_col_s(nullptr, &doc, QStringLiteral("t_mode"), false, attributeForQuery);
    t_col_s.setXmlDescription(t_col_s.xmlDescription());
    t_col_s.text();

    SKGPredicatCreator t_col_u(nullptr, &doc, QStringLiteral("t_mode"), true, attributeForQuery);
    t_col_u.setXmlDescription(t_col_s.xmlDescription());
    t_col_u.text();

    SKGPredicatCreator t2_col_s(nullptr, &doc, QStringLiteral("t_PAYEE"), false, attributeForQuery);
    t2_col_s.setXmlDescription(t2_col_s.xmlDescription());
    t2_col_s.text();

    SKGPredicatCreator t2_col_u(nullptr, &doc, QStringLiteral("t_PAYEE"), true, attributeForQuery);
    t2_col_u.setXmlDescription(t2_col_u.xmlDescription());
    t2_col_u.text();

    SKGPredicatCreator p_col_s(nullptr, &doc, QStringLiteral("p_prop"), false, attributeForQuery);
    p_col_s.setXmlDescription(p_col_s.xmlDescription());
    p_col_s.text();

    SKGPredicatCreator p_col_u(nullptr, &doc, QStringLiteral("p_prop"), true, attributeForQuery);
    p_col_u.setXmlDescription(p_col_u.xmlDescription());
    p_col_u.text();

    SKGPredicatCreator t3_col_s(nullptr, &doc, QStringLiteral("t_status"), false, attributeForQuery);
    t3_col_s.setXmlDescription(t3_col_s.xmlDescription());
    t3_col_s.text();

    SKGPredicatCreator t3_col_u(nullptr, &doc, QStringLiteral("t_status"), true, attributeForQuery);
    t3_col_u.setXmlDescription(t3_col_u.xmlDescription());
    t3_col_u.text();

    QString xml = QStringLiteral(
        "<!DOCTYPE SKGML>"
        "<element> <!--OR-->"
        "<element>  <!--AND-->"
        "<element attribute=\"d_date\" operator=\"STRFTIME('%Y',#ATT#)=STRFTIME('%Y',date('now', 'localtime', 'localtime'))\" />"
        "</element>"
        "</element>");

    SKGQueryCreator c1(nullptr);
    c1.setParameters(&doc, QStringLiteral("v_suboperation_consolidated"), attributeForQuery, false);
    c1.setXMLCondition(xml);
    c1.clearContents();

    SKGQueryCreator c2(nullptr);
    c2.setParameters(&doc, QStringLiteral("v_suboperation_consolidated"), attributeForQuery, true);
    c2.setXMLCondition(xml);
    c1.getXMLCondition();
}

QTEST_MAIN(SKGTESTPredicatCreator)
