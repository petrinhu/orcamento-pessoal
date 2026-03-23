/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGUnitObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgunitobject.h"

#include <qdesktopservices.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qglobal.h>
#include <qmath.h>
#include <qprocess.h>
#include <qregularexpression.h>
#include <qsavefile.h>
#include <qstandardpaths.h>
#include <qvariant.h>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocalizedstring.h>
#include <kpluginmetadata.h>

#include "skgdocumentbank.h"
#include "skgoperationobject.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include "skgunitvalueobject.h"
#include <utility>

SKGUnitObject::SKGUnitObject()
    : SKGUnitObject(nullptr)
{
}

SKGUnitObject::SKGUnitObject(SKGDocument *iDocument, int iID)
    : SKGNamedObject(iDocument, QStringLiteral("v_unit"), iID)
{
}

SKGUnitObject::~SKGUnitObject() = default;

SKGUnitObject::SKGUnitObject(const SKGUnitObject &iObject) = default;

SKGUnitObject::SKGUnitObject(const SKGNamedObject &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("unit")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_unit"), iObject.getID());
    }
}

SKGUnitObject::SKGUnitObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("unit")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_unit"), iObject.getID());
    }
}

SKGUnitObject &SKGUnitObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGUnitObject &SKGUnitObject::operator=(const SKGUnitObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

QString SKGUnitObject::getWhereclauseId() const
{
    QString output = SKGObjectBase::getWhereclauseId(); // clazy:exclude=skipped-base-method
    if (output.isEmpty()) {
        QString name = getName();
        if (!name.isEmpty()) {
            output = "t_name='" % SKGServices::stringToSqlString(name) % u'\'';
        }

        QString symbol = getSymbol();
        if (!symbol.isEmpty()) {
            if (!output.isEmpty()) {
                output += QStringLiteral(" OR ");
            }
            output += "t_symbol='" % SKGServices::stringToSqlString(symbol) % u'\'';
        }
        if (!output.isEmpty()) {
            output = u'(' % output % u')';
        }
    }

    return output;
}

QList<SKGServices::SKGUnitInfo> SKGUnitObject::currencies;

QStringList SKGUnitObject::getListofKnownCurrencies(bool iIncludingObsolete)
{
    SKGTRACEINFUNC(10)
    if (currencies.isEmpty()) {
        // Search currencies
        const QStringList dirs =
            QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/currency/"), QStandardPaths::LocateDirectory);
        for (const auto &dir : dirs) {
            auto listDesktopFiles = QDir(dir).entryList(QStringList() << QStringLiteral("*.desktop"));
            for (const auto &path : std::as_const(listDesktopFiles)) {
                // Read the file
                QFileInfo file(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/currency/") + path));
                KConfig cgFile(file.absoluteFilePath());
                KConfigGroup cg(&cgFile, QStringLiteral("Currency Code"));

                SKGServices::SKGUnitInfo unit;
                unit.Name = cg.readEntry(QStringLiteral("Name"), QString()) + QStringLiteral(" (")
                    + cg.readEntry(QStringLiteral("CurrencyCodeIsoAlpha3"), QString()) + QStringLiteral(")");
                unit.Symbol = cg.readEntry(QStringLiteral("CurrencyUnitSymbolUnambiguous"), QString());
                if (unit.Symbol.isEmpty()) {
                    unit.Symbol = cg.readEntry(QStringLiteral("CurrencyUnitSymbolDefault"), file.baseName());
                }
                if (unit.Symbol.isEmpty()) {
                    unit.Symbol = cg.readEntry(QStringLiteral("CurrencyCodeIsoAlpha3"), file.baseName());
                }
                unit.Value = 1;
                unit.NbDecimal = cg.readEntry(QStringLiteral("CurrencyDecimalPlacesDisplay"), 2);
                unit.Source = QStringLiteral("GrandTrunk");
                unit.Date = cg.readEntry(QStringLiteral("CurrencyIntroducedDate"), QDate::currentDate());
                unit.Obsolete =
                    (cg.readEntry(QStringLiteral("CurrencySuspendedDate"), cg.readEntry(QStringLiteral("CurrencyWithdrawnDate"), QDate())) != QDate());
                currencies.push_back(unit);
            }
        }

        // Add other units
        {
            // CAC40
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "CAC 40");
            info.Symbol = info.Name;
            info.Country = i18nc("Noun, a country", "France");
            info.Date = QDate(1987, 1, 1);
            info.Internet = QStringLiteral("^FCHI");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // NASDAQ
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "NASDAQ");
            info.Symbol = info.Name;
            info.Country = i18nc("Noun, a country", "United States");
            info.Date = QDate(1971, 2, 5);
            info.Internet = QStringLiteral("^IXIC");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // Dow Jones
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "Dow Jones (DJIA)");
            info.Symbol = QStringLiteral("DJIA");
            info.Country = i18nc("Noun, a country", "United States");
            info.Date = QDate(1884, 1, 1);
            info.Internet = QStringLiteral("^DJI");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // SBF 120
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "SBF 120");
            info.Symbol = info.Name;
            info.Country = i18nc("Noun, a country", "France");
            info.Date = QDate(1990, 12, 31);
            info.Internet = QStringLiteral("^SBF120");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // S&P 500
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "S&P 500");
            info.Symbol = info.Name;
            info.Country = i18nc("Noun, a country", "United States");
            info.Date = QDate(1920, 1, 1);
            info.Internet = QStringLiteral("^GSPC");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // FTSE 100
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "FTSE 100");
            info.Symbol = info.Name;
            info.Country = i18nc("Noun, a country", "England");
            info.Date = QDate(1984, 1, 3);
            info.Internet = QStringLiteral("^FTSE");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DAX
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "DAX");
            info.Symbol = info.Name;
            info.Country = i18nc("Noun, a country", "Germany");
            info.Date = QDate(1920, 1, 1);
            info.Internet = QStringLiteral("^GDAXI");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // NIKKEI 225
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "NIKKEI 225");
            info.Symbol = info.Name;
            info.Country = i18nc("Noun, a country", "Japan");
            info.Date = QDate(1920, 1, 1);
            info.Internet = QStringLiteral("^N225");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // HANG SENG
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "HANG SENG");
            info.Symbol = info.Name;
            info.Country = i18nc("Noun, a country", "China");
            info.Date = QDate(1920, 1, 1);
            info.Internet = QStringLiteral("^HSI");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // STRAITS TIMES
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "STRAITS TIMES");
            info.Symbol = info.Name;
            info.Date = QDate(1920, 1, 1);
            info.Country = i18nc("Noun, a country", "Singapore");
            info.Internet = QStringLiteral("^STI");
            info.Source = QStringLiteral("Yahoo");
            info.NbDecimal = 2;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BITCOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a currency", "Bitcoin");
            info.Symbol = QStringLiteral("BTC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BTC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ETHEREUM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Ethereum");
            info.Symbol = QStringLiteral("ETH");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ETH");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // RIPPLE
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Ripple");
            info.Symbol = QStringLiteral("XRP");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XRP");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BITCOIN-CASH
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Bitcoin Cash");
            info.Symbol = QStringLiteral("BCH");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BCH");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // CARDANO
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Cardano");
            info.Symbol = QStringLiteral("ADA");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ADA");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // NEM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "NEM");
            info.Symbol = QStringLiteral("XEM");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XEM");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // LITECOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Litecoin");
            info.Symbol = QStringLiteral("LTC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("LTC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // STELLAR
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Stellar");
            info.Symbol = QStringLiteral("XLM");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XLM");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // IOTA
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "IOTA");
            info.Symbol = QStringLiteral("MIOTA");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("MIOTA");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // TRON
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "TRON");
            info.Symbol = QStringLiteral("TRX");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("TRX");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DASH
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Dash");
            info.Symbol = QStringLiteral("DASH");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DASH");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // NEO
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "NEO");
            info.Symbol = QStringLiteral("NEO");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("NEO");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // MONERO
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Monero");
            info.Symbol = QStringLiteral("XMR");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XMR");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // EOS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "EOS");
            info.Symbol = QStringLiteral("EOS");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("EOS");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // QTUM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Qtum");
            info.Symbol = QStringLiteral("QTUM");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("QTUM");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ICON
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "ICON");
            info.Symbol = QStringLiteral("ICX");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ICX");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BITCOIN-GOLD
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Bitcoin Gold");
            info.Symbol = QStringLiteral("BTG");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BTG");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // LISK
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Lisk");
            info.Symbol = QStringLiteral("LSK");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("LSK");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // RAIBLOCKS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "RaiBlocks");
            info.Symbol = QStringLiteral("XRB");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XRB");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ETHEREUM-CLASSIC
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Ethereum Classic");
            info.Symbol = QStringLiteral("ETC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ETC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // VERGE
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Verge");
            info.Symbol = QStringLiteral("XVG");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XVG");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // SIACOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Siacoin");
            info.Symbol = QStringLiteral("SC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("SC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // OMISEGO
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "OmiseGO");
            info.Symbol = QStringLiteral("OMG");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("OMG");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BYTECOIN-BCN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Bytecoin");
            info.Symbol = QStringLiteral("BCN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BCN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BITCONNECT
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "BitConnect");
            info.Symbol = QStringLiteral("BCC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BCC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // POPULOUS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Populous");
            info.Symbol = QStringLiteral("PPT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("PPT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // STRATIS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Stratis");
            info.Symbol = QStringLiteral("STRAT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("STRAT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ZCASH
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Zcash");
            info.Symbol = QStringLiteral("ZEC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ZEC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DENTACOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Dentacoin");
            info.Symbol = QStringLiteral("DCN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DCN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BITSHARES
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "BitShares");
            info.Symbol = QStringLiteral("BTS");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BTS");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BINANCE-COIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Binance Coin");
            info.Symbol = QStringLiteral("BNB");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BNB");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DOGECOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Dogecoin");
            info.Symbol = QStringLiteral("DOGE");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DOGE");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // STATUS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Status");
            info.Symbol = QStringLiteral("SNT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("SNT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ARDOR
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Ardor");
            info.Symbol = QStringLiteral("ARDR");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ARDR");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // KUCOIN-SHARES
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "KuCoin Shares");
            info.Symbol = QStringLiteral("KCS");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("KCS");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // TETHER
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Tether");
            info.Symbol = QStringLiteral("USDT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("USDT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // STEEM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Steem");
            info.Symbol = QStringLiteral("STEEM");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("STEEM");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // WAVES
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Waves");
            info.Symbol = QStringLiteral("WAVES");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("WAVES");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // VECHAIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "VeChain");
            info.Symbol = QStringLiteral("VEN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("VEN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DIGIBYTE
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "DigiByte");
            info.Symbol = QStringLiteral("DGB");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DGB");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // KOMODO
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Komodo");
            info.Symbol = QStringLiteral("KMD");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("KMD");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DRAGONCHAIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Dragonchain");
            info.Symbol = QStringLiteral("DRGN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DRGN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // AUGUR
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Augur");
            info.Symbol = QStringLiteral("REP");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("REP");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // GOLEM-NETWORK-TOKENS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Golem");
            info.Symbol = QStringLiteral("GNT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("GNT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // VERITASEUM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Veritaseum");
            info.Symbol = QStringLiteral("VERI");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("VERI");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // HSHARE
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Hshare");
            info.Symbol = QStringLiteral("HSR");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("HSR");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // KIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Kin");
            info.Symbol = QStringLiteral("KIN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("KIN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // SALT
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "SALT");
            info.Symbol = QStringLiteral("SALT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("SALT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ELECTRONEUM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Electroneum");
            info.Symbol = QStringLiteral("ETN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ETN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ARK
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Ark");
            info.Symbol = QStringLiteral("ARK");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ARK");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DENT
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Dent");
            info.Symbol = QStringLiteral("DENT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DENT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ETHOS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Ethos");
            info.Symbol = QStringLiteral("ETHOS");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ETHOS");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BASIC-ATTENTION-TOKEN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Basic Attention Token");
            info.Symbol = QStringLiteral("BAT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BAT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // REDDCOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "ReddCoin");
            info.Symbol = QStringLiteral("RDD");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("RDD");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // 0X
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "0x");
            info.Symbol = QStringLiteral("ZRX");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ZRX");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DECRED
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Decred");
            info.Symbol = QStringLiteral("DCR");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DCR");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // NEXUS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Nexus");
            info.Symbol = QStringLiteral("NXS");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("NXS");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // EXPERIENCE-POINTS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Experience Points");
            info.Symbol = QStringLiteral("XP");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XP");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // QASH
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "QASH");
            info.Symbol = QStringLiteral("QASH");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("QASH");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // KYBER-NETWORK
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Kyber Network");
            info.Symbol = QStringLiteral("KNC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("KNC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // PIVX
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "PIVX");
            info.Symbol = QStringLiteral("PIVX");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("PIVX");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // FUNFAIR
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "FunFair");
            info.Symbol = QStringLiteral("FUN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("FUN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // FACTOM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Factom");
            info.Symbol = QStringLiteral("FCT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("FCT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // NEBLIO
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Neblio");
            info.Symbol = QStringLiteral("NEBL");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("NEBL");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // REQUEST-NETWORK
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Request Network");
            info.Symbol = QStringLiteral("REQ");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("REQ");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // AETERNITY
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Aeternity");
            info.Symbol = QStringLiteral("AE");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("AE");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // SUBSTRATUM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Substratum");
            info.Symbol = QStringLiteral("SUB");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("SUB");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // POWER-LEDGER
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Power Ledger");
            info.Symbol = QStringLiteral("POWR");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("POWR");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // WAX
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "WAX");
            info.Symbol = QStringLiteral("WAX");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("WAX");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // AELF
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "aelf");
            info.Symbol = QStringLiteral("ELF");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ELF");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BYTOM
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Bytom");
            info.Symbol = QStringLiteral("BTM");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BTM");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // AION
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Aion");
            info.Symbol = QStringLiteral("AION");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("AION");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // RCHAIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "RChain");
            info.Symbol = QStringLiteral("RHOC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("RHOC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DIGITALNOTE
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "DigitalNote");
            info.Symbol = QStringLiteral("XDN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XDN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ENIGMA-PROJECT
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Enigma");
            info.Symbol = QStringLiteral("ENG");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ENG");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // NXT
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Nxt");
            info.Symbol = QStringLiteral("NXT");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("NXT");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // TIME-NEW-BANK
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Time New Bank");
            info.Symbol = QStringLiteral("TNB");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("TNB");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BITCOINDARK
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "BitcoinDark");
            info.Symbol = QStringLiteral("BTCD");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("BTCD");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // MONACOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "MonaCoin");
            info.Symbol = QStringLiteral("MONA");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("MONA");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // QUANTSTAMP
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Quantstamp");
            info.Symbol = QStringLiteral("QSP");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("QSP");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // MAIDSAFECOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "MaidSafeCoin");
            info.Symbol = QStringLiteral("MAID");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("MAID");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BYTEBALL
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Byteball Bytes");
            info.Symbol = QStringLiteral("GBYTE");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("GBYTE");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // GAS
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Gas");
            info.Symbol = QStringLiteral("GAS");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("GAS");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // CHAINLINK
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "ChainLink");
            info.Symbol = QStringLiteral("LINK");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("LINK");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // SYSCOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Syscoin");
            info.Symbol = QStringLiteral("SYS");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("SYS");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // SANTIMENT
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Santiment Network Token");
            info.Symbol = QStringLiteral("SAN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("SAN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // COBINHOOD
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Cobinhood");
            info.Symbol = QStringLiteral("COB");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("COB");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // RED-PULSE
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Red Pulse");
            info.Symbol = QStringLiteral("RPX");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("RPX");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DIGIXDAO
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "DigixDAO");
            info.Symbol = QStringLiteral("DGD");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DGD");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // TENX
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "TenX");
            info.Symbol = QStringLiteral("PAY");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("PAY");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ICONOMI
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Iconomi");
            info.Symbol = QStringLiteral("ICN");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("ICN");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // POET
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Po.et");
            info.Symbol = QStringLiteral("POE");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("POE");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ZCOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "ZCoin");
            info.Symbol = QStringLiteral("XZC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("XZC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // GNOSIS-GNO
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Gnosis");
            info.Symbol = QStringLiteral("GNO");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("GNO");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // BLOCKV
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "BLOCKv");
            info.Symbol = QStringLiteral("VEE");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("VEE");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // WALTON
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Walton");
            info.Symbol = QStringLiteral("WTC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("WTC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // PACCOIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "PACcoin");
            info.Symbol = QStringLiteral("PAC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("PAC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // DEEPBRAIN-CHAIN
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "DeepBrain Chain");
            info.Symbol = QStringLiteral("DBC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("DBC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // ETHLEND
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "ETHLend");
            info.Symbol = QStringLiteral("LEND");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("LEND");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
        {
            // CIVIC
            SKGServices::SKGUnitInfo info;
            info.Name = i18nc("Noun, a cryptocurrency", "Civic");
            info.Symbol = QStringLiteral("CVC");
            info.Date = QDate(2009, 2, 4);
            info.Country = i18nc("Noun, the country of bitcoin", "Internet");
            info.Internet = QStringLiteral("CVC");
            info.Source = QStringLiteral("CoinMarketCap");
            info.Parent = QStringLiteral("USD");
            info.NbDecimal = 4;
            info.Value = -1;
            currencies.push_back(info);
        }
    }
    QStringList output;
    output.reserve(currencies.count());
    for (const auto &unit : std::as_const(currencies)) {
        if (iIncludingObsolete || !unit.Obsolete) {
            output.push_back(unit.Name);
        }
    }
    output.sort();
    return output;
}

