/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for unit management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgunitboardwidget.h"

#include <qdom.h>

#include <kcolorscheme.h>

#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgtraces.h"
#include "skgunitobject.h"

SKGUnitBoardWidget::SKGUnitBoardWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGBoardWidget(iParent, iDocument, i18nc("Title of a dashboard widget", "Quotes"))
{
    SKGTRACEINFUNC(10)

    // Create menu
    setContextMenuPolicy(Qt::ActionsContextMenu);

    m_menuFavorite = new QAction(SKGServices::fromTheme(QStringLiteral("bookmarks")), i18nc("Display only favorite accounts", "Highlighted only"), this);
    m_menuFavorite->setCheckable(true);
    m_menuFavorite->setChecked(false);
    connect(m_menuFavorite, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuFavorite);

    {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        addAction(sep);
    }

    m_menuCurrencies = new QAction(i18nc("Noun, a country's currency", "Currencies"), this);
    m_menuCurrencies->setCheckable(true);
    m_menuCurrencies->setChecked(true);
    connect(m_menuCurrencies, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuCurrencies);

    m_menuIndexes =
        new QAction(i18nc("Financial indexes, used as an indicator of financial places' health. Examples : Dow Jones, CAC40, Nikkei…", "Indexes"), this);
    m_menuIndexes->setCheckable(true);
    m_menuIndexes->setChecked(true);
    connect(m_menuIndexes, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuIndexes);

    m_menuShares = new QAction(i18nc("Shares, as in financial shares: parts of a company that you can buy or sell on financial markets", "Shares"), this);
    m_menuShares->setCheckable(true);
    m_menuShares->setChecked(true);
    connect(m_menuShares, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuShares);

    m_menuObjects = new QAction(i18nc("Noun, a physical object like a house or a car", "Objects"), this);
    m_menuObjects->setCheckable(true);
    m_menuObjects->setChecked(true);
    connect(m_menuObjects, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuObjects);

    m_menuSharesOwnedOnly = new QAction(i18nc("Only show the list of Shares (financial shares) that you own", "Shares owned only"), this);
    m_menuSharesOwnedOnly->setCheckable(true);
    m_menuSharesOwnedOnly->setChecked(false);
    connect(m_menuSharesOwnedOnly, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuSharesOwnedOnly);

    m_label = new QLabel();
    setMainWidget(m_label);

    // Refresh
    connect(getDocument(), &SKGDocument::tableModified, this, &SKGUnitBoardWidget::dataModified, Qt::QueuedConnection);
    connect(m_label, &QLabel::linkActivated, this, [](const QString &val) {
        SKGMainPanel::getMainPanel()->openPage(val);
    });
}

SKGUnitBoardWidget::~SKGUnitBoardWidget()
{
    SKGTRACEINFUNC(10)
    m_menuIndexes = nullptr;
    m_menuShares = nullptr;
    m_menuSharesOwnedOnly = nullptr;
    m_menuObjects = nullptr;
    m_menuCurrencies = nullptr;
    m_menuFavorite = nullptr;
}

QString SKGUnitBoardWidget::getState()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGBoardWidget::getState());
    QDomElement root = doc.documentElement();

    root.setAttribute(QStringLiteral("m_menuCurrencies"),
                      (m_menuCurrencies != nullptr) && m_menuCurrencies->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("m_menuObjects"), (m_menuObjects != nullptr) && m_menuObjects->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuIndexes"), (m_menuIndexes != nullptr) && m_menuIndexes->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuShares"), (m_menuShares != nullptr) && m_menuShares->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuSharesOwnedOnly"),
                      (m_menuSharesOwnedOnly != nullptr) && m_menuSharesOwnedOnly->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuFavorite"), (m_menuFavorite != nullptr) && m_menuFavorite->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    return doc.toString();
}

void SKGUnitBoardWidget::setState(const QString &iState)
{
    SKGBoardWidget::setState(iState);

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    if (m_menuCurrencies != nullptr) {
        m_menuCurrencies->setChecked(root.attribute(QStringLiteral("m_menuCurrencies")) == QStringLiteral("Y"));
    }
    if (m_menuObjects != nullptr) {
        m_menuObjects->setChecked(root.attribute(QStringLiteral("m_menuObjects")) == QStringLiteral("Y"));
    }
    if (m_menuIndexes != nullptr) {
        m_menuIndexes->setChecked(root.attribute(QStringLiteral("menuIndexes")) != QStringLiteral("N"));
    }
    if (m_menuShares != nullptr) {
        m_menuShares->setChecked(root.attribute(QStringLiteral("menuShares")) != QStringLiteral("N"));
    }
    if (m_menuSharesOwnedOnly != nullptr) {
        m_menuSharesOwnedOnly->setChecked(root.attribute(QStringLiteral("menuSharesOwnedOnly")) != QStringLiteral("N"));
    }
    if (m_menuFavorite != nullptr) {
        m_menuFavorite->setChecked(root.attribute(QStringLiteral("menuFavorite")) == QStringLiteral("Y"));
    }

    dataModified(QString(), 0);
}

