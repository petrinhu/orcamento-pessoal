/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGOBJECTBASE_H
#define SKGOBJECTBASE_H
/** @file
 * This file defines classes SKGObjectBase.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qlist.h>
#include <qstring.h>
#include <qvariant.h>

#include "skgdefine.h"
#include "skgerror.h"
#include "skgservices.h"

class SKGDocument;
class SKGPropertyObject;
class SKGObjectBasePrivate;

/**
 * This class is the base class for all objects.
 * This class is a generic way to manipulate objects.
 * Some important rules:
 * R1: Each table must have a view named v_[tablename] where [tablename] is the name of the table.
 * R2: Each computed attribute of the view must have a name starting with v_
 */
class SKGBASEMODELER_EXPORT SKGObjectBase
{
    /**
     * Unique ide  of the object
     */
    Q_PROPERTY(QString uniqueID READ getUniqueID CONSTANT)
    /**
     * Id of the object
     */
    Q_PROPERTY(int ID READ getID CONSTANT)
    /**
     * Table of the object
     */
    Q_PROPERTY(QString table READ getTable CONSTANT)

public:
    /**
     * A list of SKGObjectBase ==> SKGListSKGObjectBase
     */
    using SKGListSKGObjectBase = QVector<SKGObjectBase>;

    /**
     * An iterator for SKGListSKGObjectBase
     */
    using SKGListSKGObjectBaseIterator = QVector<SKGObjectBase>::Iterator;