QString SKGUnitObject::getInternationalCode(const QString &iUnitName)
{
    SKGTRACEINFUNC(10)
    QString output = iUnitName;
    static QRegularExpression reg(QStringLiteral(".*\\(([^\\(\\)]+)\\)[^\\(\\)]*"));
    auto match = reg.match(iUnitName);
    if (match.hasMatch()) {
        output = match.captured(1);
    }

    return output;
}

SKGServices::SKGUnitInfo SKGUnitObject::getUnitInfo()
{
    SKGTRACEINFUNC(10)
    SKGServices::SKGUnitInfo info;
    info.Name = getName();
    info.Value = getAmount();
    info.NbDecimal = getNumberDecimal();
    info.Symbol = getSymbol();
    info.Country = getCountry();
    info.Internet = getInternetCode();
    info.Date = QDate::currentDate();

    return info;
}

SKGServices::SKGUnitInfo SKGUnitObject::getUnitInfo(const QString &iUnitName)
{
    SKGTRACEINFUNC(10)
    SKGServices::SKGUnitInfo info;
    if (currencies.isEmpty()) {
        getListofKnownCurrencies(false);
    }
    QString isoCode = getInternationalCode(iUnitName);
    for (const auto &unit : std::as_const(currencies)) {
        if (getInternationalCode(unit.Name) == isoCode) {
            info = unit;
            break;
        }
    }
    return info;
}

