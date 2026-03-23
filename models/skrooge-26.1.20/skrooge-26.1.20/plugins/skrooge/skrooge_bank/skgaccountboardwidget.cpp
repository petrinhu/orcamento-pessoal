/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgaccountboardwidget.h"

#include <qaction.h>
#include <qdom.h>

#include <kcolorscheme.h>

#include "skgaccountobject.h"
#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgunitobject.h"

SKGAccountBoardWidget::SKGAccountBoardWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGBoardWidget(iParent, iDocument, i18nc("Title of a dashboard widget", "Accounts"))
    , m_refreshNeeded(true)
{
    SKGTRACEINFUNC(10)

    // Create menu
    setContextMenuPolicy(Qt::ActionsContextMenu);

    // menu
    m_menuFavorite = new QAction(SKGServices::fromTheme(QStringLiteral("bookmarks")), i18nc("Display only favorite accounts", "Highlighted only"), this);
    m_menuFavorite->setCheckable(true);
    m_menuFavorite->setChecked(false);
    connect(m_menuFavorite, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuFavorite);

    m_menuPastOperations = new QAction(i18nc("Noun, a type of account", "Only past transactions"), this);
    m_menuPastOperations->setCheckable(true);
    m_menuPastOperations->setChecked(false);
    connect(m_menuPastOperations, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuPastOperations);

    {
        auto sep = new QAction(this);
        sep->setSeparator(true);
        addAction(sep);
    }

    m_menuCurrent = new QAction(i18nc("Noun, a type of account", "Current"), this);
    m_menuCurrent->setCheckable(true);
    m_menuCurrent->setChecked(true);
    connect(m_menuCurrent, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuCurrent);

    m_menuCreditCard = new QAction(i18nc("Noun, a type of account", "Credit card"), this);
    m_menuCreditCard->setCheckable(true);
    m_menuCreditCard->setChecked(true);
    connect(m_menuCreditCard, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuCreditCard);

    m_menuSaving = new QAction(i18nc("Noun, a type of account", "Saving"), this);
    m_menuSaving->setCheckable(true);
    m_menuSaving->setChecked(true);
    connect(m_menuSaving, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuSaving);

    m_menuInvestment = new QAction(i18nc("Noun, a type of account", "Investment"), this);
    m_menuInvestment->setCheckable(true);
    m_menuInvestment->setChecked(true);
    connect(m_menuInvestment, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuInvestment);

    m_menuAssets = new QAction(i18nc("Noun, a type of account", "Assets"), this);
    m_menuAssets->setCheckable(true);
    m_menuAssets->setChecked(true);
    connect(m_menuAssets, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuAssets);

    m_menuLoan = new QAction(i18nc("Noun, a type of account", "Loan"), this);
    m_menuLoan->setCheckable(true);
    m_menuLoan->setChecked(true);
    connect(m_menuLoan, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuLoan);

    m_menuPension = new QAction(i18nc("Noun, a type of account", "Pension"), this);
    m_menuPension->setCheckable(true);
    m_menuPension->setChecked(true);
    connect(m_menuPension, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuPension);

    m_menuWallet = new QAction(i18nc("Noun, a type of account", "Wallet"), this);
    m_menuWallet->setCheckable(true);
    m_menuWallet->setChecked(true);
    connect(m_menuWallet, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuWallet);

    m_menuOther = new QAction(i18nc("Noun, a type of account", "Other"), this);
    m_menuOther->setCheckable(true);
    m_menuOther->setChecked(true);
    connect(m_menuOther, &QAction::triggered, this, [this]() {
        this->dataModified();
    });
    addAction(m_menuOther);

    m_label = new QLabel();
    setMainWidget(m_label);

    // Refresh
    connect(getDocument(), &SKGDocument::tableModified, this, &SKGAccountBoardWidget::dataModified, Qt::QueuedConnection);
    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::currentPageChanged, this, &SKGAccountBoardWidget::pageChanged, Qt::QueuedConnection);
    connect(m_label, &QLabel::linkActivated, this, [](const QString &val) {
        SKGMainPanel::getMainPanel()->openPage(val);
    });
}

SKGAccountBoardWidget::~SKGAccountBoardWidget()
{
    SKGTRACEINFUNC(10)
    m_menuAssets = nullptr;
    m_menuCurrent = nullptr;
    m_menuCreditCard = nullptr;
    m_menuSaving = nullptr;
    m_menuInvestment = nullptr;
    m_menuWallet = nullptr;
    m_menuLoan = nullptr;
    m_menuPension = nullptr;
    m_menuOther = nullptr;
    m_menuFavorite = nullptr;
    m_menuPastOperations = nullptr;
}

