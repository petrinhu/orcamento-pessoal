#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;
class QLabel;
class QComboBox;
class QStackedWidget;
class QTimer;
class MarketDataService;

// BolsaWidget (#50) - aba "Bolsa" do app. Infraestrutura mínima:
//   - busca de ticker
//   - seletor de provider em runtime (none/frankfurter/brapi/b3direct/cotahist)
//   - tabela de cotações listadas
//   - placeholder para charts/indicadores (entram via #51 #52 #53)
//
// O service de fundo (MarketDataService) cuida do roteamento entre providers.
// Aqui o widget apenas chama search()/fetchQuote() e renderiza resultados.
//
// UX-02 (auditoria 2026-06-22): combo mostrava chave interna crua e mentia
// sucesso quando o provider pedido não estava registrado (caía em silêncio
// pra outro); busca sem resposta ficava "Buscando..." pra sempre. Fix:
// rótulos amigáveis via MarketProviderCatalog, status sempre reflete o
// provider REAL ativo (MarketDataService::activeProviderName), e busca tem
// timeout com mensagem de erro acionável.
class BolsaWidget final : public QWidget {
    Q_OBJECT
public:
    explicit BolsaWidget(MarketDataService *service = nullptr,
                         QWidget *parent = nullptr);

    // Pura, sem estado — testável sem instanciar o widget. Monta a mensagem
    // de status honesta após uma troca de provider: se `sucesso` é falso
    // (provider pedido não registrado), a mensagem SEMPRE reflete quem ficou
    // realmente ativo (`ativoReal`), nunca confirma `solicitado` como ativo.
    static QString mensagemStatusProvider(const QString &solicitado,
                                          bool sucesso,
                                          const QString &ativoReal);

    // Pura — mensagem de erro acionável exibida quando a busca expira sem
    // resposta (nunca deixa "Buscando..." pendurado).
    static QString mensagemTimeoutBusca(const QString &query);

    // Timeout (ms) da busca. Exposto só para encurtar em teste
    // (test_ux02_provider_bolsa.cpp) — produção usa o default.
    void setBuscaTimeoutMsParaTeste(int ms);

private slots:
    void onBuscar();
    void onProviderChanged(int idx);

private:
    void buildUi();
    void carregarProvider();

    static constexpr int kBuscaTimeoutMsDefault = 15000;

    MarketDataService *m_service    = nullptr;
    QComboBox         *m_cbProvider = nullptr;
    QLineEdit         *m_edtBusca   = nullptr;
    QPushButton       *m_btnBuscar  = nullptr;
    QStackedWidget    *m_stack      = nullptr; // 0=emptyState, 1=tabela
    QTableWidget      *m_tabela     = nullptr;
    QLabel            *m_lblStatus  = nullptr;
    QTimer            *m_timeoutBusca = nullptr;
    quint64            m_buscaSeq   = 0; // invalida callback/timeout de busca anterior
    int                m_buscaTimeoutMs = kBuscaTimeoutMsDefault;
};
