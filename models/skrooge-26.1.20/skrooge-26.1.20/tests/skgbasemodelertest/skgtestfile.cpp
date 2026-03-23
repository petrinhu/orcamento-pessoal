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
#include <qfile.h>
#include <qsqldatabase.h>

#include "skgtestmacro.h"

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
        SKGTESTBOOL("QSQLITE:isDriverAvailable", QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE")), true)
        SKGTESTBOOL("SKGSQLCIPHER:isDriverAvailable", QSqlDatabase::isDriverAvailable(QStringLiteral("SKGSQLCIPHER")), true)
    }
    {
        // test class SKGDocument / LOAD / SAVE
        QString filename1 = SKGTest::getTestPath(QStringLiteral("IN")) % QStringLiteral("/filename1_e.skg");
        qDebug("filename1=%s", filename1.toUtf8().constData());
        {
            SKGDocument document1;
            QFile(filename1).remove();
            SKGTESTERROR(QStringLiteral("DOC:initialize"), document1.initialize(), true)
            SKGTESTERROR(QStringLiteral("DOC:setLanguage"), document1.setLanguage(QStringLiteral("en")), true)
            SKGTESTERROR(QStringLiteral("DOC:setLanguage"), document1.setLanguage(QStringLiteral("fr")), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), false)

            SKGTESTERROR(QStringLiteral("DOC:beginTransaction"), document1.beginTransaction(QStringLiteral("tt")), true)
            SKGTESTERROR(QStringLiteral("DOC:setParameter"), document1.setParameter(QStringLiteral("ATT3"), QStringLiteral("VAL3")), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), true)
            SKGTESTERROR(QStringLiteral("DOC:endTransaction"), document1.endTransaction(false), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), false)

            SKGTESTERROR(QStringLiteral("DOC:beginTransaction"), document1.beginTransaction(QStringLiteral("tt")), true)
            SKGTESTERROR(QStringLiteral("DOC:setParameter"), document1.setParameter(QStringLiteral("ATT3"), QStringLiteral("VAL3")), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), true)
            SKGTESTERROR(QStringLiteral("DOC:endTransaction"), document1.endTransaction(true), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), true)

            document1.setFileNotModified();
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), false)

            SKGTEST(QStringLiteral("DOC:getDisplay"), document1.getDisplay(QStringLiteral("t_name")), QStringLiteral("Name"))
            SKGTEST(QStringLiteral("DOC:getDisplay"), document1.getDisplay(QStringLiteral("node.t_name")), QStringLiteral("Name"))
            SKGTEST(QStringLiteral("DOC:getDisplay"), document1.getDisplay(QStringLiteral("v_node.t_name")), QStringLiteral("Name"))
            SKGTEST(QStringLiteral("DOC:getDisplay"), document1.getDisplay(QStringLiteral("v_node.unknown")), QStringLiteral("v_node.unknown"))

            QString oResult;
            SKGTESTERROR(QStringLiteral("DOC:isFileModified"), document1.dumpSelectSqliteOrder(QStringLiteral("select * from parameters"), oResult), true)
        }
        {
            SKGDocument document1;
            QFile(filename1).remove();
            SKGTESTERROR(QStringLiteral("DOC:initialize"), document1.initialize(), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), false)

            SKGTESTERROR(QStringLiteral("DOC:beginTransaction"), document1.beginTransaction(QStringLiteral("tt")), true)
            SKGTESTERROR(QStringLiteral("DOC:endTransaction"), document1.endTransaction(true), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), false) // because the transaction is empty
        }
        {
            SKGDocument document1;
            QFile(filename1).remove();
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(filename1, QString()), false)
            SKGTESTERROR(QStringLiteral("DOC:saveAs+invalid path"), document1.saveAs(QStringLiteral("/notfound/file.skg"), true), false)
            SKGTEST(QStringLiteral("DOC:getCurrentFileName"), document1.getCurrentFileName(), QString())
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), false)
            SKGTESTERROR(QStringLiteral("DOC:initialize"), document1.initialize(), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), false)
            SKGTESTERROR(QStringLiteral("DOC:beginTransaction"), document1.beginTransaction(QStringLiteral("t1")), true)
            SKGTEST(QStringLiteral("DOC:getCurrentFileName"), document1.getCurrentFileName(), QString())
            SKGTESTERROR(QStringLiteral("DOC:save"), document1.save(), false)
            SKGTEST(QStringLiteral("DOC:getCurrentFileName"), document1.getCurrentFileName(), QString())
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), true) // due to the beginTransaction
            SKGTEST(QStringLiteral("DOC:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QString())
            SKGTESTERROR(QStringLiteral("DOC:setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1")), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), true)
            SKGTEST(QStringLiteral("DOC:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
            SKGTESTERROR(QStringLiteral("DOC:endTransaction"), document1.endTransaction(true), true)
            SKGTESTERROR(QStringLiteral("DOC:saveAs+overwrite=false"), document1.saveAs(filename1, false), true)
            SKGTESTBOOL("DOC:exist", QFile(filename1).exists(), true)
            SKGTEST(QStringLiteral("DOC:getCurrentFileName"), document1.getCurrentFileName(), filename1)
            SKGTESTERROR(QStringLiteral("DOC:saveAs+overwrite=false"), document1.saveAs(filename1, false), false)
            SKGTESTBOOL("DOC:exist", QFile(filename1).exists(), true)
            SKGTEST(QStringLiteral("DOC:getCurrentFileName"), document1.getCurrentFileName(), filename1)
            SKGTESTERROR(QStringLiteral("DOC:save"), document1.save(), true)
            SKGTESTBOOL("DOC:exist", QFile(filename1).exists(), true)
            SKGTEST(QStringLiteral("DOC:getCurrentFileName"), document1.getCurrentFileName(), filename1)

            SKGTESTERROR(QStringLiteral("DOC:beginTransaction"), document1.beginTransaction(QStringLiteral("t1")), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), true) // due to the beginTransaction
            SKGTEST(QStringLiteral("DOC:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
            SKGTESTERROR(QStringLiteral("DOC:setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL2")), true)
            SKGTEST(QStringLiteral("DOC:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL2"))
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), true)
            SKGTESTERROR(QStringLiteral("DOC:endTransaction"), document1.endTransaction(true), true)

            SKGTESTBOOL("DOC:exist", QFile(filename1).exists(), true)
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(filename1, QString()), true)
            SKGTESTBOOL("DOC:isFileModified", document1.isFileModified(), false)
            SKGTESTBOOL("DOC:getCurrentFileName", (!document1.getCurrentFileName().isEmpty()), true)
            SKGTEST(QStringLiteral("DOC:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
        }

        {
            SKGDocument document1;
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(filename1, QString()), true)
            SKGTESTERROR(QStringLiteral("DOC:beginTransaction"), document1.beginTransaction(QStringLiteral("t3")), true)
            SKGTESTERROR(QStringLiteral("DOC:setParameter"), document1.setParameter(QStringLiteral("ATT3"), QStringLiteral("VAL3")), true)
            SKGTESTERROR(QStringLiteral("DOC:endTransaction"), document1.endTransaction(true), true)
            SKGTESTERROR(QStringLiteral("DOC:save"), document1.save(), true)

            SKGTESTERROR(QStringLiteral("DOC:changePassword"), document1.changePassword(QStringLiteral("pwd1")), true)
            SKGTESTERROR(QStringLiteral("DOC:changePassword"), document1.changePassword(QStringLiteral("pwd")), true)
            SKGTESTERROR(QStringLiteral("DOC:beginTransaction"), document1.beginTransaction(QStringLiteral("tt")), true)
            SKGTESTERROR(QStringLiteral("DOC:setParameter"), document1.setParameter(QStringLiteral("SAVE"), QStringLiteral("1")), true)
            SKGTESTERROR(QStringLiteral("DOC:endTransaction"), document1.endTransaction(true), true)

            SKGTEST(QStringLiteral("DOC:getParameter"), document1.getParameter(QStringLiteral("SAVE")), QStringLiteral("1"))
            SKGTESTERROR(QStringLiteral("DOC:save"), document1.save(), true)
        }
        {
            SKGDocument document1;
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(filename1, QString()), false)
            SKGTEST(QStringLiteral("DOC:load"), document1.load(filename1, QStringLiteral("wrong pwd")).getReturnCode(), ERR_ENCRYPTION)
        }
        {
            SKGDocument document1;
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(filename1, QStringLiteral("pwd")), true)
            SKGTESTERROR(QStringLiteral("DOC:changePassword"), document1.changePassword(QStringLiteral("pwd1")), true)
            SKGTESTERROR(QStringLiteral("DOC:changePassword"), document1.changePassword(QString()), true)
            SKGTESTERROR(QStringLiteral("DOC:save"), document1.save(), true)
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(filename1, QString()), true)
        }

        {
            SKGDocument document1;
            QFile(filename1).remove();
            SKGTESTERROR(QStringLiteral("DOC:initialize"), document1.initialize(), true)
            document1.setBackupParameters(QString(), QStringLiteral("/invalid suffix"));
            SKGTESTERROR(QStringLiteral("DOC:saveAs"), document1.saveAs(filename1, true), true)
            SKGTESTERROR(QStringLiteral("DOC:saveAs"), document1.saveAs(filename1, true), true)

            document1.setBackupParameters(QStringLiteral("."), QStringLiteral(".old"));
            SKGTEST(QStringLiteral("DOC:getBackupFile"), document1.getBackupFile(QStringLiteral("/tmp/file.skg")), QStringLiteral("/tmp/.file.skg.old"))

            document1.setBackupParameters(QString(), QStringLiteral("_<DATE>.backup"));
            SKGTEST(QStringLiteral("DOC:getBackupFile"),
                    document1.getBackupFile(QStringLiteral("/tmp/file.skg")),
                    QStringLiteral("/tmp/file.skg_") % SKGServices::dateToSqlString(QDateTime::currentDateTime().date()) % ".backup")

            document1.setBackupParameters(QString(), QStringLiteral("_<DATE>.skg"));
            SKGTEST(QStringLiteral("DOC:getBackupFile"),
                    document1.getBackupFile(QStringLiteral("/tmp/file.skg")),
                    QStringLiteral("/tmp/file_") % SKGServices::dateToSqlString(QDateTime::currentDateTime().date()) % ".skg")

            document1.setBackupParameters(QString(), QStringLiteral("_<TIME>.backup"));
            SKGTEST(QStringLiteral("DOC:getBackupFile"),
                    document1.getBackupFile(QStringLiteral("/tmp/file.skg")),
                    QStringLiteral("/tmp/file.skg_") % SKGServices::timeToString(QDateTime::currentDateTime()) % ".backup")

            document1.setBackupParameters(QStringLiteral("/tmp/"), QStringLiteral(".old"));
            SKGTEST(QStringLiteral("DOC:getBackupFile"), document1.getBackupFile(QStringLiteral("/home/user1/file.skg")), QStringLiteral("/tmp/file.skg.old"))
        }

        {
            SKGDocument document1;
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(filename1, QString()), true)
            SKGTESTERROR(QStringLiteral("DOC:beginTransaction"), document1.beginTransaction(QStringLiteral("tt")), true)
            SKGTESTERROR(QStringLiteral("DOC:setParameter"), document1.setParameter(QStringLiteral("SAVE"), QStringLiteral("1")), true)
            SKGTEST(QStringLiteral("DOC:getParameter"), document1.getParameter(QStringLiteral("SAVE")), QStringLiteral("1"))
            SKGTESTERROR(QStringLiteral("DOC:endTransaction"), document1.endTransaction(true), true)
            SKGTESTERROR(QStringLiteral("DOC:save"), document1.save(), true)

            // Check save when file system is full
            qputenv("SKGFILESYSTEMFULL", "Y");
            SKGTEST(QStringLiteral("DOC:getParameter"), static_cast<unsigned int>(QFile(filename1).exists()), static_cast<unsigned int>(true))
            SKGTESTERROR(QStringLiteral("DOC:save"), document1.save(), false)
            SKGTEST(QStringLiteral("DOC:getParameter"), static_cast<unsigned int>(QFile(filename1).exists()), static_cast<unsigned int>(true))
        }

        QFile(filename1).remove();
        QFile(filename1 % ".old").remove();
    }
    // End test
    SKGENDTEST()
}
