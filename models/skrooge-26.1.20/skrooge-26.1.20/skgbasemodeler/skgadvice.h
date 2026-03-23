/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGADVICE_H
#define SKGADVICE_H
/** @file
 * This file defines classes SKGAdvice .
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qmetatype.h>
#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qvector.h>

#include "skgbasemodeler_export.h"

/**
 * This class manages errors
 */
class SKGBASEMODELER_EXPORT SKGAdvice final : public QObject
{
    Q_OBJECT
public:
    /**
     * Advice action
     */
    struct SKGAdviceAction {
        /** The id of the action */
        QString id;

        /** The title of the action */
        QString Title;

        /** The name of the icon of the action */
        QString IconName;

        /** To know if this action is recommended*/
        bool IsRecommended{};
    };

    /**
     * List of advice action
     */
    using SKGAdviceActionList = QVector<SKGAdvice::SKGAdviceAction>;

    /**
     * Priority
     */
    Q_PROPERTY(int priority READ getPriority WRITE setPriority NOTIFY modified)
    /**
     * Unique identifier
     */
    Q_PROPERTY(QString uuid READ getUUID WRITE setUUID NOTIFY modified)
    /**
     * Short message
     */
    Q_PROPERTY(QString shortMessage READ getShortMessage WRITE setShortMessage NOTIFY modified)
    /**
     * Long message
     */
    Q_PROPERTY(QString longMessage READ getLongMessage WRITE setLongMessage NOTIFY modified)
    /**
     * Auto corrections
     */
    Q_PROPERTY(SKGAdvice::SKGAdviceActionList autoCorrections READ getAutoCorrections WRITE setAutoCorrections NOTIFY modified)

    /**
     * Constructor
     */
    explicit SKGAdvice();

    /**
     * Copy constructor
     * @param iAdvice the advice to copy
     */
    SKGAdvice(const SKGAdvice &iAdvice);

    /**
     * Destructor
     */
    ~SKGAdvice() override;

    /**
     * Operator affectation
     * @param iAdvice the advice to copy
     */
    SKGAdvice &operator=(const SKGAdvice &iAdvice);

    /**
     * Return the unique identifier
     * @return the unique identifier
     */
    QString getUUID() const;

    /**
     * Return the priority
     * @return the priority
     */
    int getPriority() const;

    /**
     * Return the short message
     * @return the short message
     */
    QString getShortMessage() const;

    /**
     * Return the long message
     * @return the long message
     */
    QString getLongMessage() const;

    /**
     * Return the auto corrections
     * @return the auto corrections
     */
    SKGAdvice::SKGAdviceActionList getAutoCorrections() const;

public Q_SLOTS:
    /**
     * Set the unique identifier
     * @param iUUID the unique identifier
     */
    void setUUID(const QString &iUUID);

    /**
     * Set the priority
     * @param iPriority the priority
     */
    void setPriority(int iPriority);

    /**
     * Set the short message
     * @param iMessage the short message
     */
    void setShortMessage(const QString &iMessage);

    /**
     * Set the long message
     * @param iMessage the long message
     */
    void setLongMessage(const QString &iMessage);

    /**
     * Set the auto corrections
     * @param iCorrections the auto corrections
     */
    void setAutoCorrections(const QStringList &iCorrections);

    /**
     * Set the auto corrections
     * @param iCorrections the auto corrections
     */
    void setAutoCorrections(const SKGAdvice::SKGAdviceActionList &iCorrections);

Q_SIGNALS:
    /**
     * This signal is launched when the object is modified
     */
    void modified();

private:
    /**
     * the unique identifier
     */
    QString m_uuid;

    /**
     * the priority
     */
    int m_priority{1};

    /**
     * the short message
     */
    QString m_shortMessage;

    /**
     * the short message
     */
    QString m_longMessage;

    /**
     * the list of auto corrections
     */
    SKGAdvice::SKGAdviceActionList m_autoCorrections;
};
/**
 * Declare the meta type
 */
Q_DECLARE_METATYPE(SKGAdvice)

/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGAdvice, Q_MOVABLE_TYPE);

/**
 * the SKGAdviceList
 */
using SKGAdviceList = QVector<SKGAdvice>;

/**
 * Declare the meta type
 */
Q_DECLARE_METATYPE(QVector<SKGAdvice>)
#endif // SKGADVICE_H
