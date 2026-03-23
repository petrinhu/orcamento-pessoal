/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a query creator for skrooge
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgpredicatcreator.h"

#include <qapplication.h>
#include <qcheckbox.h>
#include <qdom.h>
#include <qlineedit.h>
#include <qtablewidget.h>

#include "skgcalculatoredit.h"
#include "skgcombobox.h"
#include "skgdateedit.h"
#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgruleobject.h"
#include "skgservices.h"
#include <utility>

SKGPredicatCreator::SKGPredicatCreator(QWidget *iParent, SKGDocument *document, const QString &attribute, bool iModeUpdate, const QStringList &iListAtt)
    : QWidget(iParent)
    , m_updateMode(iModeUpdate)
    , m_kValue1(nullptr)
    , m_kValue2(nullptr)
    , m_kAttributes(nullptr)
{
    SKGServices::AttributeType attType = SKGServices::TEXT;
    if (document != nullptr) {
        attType = document->getAttributeType(attribute);
    }

    // Build
    this->setAutoFillBackground(true);
    this->resize(491, 25);
    auto horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setSpacing(2);
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
    m_kOperator = new SKGComboBox(this);
    m_kOperator->setObjectName(QStringLiteral("kOperator"));
    m_kOperator->setSizeAdjustPolicy(KComboBox::AdjustToContentsOnFirstShow);
    QSizePolicy newSizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
    newSizePolicy2.setHorizontalStretch(0);
    newSizePolicy2.setVerticalStretch(0);
    newSizePolicy2.setHeightForWidth(m_kOperator->sizePolicy().hasHeightForWidth());
    m_kOperator->setSizePolicy(newSizePolicy2);

    horizontalLayout->addWidget(m_kOperator);
    int nbAtt = iListAtt.count();
    if (nbAtt != 0) {
        m_kAttributes = new SKGComboBox(this);
        if (m_kAttributes != nullptr) {
            m_kAttributes->setObjectName(QStringLiteral("kAttributes"));
            m_kAttributes->setMinimumSize(QSize(100, 0));
            m_kAttributes->setEditable(false);
            QSizePolicy newSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            newSizePolicy.setHorizontalStretch(0);
            newSizePolicy.setVerticalStretch(0);
            newSizePolicy.setHeightForWidth(m_kAttributes->sizePolicy().hasHeightForWidth());
            m_kAttributes->setSizePolicy(newSizePolicy);

            horizontalLayout->addWidget(m_kAttributes);

            for (const auto &att : std::as_const(iListAtt)) {
                if (document != nullptr) {
                    m_kAttributes->addItem(document->getIcon(att), document->getDisplay(att), att);
                }
            }
        }
    }
    if (attType == SKGServices::TEXT) {
        auto cmbVal1 = new SKGComboBox(this);
        if (cmbVal1 != nullptr) {
            cmbVal1->setObjectName(QStringLiteral("cmbVal1"));
            cmbVal1->setMinimumSize(QSize(100, 0));
            cmbVal1->setEditable(true);
            QSizePolicy newSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            newSizePolicy.setHorizontalStretch(0);
            newSizePolicy.setVerticalStretch(0);
            newSizePolicy.setHeightForWidth(cmbVal1->sizePolicy().hasHeightForWidth());
            cmbVal1->setSizePolicy(newSizePolicy);
            cmbVal1->lineEdit()->setPlaceholderText(i18n("Value"));

            horizontalLayout->addWidget(cmbVal1);
            m_kValue1 = cmbVal1;
        }
        auto cmbVal2 = new SKGComboBox(this);
        if (cmbVal2 != nullptr) {
            cmbVal2->setObjectName(QStringLiteral("cmbVal2"));
            cmbVal2->setMinimumSize(QSize(100, 0));
            cmbVal2->setEditable(true);
            QSizePolicy newSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            newSizePolicy.setHorizontalStretch(0);
            newSizePolicy.setVerticalStretch(0);
            newSizePolicy.setHeightForWidth(cmbVal2->sizePolicy().hasHeightForWidth());
            cmbVal2->setSizePolicy(newSizePolicy);
            cmbVal2->lineEdit()->setPlaceholderText(i18n("Value"));

            horizontalLayout->addWidget(cmbVal2);
            m_kValue2 = cmbVal2;
        }

        if (document != nullptr) {
            QString realTable = QStringLiteral("operation");
            QString realAtt = attribute;
            QString realCond = QString();
            if (attribute == QStringLiteral("t_REALCATEGORY")) {
                realTable = QStringLiteral("category");
                realAtt = QStringLiteral("t_fullname");
            } else if (attribute == QStringLiteral("t_BANK")) {
                realTable = QStringLiteral("bank");
                realAtt = QStringLiteral("t_name");
            } else if (attribute == QStringLiteral("t_ACCOUNT") || attribute == QStringLiteral("t_TOACCOUNT")) {
                realTable = QStringLiteral("account");
                realAtt = QStringLiteral("t_name");
            } else if (attribute == QStringLiteral("t_UNIT")) {
                realTable = QStringLiteral("unit");
                realAtt = QStringLiteral("t_name");
            } else if (attribute == QStringLiteral("t_REALCOMMENT")) {
                realTable = QStringLiteral("suboperation");
                realAtt = QStringLiteral("t_comment");
            } else if (attribute == QStringLiteral("t_REALREFUND")) {
                realTable = QStringLiteral("refund");
                realAtt = QStringLiteral("t_name");
            } else if (attribute == QStringLiteral("t_PAYEE")) {
                realTable = QStringLiteral("payee");
                realAtt = QStringLiteral("t_name");
            } else if (attribute.startsWith(QStringLiteral("p_"))) {
                realTable = QStringLiteral("parameters");
                realAtt = QStringLiteral("t_value");
                realCond = "t_name='" % attribute.right(attribute.length() - 2) % u'\'';
            }
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << cmbVal1 << cmbVal2, document, realTable, realAtt, realCond);
        }
    } else if (attType == SKGServices::INTEGER || attType == SKGServices::FLOAT) {
        auto cmbVal1 = new SKGCalculatorEdit(this);
        if (cmbVal1 != nullptr) {
            cmbVal1->setObjectName(QStringLiteral("cmbVal1"));
            cmbVal1->setMode(SKGCalculatorEdit::EXPRESSION);
            cmbVal1->setMinimumSize(QSize(100, 0));

            horizontalLayout->addWidget(cmbVal1);
            m_kValue1 = cmbVal1;
        }
        auto cmbVal2 = new SKGCalculatorEdit(this);
        if (cmbVal2 != nullptr) {
            cmbVal2->setObjectName(QStringLiteral("cmbVal2"));
            cmbVal2->setMode(SKGCalculatorEdit::EXPRESSION);
            cmbVal2->setMinimumSize(QSize(100, 0));

            horizontalLayout->addWidget(cmbVal2);
            m_kValue2 = cmbVal2;
        }
    } else if (attType == SKGServices::DATE) {
        if (iModeUpdate) {
            auto cmbVal1 = new SKGComboBox(this);
            if (cmbVal1 != nullptr) {
                cmbVal1->setObjectName(QStringLiteral("cmbVal1"));
                cmbVal1->setMinimumSize(QSize(100, 0));
                cmbVal1->setEditable(true);
                QSizePolicy newSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                newSizePolicy.setHorizontalStretch(0);
                newSizePolicy.setVerticalStretch(0);
                newSizePolicy.setHeightForWidth(cmbVal1->sizePolicy().hasHeightForWidth());
                cmbVal1->setSizePolicy(newSizePolicy);
                cmbVal1->lineEdit()->setPlaceholderText(i18n("Value"));

                horizontalLayout->addWidget(cmbVal1);
                m_kValue1 = cmbVal1;
            }

            auto cmbVal2 = new SKGComboBox(this);
            if (cmbVal2 != nullptr) {
                cmbVal2->setObjectName(QStringLiteral("cmbVal2"));
                cmbVal2->setMinimumSize(QSize(100, 0));
                cmbVal2->setEditable(false);
                QSizePolicy newSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                newSizePolicy.setHorizontalStretch(0);
                newSizePolicy.setVerticalStretch(0);
                newSizePolicy.setHeightForWidth(cmbVal2->sizePolicy().hasHeightForWidth());
                cmbVal2->setSizePolicy(newSizePolicy);
                cmbVal2->addItems(QStringList() << QStringLiteral("YYYYMMDD") << QStringLiteral("MMDDYYYY") << QStringLiteral("DDMMYYYY")
                                                << QStringLiteral("MM-DD-YY") << QStringLiteral("DD-MM-YY") << QStringLiteral("MM-DD-YYYY")
                                                << QStringLiteral("DD-MM-YYYY") << QStringLiteral("YYYY-MM-DD"));
                horizontalLayout->addWidget(cmbVal2);
                m_kValue2 = cmbVal2;
            }
        } else {
            auto cmbVal1 = new SKGDateEdit(this);
            if (cmbVal1 != nullptr) {
                cmbVal1->setObjectName(QStringLiteral("cmbVal1"));
                cmbVal1->setMinimumSize(QSize(100, 0));

                horizontalLayout->addWidget(cmbVal1);
                m_kValue1 = cmbVal1;
            }

            auto cmbVal2 = new SKGDateEdit(this);
            if (cmbVal2 != nullptr) {
                cmbVal2->setObjectName(QStringLiteral("cmbVal2"));
                cmbVal2->setMinimumSize(QSize(100, 0));

                horizontalLayout->addWidget(cmbVal2);
                m_kValue2 = cmbVal2;
            }
        }
    } else if (attType == SKGServices::BOOL || attType == SKGServices::TRISTATE) {
        auto cmbVal1 = new QCheckBox(this);
        if (cmbVal1 != nullptr) {
            cmbVal1->setObjectName(QStringLiteral("cmbVal1"));
            cmbVal1->setMinimumSize(QSize(100, 0));
            cmbVal1->setTristate(attType == SKGServices::TRISTATE);

            horizontalLayout->addWidget(cmbVal1);
            m_kValue1 = cmbVal1;
        }
    }

    // Fill combo boxes
    m_kOperator->clear();
    m_kOperator->addItem(QString(), "");

    if (m_kValue1 != nullptr) {
        m_kValue1->installEventFilter(this);
    }
    if (m_kValue2 != nullptr) {
        m_kValue2->installEventFilter(this);
    }
    if (m_kOperator != nullptr) {
        m_kOperator->installEventFilter(this);
    }
    if (m_kAttributes != nullptr) {
        m_kAttributes->installEventFilter(this);
    }

    QStringList listOps = SKGRuleObject::getListOfOperators(attType, m_updateMode ? SKGRuleObject::UPDATE : SKGRuleObject::SEARCH);
    int nb = listOps.count();
    if (iModeUpdate && attribute == QStringLiteral("t_REALCATEGORY") && nb > 1) {
        nb = 1; // TODO(Stephane MANKOWSKI): unblock t_REALCATEGORY
    }
    for (int i = 0; i < nb; ++i) {
        const QString &op = listOps.at(i);
        QString nlsOp =
            SKGRuleObject::getDisplayForOperator(op, i18nc("Default value", "…"), i18nc("Default value", "…"), i18nc("Noun, an item's attribute", "attribute"));
        if (m_kOperator != nullptr) {
            m_kOperator->addItem(nlsOp, op);
        }
    }

    connect(m_kOperator, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged), this, &SKGPredicatCreator::onOperatorChanged);
    onOperatorChanged();
}

