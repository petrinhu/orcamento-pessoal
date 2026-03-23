/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGCategoryObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgcategoryobject.h"

#include <klocalizedstring.h>

#include "skgdocumentbank.h"
#include "skgsuboperationobject.h"
#include "skgtraces.h"

SKGCategoryObject::SKGCategoryObject()
    : SKGCategoryObject(nullptr)
{
}

SKGCategoryObject::SKGCategoryObject(SKGDocument *iDocument, int iID)
    : SKGNamedObject(iDocument, QStringLiteral("v_category"), iID)
{
}

SKGCategoryObject::~SKGCategoryObject() = default;

SKGCategoryObject::SKGCategoryObject(const SKGCategoryObject &iObject) = default;

SKGCategoryObject::SKGCategoryObject(const SKGObjectBase &iObject)

{
    if (iObject.getRealTable() == QStringLiteral("category")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_category"), iObject.getID());
    }
}

SKGCategoryObject &SKGCategoryObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGCategoryObject &SKGCategoryObject::operator=(const SKGCategoryObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGCategoryObject::setName(const QString &iName)
{
    SKGError err;
    if (iName.contains(OBJECTSEPARATOR)) {
        err = SKGError(ERR_FAIL, i18nc("Error message", "Invalid name '%1' because of the name cannot contain '%2'", iName, OBJECTSEPARATOR));
    } else {
        err = SKGNamedObject::setName(iName);
    }
    return err;
}

QString SKGCategoryObject::getWhereclauseId() const
{
    // Could we use the id
    QString output = SKGObjectBase::getWhereclauseId(); // clazy:exclude=skipped-base-method
    if (output.isEmpty()) {
        if (!(getAttribute(QStringLiteral("t_name")).isEmpty())) {
            output = "t_name='" % SKGServices::stringToSqlString(getAttribute(QStringLiteral("t_name"))) % u'\'';
        }
        QString rd_category_id = getAttribute(QStringLiteral("rd_category_id"));
        if (!output.isEmpty()) {
            output += QStringLiteral(" AND ");
        }
        if (rd_category_id.isEmpty()) {
            output += QStringLiteral("(rd_category_id=0 OR rd_category_id IS NULL OR rd_category_id='')");
        } else {
            output += "rd_category_id=" % rd_category_id;
        }
    }
    return output;
}

QString SKGCategoryObject::getFullName() const
{
    return getAttribute(QStringLiteral("t_fullname"));
}

SKGError SKGCategoryObject::createPathCategory(SKGDocumentBank *iDocument,
                                               const QString &iFullPath,
                                               SKGCategoryObject &oCategory,
                                               bool iSendPopupMessageOnCreation,
                                               bool iRenameIfAlreadyExist)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Check if category is already existing
    if (iFullPath.isEmpty()) {
        oCategory = SKGCategoryObject(nullptr, 0);
    } else if (iDocument != nullptr) {
        if (!iRenameIfAlreadyExist) {
            iDocument->getObject(QStringLiteral("v_category"), "t_fullname='" % SKGServices::stringToSqlString(iFullPath) % u'\'', oCategory);
        } else {
            oCategory.resetID();
        }
        if (oCategory.getID() == 0) {
            // No, we have to create it
            // Search category separator
            int posSeparator = iFullPath.lastIndexOf(OBJECTSEPARATOR);
            if (posSeparator == -1) {
                oCategory = SKGCategoryObject(iDocument);
                err = oCategory.setName(iFullPath);

                // Check if already existing
                if (!err && iRenameIfAlreadyExist) {
                    int index = 1;
                    while (!err && oCategory.exist()) {
                        index++;
                        err = oCategory.setName(iFullPath % " (" % SKGServices::intToString(index) % u')');
                    }
                }

                IFOKDO(err, oCategory.save())
            } else {
                // Get first and second parts of the branch
                QString first = iFullPath.mid(0, posSeparator);
                QString second = iFullPath.mid(posSeparator + OBJECTSEPARATOR.length(), iFullPath.length() - posSeparator - OBJECTSEPARATOR.length());

                // Get first category
                SKGCategoryObject FirstCategory;
                err = SKGCategoryObject::createPathCategory(iDocument, first, FirstCategory);

                IFOK(err)
                {
                    // Get second category
                    err = FirstCategory.addCategory(oCategory);

                    // Add second under first
                    IFOKDO(err, oCategory.setName(second))

                    // Check if already existing
                    if (!err && iRenameIfAlreadyExist) {
                        int index = 2;
                        while (!err && oCategory.exist()) {
                            err = oCategory.setName(second % " (" % SKGServices::intToString(index) % u')');
                            ++index;
                        }
                    }

                    // save
                    IFOKDO(err, oCategory.save())
                }
            }

            if (!err && iSendPopupMessageOnCreation) {
                iDocument->sendMessage(i18nc("Information message", "The category '%1' has been created", iFullPath), SKGDocument::Positive);
            }
        }
    }

    return err;
}

