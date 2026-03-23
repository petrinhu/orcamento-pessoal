/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGERROR_H
#define SKGERROR_H
/** @file
 * This file defines classes SKGError and macros.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qobject.h>
#include <qstring.h>

#include "skgbasemodeler_export.h"
#include "skgdefine.h"

/**
 * To facilitate the error management
 */
#define IFKO(ERROR) if (Q_UNLIKELY(ERROR))

/**
 * To facilitate the error management
 */
#define IFOK(ERROR) if (Q_LIKELY(!(ERROR)))

/**
 * To facilitate the error management
 */
#define IFOKDO(ERROR, ACTION)                                                                                                                                  \
    IFOK(ERROR)                                                                                                                                                \
    {                                                                                                                                                          \
        (ERROR) = ACTION;                                                                                                                                      \
    }

/**
 * This class manages errors
 */
class SKGBASEMODELER_EXPORT SKGError final
{
    /**
     * Return code of the error
     */
    Q_PROPERTY(int returnCode READ getReturnCode WRITE setReturnCode)
    /**
     * Message of the error
     */
    Q_PROPERTY(QString message READ getMessage WRITE setMessage)
    /**
     * Internal data of the error
     */
    Q_PROPERTY(QString property READ getProperty WRITE setProperty)
    /**
     * Action of the error
     */
    Q_PROPERTY(QString action READ getAction WRITE setAction)
    /**
     * To know if it is a success of the error
     */
    Q_PROPERTY(bool succeeded READ isSucceeded)
    /**
     * To know if it is a failure of the error
     */
    Q_PROPERTY(bool failed READ isFailed)
public:
    /**
     * Constructor
     */
    explicit SKGError();

    /**
     * Copy constructor
     * @param iError the error to copy
     */
    SKGError(const SKGError &iError);

    /**
     * Move constructor
     * @param iError the error to copy
     */
    SKGError(SKGError &&iError) noexcept;

    /**
     * Constructor
     * @param iRc the error code
     * @param iMessage the error message
     * @param iAction the error action. This is normally a recovery action (example: skg://file_save, https://google.com, …)
     */
    SKGError(int iRc, QString iMessage, QString iAction = QString());

    /**
     * Destructor
     */
    ~SKGError();

    /**
     * Operator affectation
     * @param iError the error to copy
     */
    SKGError &operator=(const SKGError &iError);

    /**
     * To know if this is an error or not. Equivalent to @see isSucceeded
     * @return true or false
     */
    bool operator!() const;

    /**
     * To know if this is an error or not. Equivalent to @see isFailed
     * @return true or false
     */
    operator bool() const;

    /**
     * To know if it is an error or not
     * @return true: It is an error
     *         false: It is not an error (it could be a warning)
     */
    bool isFailed() const;

    /**
     * To know if it is an error or not
     * @return true: It is not an error (it could be a warning)
     *         false: It is an error
     */
    bool isSucceeded() const;

    /**
     * To know if it is a warning or not
     * @return true: It is a warning
     *         false: It is not a warning
     */
    bool isWarning() const;

    /**
     * Return the return code associated to this error
     * @return 0 : It is not an error (SUCCEEDED)
     *         <0: It is just a warning (SUCCEEDED)
     *         >0: It is not error (FAILED)
     */
    int getReturnCode() const;

    /**
     * Return the message associated to this error
     * @return the message
     */
    QString getMessage() const;

    /**
     * Return the internal data associated to this error
     * @return the internal data
     */
    QString getProperty() const;

    /**
     * Return the full message associated to this error
     * @return the full message
     */
    QString getFullMessage() const;

    /**
     * Return the full message with historical associated to this error
     * @return the full message
     */
    QString getFullMessageWithHistorical() const;

    /**
     * Return the size of the historical associated to this error
     * @return the size
     */
    int getHistoricalSize() const;

    /**
     * Return the action associated to this error
     * @return the action
     */
    QString getAction() const;

    /**
     * Return previous error associated to this SKGError in the historical
     * @return previous error (null if not exist)
     * WARNING: this pointer mustn't be deleted
     */
    SKGError *getPreviousError() const;

public Q_SLOTS:
    /**
     * Set the return code associated to this error
     * @param iReturnCode the return code
     *         0 : It is not an error (SUCCEEDED)
     *         <0: It is just a warning (SUCCEEDED)
     *         >0: It is not error (FAILED)
     * @return itself to facilitate usage
     */
    SKGError &setReturnCode(int iReturnCode);

    /**
     * Set the message associated to this error
     * @param iMessage the message
     * @return itself to facilitate usage
     */
    SKGError &setMessage(const QString &iMessage);

    /**
     * Set internal data associated to this error
     * @param iProperty the internal data
     * @return itself to facilitate usage
     */
    SKGError &setProperty(const QString &iProperty);

    /**
     * Set the action associated to this error
     * @param iAction the action
     * @return itself to facilitate usage
     */
    SKGError &setAction(const QString &iAction);

    /**
     * Add a new historical message to the current error.
     * @param iRc the error code
     * @param iMessage the error message
     * @param iAction the error action. This is normally a recovery action (example: skg://file_save, https://google.com, …)
     * @return itself to facilitate usage
     */
    SKGError &addError(int iRc, const QString &iMessage, const QString &iAction = QString());

    /**
     * Add a new historical message to the current error.
     * @param iError the error
     * @return itself to facilitate usage
     */
    SKGError &addError(const SKGError &iError);

private:
    Q_GADGET
    /**
     * the return code of the error
     *         0 : It is not an error (SUCCEEDED)
     *         <0: It is just a warning (SUCCEEDED)
     *         >0: It is not error (FAILED)
     */
    int m_rc{0};

    /**
     * the message of the error
     */
    QString m_message;

    /**
     * the property of the error
     */
    QString m_property;

    /**
     * the action of the error
     */
    QString m_action;

    /**
     * the previous error on this branch
     */
    SKGError *m_previousError{nullptr};
};
#endif // SKGERROR_H