SKGPredicatCreator::~SKGPredicatCreator()
{
    m_kOperator = nullptr;
    m_kValue1 = nullptr;
    m_kValue2 = nullptr;
    m_kAttributes = nullptr;
}

bool SKGPredicatCreator::eventFilter(QObject *iObject, QEvent *iEvent)
{
    Q_UNUSED(iObject)
    if ((iEvent != nullptr) && (iEvent->type() == QEvent::FocusIn || iEvent->type() == QEvent::FocusOut)) {
        QObject *appliFocus = QApplication::focusWidget();

        while (appliFocus != nullptr) {
            if (appliFocus == this) {
                break;
            }
            appliFocus = appliFocus->parent();
        }
        if (appliFocus == nullptr) {
            Q_EMIT editingFinished();
        }
    }
    return false;
}

void SKGPredicatCreator::onOperatorChanged()
{
    QString req;
    if (m_kOperator != nullptr) {
        req = m_kOperator->itemData(m_kOperator->currentIndex()).toString();

        m_kOperator->setToolTip(SKGRuleObject::getToolTipForOperator(req));
    }
    if (m_kValue1 != nullptr) {
        m_kValue1->setVisible(req.contains(QStringLiteral("#V1S#")) || req.contains(QStringLiteral("#V1#")));
    }
    if (m_kValue2 != nullptr) {
        m_kValue2->setVisible(req.contains(QStringLiteral("#V2S#")) || req.contains(QStringLiteral("#V2#")) || req.contains(QStringLiteral("#DF#")));
    }
    if (m_kAttributes != nullptr) {
        m_kAttributes->setVisible(req.contains(QStringLiteral("#ATT2#")));
    }
}