void SKGUnitBoardWidget::dataModified(const QString &iTableName, int iIdTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    if (iTableName == QStringLiteral("v_unit_display") || iTableName.isEmpty()) {
        auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
        if (doc != nullptr) {
            SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();

            // Build where clause
            QString wc;
            if ((m_menuSharesOwnedOnly != nullptr) && (m_menuShares != nullptr) && (m_menuIndexes != nullptr)) {
                m_menuSharesOwnedOnly->setEnabled(m_menuShares->isChecked());
                m_menuShares->setEnabled(!m_menuSharesOwnedOnly->isChecked());
                if (!m_menuShares->isChecked()) {
                    m_menuSharesOwnedOnly->setChecked(false);
                }

                if (m_menuIndexes->isChecked()) {
                    wc = QStringLiteral("t_type='I'");
                }
                if (m_menuShares->isChecked()) {
                    if (!wc.isEmpty()) {
                        wc += QStringLiteral(" OR ");
                    }
                    if (m_menuSharesOwnedOnly->isChecked()) {
                        wc += QStringLiteral("(t_type='S' AND f_QUANTITYOWNED>0)");
                    } else {
                        wc += QStringLiteral("t_type='S'");
                    }
                }
                if (m_menuCurrencies->isChecked()) {
                    if (!wc.isEmpty()) {
                        wc += QStringLiteral(" OR ");
                    }
                    wc += QStringLiteral("t_type IN ('1','2','C')");
                }
                if (m_menuObjects->isChecked()) {
                    if (!wc.isEmpty()) {
                        wc += QStringLiteral(" OR ");
                    }
                    wc += QStringLiteral("t_type='O'");
                }
            }

            if (wc.isEmpty()) {
                wc = QStringLiteral("1=0");
            } else if ((m_menuFavorite != nullptr) && m_menuFavorite->isChecked()) {
                wc = "t_bookmarked='Y' AND (" % wc % u')';
            }

            SKGObjectBase::SKGListSKGObjectBase objs;
            SKGError err = getDocument()->getObjects(QStringLiteral("v_unit_display"), wc % " ORDER BY t_type DESC, t_name ASC", objs);
            IFOK(err)
            {
                KColorScheme scheme(QPalette::Normal, KColorScheme::Window);
                auto color = scheme.foreground(KColorScheme::NormalText).color().name().right(6);
                QString html = QStringLiteral("<html><head><style>a {color: #") + color + ";}</style></head><body>";
                int nb = objs.count();
                if (nb != 0) {
                    html += QStringLiteral("<table>");
                    QString lastTitle;
                    for (int i = 0; i < nb; ++i) {
                        SKGUnitObject obj(objs.at(i));
                        QString type = obj.getAttribute(QStringLiteral("t_TYPENLS"));
                        if (lastTitle != type) {
                            lastTitle = type;
                            html += "<tr><td><b>" % SKGServices::stringToHtml(lastTitle) % "</b></td></tr>";
                        }
                        html += QString("<tr><td><a href=\"skg://Skrooge_operation_plugin/?operationWhereClause=rc_unit_id="
                                        % SKGServices::intToString(obj.getID()) % "&title="
                                        % SKGServices::encodeForUrl(
                                            i18nc("A list of transactions made on the specified unit", "Transactions with unit equal to '%1'", obj.getName()))
                                        % "&title_icon=taxes-finances&currentPage=-1" % "\">")
                            % SKGServices::stringToHtml(obj.getDisplayName()) % "</a></td><td align=\"right\">";
                        if (obj.getType() == SKGUnitObject::INDEX) {
                            primary.Symbol = ' ';
                        }
                        html += doc->formatMoney(SKGServices::stringToDouble(obj.getAttribute(QStringLiteral("f_CURRENTAMOUNT"))), primary);
                        html += QStringLiteral("</td><td>(");
                        double amountOneYearBefore = obj.getAmount(QDate::currentDate().addYears(-1));
                        double val = amountOneYearBefore != 0 ? 100.0 * (obj.getAmount() - amountOneYearBefore) / amountOneYearBefore : 0;
                        html += doc->formatPercentage(val);
                        html += QStringLiteral(")</td></tr>");
                    }
                    html += QStringLiteral("</table>");
                } else {
                    html += i18nc("Message about not having any financial Share or financial index in the document",
                                  R"(No share or index defined<br>on the <a href="%1">"Units"</a> page.)",
                                  "skg://Skrooge_unit_plugin");
                }
                html += QStringLiteral("</body></html>");
                m_label->setText(html);
            }

            // No widget if no account
            bool exist = false;
            getDocument()->existObjects(QStringLiteral("account"), QString(), exist);
            if (parentWidget() != nullptr) {
                setVisible(exist);
            }
        }
    }
}