SKGError SKGUnitObject::createCurrencyUnit(SKGDocumentBank *iDocument, const QString &iUnitName, SKGUnitObject &oUnit)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (iDocument != nullptr) {
        SKGUnitObject parentUnit;
        oUnit = SKGUnitObject(iDocument);

        SKGUnitObject::UnitType type = SKGUnitObject::CURRENCY;
        SKGServices::SKGUnitInfo prim = iDocument->getPrimaryUnit();
        SKGServices::SKGUnitInfo seco = iDocument->getSecondaryUnit();

        // Get information on the unit
        SKGServices::SKGUnitInfo info = getUnitInfo(iUnitName);
        if (info.Name.isEmpty()) {
            err = SKGError(ERR_INVALIDARG, i18nc("Error message", "Unknown unit '%1'", iUnitName));
        }
        if (!err && !info.Parent.isEmpty()) {
            err = createCurrencyUnit(iDocument, info.Parent, parentUnit);
        }

        // Set the type
        if (info.Name == info.Symbol) {
            // This is an index
            type = SKGUnitObject::INDEX;
        } else if (!info.Parent.isEmpty()) {
            // This is a secondary unit
            type = (seco.Symbol.isEmpty() || seco.Symbol == info.Symbol ? SKGUnitObject::SECONDARY : SKGUnitObject::CURRENCY);
        } else {
            // As primary
            type = (prim.Symbol.isEmpty() || prim.Symbol == info.Symbol ? SKGUnitObject::PRIMARY : SKGUnitObject::CURRENCY);
        }

        // Point on primary unit
        if (info.Value == 1 && !err && (type == SKGUnitObject::CURRENCY || type == SKGUnitObject::SECONDARY)) {
            SKGUnitObject primunit(iDocument);
            err = primunit.setSymbol(prim.Symbol);
            IFOKDO(err, primunit.load())
            IFOK(err)
            {
                QString codeprimunit = getInternationalCode(primunit.getName());
                QString codeunit = getInternationalCode(info.Name);
                if (!codeprimunit.isEmpty()) {
                    info.Internet = codeunit % u'/' % codeprimunit;
                    info.Value = -1;

                    parentUnit = SKGUnitObject(iDocument);
                    err = parentUnit.setSymbol(prim.Symbol);
                    IFOKDO(err, parentUnit.load())
                }
            }
        }

        IFOKDO(err, oUnit.setName(info.Name))
        if (!err && oUnit.exist()) {
            err = oUnit.load();
        }
        IFOKDO(err, oUnit.setType(type))
        IFOKDO(err, oUnit.setSymbol(info.Symbol))
        IFOKDO(err, oUnit.setInternetCode(info.Internet))
        IFOKDO(err, oUnit.setDownloadSource(info.Source))
        IFOKDO(err, oUnit.setCountry(info.Country))
        IFOKDO(err, oUnit.setNumberDecimal(info.NbDecimal))
        if (!err && parentUnit.exist()) {
            err = oUnit.setUnit(parentUnit);
        }
        IFOKDO(err, oUnit.save())

        // Creation of the value
        if (info.Value > 0) {
            SKGUnitValueObject unitValue;
            IFOKDO(err, oUnit.addUnitValue(unitValue))
            IFOKDO(err, unitValue.setDate(info.Date))
            IFOKDO(err, unitValue.setQuantity(info.Value))
            IFOKDO(err, unitValue.save())
        }
    }
    return err;
}

