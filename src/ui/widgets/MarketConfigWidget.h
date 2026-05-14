#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

// MarketConfigWidget (#46) - aba Mercado de PrefsWidget.
//
// Persiste em AppSettings:
//   - provider padrao (none/frankfurter/brapi/b3direct/cotahist)
//   - credenciais Brapi (token + plano Free/Startup/Pro)
//   - intervalo de polling (segundos)
//   - COTAHIST: enabled + anos historico
//   - slots de moeda 1/2 do painel cambio (#47)
class MarketConfigWidget final : public QWidget {
    Q_OBJECT
public:
    explicit MarketConfigWidget(QWidget *parent = nullptr);

private slots:
    void onSalvar();
    void onTestarBrapi();

private:
    void buildUi();
    void carregarConfig();

    QComboBox   *m_cbProvider  = nullptr;
    QLineEdit   *m_edtToken    = nullptr;
    QComboBox   *m_cbPlano     = nullptr;
    QPushButton *m_btnTestar   = nullptr;
    QSpinBox    *m_spnPolling  = nullptr;
    QCheckBox   *m_chkCotahist = nullptr;
    QSpinBox    *m_spnAnos     = nullptr;
    QComboBox   *m_cbSlot1     = nullptr;
    QComboBox   *m_cbSlot2     = nullptr;
    QPushButton *m_btnSalvar   = nullptr;
};
