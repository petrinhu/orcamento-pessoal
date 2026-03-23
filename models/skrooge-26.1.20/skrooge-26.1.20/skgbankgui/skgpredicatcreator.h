/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPREDICATCREATOR_H
#define SKGPREDICATCREATOR_H
/** @file
 * A query creator for skrooge.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qwidget.h>

#include "skgbankgui_export.h"

class SKGDocument;
class SKGComboBox;

/**
 * This file is a query creator for skrooge
 */
class SKGBANKGUI_EXPORT SKGPredicatCreator : public QWidget
{
    Q_OBJECT
    /**
     * Text of the predicat
     */
    Q_PROPERTY(QString text READ text USER true NOTIFY xmlDescriptionChanged)

    /**
     * XML description of the predicat
     */
    Q_PROPERTY(QString xmlDescription READ xmlDescription WRITE setXmlDescription NOTIFY xmlDescriptionChanged)

public:
    /**
     * Default Constructor
     * @param iParent the parent
     * @param document the document
     * @param attribute name of the attribute
     * @param iModeUpdate to enable mode update
     * @param iListAtt list of attribute
     */
    explicit SKGPredicatCreator(QWidget *iParent,
                                SKGDocument *document,
                                const QString &attribute = QString(),
                                bool iModeUpdate = false,
                                const QStringList &iListAtt = QStringList());

    /**
     * Default Destructor
     */
    ~SKGPredicatCreator() override;

    /**
     * Get text
     * @return text
     */
    virtual QString text();

    /**
     * Get Text from XML description
     * @param iXML the description
     */
    static QString getTextFromXml(const QString &iXML);

    /**
     * Get XML description
     * @return description
     */
    virtual QString xmlDescription();

    /**
     * Set XML description
     * @param iXML the description
     */
    virtual void setXmlDescription(const QString &iXML);

Q_SIGNALS:
    /**
     * Sent when edition is finished
     */
    void editingFinished();

    /**
     * Sent when edition changed
     */
    void xmlDescriptionChanged();

protected:
    /**
     * Event filtering
     * @param iObject object
     * @param iEvent event
     * @return In your reimplementation of this function, if you want to filter the event out, i.e. stop it being handled further, return true; otherwise return
     * false.
     */
    bool eventFilter(QObject *iObject, QEvent *iEvent) override;

private Q_SLOTS:
    void onOperatorChanged();

private:
    Q_DISABLE_COPY(SKGPredicatCreator)
    bool m_updateMode;

    SKGComboBox *m_kOperator;
    QWidget *m_kValue1;
    QWidget *m_kValue2;
    SKGComboBox *m_kAttributes;
};

#endif // SKGPREDICATCREATOR_H
