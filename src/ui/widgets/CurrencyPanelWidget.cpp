#include "CurrencyPanelWidget.h"

#include "core/settings/AppSettings.h"
#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>

namespace {

// Formata taxa: "--" quando ausente, 4 casas para < 1000, 2 casas para >= 1000.
QString formatarTaxa(double valor)
{
    if (valor <= 0.0)
        return QStringLiteral("--");
    return QLocale(QLocale::Portuguese, QLocale::Brazil)
           .toString(valor, 'f', valor >= 1000.0 ? 2 : 4);
}

// Indicador inline moderno: "PAR  VALOR" em UMA linha, sem caixa preta.
// Mantém altura compatível com a statusbar navy (~24px de texto, ~32px total).
// Par: cinza-claro (sidebarText). Valor: verde (positivo) ou subtexto quando "--".
// Separador entre os dois slots: "·" em subtexto. Estilo via Theme:: (tokens).
void aplicarEstiloModerno(QLabel *lblPar, QLabel *lblValor, bool semValor)
{
    // Par sempre em sidebarText (legível sobre navy, discreto).
    lblPar->setStyleSheet(
        QStringLiteral("QLabel { color: %1; font-size: 12px; font-weight: 500;"
                       " background: transparent; }")
        .arg(Theme::sidebarText()));

    // Valor: verde quando presente, subtexto quando "--".
    // A11Y-05: NÃO trocar por positivoTexto() aqui — este label fica sobre o
    // navy fixo da statusbar (@@SIDEBAR2@@), não sobre @@SURF@@/@@BG@@.
    // positivo() cru já passa AA sobre o navy (6.56:1/7.14:1); positivoTexto()
    // é uma variante ESCURA pensada para fundo claro e sobre o navy regrediria
    // (2.69:1, reprova). Ver Theme.h (positivoTexto() é para @@SURF@@/@@BG@@).
    const QString corValor = semValor ? Theme::subtexto() : Theme::positivo();
    lblValor->setStyleSheet(
        QStringLiteral("QLabel { color: %1; font-size: 13px; font-weight: 600;"
                       " background: transparent; }")
        .arg(corValor));
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
    // Margens mínimas: o widget fica embutido na statusbar navy.
    root->setContentsMargins(4, 0, 8, 0);
    root->setSpacing(6);

    // Slot 1: par + valor lado a lado
    m_lblCode1  = new QLabel(QStringLiteral("---"), this);
    m_lblValue1 = new QLabel(QStringLiteral("--"),  this);
    m_lblCode1->setObjectName(QStringLiteral("cambioPar"));
    m_lblValue1->setObjectName(QStringLiteral("cambioValor"));
    m_lblCode1->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_lblValue1->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    // Separador discreto entre os slots
    auto *sep = new QLabel(QStringLiteral("·"), this);
    sep->setStyleSheet(
        QStringLiteral("QLabel { color: %1; font-size: 13px; background: transparent; }")
        .arg(Theme::subtexto()));
    sep->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

    // Slot 2: par + valor lado a lado
    m_lblCode2  = new QLabel(QStringLiteral("---"), this);
    m_lblValue2 = new QLabel(QStringLiteral("--"),  this);
    m_lblCode2->setObjectName(QStringLiteral("cambioPar"));
    m_lblValue2->setObjectName(QStringLiteral("cambioValor"));
    m_lblCode2->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_lblValue2->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    root->addWidget(m_lblCode1);
    root->addWidget(m_lblValue1);
    root->addSpacing(4);
    root->addWidget(sep);
    root->addSpacing(4);
    root->addWidget(m_lblCode2);
    root->addWidget(m_lblValue2);
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
    lblCode->setText(par + QStringLiteral("  "));
    const double taxa = m_rates.value(codigo, 0.0);
    const QString valorTexto = formatarTaxa(taxa);
    lblValue->setText(valorTexto);
    // Reaplica estilos para refletir presença/ausência de taxa.
    aplicarEstiloModerno(lblCode, lblValue, taxa <= 0.0);
}
