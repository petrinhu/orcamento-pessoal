/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGREPORT_H
#define SKGREPORT_H
/** @file
 * A report class for document
 *
 * @author Stephane MANKOWSKI
 */
#include <qobject.h>
#include <qtypeinfo.h>
#include <qvariant.h>

#include "skgbasemodeler_export.h"
#include "skgerror.h"

class SKGDocument;
/**
 * A report class for document
 */
class SKGBASEMODELER_EXPORT SKGReport : public QObject
{
    Q_OBJECT
    /**
     * The period
     */
    Q_PROPERTY(QString period READ getPeriod WRITE setPeriod NOTIFY changed)

    /**
     * The month (for compatibility in templates)
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QString month READ getPeriod NOTIFY changed)

    /**
     * The previous object
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(SKGReport *previous READ getPrevious NOTIFY changed)

    /**
     * The previous period
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QString previous_period READ getPreviousPeriod NOTIFY changed)

    /**
     * The previous month (for compatibility in templates)
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QString previous_month READ getPreviousPeriod NOTIFY changed)

    /**
     * The zoom factor
     */
    Q_PROPERTY(double point_size READ getPointSize WRITE setPointSize NOTIFY changed)

    /**
     * The tips of the day
     */
    Q_PROPERTY(QStringList tips_of_day READ getTipsOfDay WRITE setTipsOfDay NOTIFY changed)

    /**
     * The tip of the day
     */
    Q_PROPERTY(QString tip_of_day READ getTipOfDay NOTIFY changed)

public:
    /**
     * Default Constructor
     */
    explicit SKGReport(SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGReport() override;

    /**
     * Get the parent document
     * @return the parent document of the report
     */
    virtual SKGDocument *getDocument() const;

    /**
     * Set the current period
     * @param iPeriod the period
     */
    Q_INVOKABLE virtual void setPeriod(const QString &iPeriod);

    /**
     * Get the current period
     * @return the current period
     */
    Q_INVOKABLE virtual QString getPeriod();

    /**
     * Set the SQL filter
     * @param iFilter the filter
     */
    Q_INVOKABLE virtual void setSqlFilter(const QString &iFilter);

    /**
     * Get the SQL filter
     * @return the SQL filter
     */
    Q_INVOKABLE virtual QString getSqlFilter();

    /**
     * Get the previous period
     * @return the previous period
     */
    Q_INVOKABLE virtual QString getPreviousPeriod();

    /**
     * Get the previous report
     * @return the previous report (MUST NOT BE REMOVED)
     */
    Q_INVOKABLE virtual SKGReport *getPrevious();

    /**
     * Set the font point size
     * @param iPointSize font point size
     */
    Q_INVOKABLE virtual void setPointSize(int iPointSize);

    /**
     * Get the font point
     * @return the font point
     */
    Q_INVOKABLE virtual int getPointSize() const;

    /**
     * Set the tips of the day
     * @param iTipsOfDays the tips of the day
     */
    Q_INVOKABLE virtual void setTipsOfDay(const QStringList &iTipsOfDays);

    /**
     * Get the tip of the day
     * @return the tip of the day
     */
    Q_INVOKABLE virtual QString getTipOfDay() const;

    /**
     * Get the tips of the day
     * @return the tips of the day
     */
    Q_INVOKABLE virtual QStringList getTipsOfDay() const;

    /**
     * Clean the cache
     * @param iEmitSignal to emit modification signal
     */
    Q_INVOKABLE virtual void cleanCache(bool iEmitSignal = true);

    /**
     * The context properties
     */
    Q_INVOKABLE virtual QVariantHash getContextProperty();

    /**
     * To add a parameter for a computation
     * @param iName the name of the parameter
     * @param ivalue the value of the parameter
     */
    Q_INVOKABLE virtual void addParameter(const QString &iName, const QVariant &ivalue);

    /**
     * Get report
     * @param iReport the report
     * @param iFile the template file name
     * @param oHtml the html report
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError getReportFromTemplate(SKGReport *iReport, const QString &iFile, QString &oHtml);

Q_SIGNALS:
    /**
     * Emitted when the report changed
     */
    void changed();

protected:
    /**
     * Enrich the grantlee mapping
     * @param iMapping the mapping
     */
    Q_INVOKABLE virtual void addItemsInMapping(QVariantHash &iMapping);

    SKGDocument *m_document;
    SKGReport *m_previous;
    QHash<QString, QVariant> m_cache;
    QHash<QString, QVariant> m_parameters;
    int m_pointSize;
    QStringList m_tipsOfTheDay;

private:
    Q_DISABLE_COPY(SKGReport)
};

/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGReport, Q_COMPLEX_TYPE);
#endif // SKGREPORT_H
