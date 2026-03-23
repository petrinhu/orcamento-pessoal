/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGSQLCipherDriverPlugin.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "q6sql_sqlite_p.h"
#include <qsqldriverplugin.h>
#include <qstringlist.h>

QT_BEGIN_NAMESPACE

class SKGSQLCipherDriverPlugin : public QSqlDriverPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QSqlDriverFactoryInterface" FILE "skgsqlcipherdriverplugin.json")

public:
    SKGSQLCipherDriverPlugin() = default;

    QSqlDriver *create(const QString &iName) override
    {
        if (iName == QStringLiteral("SKGSQLCIPHER")) {
            auto driver = new QSQLiteDriver();
            return driver;
        }
        return nullptr;
    }
};

QT_END_NAMESPACE

#include "skgsqlcipherdriverplugin.moc"