double SKGUnitObject::convert(double iValue, const SKGUnitObject &iUnitFrom, const SKGUnitObject &iUnitTo, QDate iDate)
{
    double output = iValue;
    if (iUnitFrom != iUnitTo) {
        double valFrom = iUnitFrom.getAmount(iDate);
        double valTo = iUnitTo.getAmount(iDate);
        if (valTo != 0)
            output = iValue * valFrom / valTo;
    }
    return output;
}

SKGError SKGUnitObject::setSymbol(const QString &iSymbol)
{
    return setAttribute(QStringLiteral("t_symbol"), iSymbol);
}

QString SKGUnitObject::getSymbol() const
{
    return getAttribute(QStringLiteral("t_symbol"));
}

QString SKGUnitObject::getDownloadSource() const
{
    return getAttribute(QStringLiteral("t_source"));
}

SKGError SKGUnitObject::setDownloadSource(const QString &iSource)
{
    return setAttribute(QStringLiteral("t_source"), iSource);
}

SKGError SKGUnitObject::setNumberDecimal(int iNb)
{
    return setAttribute(QStringLiteral("i_nbdecimal"), SKGServices::intToString(iNb));
}

int SKGUnitObject::getNumberDecimal() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_nbdecimal")));
}

SKGError SKGUnitObject::setCountry(const QString &iCountry)
{
    return setAttribute(QStringLiteral("t_country"), iCountry);
}

QString SKGUnitObject::getCountry() const
{
    return getAttribute(QStringLiteral("t_country"));
}

SKGError SKGUnitObject::setInternetCode(const QString &iCode)
{
    return setAttribute(QStringLiteral("t_internet_code"), iCode);
}

QString SKGUnitObject::getInternetCode() const
{
    return getAttribute(QStringLiteral("t_internet_code"));
}

SKGError SKGUnitObject::setType(SKGUnitObject::UnitType iType)
{
    SKGError err;
    if (getAttribute(QStringLiteral("t_type")).isEmpty() || this->getType() != iType) {
        // Guaranty that PRIMARY and SECONDARY is unique
        if (iType == PRIMARY || iType == SECONDARY) {
            // Set old SECONDARY as CURRENCY
            err = getDocument()->executeSqliteOrder(QStringLiteral("UPDATE unit SET t_type='C' WHERE t_type='2'"));

            // Set old PRIMARY as SECONDARY
            if (!err && iType == PRIMARY) {
                err = getDocument()->executeSqliteOrder(QStringLiteral("UPDATE unit SET t_type='2' WHERE t_type='1'"));
            }
        }
    }
    IFOKDO(err,
           setAttribute(QStringLiteral("t_type"),
                        (iType == CURRENCY
                             ? QStringLiteral("C")
                             : (iType == PRIMARY ? QStringLiteral("1")
                                                 : (iType == SECONDARY ? QStringLiteral("2")
                                                                       : (iType == SHARE ? QStringLiteral("S")
                                                                                         : (iType == INDEX ? QStringLiteral("I") : QStringLiteral("O"))))))))
    return err;
}

SKGUnitObject::UnitType SKGUnitObject::getType() const
{
    QString typeString = getAttribute(QStringLiteral("t_type"));
    return (typeString == QStringLiteral("C")
                ? CURRENCY
                : (typeString == QStringLiteral("S")
                       ? SHARE
                       : (typeString == QStringLiteral("1")
                              ? PRIMARY
                              : (typeString == QStringLiteral("2") ? SECONDARY : (typeString == QStringLiteral("I") ? INDEX : OBJECT)))));
}

SKGError SKGUnitObject::getUnit(SKGUnitObject &oUnit) const
{
    SKGError err;
    if (getDocument() != nullptr) {
        err = getDocument()->getObject(QStringLiteral("v_unit"), "id=" % getAttribute(QStringLiteral("rd_unit_id")), oUnit);
    }
    return err;
}

SKGError SKGUnitObject::setUnit(const SKGUnitObject &iUnit)
{
    SKGError err;
    if (*this == iUnit && iUnit.getID() != 0) {
        err = SKGError(ERR_INVALIDARG, i18nc("Error message", "Reference unit of a unit cannot be itself."));
    } else {
        err = setAttribute(QStringLiteral("rd_unit_id"), SKGServices::intToString(iUnit.getID()));
    }
    return err;
}

SKGError SKGUnitObject::removeUnit()
{
    return setAttribute(QStringLiteral("rd_unit_id"), QStringLiteral("0"));
}