QString SKGPredicatCreator::text()
{
    return SKGPredicatCreator::getTextFromXml(xmlDescription());
}

QString SKGPredicatCreator::getTextFromXml(const QString &iXML)
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iXML);
    QDomElement root = doc.documentElement();
    QString output = SKGRuleObject::getDisplayForOperator(root.attribute(QStringLiteral("operator")),
                                                          root.attribute(QStringLiteral("value")),
                                                          root.attribute(QStringLiteral("value2")),
                                                          root.attribute(QStringLiteral("att2s")));
    return output;
}

QString SKGPredicatCreator::xmlDescription()
{
    QString output;
    if (m_kOperator != nullptr) {
        QString op = m_kOperator->itemData(m_kOperator->currentIndex()).toString();
        if (!op.isEmpty()) {
            QDomDocument doc(QStringLiteral("SKGML"));
            QDomElement root = doc.createElement(QStringLiteral("element"));
            doc.appendChild(root);

            // Condition
            root.setAttribute(QStringLiteral("operator"), op);
            if ((m_kValue1 != nullptr) && m_kValue1->isVisible()) {
                auto *cmbVal1 = qobject_cast<SKGDateEdit *>(m_kValue1);
                if (cmbVal1 != nullptr) {
                    root.setAttribute(QStringLiteral("value"), SKGServices::dateToSqlString(cmbVal1->date()));
                } else {
                    auto *cmbVal12 = qobject_cast<SKGCalculatorEdit *>(m_kValue1);
                    if (cmbVal12 != nullptr) {
                        root.setAttribute(QStringLiteral("value"), cmbVal12->text());
                    } else {
                        auto *cmbVal13 = qobject_cast<QCheckBox *>(m_kValue1);
                        if (cmbVal13 != nullptr) {
                            root.setAttribute(QStringLiteral("value"),
                                              cmbVal13->checkState() == Qt::Checked         ? QStringLiteral("Y")
                                                  : cmbVal13->checkState() == Qt::Unchecked ? QStringLiteral("N")
                                                                                            : QStringLiteral("P"));
                        } else {
                            auto *cmbVal14 = qobject_cast<SKGComboBox *>(m_kValue1);
                            if (cmbVal14 != nullptr) {
                                root.setAttribute(QStringLiteral("value"), cmbVal14->text());
                            }
                        }
                    }
                }
            }
            if ((m_kValue2 != nullptr) && m_kValue2->isVisible()) {
                auto *cmbVal2 = qobject_cast<SKGDateEdit *>(m_kValue2);
                if (cmbVal2 != nullptr) {
                    root.setAttribute(QStringLiteral("value2"), SKGServices::dateToSqlString(cmbVal2->date()));
                } else {
                    auto *cmbVal21 = qobject_cast<SKGCalculatorEdit *>(m_kValue2);
                    if (cmbVal21 != nullptr) {
                        root.setAttribute(QStringLiteral("value2"), cmbVal21->text());
                    } else {
                        auto *cmbVal22 = qobject_cast<SKGComboBox *>(m_kValue2);
                        if (cmbVal22 != nullptr) {
                            root.setAttribute(QStringLiteral("value2"), cmbVal22->text());
                        }
                    }
                }
            }

            if ((m_kAttributes != nullptr) && m_kAttributes->isVisible()) {
                root.setAttribute(QStringLiteral("att2"), m_kAttributes->itemData(m_kAttributes->currentIndex()).toString());
                root.setAttribute(QStringLiteral("att2s"), m_kAttributes->text());
            }

            output = doc.toString();
        }
    }
    return output;
}

