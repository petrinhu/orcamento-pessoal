/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGNodeObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgnodeobject.h"

#include <klocalizedstring.h>
#include <qicon.h>

#include "skgdefine.h"
#include "skgdocument.h"
#include "skgtraces.h"

SKGNodeObject::SKGNodeObject()
    : SKGNodeObject(nullptr)
{
}

SKGNodeObject::SKGNodeObject(SKGDocument *iDocument, int iID)
    : SKGNamedObject(iDocument, QStringLiteral("v_node"), iID)
    , opened(false)
{
}

SKGNodeObject::~SKGNodeObject() = default;

SKGNodeObject::SKGNodeObject(const SKGNodeObject &iObject)
    : SKGNamedObject(iObject)
    , opened(false)
{
}

SKGNodeObject::SKGNodeObject(const SKGObjectBase &iObject)
    : SKGNamedObject(iObject.getDocument(), QStringLiteral("v_node"), iObject.getID())
    , opened(false)
{
}

SKGNodeObject &SKGNodeObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGNodeObject &SKGNodeObject::operator=(const SKGNodeObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGNodeObject::setName(const QString &iName)
{
    SKGError err;
    if (iName.contains(OBJECTSEPARATOR)) {
        err = SKGError(
            ERR_FAIL,
            i18nc("Error message: an invalid character was found", "The name '%1' is invalid : the '%2' character is forbidden ", iName, OBJECTSEPARATOR));
    } else {
        err = SKGNamedObject::setName(iName);
    }
    return err;
}

QString SKGNodeObject::getWhereclauseId() const
{
    // Could we use the id
    QString output = SKGObjectBase::getWhereclauseId(); // clazy:exclude=skipped-base-method
    if (output.isEmpty()) {
        if (!(getAttribute(QStringLiteral("t_name")).isEmpty())) {
            output = QStringLiteral("t_name='") % SKGServices::stringToSqlString(getAttribute(QStringLiteral("t_name"))) % u'\'';
        }
        QString rd_node_id = getAttribute(QStringLiteral("rd_node_id"));
        if (!output.isEmpty()) {
            output += QStringLiteral(" AND ");
        }
        if (rd_node_id.isEmpty()) {
            output += QStringLiteral("(rd_node_id=0 OR rd_node_id IS NULL OR rd_node_id='')");
        } else {
            output += QStringLiteral("rd_node_id=") % rd_node_id;
        }
    }
    return output;
}

QString SKGNodeObject::getFullName() const
{
    return getAttribute(QStringLiteral("t_fullname"));
}

SKGError SKGNodeObject::setData(const QString &iData)
{
    return setAttribute(QStringLiteral("t_data"), iData);
}

QString SKGNodeObject::getData() const
{
    return getAttribute(QStringLiteral("t_data"));
}

bool SKGNodeObject::isFolder() const
{
    return getData().isEmpty();
}

SKGError SKGNodeObject::setIcon(const QString &iIcon)
{
    return setAttribute(QStringLiteral("t_icon"), iIcon);
}

QIcon SKGNodeObject::getIcon() const
{
    QStringList overlay;
    if (isAutoStart()) {
        overlay.push_back(QStringLiteral("media-playback-start"));
    }
    return SKGServices::fromTheme(getAttribute(QStringLiteral("t_icon")), overlay);
}

SKGError SKGNodeObject::setAutoStart(bool iAutoStart)
{
    return setAttribute(QStringLiteral("t_autostart"), iAutoStart ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGNodeObject::isAutoStart() const
{
    return (getAttribute(QStringLiteral("t_autostart")) == QStringLiteral("Y"));
}

SKGError SKGNodeObject::setOrder(double iOrder)
{
    SKGError err;
    double order = iOrder;
    if (order == -1) {
        order = 1;
        SKGStringListList result;
        err = getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT max(f_sortorder) from node"), result);
        if (!err && result.count() == 2) {
            order = SKGServices::stringToDouble(result.at(1).at(0)) + 1;
        }
    }
    IFOKDO(err, setAttribute(QStringLiteral("f_sortorder"), SKGServices::doubleToString(order)))
    return err;
}

double SKGNodeObject::getOrder() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_sortorder")));
}

SKGError SKGNodeObject::createPathNode(SKGDocument *iDocument, const QString &iFullPath, SKGNodeObject &oNode, bool iRenameIfAlreadyExist)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    SKGTRACEL(10) << "Input parameter [iFullPath]=" << iFullPath << Qt::endl;
    // Check if node is already existing
    if (!iRenameIfAlreadyExist && iDocument != nullptr) {
        iDocument->getObject(QStringLiteral("v_node"), "t_fullname='" % SKGServices::stringToSqlString(iFullPath) % u'\'', oNode);
    }
    if (oNode.getID() == 0) {
        // No, we have to create it
        // Search node separator
        int posSeparator = iFullPath.lastIndexOf(OBJECTSEPARATOR);
        if (posSeparator == -1) {
            oNode = SKGNodeObject(iDocument);
            err = oNode.setName(iFullPath);

            // Check if already existing
            if (!err && iRenameIfAlreadyExist) {
                int index = 1;
                while (!err && oNode.exist()) {
                    index++;
                    err = oNode.setName(iFullPath % " (" % SKGServices::intToString(index) % u')');
                }
            }

            IFOKDO(err, oNode.setIcon(QStringLiteral("folder-orange")))
            IFOKDO(err, oNode.setOrder(-1))
            IFOKDO(err, oNode.save())
        } else {
            // Get first and second parts of the branch
            QString first = iFullPath.mid(0, posSeparator);
            QString second = iFullPath.mid(posSeparator + OBJECTSEPARATOR.length(), iFullPath.length() - posSeparator - OBJECTSEPARATOR.length());

            // Get first node
            SKGNodeObject FirstNode;
            err = SKGNodeObject::createPathNode(iDocument, first, FirstNode);

            IFOK(err)
            {
                // Get second node
                err = FirstNode.addNode(oNode);

                // Add second under first
                IFOKDO(err, oNode.setName(second))

                // Check if already existing
                if (!err && iRenameIfAlreadyExist) {
                    int index = 2;
                    while (!err && oNode.exist()) {
                        err = oNode.setName(second % " (" % SKGServices::intToString(index) % u')');
                        ++index;
                    }
                }

                // save
                IFOKDO(err, oNode.setIcon(QStringLiteral("folder-orange")))
                IFOKDO(err, oNode.setOrder(-1))
                IFOKDO(err, oNode.save())
            }
        }
    }

    return err;
}

SKGError SKGNodeObject::addNode(SKGNodeObject &oNode)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (getID() == 0) {
        err = SKGError(ERR_FAIL,
                       i18nc("Error message: Something failed because of a database issue",
                             "%1 failed because linked object is not yet saved in the database.",
                             QStringLiteral("SKGNodeObject::addNode")));
    } else {
        oNode = SKGNodeObject(getDocument());
        err = oNode.setAttribute(QStringLiteral("rd_node_id"), SKGServices::intToString(getID()));
    }
    return err;
}

