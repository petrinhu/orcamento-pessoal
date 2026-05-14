#include "CurrencyPanelWidget.h"

#include "core/settings/AppSettings.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QVBoxLayout>

namespace {

// QSS emulando display LED DSEG7: fundo escuro, fonte monoespaçada larga,
// glow verde para o valor e laranja para o código (sub-label).
constexpr const char *kQssCode =
    "QLabel { "
    "background-color: #1a1a1a; "
    "color: #ffae42; "
    "font-family: \"DejaVu Sans Mono\", monospace; "
    "font-size: 12px; "
    "font-weight: bold; "
    "padding: 4px 12px 0 12px; "
    "border-top-left-radius: 6px; "
    "border-top-right-radius: 6px; "
    "border: 1px solid #333; "
    "border-bottom: 0; "
    "}";

constexpr const char *kQssValue =
    "QLabel { "
    "background-color: #1a1a1a; "
    "color: #00ff88; "
    "font-family: \"DejaVu Sans Mono\", monospace; "
    "font-size: 24px; "
    "font-weight: bold; "
    "padding: 0 12px 6px 12px; "
    "border-bottom-left-radius: 6px; "
    "border-bottom-right-radius: 6px; "
    "border: 1px solid #333; "
    "border-top: 0; "
    "}";

QString formatarTaxa(double valor)
{
    if (valor <= 0.0)
        return QStringLiteral("--");
    return QLocale(QLocale::Portuguese, QLocale::Brazil)
           .toString(valor, 'f', valor >= 1000.0 ? 2 : 4);
}

} // anon namespace

CurrencyPanelWidget::CurrencyPanelWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregarSlots();
}

void CurrencyPanelWidget::buildUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(8);

    auto buildSlot = [](QLabel *&lblCode, QLabel *&lblValue) {
        auto *col = new QVBoxLayout;
        col->setContentsMargins(0, 0, 0, 0);
        col->setSpacing(0);
        lblCode  = new QLabel(QStringLiteral("---"));
        lblValue = new QLabel(QStringLiteral("--"));
        lblCode->setStyleSheet(QString::fromLatin1(kQssCode));
        lblValue->setStyleSheet(QString::fromLatin1(kQssValue));
        lblCode->setAlignment(Qt::AlignCenter);
        lblValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        col->addWidget(lblCode);
        col->addWidget(lblValue);
        return col;
    };

    root->addLayout(buildSlot(m_lblCode1, m_lblValue1));
    root->addLayout(buildSlot(m_lblCode2, m_lblValue2));
    root->addStretch();
}

void CurrencyPanelWidget::setRates(const QMap<QString, double> &rates)
{
    m_rates = rates;
    recarregarSlots();
}

void CurrencyPanelWidget::recarregarSlots()
{
    const QString slot1 = AppSettings::instance().currencySlot1();
    const QString slot2 = AppSettings::instance().currencySlot2();
    renderSlot(m_lblCode1, m_lblValue1, slot1);
    renderSlot(m_lblCode2, m_lblValue2, slot2);
}

void CurrencyPanelWidget::renderSlot(QLabel *lblCode,
                                      QLabel *lblValue,
                                      const QString &codigo)
{
    if (!lblCode || !lblValue) return;
    const QString par = codigo.isEmpty()
                        ? QStringLiteral("---")
                        : QStringLiteral("%1/BRL").arg(codigo);
    lblCode->setText(par);
    const double taxa = m_rates.value(codigo, 0.0);
    lblValue->setText(formatarTaxa(taxa));
}
