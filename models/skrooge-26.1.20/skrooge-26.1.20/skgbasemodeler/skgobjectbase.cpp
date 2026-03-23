/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGObjectBase.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgobjectbase.h"

#include "qregularexpression.h"
#include <qsqldatabase.h>

#include <klocalizedstring.h>
#include <kstringhandler.h>

#include <algorithm>

#include "skgdocument.h"
#include "skgnamedobject.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This private class of SKGObjectBase
 */
class SKGObjectBasePrivate
{
public:
    /**
     * internal id
     */
    int id = 0;

    /**
     * internal table
     */
    QString table;

    /**
     * internal document
     */
    SKGDocument *document = nullptr;

    /**
     * internal attributes
     */
    SKGQStringQStringMap attributes;

    /**
     * objects to save
     */
    SKGObjectBase::SKGListSKGObjectBase objects;
};

SKGObjectBase::SKGObjectBase()
    : SKGObjectBase(nullptr)
{
}

SKGObjectBase::SKGObjectBase(SKGDocument *iDocument, const QString &iTable, int iID)
    : d(new SKGObjectBasePrivate)
{
    d->id = iID;
    d->table = iTable;
    d->document = iDocument;
    if (Q_LIKELY(d->id != 0)) {
        load();
    }
}

SKGObjectBase::~SKGObjectBase()
{
    delete d;
}

SKGObjectBase::SKGObjectBase(const SKGObjectBase &iObject)
    : d(new SKGObjectBasePrivate)
{
    copyFrom(iObject);
}

SKGObjectBase::SKGObjectBase(SKGObjectBase &&iObject) noexcept
    : d(iObject.d)
{
    iObject.d = nullptr;
}

SKGObjectBase &SKGObjectBase::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

bool SKGObjectBase::operator==(const SKGObjectBase &iObject) const
{
    return (getUniqueID() == iObject.getUniqueID());
}

bool SKGObjectBase::operator!=(const SKGObjectBase &iObject) const
{
    return !(*this == iObject);
}

bool SKGObjectBase::operator<(const SKGObjectBase &iObject) const
{
    double d1 = SKGServices::stringToDouble(getAttribute(QStringLiteral("f_sortorder")));
    double d2 = SKGServices::stringToDouble(iObject.getAttribute(QStringLiteral("f_sortorder")));
    return (d1 < d2);
}

bool SKGObjectBase::operator>(const SKGObjectBase &iObject) const
{
    double d1 = SKGServices::stringToDouble(getAttribute(QStringLiteral("f_sortorder")));
    double d2 = SKGServices::stringToDouble(iObject.getAttribute(QStringLiteral("f_sortorder")));
    return (d1 > d2);
}

QString SKGObjectBase::getUniqueID() const
{
    return SKGServices::intToString(d->id) % u'-' % getRealTable();
}

int SKGObjectBase::getID() const
{
    return d->id;
}

QString SKGObjectBase::getAttributeFromView(const QString &iView, const QString &iName) const
{
    QString output;

    SKGStringListList result;
    QString wc = getWhereclauseId();
    if (wc.isEmpty()) {
        wc = "id=" % SKGServices::intToString(d->id);
    }
    QString sql = "SELECT " % iName % " FROM " % iView % " WHERE " % wc;
    if (getDocument() != nullptr) {
        getDocument()->executeSelectSqliteOrder(sql, result, false);
    }
    if (result.count() == 2) {
        output = result.at(1).at(0);
    }

    return output;
}

QString SKGObjectBase::getDisplayName() const
{
    return getAttributeFromView("v_" % getRealTable() % "_displayname", QStringLiteral("t_displayname"));
}

void SKGObjectBase::copyFrom(const SKGObjectBase &iObject)
{
    if (d == nullptr) {
        d = new SKGObjectBasePrivate;
    }
    d->id = iObject.d->id;
    d->table = iObject.d->table;
    d->document = iObject.d->document;
    d->attributes = iObject.d->attributes;
}

SKGObjectBase SKGObjectBase::cloneInto()
{
    return cloneInto(nullptr);
}

SKGObjectBase SKGObjectBase::cloneInto(SKGDocument *iDocument)
{
    SKGDocument *targetDocument = iDocument;
    if (targetDocument == nullptr) {
        targetDocument = d->document;
    }

    SKGObjectBase output;
    output.copyFrom(*this);
    output.d->id = 0;
    output.d->document = targetDocument;
    return output;
}

SKGError SKGObjectBase::resetID()
{
    d->id = 0;
    return SKGError();
}

QString SKGObjectBase::getTable() const
{
    return d->table;
}

QString SKGObjectBase::getRealTable() const
{
    return SKGServices::getRealTable(d->table);
}

