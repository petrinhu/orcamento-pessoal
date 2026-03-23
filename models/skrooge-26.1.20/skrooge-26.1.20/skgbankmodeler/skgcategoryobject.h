/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGCATEGORYOBJECT_H
#define SKGCATEGORYOBJECT_H
/** @file
 * This file defines classes SKGCategoryObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgnamedobject.h"
class SKGDocumentBank;
/**
 * This class manages category object
 */
class SKGBANKMODELER_EXPORT SKGCategoryObject final : public SKGNamedObject
{
public:
    /**
     * Default constructor
     */
    explicit SKGCategoryObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGCategoryObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGCategoryObject(const SKGCategoryObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGCategoryObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGCategoryObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGCategoryObject &operator=(const SKGCategoryObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGCategoryObject();

    /**
     * Create a category branch if needed and return the leaf of the branch
     * @param iDocument the document where to create
     * @param iFullPath the full path. Example: cat1 > cat2 > cat3
     * @param oCategory the leaf of the branch
     * @param iSendPopupMessageOnCreation to send a creation message if the category is created
     * @param iRenameIfAlreadyExist if a leaf with the expected name already exist than the leaf will be renamed and created
     * @return an object managing the error.
     *   @see SKGError
     */
    static SKGError createPathCategory(SKGDocumentBank *iDocument,
                                       const QString &iFullPath,
                                       SKGCategoryObject &oCategory,
                                       bool iSendPopupMessageOnCreation = false,
                                       bool iRenameIfAlreadyExist = false);

    /**
     * Set the name of this object
     * @param iName the name
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setName(const QString &iName) override;

    /**
     * To bookmark or not a category
     * @param iBookmark the bookmark: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError bookmark(bool iBookmark);

    /**
     * To know if the category is bookmarked
     * @return an object managing the error
     *   @see SKGError
     */
    bool isBookmarked() const;

    /**
     * To set the closed attribute of a payee
     * @param iClosed the closed attribute: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setClosed(bool iClosed);

    /**
     * To know if the payee has been closed or not
     * @return an object managing the error
     *   @see SKGError
     */
    virtual bool isClosed() const;

    /**
     * Get the full name of this category.
     * The full name is the unique name of the category.
     * It is computed by the concatenation of names for all
     * the fathers of this category.
     * @return the full name
     */
    QString getFullName() const;

    /**
     * Add a category
     * @param oCategory the created category
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError addCategory(SKGCategoryObject &oCategory);

    /**
     * Move the category by changing the parent
     * @param iCategory the parent category
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setParentCategory(const SKGCategoryObject &iCategory);

    /**
     * Remove the parent category. The category will be a root.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError removeParentCategory();

    /**
     * Get the parent category
     * @param oCategory the parent category
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getParentCategory(SKGCategoryObject &oCategory) const;

    /**
     * Get the root category
     * @param oCategory the root category
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getRootCategory(SKGCategoryObject &oCategory) const;

    /**
     * Get categories
     * @param oCategoryList the list of categories under the current one
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getCategories(SKGListSKGObjectBase &oCategoryList) const;

    /**
     * Get the current amount
     * @return the current amount
     */
    double getCurrentAmount() const;

    /**
     * Get all sub transactions of this category
     * @param oSubOperations all sub transactions of this category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getSubOperations(SKGListSKGObjectBase &oSubOperations) const;

    /**
     * Merge iCategory in current category
     * @param iCategory the category. All sub transactions will be transferred into this category. The category will be removed
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError merge(const SKGCategoryObject &iCategory);

protected:
    /**
     * Get where clause needed to identify objects.
     * For this class, the whereclause is based on name + rd_category_id
     * @return the where clause
     */
    QString getWhereclauseId() const override;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGCategoryObject, Q_MOVABLE_TYPE);
#endif
