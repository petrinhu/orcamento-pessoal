/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * The grantlee's plugin to define filters.
 *
 * @author Stephane MANKOWSKI
 */
#include "skggrantleefilters.h"

#include "skgdocumentfilter.h"
#include "skgobjectfilter.h"

SKGGrantleeFilters::SKGGrantleeFilters(QObject *iParent)
    : QObject(iParent)
{
}

SKGGrantleeFilters::~SKGGrantleeFilters() = default;

QHash<QString, KTextTemplate::Filter *> SKGGrantleeFilters::filters(const QString &iName)
{
    Q_UNUSED(iName)

    QHash<QString, KTextTemplate::Filter *> filtersList;
    filtersList.insert(QStringLiteral("query"), new SKGDocumentQueryFilter());
    filtersList.insert(QStringLiteral("table"), new SKGDocumentTableFilter());
    filtersList.insert(QStringLiteral("display"), new SKGDocumentDisplayFilter());
    filtersList.insert(QStringLiteral("att"), new SKGObjectFilter());
    filtersList.insert(QStringLiteral("money"), new SKGMoneyFilter());
    filtersList.insert(QStringLiteral("percent"), new SKGPercentFilter());
    filtersList.insert(QStringLiteral("filesizeformat"), new SKGFileSizeFilter());
    filtersList.insert(QStringLiteral("dump"), new SKGDumpFilter());
    filtersList.insert(QStringLiteral("encode"), new SKGUrlEncodeFilter());
    filtersList.insert(QStringLiteral("replace"), new SKGReplaceFilter());
    return filtersList;
}
