/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#include "skgaddoperation.h"

#include <qdir.h>
#include <qicon.h>
#include <qsavefile.h>
#include <qstandardpaths.h>
#include <qstringbuilder.h>
#include <qtextstream.h>
#include <quuid.h>

#include <kconfiggroup.h>
#include <klocalizedstring.h>
#include <kmessagebox.h>

K_PLUGIN_CLASS_WITH_JSON(SKGAddOperation, "metadata.json")

SKGAddOperation::SKGAddOperation(QObject *iParent, const KPluginMetaData &pluginMetaData, const QVariantList &args)
    : KRunner::AbstractRunner(iParent, pluginMetaData)
{
}

void SKGAddOperation::reloadConfiguration()
{
    m_triggerWord = config().readEntry("buy", i18nc("default keyword for krunner plugin", "buy"));
    addSyntax(i18nc("Example of krunner command", "%1 10 shop", m_triggerWord), i18n("Add a new transaction in skrooge"));
    setTriggerWords({m_triggerWord});
}

void SKGAddOperation::match(KRunner::RunnerContext &iContext)
{
    QString query = iContext.query().remove(0, m_triggerWord.length() + 1);
    KRunner::QueryMatch m(this);
    m.setText(i18n("Add transaction %1", query));
    m.setData(query);
    m.setIconName(QStringLiteral("skrooge"));
    m.setId(query);

    iContext.addMatch(m);
}

void SKGAddOperation::run(const KRunner::RunnerContext &iContext, const KRunner::QueryMatch &iMatch)
{
    Q_UNUSED(iContext)

    QString dirName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/skrooge");
    QDir().mkpath(dirName);
    QString fileName = QDir(dirName).filePath(QStringLiteral("add_operation_") + QUuid::createUuid().toString() + QStringLiteral(".txt"));
    QSaveFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream stream(&file);
        stream << "buy" << Qt::endl;
        stream << QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")) << Qt::endl;
        QString s = iMatch.id().remove(0, QString("skroogeaddoperation_").length());
        int pos = s.indexOf(QStringLiteral(" "));
        if (pos == -1) {
            stream << s << Qt::endl;
            stream << "" << Qt::endl;
        } else {
            stream << s.left(pos).trimmed() << Qt::endl;
            stream << s.right(s.length() - pos - 1).trimmed() << Qt::endl;
        }

        // Close file
        file.commit();

        KMessageBox::information(nullptr, i18nc("Information message", "Transaction created"));
    } else {
        KMessageBox::error(nullptr, i18nc("Error message: Could not create a file", "Creation of file %1 failed", fileName));
    }
}

#include "skgaddoperation.moc"