SKGDocument *SKGObjectBase::getDocument() const
{
    return d->document;
}

SKGQStringQStringMap SKGObjectBase::getAttributes() const
{
    return d->attributes;
}

int SKGObjectBase::getNbAttributes() const
{
    return d->attributes.count();
}

SKGError SKGObjectBase::setAttributes(const QStringList &iNames, const QStringList &iValues)
{
    SKGError err;
    int nb = iNames.size();
    for (int i = 0; !err && i < nb; ++i) {
        const auto &att = iNames.at(i);
        const auto &val = iValues.at(i);

        if (Q_LIKELY(att != QStringLiteral("id"))) {
            err = setAttribute(att, val);
        } else {
            d->id = SKGServices::stringToInt(val);
        }
    }
    return err;
}

SKGError SKGObjectBase::setAttribute(const QString &iName, const QString &iValue)
{
    SKGError err;
    if (Q_LIKELY(iValue != NOUPDATE)) {
        QString val = iValue;

        // Case modification on marked document
        if (this->getRealTable() == this->getTable() && iName != iName.toLower()) {
            QString realAttribute = d->document->getRealAttribute(iName);
            if (!realAttribute.isEmpty()) {
                QStringList l = realAttribute.split(u'.');
                if (l.size() == 3) {
                    SKGObjectBase obj;
                    QString refId = getAttribute(l.at(1));
                    if (!refId.isEmpty()) {
                        err = getDocument()->getObject("v_" % l.at(0), "id=" % refId, obj);
                        IFOK(err)
                        {
                            err = obj.setAttribute(l.at(2), iValue);
                            d->objects.push_back(obj);
                        }
                    }
                }
            }
        } else if (iValue.startsWith(QStringLiteral("="))) {
            // Case modificator
            QString op = iValue.right(iValue.length() - 1).toLower();
            val = d->attributes[iName];
            if (op == i18nc("Key word to modify a string into a field", "lower")) {
                val = val.toLower();
            } else if (op == i18nc("Key word to modify a string into a field", "upper")) {
                val = val.toUpper();
            } else if (op == i18nc("Key word to modify a string into a field", "capwords")) {
                val = KStringHandler::capwords(val.toLower());
            } else if (op == i18nc("Key word to modify a string into a field", "capitalize")) {
                val = val.at(0).toUpper() % val.right(val.length() - 1).toLower();
            } else if (op == i18nc("Key word to modify a string into a field", "trim")) {
                val = val.trimmed();
            } else {
                val = iValue;
            }
        }
        d->attributes[iName] = std::move(val);
    }
    return err;
}

QString SKGObjectBase::getAttribute(const QString &iName) const
{
    QString output;
    if (Q_LIKELY(d->attributes.contains(iName))) {
        output = d->attributes[iName];
    } else if (iName == QStringLiteral("id")) {
        output = SKGServices::intToString(getID());
    }
    if (iName.startsWith(QStringLiteral("p_"))) {
        output = getProperty(iName.right(iName.length() - 2));
    } else {
        // Is the iName a number ?
        bool ok;
        int pos = iName.toInt(&ok);
        if (ok) {
            // What is the key corresponding to this name ?
            QStringList keys = d->attributes.keys();
            std::sort(keys.begin(), keys.end());
            if (pos >= 0 && pos < keys.count()) {
                output = d->attributes[keys.at(pos)];
            }
        } else {
            // Case modification on marked document iName1.(iTable2)iName2 or iName1.iName2
            int pos = iName.indexOf(u'.');
            if (pos != -1) {
                QString attributeref = iName.left(pos);
                QString refId = getAttribute(attributeref);
                if (!refId.isEmpty()) {
                    QString rest = iName.right(iName.length() - pos - 1);

                    QString table;
                    QString att;
                    static QRegularExpression reg1(QStringLiteral("^\\(([^\\)]*)\\)(.*)$"));
                    auto rx = reg1.match(rest);
                    if (rx.hasMatch()) {
                        // Case iName1.(iTable2)iName2
                        table = rx.captured(1);
                        att = rx.captured(2);
                    } else {
                        static QRegularExpression reg2(QStringLiteral("^.*_(.*)_.*$"));
                        auto rx2 = reg2.match(attributeref);
                        if (rx2.hasMatch()) {
                            // Case iName1.iName2
                            table = "v_" % rx2.captured(1);
                            att = rest;
                        }
                    }

                    SKGObjectBase obj;
                    SKGError err = getDocument()->getObject(table, "id=" % refId, obj);
                    IFOK(err)
                    {
                        output = obj.getAttribute(att);
                    }
                }
            }
        }
    }

    return output;
}