SKGError SKGUnitObject::addUnitValue(SKGUnitValueObject &oUnitValue)
{
    SKGError err;
    if (getID() == 0) {
        err = SKGError(
            ERR_FAIL,
            i18nc("Error message", "%1 failed because linked object is not yet saved in the database.", QStringLiteral("SKGUnitObject::addUnitValue")));
    } else {
        oUnitValue = SKGUnitValueObject(qobject_cast<SKGDocumentBank *>(getDocument()));
        err = oUnitValue.setAttribute(QStringLiteral("rd_unit_id"), SKGServices::intToString(getID()));
    }
    return err;
}

SKGError SKGUnitObject::simplify()
{
    SKGListSKGObjectBase values;
    SKGError err = getUnitValues(values);
    int nb = values.count();
    if (!err && (nb != 0)) {
        QHash<QString, SKGListSKGObjectBase> groups;

        SKGBEGINPROGRESSTRANSACTION(*getDocument(), "#INTERNAL#" % i18nc("Progression step", "Simplify unit"), err, 2)
        // Build groups
        {
            SKGBEGINPROGRESSTRANSACTION(*getDocument(), "#INTERNAL#" % i18nc("Progression step", "Analyze unit"), err, nb)
            QDate limit1 = QDate::currentDate().addMonths(-3);
            QDate limit2 = QDate::currentDate().addYears(-1);
            QDate limit3 = QDate::currentDate().addYears(-3);
            for (int i = nb - 1; !err && i >= 0; --i) {
                SKGUnitValueObject v(values.at(i));
                QDate date = v.getDate();
                if (date >= limit1) {
                    // No simplification ==> nothing to do
                } else if (date >= limit2) {
                    // Simplification by group of 30 days
                    QString key = limit1.addDays(30 * (limit1.daysTo(date) / 30)).toString();
                    SKGListSKGObjectBase group = groups[key];
                    group.push_back(v);
                    groups[key] = group;

                } else if (date >= limit3) {
                    // Simplification by group of 2 months
                    QString key = limit2.addDays(60 * (limit2.daysTo(date) / 60)).toString();
                    SKGListSKGObjectBase group = groups[key];
                    group.push_back(v);
                    groups[key] = std::move(group);
                } else {
                    // Simplification by group of 6 months
                    QString key = limit3.addDays(180 * (limit2.daysTo(date) / 180)).toString();
                    SKGListSKGObjectBase group = groups[key];
                    group.push_back(v);
                    groups[key] = group;
                }

                IFOKDO(err, getDocument()->stepForward(nb - i))
            }
        }
        IFOKDO(err, getDocument()->stepForward(1))

        // Simplify
        {
            QList<QString> keys = groups.keys();
            nb = keys.count();
            SKGBEGINPROGRESSTRANSACTION(*getDocument(), "#INTERNAL#" % i18nc("Progression step", "Remove useless values"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                const QString &k = keys.at(i);
                SKGListSKGObjectBase group = groups[k];

                // Simplify the group
                int nb2 = group.count();

                // Compute min and max
                double min = 10e20;
                double max = 0;
                for (int j = 0; j < nb2; ++j) {
                    SKGUnitValueObject v1(group.at(j));
                    double y1 = v1.getQuantity();
                    min = qMin(y1, min);
                    max = qMax(y1, max);
                }

                // Simplify
                for (int j = 1; !err && j < nb2 - 1; ++j) {
                    SKGUnitValueObject v1(group.at(j));
                    double y1 = v1.getQuantity();
                    if (y1 != min && y1 != max) {
                        err = v1.remove();
                    }
                }

                IFOKDO(err, getDocument()->stepForward(i + 1))
            }
        }
        IFOKDO(err, getDocument()->stepForward(2))
    }
    return err;
}

SKGError SKGUnitObject::getUnitValues(SKGListSKGObjectBase &oUnitValueList) const
{
    SKGError err =
        getDocument()->getObjects(QStringLiteral("v_unitvalue"), "rd_unit_id=" % SKGServices::intToString(getID()) % " ORDER BY d_date", oUnitValueList);
    return err;
}

SKGError SKGUnitObject::getLastUnitValue(SKGUnitValueObject &oUnitValue) const
{
    return SKGObjectBase::getDocument()->getObject(QStringLiteral("v_unitvalue"),
                                                   "rd_unit_id=" % SKGServices::intToString(getID())
                                                       % " AND d_date=(select MAX(u2.d_date) from unitvalue u2 where u2.rd_unit_id="
                                                       % SKGServices::intToString(getID()) % u')',
                                                   oUnitValue);
}

SKGError SKGUnitObject::getUnitValue(QDate iDate, SKGUnitValueObject &oUnitValue) const
{
    QString ids = SKGServices::intToString(getID());
    QString dates = SKGServices::dateToSqlString(iDate);
    SKGError err = SKGObjectBase::getDocument()->getObject(
        QStringLiteral("v_unitvalue"),
        "rd_unit_id=" % ids % " AND d_date<='" % dates % "' AND  ABS(strftime('%s','" % dates % "')-strftime('%s',d_date))=(select MIN(ABS(strftime('%s','"
            % dates % "')-strftime('%s',u2.d_date))) from unitvalue u2 where u2.rd_unit_id=" % ids % " AND u2.d_date<='" % dates % "')",
        oUnitValue);

    // If not found then get first
    IFKO(err)
    err = SKGObjectBase::getDocument()->getObject(QStringLiteral("v_unitvalue"),
                                                  "rd_unit_id=" % SKGServices::intToString(getID())
                                                      % " AND d_date=(select MIN(d_date) from unitvalue where rd_unit_id=" % SKGServices::intToString(getID())
                                                      % u')',
                                                  oUnitValue);
    return err;
}

double SKGUnitObject::getAmount(QDate iDate) const
{
    SKGTRACEINFUNC(10)
    double output = 0;
    if (getType() == SKGUnitObject::PRIMARY) {
        output = 1.0;
    } else if (getDocument() != nullptr) {
        // Search result in cache
        QString ids = SKGServices::intToString(getID());
        QString dates = SKGServices::dateToSqlString(iDate);
        QString key = "unitvalue-" % ids % u'-' % dates;
        QString val = getDocument()->getCachedValue(key);
        if (val.isEmpty()) {
            // Get quantity
            double quantity = 1;
            SKGUnitValueObject uv;
            if (getUnitValue(iDate, uv).isSucceeded()) {
                quantity = uv.getQuantity();
            }

            SKGUnitObject unit;
            double coef = 1;
            if (getUnit(unit).isSucceeded()) {
                if (unit != *this) {
                    coef = unit.getAmount(iDate);
                }
            }

            output = coef * quantity;
            getDocument()->addValueInCache(key, SKGServices::doubleToString(output));

            if (getAttribute(QStringLiteral("i_NBVALUES")) == QStringLiteral("1")) {
                // Store value for this symbol for all date
                getDocument()->addValueInCache("unitvalue-" % ids, SKGServices::doubleToString(output));
            }
        } else {
            output = SKGServices::stringToDouble(val);
        }
    }
    return output;
}

double SKGUnitObject::getDailyChange(QDate iDate) const
{
    double output = 0;
    SKGStringListList result;
    SKGError err = getDocument()->executeSelectSqliteOrder("SELECT d_date, f_quantity from unitvalue where rd_unit_id=" % SKGServices::intToString(getID())
                                                               % " AND d_date<='" % SKGServices::dateToSqlString(iDate) % "' ORDER BY d_date DESC LIMIT 2",
                                                           result);
    if (!err && result.count() == 3) {
        double v2 = SKGServices::stringToDouble(result.at(1).at(1));
        double v1 = SKGServices::stringToDouble(result.at(2).at(1));

        QDate d2 = SKGServices::stringToDate(result.at(1).at(0));
        QDate d1 = SKGServices::stringToDate(result.at(2).at(0));

        const auto div = SKGServices::nbWorkingDays(d1, d2);

        if (div != 0)
            output = 100 * (qExp(qLn(v2 / v1) / div) - 1);
    }
    return output;
}

SKGError SKGUnitObject::split(double iRatio) const
{
    SKGError err;
    if (iRatio > 0) {
        err = getDocument()->executeSqliteOrder("UPDATE unitvalue SET f_quantity=f_quantity/" % SKGServices::doubleToString(iRatio) % " WHERE rd_unit_id="
                                                % SKGServices::intToString(getID()));
        IFOKDO(err,
               getDocument()->executeSqliteOrder("UPDATE suboperation SET f_value=f_value*" % SKGServices::doubleToString(iRatio)
                                                 % " WHERE rd_operation_id IN (SELECT id FROM operation WHERE rc_unit_id=" % SKGServices::intToString(getID())
                                                 % QLatin1Char(')')));
    } else {
        err = SKGError(ERR_INVALIDARG, i18nc("Error message", "Invalid ratio. Ratio must be greater than 0."));
    }
    return err;
}

QStringList SKGUnitObject::downloadSources()
{
    QStringList sources;
    // Get sources from .txt file (old mode) TODO: Remove
    QString a = QStringLiteral("skrooge/quotes");
    const auto list = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, a, QStandardPaths::LocateDirectory);
    for (const auto &dir : list) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.txt"));
        while (it.hasNext()) {
            QFileInfo f(it.next());
            QString file2 = f.completeBaseName();
            if (!sources.contains(file2)) {
                sources.push_back(file2);
            }
        }
    }

    // Get sources from .json file
    const auto list2 = SKGServices::findDataPlugins(QStringLiteral("skrooge_source"));
    for (const auto &service : list2) {
        auto name = service.pluginId().remove(QStringLiteral("org.kde.skrooge-source-")).toLower();
        if (!sources.contains(name)) {
            sources.push_back(name);
        }
    }
    sources.sort();
    return sources;
}

