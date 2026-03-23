/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGPropertyObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgpropertyobject.h"
#include <qdir.h>

SKGPropertyObject::SKGPropertyObject()
    : SKGPropertyObject(nullptr)
{
}

SKGPropertyObject::SKGPropertyObject(SKGDocument *iDocument, int iID)
    : SKGNamedObject(iDocument, QStringLiteral("parameters"), iID)
{
}

SKGPropertyObject::~SKGPropertyObject() = default;

SKGPropertyObject::SKGPropertyObject(const SKGPropertyObject &iObject) = default;

SKGPropertyObject::SKGPropertyObject(const SKGObjectBase &iObject)
    : SKGNamedObject(iObject)
{
}

SKGPropertyObject &SKGPropertyObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGPropertyObject &SKGPropertyObject::operator=(const SKGPropertyObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGPropertyObject::setParentId(const QString &iParentId)
{
    return setAttribute(QStringLiteral("t_uuid_parent"), iParentId);
}

QString SKGPropertyObject::getParentId() const
{
    return getAttribute(QStringLiteral("t_uuid_parent"));
}

SKGError SKGPropertyObject::setValue(const QString &iValue)
{
    return setAttribute(QStringLiteral("t_value"), iValue);
}

QString SKGPropertyObject::getValue() const
{
    return getAttribute(QStringLiteral("t_value"));
}

QUrl SKGPropertyObject::getUrl(bool iBuildTemporaryFile) const
{
    QUrl url;
    if (getID() != 0) {
        QStringList uuid = getParentId().split(u'-');
        if (uuid.count() == 2) {
            SKGObjectBase p(getDocument(), uuid.at(1), SKGServices::stringToInt(uuid.at(0)));
            QVariant blob = p.getPropertyBlob(getName());

            // Is it a copied file ?
            if (!blob.isNull()) {
                QByteArray blob_bytes = blob.toByteArray();
                if (blob_bytes.length() > 0) {
                    // Yes, this is a file
                    QString fileName = QDir::tempPath() % u'/' % QFileInfo(getValue()).fileName();
                    if (iBuildTemporaryFile) {
                        // Save temporary file
                        QFile file(fileName);
                        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
                        if (file.open(QIODevice::WriteOnly)) {
                            file.write(blob_bytes);
                            file.flush();
                            file.close();
                            file.setPermissions(QFile::ReadOwner); // To be sure that no modifications are done
                        }
                    }
                    url = QUrl::fromLocalFile(fileName);
                }
            } else if (QFile(getValue()).exists()) {
                // Is it a linked file? Yes
                url = QUrl::fromLocalFile(getValue());
            } else {
                // Is it a linked file?  No, Is it a http url ?
                QUrl url2 = QUrl(getValue());
                if (!url2.scheme().isEmpty()) {
                    url = std::move(url2);
                }
            }
        }
    }
    return url;
}

QString SKGPropertyObject::getWhereclauseId() const
{
    // Could we use the id
    QString output = SKGObjectBase::getWhereclauseId(); // clazy:exclude=skipped-base-method
    if (output.isEmpty()) {
        if (!(getAttribute(QStringLiteral("t_name")).isEmpty())) {
            output = "t_name='" % SKGServices::stringToSqlString(getAttribute(QStringLiteral("t_name"))) % u'\'';
        }
        if (!(getParentId().isEmpty())) {
            if (!output.isEmpty()) {
                output += QStringLiteral(" AND ");
            }
            output += "t_uuid_parent='" % getParentId() % u'\'';
        }
    }
    return output;
}