SKGError SKGCategoryObject::addCategory(SKGCategoryObject &oCategory)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (getID() == 0) {
        err = SKGError(
            ERR_FAIL,
            i18nc("Error message", "%1 failed because linked object is not yet saved in the database.", QStringLiteral("SKGCategoryObject::addCategory")));
    } else {
        oCategory = SKGCategoryObject(qobject_cast<SKGDocumentBank *>(getDocument()));
        err = oCategory.setAttribute(QStringLiteral("rd_category_id"), SKGServices::intToString(getID()));
    }
    return err;
}

SKGError SKGCategoryObject::setParentCategory(const SKGCategoryObject &iCategory)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (iCategory.getID() == 0) {
        err = SKGError(ERR_FAIL,
                       i18nc("Error message",
                             "%1 failed because linked object is not yet saved in the database.",
                             QStringLiteral("SKGCategoryObject::setParentCategory")));
    } else {
        // Check if it is a loop
        SKGCategoryObject current = iCategory;
        do {
            if (current == *this) {
                err = SKGError(ERR_FAIL, i18nc("Error message", "You cannot create a loop."));
            } else {
                SKGCategoryObject parent2;
                current.getParentCategory(parent2);
                current = parent2;
            }
        } while (!err && current.getID() != 0);

        IFOKDO(err, setAttribute(QStringLiteral("rd_category_id"), SKGServices::intToString(iCategory.getID())))
    }
    return err;
}

SKGError SKGCategoryObject::removeParentCategory()
{
    return setAttribute(QStringLiteral("rd_category_id"), QStringLiteral("0"));
}

SKGError SKGCategoryObject::getParentCategory(SKGCategoryObject &oCategory) const
{
    SKGError err;
    QString parent_id = getAttribute(QStringLiteral("rd_category_id"));
    if (!parent_id.isEmpty() && parent_id != QStringLiteral("0")) {
        err = getDocument()->getObject(QStringLiteral("v_category"), "id=" % parent_id, oCategory);
    }
    return err;
}

SKGError SKGCategoryObject::getRootCategory(SKGCategoryObject &oCategory) const
{
    SKGError err;
    SKGCategoryObject parent2;
    err = getParentCategory(parent2);
    IFOK(err)
    {
        if (!parent2.exist()) {
            // No parent
            oCategory = *this;
        } else {
            // Parent exist
            err = parent2.getRootCategory(oCategory);
        }
    }
    return err;
}

SKGError SKGCategoryObject::getCategories(SKGListSKGObjectBase &oCategoryList) const
{
    return getDocument()->getObjects(QStringLiteral("v_category"), "rd_category_id=" % SKGServices::intToString(getID()), oCategoryList);
}

double SKGCategoryObject::getCurrentAmount() const
{
    QString v = getAttribute(QStringLiteral("f_SUMCURRENTAMOUNT"));
    if (v.isEmpty()) {
        SKGNamedObject cat(getDocument(), QStringLiteral("v_category_display"), getID());
        v = cat.getAttribute(QStringLiteral("f_SUMCURRENTAMOUNT"));
    }
    return SKGServices::stringToDouble(v);
}

SKGError SKGCategoryObject::getSubOperations(SKGListSKGObjectBase &oSubOperations) const
{
    SKGError err = getDocument()->getObjects(QStringLiteral("v_suboperation"), "r_category_id=" % SKGServices::intToString(getID()), oSubOperations);
    return err;
}

SKGError SKGCategoryObject::bookmark(bool iBookmark)
{
    return setAttribute(QStringLiteral("t_bookmarked"), iBookmark ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGCategoryObject::isBookmarked() const
{
    return (getAttribute(QStringLiteral("t_bookmarked")) == QStringLiteral("Y"));
}

SKGError SKGCategoryObject::setClosed(bool iClosed)
{
    return setAttribute(QStringLiteral("t_close"), iClosed ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGCategoryObject::isClosed() const
{
    return (getAttribute(QStringLiteral("t_close")) == QStringLiteral("Y"));
}

SKGError SKGCategoryObject::merge(const SKGCategoryObject &iCategory)
{
    SKGError err;

    SKGObjectBase::SKGListSKGObjectBase ops;
    IFOKDO(err, iCategory.getSubOperations(ops))
    int nb = ops.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGSubOperationObject op(ops.at(i));
        err = op.setCategory(*this);
        IFOKDO(err, op.save(true, false))
    }

    SKGObjectBase::SKGListSKGObjectBase cats;
    IFOKDO(err, iCategory.getCategories(cats))
    nb = cats.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGCategoryObject cat(cats.at(i));
        err = cat.setParentCategory(*this);
        IFOKDO(err, cat.save(true, false))
    }

    IFOKDO(err, iCategory.remove(false))
    return err;
}