bool SKGObjectBase::exist() const
{
    SKGTRACEINFUNC(20)

    SKGStringListList result;
    QString wc = getWhereclauseId();
    if (wc.isEmpty() && d->id != 0) {
        wc = "id=" % SKGServices::intToString(d->id);
    }
    if (wc.isEmpty()) {
        return false;
    }

    QString sql = "SELECT count(1) FROM " % d->table % " WHERE " % wc;
    if (getDocument() != nullptr) {
        getDocument()->executeSelectSqliteOrder(sql, result, false);
    }
    return (result.size() >= 2 && result.at(1).at(0) != QStringLiteral("0"));
}

SKGError SKGObjectBase::load()
{
    SKGError err;
    SKGTRACEINFUNCRC(20, err)

    if (Q_LIKELY(getDocument() && !getTable().isEmpty())) {
        // Prepare where clause
        QString wc = getWhereclauseId();
        if (wc.isEmpty()) {
            wc = "id=" % SKGServices::intToString(d->id);
        }

        // Execute sql order
        SKGStringListList result;
        err = getDocument()->executeSelectSqliteOrder("SELECT * FROM " % d->table % " WHERE " % wc, result, false);
        IFOK(err)
        {
            int size = result.size();
            if (Q_UNLIKELY(size == 1)) {
                err = SKGError(ERR_INVALIDARG,
                               i18nc("Error message: Could not load something because it is not in the database",
                                     "Load of '%1' with '%2' failed because it was not found in the database",
                                     d->table,
                                     wc));
            } else if (Q_UNLIKELY(size != 2)) {
                err = SKGError(ERR_INVALIDARG,
                               i18np("Load of '%2' with '%3' failed because of bad size of result (found one object)",
                                     "Load of '%2' with '%3' failed because of bad size of result (found %1 objects)",
                                     size - 1,
                                     d->table,
                                     wc));
            } else {
                SKGStringListList::const_iterator itrow = result.constBegin();
                QStringList columns = *(itrow);
                ++itrow;
                QStringList values = *(itrow);
                err = setAttributes(columns, values);
            }
        }
    }
    return err;
}

QString SKGObjectBase::getWhereclauseId() const
{
    int id = getID();
    if (id != 0) {
        return "id=" % SKGServices::intToString(id);
    }
    return QString();
}

SKGError SKGObjectBase::save(bool iInsertOrUpdate, bool iReloadAfterSave)
{
    SKGError err;
    SKGTRACEINFUNCRC(20, err)

    if (Q_UNLIKELY(!d->document)) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "Transaction impossible because the document is missing"));
    } else {
        // Save linking objects
        int nb = d->objects.count();
        for (int i = 0; !err && i < nb; ++i) {
            SKGObjectBase ref = d->objects.at(i);
            err = ref.save(iInsertOrUpdate, iReloadAfterSave);
        }

        // Check if we are in a transaction
        IFOKDO(err, d->document->checkExistingTransaction())
        IFOK(err)
        {
            // Table to use
            QString tablename = getRealTable();

            // Build order
            QString part1Insert;
            QString part2Insert;
            QString partUpdate;

            SKGQStringQStringMap::const_iterator it;
            for (it = d->attributes.constBegin(); it != d->attributes.constEnd(); ++it) {
                QString att = SKGServices::stringToSqlString(it.key());
                QString attlower = att.toLower();
                if (att.length() > 2 && att == attlower) { // We must ignore attributes coming from views
                    QString value = '\'' % SKGServices::stringToSqlString(it.value()) % u'\'';

                    if (!part1Insert.isEmpty()) {
                        part1Insert.append(u',');
                        part2Insert.append(u',');
                        partUpdate.append(u',');
                    }
                    // Attribute
                    part1Insert.append('\'' % att % u'\'');

                    // Value
                    part2Insert.append(value);

                    // Attribute=Value for update
                    partUpdate.append(att % u'=' % value);
                }
            }

            // We try an Insert
            if (d->id == 0) {
                // We have to try un insert
                err = getDocument()->executeSqliteOrder("INSERT INTO " % tablename % " (" % part1Insert % ") VALUES (" % part2Insert % u')', &(d->id));
            } else {
                // We must try an update
                err = SKGError(ERR_ABORT, QString()); // Just to go in UPDATE code
            }

            if (err && iInsertOrUpdate) {
                // INSERT failed, could we try an update ?
                QString wc = this->getWhereclauseId();
                if (!wc.isEmpty()) {
                    // Yes ==> Update
                    err = getDocument()->executeSqliteOrder("UPDATE " % tablename % " SET " % partUpdate % " WHERE " % wc);
                }
            }
        }
    }

    // Reload object is updated
    if (!err && iReloadAfterSave) {
        // The object has been updated ==>load
        err = load();
    }

    return err;
}