QString SKGUnitObject::getCommentFromSource(const QString &iSource)
{
    QString output;
    const auto list2 = SKGServices::findDataPlugins(QStringLiteral("skrooge_source"));
    for (const auto &service : list2) {
        auto name = service.pluginId().remove(QStringLiteral("org.kde.skrooge-source-")).toLower();
        if (name == iSource.toLower()) {
            output = service.description();
            break;
        }
    }
    return output;
}

SKGError SKGUnitObject::addSource(const QString &iNewSource, bool iOpenSource)
{
    SKGError err;

    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir(path).mkpath(QStringLiteral("skrooge/quotes/"));

    QString newfile = path + "/skrooge/quotes/" % iNewSource % ".txt";

    // Create the new file
    QSaveFile file(newfile);
    // Check if file already existing
    if (!QFile(newfile).exists()) {
        // Creation of the template
        if (!file.open(QIODevice::WriteOnly)) {
            err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", newfile));
        } else {
            QTextStream out(&file);

            out << "#"
                << i18nc("Description test for a text file used to define a source of download",
                         "The URL or the SCRIPT of the source. %1 will be replaced by the internet code of the unit",
                         "%1")
                << Qt::endl;
            out << "#"
                << i18nc("Description test for a text file used to define a source of download",
                         "%1 will be replaced by the current day in format yyyy-MM-dd",
                         "%2")
                << Qt::endl;
            out << "#"
                << i18nc("Description test for a text file used to define a source of download",
                         "%1 will be replaced by the previous date in format yyyy-MM-dd",
                         "%3")
                << Qt::endl;
            out << "url=https://server/?s=%1" << Qt::endl;
            out << "or" << Qt::endl;
            out << "script=mydownloadscript %1" << Qt::endl << Qt::endl;

            out << "#"
                << i18nc("Description test for a text file used to define a source of download",
                         "The mode (HTML or CSV or CSVR). In HTML mode, only one value will be extracted from downloaded page. In CSV mode, a value per line "
                         "will be extracted. CSVR means CSV in reverse mode.")
                << Qt::endl;
            out << "mode=CSV, CSVR or or HTML" << Qt::endl << Qt::endl;

            out << "#"
                << i18nc("Description test for a text file used to define a source of download",
                         "The regular expression for the price (see %1)",
                         "https://doc.qt.io/qt-5/qregularexpression.html")
                << Qt::endl;
            out << "price=" << Qt::endl << Qt::endl;

            out << "#"
                << i18nc("Description test for a text file used to define a source of download",
                         "The regular expression for the date (see %1)",
                         "https://doc.qt.io/qt-5/qregularexpression.html")
                << Qt::endl;
            out << "date=" << Qt::endl << Qt::endl;

            out << "#"
                << i18nc("Description test for a text file used to define a source of download",
                         "The format of the date (see %1) or \"UNIX\" for unix time",
                         "https://doc.qt.io/qt-5/qdate.html#fromString-1")
                << Qt::endl;
            out << "dateformat=yyyy-MM-dd" << Qt::endl;

            // Close file
            file.commit();
        }
    }

    // Open the created or already existing file
    if (iOpenSource) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(newfile));
    }
    return err;
}

SKGError SKGUnitObject::deleteSource(const QString &iSource)
{
    SKGError err;

    QString fileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/' + QStringLiteral("skrooge/quotes/") % iSource % ".txt";

    // Delete the file
    QFile file(fileName);
    if (!file.remove()) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Deletion of '%1' failed", fileName));
    }
    return err;
}

bool SKGUnitObject::isWritable(const QString &iSource)
{
    QString fileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/' + QStringLiteral("skrooge/quotes/") % iSource % ".txt";
    return QFileInfo(fileName).isWritable();
}