void SKGPredicatCreator::setXmlDescription(const QString &iXML)
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iXML);
    QDomElement root = doc.documentElement();

    // Condition
    if (m_kOperator != nullptr) {
        m_kOperator->setCurrentIndex(m_kOperator->findData(root.attribute(QStringLiteral("operator"))));
        auto *cmbVal1 = qobject_cast<SKGDateEdit *>(m_kValue1);
        if (cmbVal1 != nullptr) {
            cmbVal1->setDate(SKGServices::stringToDate(root.attribute(QStringLiteral("value"))));
        } else {
            auto *cmbVal12 = qobject_cast<SKGCalculatorEdit *>(m_kValue1);
            if (cmbVal12 != nullptr) {
                cmbVal12->setText(root.attribute(QStringLiteral("value")));
            } else {
                auto *cmbVal13 = qobject_cast<QCheckBox *>(m_kValue1);
                if (cmbVal13 != nullptr) {
                    cmbVal13->setCheckState(root.attribute(QStringLiteral("value")) == QStringLiteral("Y")       ? Qt::Checked
                                                : root.attribute(QStringLiteral("value")) == QStringLiteral("P") ? Qt::PartiallyChecked
                                                                                                                 : Qt::Unchecked);
                } else {
                    auto *cmbVal14 = qobject_cast<SKGComboBox *>(m_kValue1);
                    if (cmbVal14 != nullptr) {
                        cmbVal14->setText(root.attribute(QStringLiteral("value")));
                    }
                }
            }
        }

        auto *cmbVal2 = qobject_cast<SKGDateEdit *>(m_kValue2);
        if (cmbVal2 != nullptr) {
            cmbVal2->setDate(SKGServices::stringToDate(root.attribute(QStringLiteral("value2"))));
        } else {
            auto *cmbVal22 = qobject_cast<SKGCalculatorEdit *>(m_kValue2);
            if (cmbVal22 != nullptr) {
                cmbVal22->setText(root.attribute(QStringLiteral("value2")));
            } else {
                auto *cmbVal23 = qobject_cast<SKGComboBox *>(m_kValue2);
                if (cmbVal23 != nullptr) {
                    cmbVal23->setText(root.attribute(QStringLiteral("value2")));
                }
            }
        }

        if (m_kAttributes != nullptr) {
            m_kAttributes->setCurrentIndex(m_kAttributes->findData(root.attribute(QStringLiteral("att2"))));
        }
    }

    Q_EMIT xmlDescriptionChanged();
}
