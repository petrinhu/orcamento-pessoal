#pragma once

#include <QMap>
#include <QString>
#include <QWidget>

class QLabel;

// CurrencyPanelWidget (#47) - painel câmbio inline moderno na statusbar navy.
// Exibe dois slots de moeda configuráveis (lidos de AppSettings::currencySlot1/2)
// relativos a BRL. Layout horizontal "PAR  VALOR · PAR  VALOR" em UMA linha,
// sem caixa preta — compatível com a statusbar de ~32px de altura.
// Provider real (Brapi/Frankfurter/CoinGecko) entra em #58.
//
// SRP: apenas renderização. Alimentação de taxas via setRates() injetado pelo
// MarketDataService (futuro) ou por mock em testes.
class CurrencyPanelWidget final : public QWidget {
    Q_OBJECT
public:
    explicit CurrencyPanelWidget(QWidget *parent = nullptr);

    // Atualiza taxas manualmente (alimentado pelo provider quando vier).
    // rates: {"USD": 5.15, "EUR": 5.60, "BTC": 350000.00, ...} relativo a BRL.
    void setRates(const QMap<QString, double> &rates);

    // Slots ativos (lidos de AppSettings#currencySlot1/2). Força refresh visual.
    void recarregarSlots();

private:
    void buildUi();
    void renderSlot(QLabel *lblCode, QLabel *lblValue, const QString &codigo);

    QLabel *m_lblCode1  = nullptr;
    QLabel *m_lblValue1 = nullptr;
    QLabel *m_lblCode2  = nullptr;
    QLabel *m_lblValue2 = nullptr;
    QMap<QString, double> m_rates;
};