SKGError SKGUnitObject::downloadUnitValue(UnitDownloadMode iMode, int iNbMaxValues)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    QString unitname = getName();
    QString code = getInternetCode();
    bool invert = code.contains(QStringLiteral(" /"));
    code.remove(QStringLiteral(" /"));
    QString source = getDownloadSource();
    auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
    if (!code.trimmed().isEmpty() && (doc != nullptr)) {
        // Get last date
        QDate firstDate;
        SKGStringListList result;
        doc->executeSelectSqliteOrder("SELECT MAX(d_date) FROM unitvalue where rd_unit_id=" % SKGServices::intToString(getID()), result);
        if (result.count() == 2) {
            firstDate = SKGServices::stringToDate(result.at(1).at(0)).addDays(-1);
        }

        if (code.startsWith(QStringLiteral("="))) {
            // MODE FORMULAR
            // Set 1st january if date is not found
            if (!firstDate.isValid()) {
                firstDate.setDate(QDate::currentDate().year(), 1, 1);
            }

            // Compute yearly rate
            double rate = SKGServices::stringToDouble(code.right(code.length() - 1));

            // Compute rate for step
            double step = (iMode == LAST_MONTHLY || iMode == ALL_MONTHLY ? 12.0 : (iMode == LAST_WEEKLY || iMode == ALL_WEEKLY ? 365.0 / 7.0 : 365));
            double rate2 = 100.0 * (qExp(qLn(1 + rate / 100.0) / step) - 1.0);

            // Get last value
            SKGStringListList result2;
            double value = 100;
            doc->executeSelectSqliteOrder("SELECT f_quantity FROM unitvalue where rd_unit_id=(SELECT id from unit where t_name='"
                                              % SKGServices::stringToSqlString(unitname) % "') AND d_date='" % SKGServices::dateToSqlString(firstDate) % u'\'',
                                          result2);
            if (result2.count() == 2) {
                value = SKGServices::stringToDouble(result2.at(1).at(0));
            }

            // Compute and add values
            while (!err && firstDate <= QDate::currentDate()) {
                // Compute next date and value
                if (iMode == LAST_MONTHLY || iMode == ALL_MONTHLY) {
                    firstDate = firstDate.addMonths(1);
                } else if (iMode == LAST_WEEKLY || iMode == ALL_WEEKLY) {
                    firstDate = firstDate.addDays(7);
                } else {
                    firstDate = firstDate.addDays(1);
                }

                value *= (1 + rate2 / 100.0);

                // Create value
                SKGUnitValueObject val;
                err = addUnitValue(val);
                IFOKDO(err, val.setDate(firstDate))
                IFOKDO(err, val.setQuantity(value))
                IFOKDO(err, val.save())
            }
        } else if (!source.trimmed().isEmpty()) {
            // Quote download

            // Set 1st january 1970 if date is not found
            if (!firstDate.isValid()) {
                firstDate.setDate(1970, 1, 1);
            }

            QString interval = QStringLiteral("1d");
            bool last = (iMode == LAST);
            if (last) {
                interval = QStringLiteral("1d");
            } else if (iMode == LAST_MONTHLY) {
                interval = QStringLiteral("1mo");
            } else if (iMode == LAST_WEEKLY) {
                interval = QStringLiteral("1wk");
            } else if (iMode == LAST_DAILY) {
                interval = QStringLiteral("1d");
            } else if (iMode == ALL_MONTHLY) {
                firstDate = QDate(1970, 01, 01);
                interval = QStringLiteral("1mo");
            } else if (iMode == ALL_WEEKLY) {
                firstDate = QDate(1970, 01, 01);
                interval = QStringLiteral("1wk");
            } else if (iMode == ALL_DAILY) {
                firstDate = QDate(1970, 01, 01);
                interval = QStringLiteral("1d");
            }

            bool modeScript = false;
            QString path;
            QString mode;
            QString price;
            QString date;
            QString dateFormat;
            QString apiKey;
            QString fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "skrooge/quotes/" % source % ".txt");
            if (fileName.isEmpty()) {
                const auto list2 = SKGServices::findDataPlugins(QStringLiteral("skrooge_source"));
                for (const auto &service : list2) {
                    auto name = service.pluginId().remove(QStringLiteral("org.kde.skrooge-source-")).toLower();
                    if (name == source.toLower()) {
                        path = service.value(QStringLiteral("X-SKROOGE-url"), QString());
                        if (path.isEmpty()) {
                            path = service.value(QStringLiteral("X-SKROOGE-script"), QString());
                            modeScript = true;
                        }

                        mode = service.value(QStringLiteral("X-SKROOGE-mode"), QString()).toUpper();
                        price = service.value(QStringLiteral("X-SKROOGE-price"), QString()).replace(QStringLiteral("%1"), code);
                        date = service.value(QStringLiteral("X-SKROOGE-date"), QString()).replace(QStringLiteral("%1"), code);
                        dateFormat = service.value(QStringLiteral("X-SKROOGE-dateformat"), QString());
                        if (service.value(QStringLiteral("X-SKROOGE-keyAPI"), false)) {
                            apiKey = getDocument()->getParameter("KEYAPI_" + source);
                        }
                        break;
                    }
                }
                if (path.isEmpty()) {
                    err = SKGError(ERR_FAIL, i18nc("Error message", "Source of download %1 is not installed.", source));
                }
            } else {
                // Read source file from .txt file
                QHash<QString, QString> properties;
                err = SKGServices::readPropertyFile(fileName, properties);
                IFOK(err)
                {
                    path = properties[QStringLiteral("url")];
                    if (path.isEmpty()) {
                        path = properties[QStringLiteral("script")];
                        modeScript = true;
                    }
                    mode = properties[QStringLiteral("mode")].toUpper();
                    price = properties[QStringLiteral("price")].replace(QStringLiteral("%1"), code);
                    date = properties[QStringLiteral("date")].replace(QStringLiteral("%1"), code);
                    dateFormat = properties[QStringLiteral("dateformat")];
                }
                else
                {
                    doc->sendMessage(i18nc("An information message", "Open url '%1' failed", path), SKGDocument::Warning);
                }
            }

            IFOK(err)
            {
                path = path.replace(QStringLiteral("%1"), code);
                path = path.replace(QStringLiteral("%2"), QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
                path = path.replace(QStringLiteral("%3"), firstDate.toString(QStringLiteral("yyyy-MM-dd")));
                path = path.replace(QStringLiteral("%4"), interval);
                path = path.replace(QStringLiteral("%5"), apiKey);

                SKGTRACEL(1) << "path=[" << path << "]" << Qt::endl;
                SKGTRACEL(1) << "mode=[" << mode << "]" << Qt::endl;
                SKGTRACEL(1) << "price=[" << price << "]" << Qt::endl;
                SKGTRACEL(1) << "date=[" << date << "]" << Qt::endl;
                SKGTRACEL(1) << "dateFormat=[" << dateFormat << "]" << Qt::endl;

                // Download url
                QByteArray stream;
                if (modeScript) {
                    path = SKGServices::getFullPathCommandLine(path);
                    SKGTRACEL(1) << "full path=[" << path << "]" << Qt::endl;

                    QProcess p;
                    p.start(QStringLiteral("/bin/bash"), QStringList() << QStringLiteral("-c") << path);
                    if (p.waitForFinished(1000 * 60 * 2) && p.exitCode() == 0) {
                        stream = p.readAllStandardOutput();
                    } else {
                        err.setReturnCode(ERR_FAIL).setMessage(
                            i18nc("Error message", "The following command line failed with code %2:\n'%1'", path, p.exitCode()));
                    }
                } else {
                    SKGServices::downloadToStream(QUrl::fromUserInput(path), stream);
                }
                if (!err && !stream.isEmpty()) {
                    SKGTRACEL(1) << "stream=[" << stream << "]" << Qt::endl;

                    // Parse data
                    QRegularExpression priceRegExp(price, QRegularExpression::CaseInsensitiveOption);
                    QRegularExpression dateRegExp(date, QRegularExpression::CaseInsensitiveOption);

                    QStringList lines =
                        (mode.startsWith(QStringLiteral("CSV")) ? SKGServices::splitCSVLine(stream, '\n') : QStringList() << QString() << stream);
                    int nb = lines.count();
                    int nbLoaded = 0;
                    for (int i = 1; i < nb && !err && (i < iNbMaxValues || iNbMaxValues == 0); ++i) {
                        QString data = lines.at(mode == QStringLiteral("CSVR") ? nb - i : i).trimmed();
                        if (!data.isEmpty()) {
                            SKGTRACEL(1) << "Downloaded data from [" << path << "]=[" << data << "]" << Qt::endl;

                            double val = 0.0;
                            auto match = priceRegExp.match(data);
                            if (match.hasMatch()) {
                                val = SKGServices::stringToDouble(match.captured(1));
                            }
                            QString date2;
                            match = dateRegExp.match(data);
                            if (match.hasMatch()) {
                                date2 = match.captured(1);
                            }
                            SKGTRACEL(1) << "Price found=[" << val << "]" << Qt::endl;
                            SKGTRACEL(1) << "Date found=[" << date2 << "]" << Qt::endl;

                            // Set value
                            if (val != 0.0) {
                                QDate ds;
                                if (dateFormat == QStringLiteral("UNIX")) {
                                    ds = QDateTime::fromSecsSinceEpoch(SKGServices::stringToInt(date2)).date();
                                } else {
                                    ds = QDate::fromString(date2, dateFormat);
                                    // Try with an english locale
                                    if (!ds.isValid()) {
                                        QLocale en(QStringLiteral("en_EN"));
                                        ds = en.toDate(date2, dateFormat);
                                    }
                                }

                                if (!ds.isValid()) {
                                    ds = QDate::currentDate();
                                    SKGTRACE << "WARNING:" << date2 << " not well parsed with format " << dateFormat << Qt::endl;
                                }
                                if (!dateFormat.contains(QStringLiteral("yyyy")) && ds.year() < 2000) {
                                    ds = ds.addYears(100);
                                }

                                // Creation or update of the value
                                SKGUnitValueObject value;
                                IFOKDO(err, addUnitValue(value))
                                IFOKDO(err, value.setDate(ds))
                                IFOKDO(err, value.setQuantity(invert && val != 0 ? 1 / val : val))
                                IFOKDO(err, value.save())

                                if (last) {
                                    break;
                                }

                                nbLoaded++;
                            }
                        }
                        if (nbLoaded == 0 && !data.isEmpty()) {
                            err = doc->sendMessage(i18nc("Information message",
                                                         "Price not found for '%1' with regular expression '%2' in line '%3'",
                                                         getName(),
                                                         SKGServices::stringToHtml(price),
                                                         data),
                                                   SKGDocument::Warning); // TODO(Stephane MANKOWSKI) does not work with html
                        }
                    }
                }
            }
        }
    }

    IFKO(err)
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Impossible to download unit %1 with Internet code %2 on the source %3.", unitname, code, source));
    }

    return err;
}