QString SKGAccountBoardWidget::getState()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGBoardWidget::getState());
    QDomElement root = doc.documentElement();

    root.setAttribute(QStringLiteral("menuFavorite"), (m_menuFavorite != nullptr) && m_menuFavorite->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuAssets"), (m_menuAssets != nullptr) && m_menuAssets->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuCurrent"), (m_menuCurrent != nullptr) && m_menuCurrent->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuCreditCard"),
                      (m_menuCreditCard != nullptr) && m_menuCreditCard->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuSaving"), (m_menuSaving != nullptr) && m_menuSaving->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuInvestment"),
                      (m_menuInvestment != nullptr) && m_menuInvestment->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuWallet"), (m_menuWallet != nullptr) && m_menuWallet->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuLoan"), (m_menuLoan != nullptr) && m_menuLoan->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuPension"), (m_menuPension != nullptr) && m_menuPension->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuOther"), (m_menuOther != nullptr) && m_menuOther->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("menuPastOperations"),
                      (m_menuPastOperations != nullptr) && m_menuPastOperations->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));

    return doc.toString();
}

void SKGAccountBoardWidget::setState(const QString &iState)
{
    SKGBoardWidget::setState(iState);

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    if (m_menuFavorite != nullptr) {
        m_menuFavorite->setChecked(root.attribute(QStringLiteral("menuFavorite")) == QStringLiteral("Y"));
    }
    if (m_menuAssets != nullptr) {
        m_menuAssets->setChecked(root.attribute(QStringLiteral("menuAssets")) != QStringLiteral("N"));
    }
    if (m_menuCurrent != nullptr) {
        m_menuCurrent->setChecked(root.attribute(QStringLiteral("menuCurrent")) != QStringLiteral("N"));
    }
    if (m_menuCreditCard != nullptr) {
        m_menuCreditCard->setChecked(root.attribute(QStringLiteral("menuCreditCard")) != QStringLiteral("N"));
    }
    if (m_menuSaving != nullptr) {
        m_menuSaving->setChecked(root.attribute(QStringLiteral("menuSaving")) != QStringLiteral("N"));
    }
    if (m_menuInvestment != nullptr) {
        m_menuInvestment->setChecked(root.attribute(QStringLiteral("menuInvestment")) != QStringLiteral("N"));
    }
    if (m_menuWallet != nullptr) {
        m_menuWallet->setChecked(root.attribute(QStringLiteral("menuWallet")) != QStringLiteral("N"));
    }
    if (m_menuLoan != nullptr) {
        m_menuLoan->setChecked(root.attribute(QStringLiteral("menuLoan")) != QStringLiteral("N"));
    }
    if (m_menuPension != nullptr) {
        m_menuPension->setChecked(root.attribute(QStringLiteral("menuPension")) != QStringLiteral("N"));
    }
    if (m_menuOther != nullptr) {
        m_menuOther->setChecked(root.attribute(QStringLiteral("menuOther")) != QStringLiteral("N"));
    }
    if (m_menuPastOperations != nullptr) {
        m_menuPastOperations->setChecked(root.attribute(QStringLiteral("menuPastOperations")) == QStringLiteral("Y"));
    }

    dataModified(QString(), 0);
}

void SKGAccountBoardWidget::pageChanged()
{
    if (m_refreshNeeded) {
        dataModified(QString(), 0);
    }
}

