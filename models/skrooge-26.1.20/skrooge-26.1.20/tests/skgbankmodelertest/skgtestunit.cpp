/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test script.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qdesktopservices.h>

#include "skgbankincludes.h"
#include "skgtestmacro.h"
#include <utility>

/**
 * The main function of the unit test
 * @param argc the number of arguments
 * @param argv the list of arguments
 */
int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Init test
    SKGINITTEST(true)

    {
        QStringList sources = SKGUnitObject::downloadSources();
        SKGTESTBOOL("UNIT:downloadSources", (sources.count() >= 4), true)
        for (const QString &source : std::as_const(sources)) {
            SKGUnitObject::getCommentFromSource(source);
        }

        // Test unit et unitvalue
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGTEST(QStringLiteral("UNIT:getNbTransaction"), document1.getNbTransaction(), 0)
        // Scope of the transaction
        SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T1"), err)

        // Create unit
        SKGUnitObject unit1(&document1);
        SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.setName(QStringLiteral("UNIT1")), true)
        SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit1.setSymbol(QStringLiteral("F")), true)
        SKGTEST(QStringLiteral("UNIT:getSymbol"), unit1.getSymbol(), QStringLiteral("F"))
        SKGTESTERROR(QStringLiteral("UNIT:setCountry"), unit1.setCountry(QStringLiteral("France")), true)
        SKGTEST(QStringLiteral("UNIT:getSymbol"), unit1.getCountry(), QStringLiteral("France"))
        SKGTESTERROR(QStringLiteral("UNIT:setInternetCode"), unit1.setInternetCode(QStringLiteral("code Internet")), true)
        SKGTEST(QStringLiteral("UNIT:getInternetCode"), unit1.getInternetCode(), QStringLiteral("code Internet"))
        SKGTESTERROR(QStringLiteral("UNIT:setType"), unit1.setType(SKGUnitObject::OBJECT), true)
        SKGTEST(QStringLiteral("UNIT:getType"), static_cast<unsigned int>(unit1.getType()), static_cast<unsigned int>(SKGUnitObject::OBJECT))
        SKGTESTERROR(QStringLiteral("UNIT:setNumberDecimal"), unit1.setNumberDecimal(4), true)
        SKGTEST(QStringLiteral("UNIT:getNumberDecimal"), unit1.getNumberDecimal(), 4)

        SKGUnitValueObject unitvalue1;
        SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit1.addUnitValue(unitvalue1), false)

        SKGTESTERROR(QStringLiteral("UNIT:save"), unit1.save(), true)

        // Create unit value
        SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit1.addUnitValue(unitvalue1), true)

        SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unitvalue1.setQuantity(119999.11), true)
        SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unitvalue1.setDate(SKGServices::stringToDate(QStringLiteral("1970-07-16"))), true)
        SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unitvalue1.save(), true)

        SKGUnitValueObject unitvalue1bis = SKGUnitValueObject(unitvalue1);
        SKGUnitValueObject unitvalue1ter = SKGUnitValueObject(static_cast<SKGObjectBase>(unitvalue1));
        SKGUnitValueObject unitvalue14(SKGNamedObject(&document1, QStringLiteral("xxx"), unitvalue1.getID()));

        SKGUnitObject unit_get;
        SKGTESTERROR(QStringLiteral("UNITVALUE:getUnit"), unitvalue1.getUnit(unit_get), true)
        SKGTESTBOOL("UNIT:comparison", (unit_get == unit1), true)

        SKGUnitValueObject unitvalue2;
        SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit1.addUnitValue(unitvalue2), true)

        SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unitvalue2.setQuantity(34.12), true)
        SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unitvalue2.setDate(SKGServices::stringToDate(QStringLiteral("1973-02-04"))), true)
        SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unitvalue2.save(), true)

        // Retrieve unit
        SKGNamedObject unit2Named;
        SKGTESTERROR(QStringLiteral("UNIT:getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("unit"), QStringLiteral("UNIT1"), unit2Named),
                     true)
        SKGUnitObject unit2(unit2Named);

        // Retrieve unit value
        SKGUnitValueObject unitvalue3;
        SKGTESTERROR(QStringLiteral("UNIT:getLastUnitValue"), unit2.getLastUnitValue(unitvalue3), true)

        SKGUnitValueObject unitvalue19700101;
        SKGTESTERROR(QStringLiteral("UNIT:getUnitValue"), unit2.getUnitValue(SKGServices::stringToDate(QStringLiteral("1970-01-01")), unitvalue19700101), true)
        SKGUnitValueObject unitvalue19700716;
        SKGTESTERROR(QStringLiteral("UNIT:getUnitValue"), unit2.getUnitValue(SKGServices::stringToDate(QStringLiteral("1970-07-16")), unitvalue19700716), true)
        SKGUnitValueObject unitvalue19720716;
        SKGTESTERROR(QStringLiteral("UNIT:getUnitValue"), unit2.getUnitValue(SKGServices::stringToDate(QStringLiteral("1972-07-16")), unitvalue19720716), true)
        SKGUnitValueObject unitvalue19730204;
        SKGTESTERROR(QStringLiteral("UNIT:getUnitValue"), unit2.getUnitValue(SKGServices::stringToDate(QStringLiteral("1973-02-04")), unitvalue19730204), true)
        SKGUnitValueObject unitvalue19750204;
        SKGTESTERROR(QStringLiteral("UNIT:getUnitValue"), unit2.getUnitValue(SKGServices::stringToDate(QStringLiteral("1975-02-04")), unitvalue19750204), true)

        // Check unit value
        SKGTESTBOOL("UNITVALUE:==", (unitvalue3 == unitvalue2), true)
        SKGTESTBOOL("UNITVALUE:==", (unitvalue19700101 == unitvalue1), true)
        SKGTESTBOOL("UNITVALUE:==", (unitvalue19700716 == unitvalue1), true)
        SKGTESTBOOL("UNITVALUE:==", (unitvalue19720716 == unitvalue1), true)
        SKGTESTBOOL("UNITVALUE:==", (unitvalue19730204 == unitvalue2), true)
        SKGTESTBOOL("UNITVALUE:==", (unitvalue19750204 == unitvalue2), true)

        SKGTEST(QStringLiteral("UNITVALUE:getQuantity"), unitvalue1.getQuantity() - 119999.11, 0)
        SKGTEST(QStringLiteral("UNITVALUE:getQuantity"), unitvalue3.getQuantity() - 34.12, 0)
        SKGTEST(QStringLiteral("UNITVALUE:getDate"), SKGServices::dateToSqlString(unitvalue3.getDate()), QStringLiteral("1973-02-04"))

        // Check cascading delete
        SKGTESTERROR(QStringLiteral("UNIT:delete"), unit2.remove(), true)

        QStringList oResult;
        SKGTESTERROR(QStringLiteral("UNIT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("unit"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("UNIT:oResult.size"), oResult.size(), 0)

        SKGTESTERROR(QStringLiteral("UNITVALUE:getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("unitvalue"), QStringLiteral("rd_unit_id"), oResult),
                     true)
        SKGTEST(QStringLiteral("UNITVALUE:oResult.size"), oResult.size(), 0)

        // Merge
        SKGTESTERROR(QStringLiteral("UNIT:merge"), unit1.merge(unit2), true)
    }

    {
        // Test unit et unitvalue
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGTEST(QStringLiteral("DOC:getPrimaryUnit"), document1.getPrimaryUnit().Symbol, QString())
        SKGTEST(QStringLiteral("DOC:getSecondaryUnit"), document1.getSecondaryUnit().Symbol, QString())
        SKGUnitObject franc(&document1);
        {SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT_1"), err)

         // Create unit
         SKGTESTERROR(QStringLiteral("FRANC:setName"), franc.setName(QStringLiteral("F")), true)
             SKGTESTERROR(QStringLiteral("FRANC:setSymbol"), franc.setSymbol(QStringLiteral("F")), true)
                 SKGTESTERROR(QStringLiteral("FRANC:setType"), franc.setType(SKGUnitObject::PRIMARY), true)
                     SKGTEST(QStringLiteral("FRANC:getType"), static_cast<unsigned int>(franc.getType()), static_cast<unsigned int>(SKGUnitObject::PRIMARY))
                         SKGTESTERROR(QStringLiteral("FRANC:save"), franc.save(), true)

                             SKGTEST(QStringLiteral("DOC:getPrimaryUnit"), document1.getPrimaryUnit().Symbol, QStringLiteral("F"))}

        SKGTEST(QStringLiteral("DOC:getPrimaryUnit"), document1.getPrimaryUnit().Symbol, QStringLiteral("F"))
            SKGTEST(QStringLiteral("DOC:getSecondaryUnit"), document1.getSecondaryUnit().Symbol, QString())
                SKGTEST(QStringLiteral("DOC:getSecondaryUnit"), document1.getSecondaryUnit().Value, 1)

        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT_2"), err)

            SKGUnitObject euro(&document1);
            SKGTESTERROR(QStringLiteral("EURO:setName"), euro.setName(QStringLiteral("E")), true)
            SKGTESTERROR(QStringLiteral("EURO:setSymbol"), euro.setSymbol(QStringLiteral("E")), true)
            SKGTESTERROR(QStringLiteral("EURO:setType"), euro.setType(SKGUnitObject::PRIMARY), true)
            SKGTEST(QStringLiteral("EURO:getType"), static_cast<unsigned int>(euro.getType()), static_cast<unsigned int>(SKGUnitObject::PRIMARY))
            SKGTESTERROR(QStringLiteral("EURO:save"), euro.save(), true)

            SKGTESTERROR(QStringLiteral("FRANC:load"), franc.load(), true)
            SKGTEST(QStringLiteral("FRANC:getType"), static_cast<unsigned int>(franc.getType()), static_cast<unsigned int>(SKGUnitObject::SECONDARY))

            SKGTEST(QStringLiteral("EURO:getType"), static_cast<unsigned int>(euro.getType()), static_cast<unsigned int>(SKGUnitObject::PRIMARY))
            SKGTESTERROR(QStringLiteral("EURO:save"), euro.save(), true)
        }

        SKGTEST(QStringLiteral("DOC:getPrimaryUnit"), document1.getPrimaryUnit().Symbol, QStringLiteral("E"))
        SKGTEST(QStringLiteral("DOC:getSecondaryUnit"), document1.getSecondaryUnit().Symbol, QStringLiteral("F"))
    }

    {
        // Test unit et unitvalue
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            QStringList units = SKGUnitObject::getListofKnownCurrencies(false);
            SKGTEST(QStringLiteral("UNIT:getListofKnownCurrencies"), units.count(), 276)
            units = SKGUnitObject::getListofKnownCurrencies(true);
            SKGTEST(QStringLiteral("UNIT:getListofKnownCurrencies"), units.count(), 316)
            for (const auto &unit : std::as_const(units)) {
                SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT_1"), err)
                SKGUnitObject unitObj;
                SKGTESTERROR(QStringLiteral("UNIT:createCurrencyUnit-") % unit, SKGUnitObject::createCurrencyUnit(&document1, unit, unitObj), true)
                unitObj.getUnitInfo();
            }
            SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT_1"), err)
            SKGUnitObject unitObj;
            SKGTESTERROR(QStringLiteral("UNIT:createCurrencyUnit"), SKGUnitObject::createCurrencyUnit(&document1, QStringLiteral("ZZZZZZZ"), unitObj), false)
        }
    }

    {
        // Compute unit amount at a date
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGUnitObject unit1(&document1);
        SKGUnitObject unit2(&document1);
        {SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT"), err) SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.setName(QStringLiteral("U1")), true)
             SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit1.setSymbol(QStringLiteral("U1")), true)
                 SKGTESTERROR(QStringLiteral("UNIT:save"), unit1.save(), true) SKGTESTERROR(
                     QStringLiteral("UNIT:addOrModifyUnitValue"),
                     document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2001-01-01")), 1),
                     true) SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                                        document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2006-01-01")), 6),
                                        true)
                     SKGTESTERROR(
                         QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2007-01-01")), 7),
                         true) SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                                            document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2008-01-01")), 8),
                                            true)

                         SKGTESTERROR(QStringLiteral("UNIT:setName"), unit2.setName(QStringLiteral("U2")), true) SKGTESTERROR(
                             QStringLiteral("UNIT:setSymbol"),
                             unit2.setSymbol(QStringLiteral("U2")),
                             true) SKGTESTERROR(QStringLiteral("UNIT:removeUnit"), unit2.removeUnit(), true)
                             SKGTESTERROR(QStringLiteral("UNIT:save"), unit2.save(), true)
                                 SKGTESTERROR(QStringLiteral("UNIT:setUnit"), unit2.setUnit(unit2), false) SKGTESTERROR(QStringLiteral("UNIT:setUnit"),
                                                                                                                        unit2.setUnit(unit1),
                                                                                                                        true)
                                     SKGTESTERROR(QStringLiteral("UNIT:save"), unit2.save(), true) SKGTESTERROR(
                                         QStringLiteral("UNIT:addOrModifyUnitValue"),
                                         document1.addOrModifyUnitValue(QStringLiteral("U2"), SKGServices::stringToDate(QStringLiteral("2001-02-01")), 10),
                                         true) SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                                                            document1.addOrModifyUnitValue(QStringLiteral("U2"),
                                                                                           SKGServices::stringToDate(QStringLiteral("2006-01-01")),
                                                                                           60),
                                                            true)
                                         SKGTESTERROR(
                                             QStringLiteral("UNIT:addOrModifyUnitValue"),
                                             document1.addOrModifyUnitValue(QStringLiteral("U2"), SKGServices::stringToDate(QStringLiteral("2007-02-01")), 70),
                                             true) SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                                                                document1.addOrModifyUnitValue(QStringLiteral("U2"),
                                                                                               SKGServices::stringToDate(QStringLiteral("2008-01-01")),
                                                                                               80),
                                                                true)}

        SKGTEST(QStringLiteral("DOC:getAmount"), unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2001-01-01"))), 1) SKGTEST(
            QStringLiteral("DOC:getAmount"),
            unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2001-02-01"))),
            1) SKGTEST(QStringLiteral("DOC:getAmount"),
                       unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2006-01-01"))),
                       6) SKGTEST(QStringLiteral("DOC:getAmount"),
                                  unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2006-02-01"))),
                                  6) SKGTEST(QStringLiteral("DOC:getAmount"), unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2007-01-01"))), 7)
            SKGTEST(QStringLiteral("DOC:getAmount"), unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2007-02-01"))), 7) SKGTEST(
                QStringLiteral("DOC:getAmount"),
                unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2008-01-01"))),
                8) SKGTEST(QStringLiteral("DOC:getAmount"), unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2008-02-01"))), 8)

                SKGTEST(QStringLiteral("DOC:getAmount"), unit2.getAmount(SKGServices::stringToDate(QStringLiteral("2001-01-01"))), 1 * 10) SKGTEST(
                    QStringLiteral("DOC:getAmount"),
                    unit2.getAmount(SKGServices::stringToDate(QStringLiteral("2001-02-01"))),
                    1 * 10) SKGTEST(QStringLiteral("DOC:getAmount"), unit2.getAmount(SKGServices::stringToDate(QStringLiteral("2006-01-01"))), 6 * 60)
                    SKGTEST(QStringLiteral("DOC:getAmount"), unit2.getAmount(SKGServices::stringToDate(QStringLiteral("2006-02-01"))), 6 * 60)
                        SKGTEST(QStringLiteral("DOC:getAmount"), unit2.getAmount(SKGServices::stringToDate(QStringLiteral("2007-01-01"))), 7 * 60)
                            SKGTEST(QStringLiteral("DOC:getAmount"), unit2.getAmount(SKGServices::stringToDate(QStringLiteral("2007-02-01"))), 7 * 70)
                                SKGTEST(QStringLiteral("DOC:getAmount"), unit2.getAmount(SKGServices::stringToDate(QStringLiteral("2008-01-01"))), 8 * 80)
                                    SKGTEST(QStringLiteral("DOC:getAmount"), unit2.getAmount(SKGServices::stringToDate(QStringLiteral("2008-02-01"))), 8 * 80)

                                        SKGTESTERROR(QStringLiteral("UNIT:load"), unit1.load(), true)
                                            SKGTESTERROR(QStringLiteral("UNIT:load"), unit2.load(), true)
                                                SKGTEST(QStringLiteral("DOC:convert"), SKGUnitObject::convert(100, unit1, unit1), 100)
                                                    SKGTEST(QStringLiteral("DOC:convert"), SKGUnitObject::convert(100, unit1, unit2), 1.25)
    }

    {
        // Split unit
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGUnitObject unit1(&document1);
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT"), err)
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.setName(QStringLiteral("U1")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit1.setSymbol(QStringLiteral("U1")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit1.save(), true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2001-01-01")), 100),
                         true)
        }
        {SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT-SPLIT"), err) SKGTESTERROR(QStringLiteral("DOC:getAmount"), unit1.split(-2), false)
             SKGTESTERROR(QStringLiteral("DOC:getAmount"), unit1.split(2), true)}

        SKGTESTERROR(QStringLiteral("UNIT:load"), unit1.load(), true)
            SKGTEST(QStringLiteral("UNIT:getAmount"), unit1.getAmount(SKGServices::stringToDate(QStringLiteral("2001-01-01"))), 50)
    }

    {
        // Unit Daily change
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGUnitObject unit1(&document1);
        {SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT"), err) SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.setName(QStringLiteral("U1")), true)
             SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit1.setSymbol(QStringLiteral("U1")), true)
                 SKGTESTERROR(QStringLiteral("UNIT:save"), unit1.save(), true)
                     SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                                  document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2009-01-01")), 100),
                                  true)
                         SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                                      document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2009-01-02")), 105),
                                      true)
                             SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                                          document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2009-02-01")), 110),
                                          true)}

        SKGTEST(QStringLiteral("UNIT:getDailyChange"),
                SKGServices::intToString(365 * unit1.getDailyChange(SKGServices::stringToDate(QStringLiteral("2009-01-02")))),
                QStringLiteral("1825")) SKGTEST(QStringLiteral("UNIT:getDailyChange"),
                                                SKGServices::intToString(365 * unit1.getDailyChange(SKGServices::stringToDate(QStringLiteral("2009-02-01")))),
                                                QStringLiteral("80"))
    }

    {
        // Unit download with internet code = " " (see https://forum.kde.org/viewtopic.php?f=210&t=163184&p=424630&hilit=skrooge#p424630)
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGUnitObject unit1(&document1);
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT"), err)
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.setName(QStringLiteral("DS")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit1.setSymbol(QStringLiteral("DS")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setInternetCode"), unit1.setInternetCode(QStringLiteral(" ")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit1.save(), true)
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), unit1.downloadUnitValue(SKGUnitObject::ALL_DAILY, 20), true)
        }
    }

    {
        // Unit download
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGUnitObject unit1(&document1);
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT"), err)
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.setName(QStringLiteral("DS")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit1.setSymbol(QStringLiteral("DS")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setDownloadSource"), unit1.setDownloadSource(QStringLiteral("Boursorama")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setInternetCode"), unit1.setInternetCode(QStringLiteral("AIRBUS")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit1.save(), true)
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), unit1.downloadUnitValue(SKGUnitObject::ALL_DAILY, 20), true)

            SKGObjectBase::SKGListSKGObjectBase values;
            SKGTESTERROR(QStringLiteral("UNIT:getUnitValues"), unit1.getUnitValues(values), true)
            SKGTESTBOOL("UNIT:getUnitValues", (!values.isEmpty()), true)

            // All download modes
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), unit1.downloadUnitValue(SKGUnitObject::LAST_WEEKLY, 20), true)
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), unit1.downloadUnitValue(SKGUnitObject::LAST_DAILY, 20), true)
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), unit1.downloadUnitValue(SKGUnitObject::LAST_MONTHLY, 20), true)
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), unit1.downloadUnitValue(SKGUnitObject::ALL_MONTHLY, 20), true)
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), unit1.downloadUnitValue(SKGUnitObject::ALL_WEEKLY, 20), true)

            SKGTESTERROR(QStringLiteral("UNIT:getUnitValues"), unit1.getUnitValues(values), true)
            SKGTESTBOOL("UNIT:getUnitValues", static_cast<unsigned int>(values.count() > 1), static_cast<unsigned int>(true))

            QUrl url;
            unit1.getUrl(url); // For coverage
        }
    }
    {
        // Unit download
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGUnitObject unit1(&document1);
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT"), err)
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.setName(QStringLiteral("FORMULA")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit1.setSymbol(QStringLiteral("FORMULA")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setInternetCode"), unit1.setInternetCode(QStringLiteral("=5")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit1.save(), true)
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), unit1.downloadUnitValue(SKGUnitObject::ALL_DAILY, 20), true)

            SKGObjectBase::SKGListSKGObjectBase values;
            SKGTESTERROR(QStringLiteral("UNIT:getUnitValues"), unit1.getUnitValues(values), true)
            SKGTESTBOOL("UNIT:getUnitValues.isEmpty", values.isEmpty(), false)
        }
    }
    {
        // Unit download
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;

        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT"), err)
            SKGUnitObject euro;
            SKGTESTERROR(QStringLiteral("UNIT:createCurrencyUnit"), SKGUnitObject::createCurrencyUnit(&document1, QStringLiteral("EUR"), euro), true)
            SKGUnitObject fr;
            SKGTESTERROR(QStringLiteral("UNIT:createCurrencyUnit"), SKGUnitObject::createCurrencyUnit(&document1, QStringLiteral("GBP"), fr), true)
            SKGTESTERROR(QStringLiteral("UNIT:downloadUnitValue"), fr.downloadUnitValue(SKGUnitObject::LAST), true)

            SKGObjectBase::SKGListSKGObjectBase values;
            SKGTESTERROR(QStringLiteral("UNIT:getUnitValues"), fr.getUnitValues(values), true)
            SKGTESTBOOL("UNIT:getUnitValues", values.isEmpty(), false)
        }
    }
    {
        // Unit simplify
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGUnitObject unit1(&document1);
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("UNIT"), err)
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.setName(QStringLiteral("U1")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit1.setSymbol(QStringLiteral("U1")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit1.save(), true)

            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addDays(-7), 1),
                         true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addDays(-6), 2),
                         true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addDays(-5), 3),
                         true)

            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addMonths(-3).addDays(-7), 1),
                         true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addMonths(-3).addDays(-6), 2),
                         true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addMonths(-3).addDays(-5), 3),
                         true)

            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addYears(-1).addDays(-7), 1),
                         true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addMonths(-1).addDays(-6), 2),
                         true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), QDate::currentDate().addMonths(-1).addDays(-5), 3),
                         true)

            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2009-01-01")), 1),
                         true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2009-01-02")), 2),
                         true)
            SKGTESTERROR(QStringLiteral("UNIT:addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("U1"), SKGServices::stringToDate(QStringLiteral("2009-01-03")), 3),
                         true)
        }

        {SKGBEGINTRANSACTION(document1, QStringLiteral("SYMPLIFY"), err)
             SKGTESTERROR(QStringLiteral("UNIT:setName"), unit1.simplify(), true)} SKGObjectBase::SKGListSKGObjectBase oUnitValueList;
        SKGTESTERROR(QStringLiteral("UNIT:getUnitValues"), unit1.getUnitValues(oUnitValueList), true)
        SKGTESTBOOL("UNIT:oUnitValueList", (oUnitValueList.count() == 10 || oUnitValueList.count() == 11), true)
    }

    {SKGTESTERROR(QStringLiteral("UNIT:addSource"), SKGUnitObject::addSource(QStringLiteral("test"), false), true)
         SKGTESTBOOL("UNIT:isWritable", SKGUnitObject::isWritable(QStringLiteral("test")), true)
             SKGTESTERROR(QStringLiteral("UNIT:deleteSource"), SKGUnitObject::deleteSource(QStringLiteral("test")), true)
                 SKGTESTERROR(QStringLiteral("UNIT:deleteSource"), SKGUnitObject::deleteSource(QStringLiteral("notfound")), false)} // End test
    SKGENDTEST()
}