SKGError SKGNodeObject::removeParentNode()
{
    return setAttribute(QStringLiteral("rd_node_id"), QString());
}

SKGError SKGNodeObject::setParentNode(const SKGNodeObject &iNode)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (iNode.getID() == 0) {
        err = SKGError(ERR_FAIL,
                       i18nc("Error message: Something failed because of a database issue",
                             "%1 failed because linked object is not yet saved in the database.",
                             QStringLiteral("SKGNodeObject::setParentNode")));
    } else {
        // Check if it is a loop
        SKGNodeObject current = iNode;
        do {
            if (current == *this) {
                err = SKGError(
                    ERR_FAIL,
                    i18nc("Error message: Loops are forbidden in Skrooge data structures",
                          "You cannot create a loop, ie parent and child with the same name. For example, A > A is a loop. A > B > A is another kind of loop"));
            } else {
                SKGNodeObject parentNode;
                current.getParentNode(parentNode);
                current = parentNode;
            }
        } while (!err && current.getID() != 0);

        IFOKDO(err, setAttribute(QStringLiteral("rd_node_id"), SKGServices::intToString(iNode.getID())))
    }
    return err;
}

SKGError SKGNodeObject::getParentNode(SKGNodeObject &oNode) const
{
    SKGError err;
    QString parent_id = getAttribute(QStringLiteral("rd_node_id"));
    if (!parent_id.isEmpty()) {
        err = getDocument()->getObject(QStringLiteral("v_node"), "id=" % parent_id, oNode);
    } else {
        oNode = SKGNodeObject();
    }
    return err;
}

SKGError SKGNodeObject::getNodes(SKGListSKGObjectBase &oNodeList) const
{
    return getDocument()->getObjects(QStringLiteral("v_node"), "rd_node_id=" % SKGServices::intToString(getID()) % " ORDER BY f_sortorder, t_name", oNodeList);
}