void SKGAccountBoardWidget::dataModified(const QString &iTableName, int iIdTransaction)
{
    Q_UNUSED(iIdTransaction)

    if (iTableName == QStringLiteral("v_account_display") || iTableName.isEmpty()) {
        SKGTRACEINFUNC(10)
        SKGTabPage *page = SKGTabPage::parentTabPage(this);
        if (page != nullptr && page != SKGMainPanel::getMainPanel()->currentPage()) {
            m_refreshNeeded = true;
            return;
        }

        m_refreshNeeded = false;
        auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
        if (doc != nullptr) {
            SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();
            bool exist = false;
            SKGError err = doc->existObjects(QStringLiteral("account"), QString(), exist);
            IFOK(err)
            {
                QString html;
                if (!exist) {
                    html = "<html><body>"
                        % i18nc("Message, do not translate URL",
                                "First, you have to create at least one account<br>from <a href=\"%1\">\"Bank and Account\"</a> page or <a "
                                "href=\"%2\">import</a> transactions.",
                                "skg://Skrooge_bank_plugin",
                                "skg://import_operation")
                        % "</body></html>";
                } else {
                    // Build where clause
                    QString wc;
                    if ((m_menuAssets != nullptr) && m_menuAssets->isChecked()) {
                        wc = QStringLiteral("t_type='A'");
                    }
                    if ((m_menuCurrent != nullptr) && m_menuCurrent->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += QStringLiteral(" OR ");
                        }
                        wc += QStringLiteral("t_type='C'");
                    }
                    if ((m_menuCreditCard != nullptr) && m_menuCreditCard->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += QStringLiteral(" OR ");
                        }
                        wc += QStringLiteral("t_type='D'");
                    }
                    if ((m_menuSaving != nullptr) && m_menuSaving->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += QStringLiteral(" OR ");
                        }
                        wc += QStringLiteral("t_type='S'");
                    }
                    if ((m_menuInvestment != nullptr) && m_menuInvestment->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += QStringLiteral(" OR ");
                        }
                        wc += QStringLiteral("t_type='I'");
                    }
                    if ((m_menuWallet != nullptr) && m_menuWallet->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += QStringLiteral(" OR ");
                        }
                        wc += QStringLiteral("t_type='W'");
                    }
                    if ((m_menuOther != nullptr) && m_menuOther->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += QStringLiteral(" OR ");
                        }
                        wc += QStringLiteral("t_type='O'");
                    }
                    if ((m_menuLoan != nullptr) && m_menuLoan->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += QStringLiteral(" OR ");
                        }
                        wc += QStringLiteral("t_type='L'");
                    }
                    if ((m_menuPension != nullptr) && m_menuPension->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += QStringLiteral(" OR ");
                        }
                        wc += QStringLiteral("t_type='P'");
                    }

                    if (wc.isEmpty()) {
                        wc = QStringLiteral("1=0");
                    } else if ((m_menuFavorite != nullptr) && m_menuFavorite->isChecked()) {
                        wc = "t_bookmarked='Y' AND (" % wc % u')';
                    }

                    // Build display
                    SKGStringListList listTmp;
                    err = doc->executeSelectSqliteOrder(
                        QStringLiteral("SELECT t_name, t_TYPENLS, t_UNIT, ")
                            % ((m_menuPastOperations != nullptr) && m_menuPastOperations->isChecked() ? "f_TODAYAMOUNT" : "f_CURRENTAMOUNT")
                            % ", t_close  from v_account_display WHERE (" % wc % ") ORDER BY t_TYPENLS, t_name",
                        listTmp);
                    IFOK(err)
                    {
                        KColorScheme scheme(QPalette::Normal, KColorScheme::Window);
                        auto color = scheme.foreground(KColorScheme::NormalText).color().name().right(6);

                        html += QStringLiteral("<html><head><style>a {color: #") + color + ";}</style></head><body><table>";
                        double sumTypeV1 = 0;
                        double sumV1 = 0;
                        int nbAdded = 0;
                        QString currentType;
                        int nb = listTmp.count();
                        for (int i = 1; i < nb; ++i) { // Ignore header
                            const QStringList &r = listTmp.at(i);
                            const QString &name = r.at(0);
                            const QString &type = r.at(1);
                            const QString &unitAccountSymbol = r.at(2);
                            double v1 = SKGServices::stringToDouble(r.at(3));
                            bool closed = (r.at(4) == QStringLiteral("Y"));

                            if (type != currentType) {
                                if (!currentType.isEmpty() && nbAdded > 0) {
                                    html += "<tr><td><b>" % SKGServices::stringToHtml(i18nc("the numerical total of a sum of values", "Total of %1", currentType)) % "</b></td>"
                                            "<td align=\"right\"><b>" % doc->formatMoney(sumTypeV1, primary) % "</b></td></tr>";
                                    sumTypeV1 = 0;
                                    nbAdded = 0;
                                }
                                currentType = type;
                            }
                            if (!closed || qAbs(v1) > 0.1) {
                                html += QString("<tr><td><a href=\"skg://Skrooge_operation_plugin/?account=" % SKGServices::encodeForUrl(name) % "\">") % SKGServices::stringToHtml(name) % "</a></td>"
                                        "<td align=\"right\">";

                                if (!unitAccountSymbol.isEmpty() && primary.Symbol != unitAccountSymbol) {
                                    SKGUnitObject unitAccount(getDocument());
                                    unitAccount.setSymbol(unitAccountSymbol);
                                    unitAccount.load();

                                    double unitAccountValue = SKGServices::stringToDouble(unitAccount.getAttribute(QStringLiteral("f_CURRENTAMOUNT")));
                                    SKGServices::SKGUnitInfo u2 = primary;
                                    u2.Symbol = unitAccountSymbol;
                                    u2.NbDecimal = unitAccount.getNumberDecimal();
                                    html += unitAccountValue != 0 ? QStringLiteral("\x221E") : doc->formatMoney(v1 / unitAccountValue, u2);
                                    html += '=';
                                }
                                html += doc->formatMoney(v1, primary);
                                html += QStringLiteral("</td></tr>");
                                ++nbAdded;
                            }
                            sumTypeV1 += v1;
                            sumV1 += v1;
                        }

                        if (!currentType.isEmpty()) {
                            html += "<tr><td><b>" % SKGServices::stringToHtml(i18nc("the numerical total of a sum of values", "Total of %1", currentType)) % "</b></td>"
                                    "<td align=\"right\"><b>" % doc->formatMoney(sumTypeV1, primary) % "</b></td>"
                                    "</tr>";
                        }
                        html += "<tr><td><b>" % SKGServices::stringToHtml(i18nc("Noun, the numerical total of a sum of values", "Total")) % "</b></td>"
                                "<td align=\"right\"><b>" % doc->formatMoney(sumV1, primary) % "</b></td>"
                                "</tr>";
                        html += QStringLiteral("</table></body></html>");
                    }
                }
                m_label->setText(html);
            }
        }
    }
}