SKGError SKGUnitObject::getUrl(QUrl &oUrl) const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    QString url;
    QString code = getInternetCode();
    code.remove(QStringLiteral(" /"));
    QString source = getDownloadSource().toLower();
    if (!code.isEmpty()) {
        if (code.startsWith(QStringLiteral("="))) {
            // MODE FORMULAR

        } else {
            // ALTERNATIVE MODE
            QString fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "skrooge/quotes/" % source % ".txt");
            if (fileName.isEmpty()) {
                const auto list2 = SKGServices::findDataPlugins(QStringLiteral("skrooge_source"));
                for (const auto &service : list2) {
                    auto name = service.pluginId().remove(QStringLiteral("org.kde.skrooge-source-")).toLower();
                    if (name == source) {
                        url = service.value(QStringLiteral("X-SKROOGE-url"), QString()).replace(QStringLiteral("%1"), code);
                        url = url.replace(QStringLiteral("%2"), QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
                        url = url.replace(QStringLiteral("%3"), QDate::currentDate().addDays(-15).toString(QStringLiteral("yyyy-MM-dd")));
                        break;
                    }
                }
                if (url.isEmpty()) {
                    err = SKGError(ERR_FAIL, i18nc("Error message", "Source of download %1 is not installed.", source));
                }
            } else {
                // Read source file
                QHash<QString, QString> properties;
                err = SKGServices::readPropertyFile(fileName, properties);

                IFOK(err)
                {
                    url = properties[QStringLiteral("url")].replace(QStringLiteral("%1"), code);
                    url = url.replace(QStringLiteral("%2"), QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
                    url = url.replace(QStringLiteral("%3"), QDate::currentDate().addDays(-15).toString(QStringLiteral("yyyy-MM-dd")));
                }
            }
        }
    }

    IFOK(err)
    {
        oUrl = QUrl(url);
    }

    return err;
}

SKGError SKGUnitObject::openURL() const
{
    QUrl url;
    SKGError err = getUrl(url);

    IFKO(err)
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Impossible to open unit %1 with Internet code %2.", getName(), getInternetCode()));
    }
    else
    {
        QDesktopServices::openUrl(url);
    }

    return err;
}

SKGError SKGUnitObject::getOperations(SKGObjectBase::SKGListSKGObjectBase &oOperations) const
{
    SKGError err = getDocument()->getObjects(QStringLiteral("v_operation"), "rc_unit_id=" % SKGServices::intToString(getID()), oOperations);
    return err;
}

SKGError SKGUnitObject::merge(const SKGUnitObject &iUnit)
{
    SKGError err;

    SKGObjectBase::SKGListSKGObjectBase ops;
    IFOKDO(err, iUnit.getOperations(ops))
    int nb = ops.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGOperationObject op(ops.at(i));
        err = op.setUnit(*this);
        IFOKDO(err, op.save(true, false))
    }

    IFOKDO(err, iUnit.remove(false))
    return err;
}