SKGError SKGObjectBase::remove(bool iSendMessage, bool iForce) const
{
    SKGError err;
    SKGTRACEINFUNCRC(20, err)
    if (Q_UNLIKELY(!d->document)) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "Transaction impossible because the document is missing"));
    } else {
        // Check if we are in a transaction
        err = d->document->checkExistingTransaction();

        // delete order
        QString viewForDelete = QStringLiteral("v_") % getRealTable() % "_delete";

        // Check if the delete view exist
        SKGStringListList temporaryResult;
        d->document->executeSelectSqliteOrder("PRAGMA table_info( " % viewForDelete % " );", temporaryResult);
        if (!iForce && temporaryResult.count() > 1) { // At least one attribute
            // Delete view exists, check if the delete is authorized
            err = d->document->executeSelectSqliteOrder("SELECT t_delete_message FROM " % viewForDelete % " WHERE id=" % SKGServices::intToString(d->id),
                                                        temporaryResult,
                                                        false);
            IFOK(err)
            {
                QString msg;
                if (temporaryResult.count() > 1) {
                    msg = temporaryResult.at(1).at(0);
                }
                // Should the string below be translated ??? It contains no word
                if (!msg.isEmpty()) {
                    err = SKGError(ERR_FORCEABLE, i18nc("Error message for an object", "'%1': %2", getDisplayName(), msg));
                }
            }
        }

        QString displayname = getDisplayName(); // Must be done before the delete order
        IFOKDO(err, d->document->executeSqliteOrder("DELETE FROM " % getRealTable() % " WHERE id=" % SKGServices::intToString(d->id)))
        if (iSendMessage && !err && !displayname.isEmpty()) {
            err = d->document->sendMessage(i18nc("An information to the user that something was deleted", "'%1' has been deleted", displayname),
                                           SKGDocument::Hidden);
        }
    }

    return err;
}

SKGError SKGObjectBase::dump() const
{
    // dump
    SKGTRACE << "=== START DUMP [" << getUniqueID() << "]===" << Qt::endl;
    SKGQStringQStringMap::const_iterator it;
    for (it = d->attributes.constBegin(); it != d->attributes.constEnd(); ++it) {
        SKGTRACE << it.key() << "=[" << it.value() << ']' << Qt::endl;
    }
    SKGTRACE << "=== END DUMP [" << getUniqueID() << "]===" << Qt::endl;
    return SKGError();
}

QStringList SKGObjectBase::getProperties() const
{
    return Q_UNLIKELY(!getDocument()) ? QStringList() : getDocument()->getParameters(getUniqueID());
}

QString SKGObjectBase::getProperty(const QString &iName) const
{
    return Q_UNLIKELY(!getDocument()) ? QString() : getDocument()->getParameter(iName, getUniqueID());
}

SKGObjectBase SKGObjectBase::getPropertyObject(const QString &iName) const
{
    SKGObjectBase property;
    if (getDocument() != nullptr) {
        getDocument()->getObject(QStringLiteral("parameters"),
                                 "t_name='" % SKGServices::stringToSqlString(iName) % "' AND t_uuid_parent='" % SKGServices::stringToSqlString(getUniqueID())
                                     % u'\'',
                                 property);
    }
    return property;
}

QVariant SKGObjectBase::getPropertyBlob(const QString &iName) const
{
    return Q_UNLIKELY(!getDocument()) ? QVariant() : getDocument()->getParameterBlob(iName, getUniqueID());
}

SKGError SKGObjectBase::setProperty(const QString &iName, const QString &iValue, const QString &iFileName, SKGPropertyObject *oObjectCreated) const
{
    SKGError err = Q_UNLIKELY(!getDocument()) ? SKGError() : getDocument()->setParameter(iName, iValue, iFileName, getUniqueID(), oObjectCreated);

    // Send message
    IFOKDO(err,
           Q_UNLIKELY(!getDocument())
               ? SKGError()
               : getDocument()->sendMessage(i18nc("An information to the user", "The property '%1=%2' has been added on '%3'", iName, iValue, getDisplayName()),
                                            SKGDocument::Hidden))

    return err;
}

SKGError SKGObjectBase::setProperty(const QString &iName, const QString &iValue, const QVariant &iBlob, SKGPropertyObject *oObjectCreated) const
{
    SKGError err = Q_UNLIKELY(!getDocument()) ? SKGError() : getDocument()->setParameter(iName, iValue, iBlob, getUniqueID(), oObjectCreated);

    // Send message
    IFOKDO(err,
           Q_UNLIKELY(!getDocument())
               ? SKGError()
               : getDocument()->sendMessage(i18nc("An information to the user", "The property '%1=%2' has been added on '%3'", iName, iValue, getDisplayName()),
                                            SKGDocument::Hidden))

    return err;
}