    /**
     * Default constructor
     */
    explicit SKGObjectBase();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iTable the table of the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGObjectBase(SKGDocument *iDocument, const QString &iTable = QString(), int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGObjectBase(const SKGObjectBase &iObject);

    /**
     * Move constructor
     * @param iObject the object to copy
     */
    SKGObjectBase(SKGObjectBase &&iObject) noexcept;

    /**
     * Operator comparison
     * @param iObject The object to compare
     */
    virtual bool operator==(const SKGObjectBase &iObject) const;

    /**
     * Operator comparison
     * @param iObject The object to compare
     */
    virtual bool operator!=(const SKGObjectBase &iObject) const;

    /**
     * Operator comparison
     * @param iObject The object to compare
     */
    virtual bool operator<(const SKGObjectBase &iObject) const;

    /**
     * Operator comparison
     * @param iObject The object to compare
     */
    virtual bool operator>(const SKGObjectBase &iObject) const;

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGObjectBase &operator=(const SKGObjectBase &iObject);

    /**
     * Clone this object into a same document. The id is not kept. The cloned object is not saved into the target document
     */
    virtual SKGObjectBase cloneInto();

    /**
     * Clone this object into a new document. The id is not kept. The cloned object is not saved into the target document
     * @param iDocument the target document.
     */
    virtual SKGObjectBase cloneInto(SKGDocument *iDocument);

    /**
     * Destructor
     */
    virtual ~SKGObjectBase();

    /**
     * Return the unique id of this object
     * @return unique id of this object
     */
    virtual QString getUniqueID() const;

    /**
     * Return the id of this object
     * @return id of this object
     */
    virtual int getID() const;

    /**
     * Return the name of the object for the display
     * @return name of the object
     */
    virtual QString getDisplayName() const;

    /**
     * Reset the ID of this object.
     * It is used to create a new object based on an existing one.
     * By resetting the ID, the save will try an INSERT instead of an UPDATE.
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError resetID();

    /**
     * Return the table name of this object
     * @return table name of this object
     */
    virtual QString getTable() const;

    /**
     * Return the real table name of this object.
     * This is useful for modification (UPDATE, DELETE)
     * @return real table name of this object
     */
    virtual QString getRealTable() const;

    /**
     * Return the document of this object
     * @return document of this object
     */
    SKGDocument *getDocument() const;

    /**
     * Return the attributes of this object
     * @return attributes of this object
     */
    virtual SKGQStringQStringMap getAttributes() const;

    /**
     * Return the number of attributes
     * @return number of attributes
     */
    virtual int getNbAttributes() const;

    /**
     * Get the value for an attribute
     * @param iName the name of the attribute.
     * This can be:
     *   The name of an attribute (eg. t_name)
     *   The name of a property (eg. p_myproperty)
     *   The name of an attribute on a marked object (eg. rd_node_id.(v_node)t_name)
     * @return the value of this attribute
     */
    virtual QString getAttribute(const QString &iName) const;

    /**
     * Get the value for an attribute for another view
     * @param iView the name of the view
     * @param iName the name of the attribute
     * @return the value of this attribute
     */
    virtual QString getAttributeFromView(const QString &iView, const QString &iName) const;

    /**
     * Set the value for an attribute
     * @param iName the name of the attribute
     * @param iValue the value of the attribute
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setAttribute(const QString &iName, const QString &iValue);

    /**
     * Set the values for attributes
     * @param iNames list of attributes
     * @param iValues list of values
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setAttributes(const QStringList &iNames, const QStringList &iValues);

    /**
     * Get the list of properties
     * @return the list of properties
     */
    virtual QStringList getProperties() const;

    /**
     * Get the value for a property
     * @param iName the name of the property
     * @return the value of this property
     */
    virtual QString getProperty(const QString &iName) const;

    /**
     * Get the property
     * @param iName the name of the property
     * @return the property
     */
    virtual SKGObjectBase getPropertyObject(const QString &iName) const;

    /**
     * Get the blob for a property
     * @param iName the name of the property
     * @return the blob of this property
     */
    virtual QVariant getPropertyBlob(const QString &iName) const;

    /**
     * Set the value for a property
     * @param iName the property name
     * @param iValue the property value
     * @param iFileName the file name.
     * @param oObjectCreated the property object created. Can be nullptr
     * @return an object managing the property
     *   @see SKGError
     */
    virtual SKGError setProperty(const QString &iName, const QString &iValue, const QString &iFileName, SKGPropertyObject *oObjectCreated = nullptr) const;

    /**
     * Set the value for a property
     * @param iName the property name
     * @param iValue the property value
     * @param iBlob the property blob
     * @param oObjectCreated the property object created. Can be nullptr
     * @return an object managing the property
     *   @see SKGError
     */
    virtual SKGError
    setProperty(const QString &iName, const QString &iValue, const QVariant &iBlob = QVariant(), SKGPropertyObject *oObjectCreated = nullptr) const;

    /**
     * To know if an object exists or not
     * @return "true" if the object exists else "false".
     */
    virtual bool exist() const;

    /**
     * load or reload the object from the database
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError load();

    /**
     * save the object into the database
     * @param iInsertOrUpdate the save mode.
     *    true: try an insert, if the insert failed then try an update.
     *    false: try an insert, if the insert failed then return an error.
     * @param iReloadAfterSave to reload the object after save. Set false if you are sure that you will not use this object after save
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError save(bool iInsertOrUpdate = true, bool iReloadAfterSave = true);

    /**
     * delete the object into the database
     * @param iSendMessage to send message when the object is deleted
     * @param iForce to force the deletion
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError remove(bool iSendMessage = true, bool iForce = false) const;

    /**
     * dump the object
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError dump() const;

protected:
    /**
     * Copy from an existing object
     * @param iObject the object to copy
     */
    void copyFrom(const SKGObjectBase &iObject);

    /**
     * Get where clause needed to identify objects.
     * For this class, the whereclause is based on id
     * @return the where clause
     */
    virtual QString getWhereclauseId() const;

private:
    Q_GADGET
    SKGObjectBasePrivate *d;
};
/**
 * Declare the meta type
 */
Q_DECLARE_METATYPE(SKGObjectBase)

/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGObjectBase, Q_MOVABLE_TYPE);

/**
 * Declare the meta type
 */
Q_DECLARE_METATYPE(SKGObjectBase::SKGListSKGObjectBase)
#endif
