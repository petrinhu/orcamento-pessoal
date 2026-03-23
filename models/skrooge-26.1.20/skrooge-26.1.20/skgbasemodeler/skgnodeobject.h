/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGNODEOBJECT_H
#define SKGNODEOBJECT_H
/** @file
 * This file defines classes SKGNodeObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgdefine.h"
#include "skgnamedobject.h"

class SKGDocument;
/**
 * This class manages node object
 */
class SKGBASEMODELER_EXPORT SKGNodeObject final : public SKGNamedObject
{
    /**
     * Order of the node
     */
    Q_PROPERTY(double order READ getOrder WRITE setOrder) // clazy:exclude=qproperty-without-notify
    /**
     * Full name of the node
     */
    Q_PROPERTY(QString fullName READ getFullName) // clazy:exclude=qproperty-without-notify
    /**
     * Data of the node
     */
    Q_PROPERTY(QString data READ getData WRITE setData) // clazy:exclude=qproperty-without-notify

public:
    /**
     * Indicates if a node is opened
     */
    bool opened;

    /**
     * default constructor
     */
    explicit SKGNodeObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGNodeObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGNodeObject(const SKGNodeObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGNodeObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGNodeObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGNodeObject &operator=(const SKGNodeObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGNodeObject();

    /**
     * Create a node branch if needed and return the leaf of the branch
     * @param iDocument the document where to create
     * @param iFullPath the full path. Example: cat1|cat2|cat3
     * @param oNode the leaf of the branch
     * @param iRenameIfAlreadyExist if a leaf with the expected name already exist than the leaf will be renamed and created
     * @return an object managing the error.
     *   @see SKGError
     */
    static SKGError createPathNode(SKGDocument *iDocument, const QString &iFullPath, SKGNodeObject &oNode, bool iRenameIfAlreadyExist = false);

    /**
     * Set the name of this object
     * @param iName the name
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setName(const QString &iName) override;

    /**
     * Get the full name of this node.
     * The full name is the unique name of the node.
     * It is computed by the concatenation of names for all
     * the fathers of this node.
     * @return the full name
     */
    QString getFullName() const;

    /**
     * Add a node
     * @param oNode the created node
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError addNode(SKGNodeObject &oNode);

    /**
     * Move the node by changing the parent
     * @param iNode the parent node
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setParentNode(const SKGNodeObject &iNode);

    /**
     * Get the parent node
     * @param oNode the parent node
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getParentNode(SKGNodeObject &oNode) const;

    /**
     * Remove the parent node. The node will be a root.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError removeParentNode();

    /**
     * Get nodes
     * @param oNodeList the list of nodes under the current one
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getNodes(SKGListSKGObjectBase &oNodeList) const;

    /**
     * Set the order for the node in its parent
     * @param iOrder the order. (-1 means "at the end")
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setOrder(double iOrder);

    /**
     * Get the order for the node in its parent
     * @return the order
     */
    double getOrder() const;

    /**
     * Set data of this node
     * @param iData the data
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setData(const QString &iData);

    /**
     * Get data of this node
     * @return the data
     */
    QString getData() const;

    /**
     * To know if the node is a folder or not
     * @return true of false
     */
    bool isFolder() const;

    /**
     * Set icon of this node
     * @param iIcon the icon name
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setIcon(const QString &iIcon);

    /**
     * Get icon of this node
     * @return the icon
     */
    QIcon getIcon() const;

    /**
     * Set autostart mode of this node
     * @param iAutoStart the autostart mode
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setAutoStart(bool iAutoStart);

    /**
     * Get autostart mode of this node
     * @return the autostart mode
     */
    bool isAutoStart() const;

protected:
    /**
     * Get where clause needed to identify objects.
     * For this class, the whereclause is based on name + rd_node_id
     * @return the where clause
     */
    QString getWhereclauseId() const override;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGNodeObject, Q_MOVABLE_TYPE);
#endif
