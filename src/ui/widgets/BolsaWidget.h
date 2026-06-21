#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;
class QLabel;
class QComboBox;
class QStackedWidget;
class MarketDataService;

// BolsaWidget (#50) - aba "Bolsa" do app. Infraestrutura mínima:
//   - busca de ticker
//   - seletor de provider em runtime (none/frankfurter/brapi/b3direct/cotahist)
//   - tabela de cotações listadas
//   - placeholder para charts/indicadores (entram via #51 #52 #53)
//
// O service de fundo (MarketDataService) cuida do roteamento entre providers.
// Aqui o widget apenas chama search()/fetchQuote() e renderiza resultados.
class BolsaWidget final : public QWidget {
    Q_OBJECT
public:
    explicit BolsaWidget(MarketDataService *service = nullptr,
                         QWidget *parent = nullptr);

private slots:
    void onBuscar();
    void onProviderChanged(int idx);

private:
    void buildUi();
    void carregarProvider();

    MarketDataService *m_service    = nullptr;
    QComboBox         *m_cbProvider = nullptr;
    QLineEdit         *m_edtBusca   = nullptr;
    QPushButton       *m_btnBuscar  = nullptr;
    QStackedWidget    *m_stack      = nullptr; // 0=emptyState, 1=tabela
    QTableWidget      *m_tabela     = nullptr;
    QLabel            *m_lblStatus  = nullptr;
};
