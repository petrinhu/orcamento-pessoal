#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;
class QLabel;
class QLineEdit;
class QComboBox;

// ProventosWidget (#66) - lista de proventos recebidos com filtros + CRUD.
// Abre via botão "Proventos" em InvestimentosWidget. Mostra tabela
// (data/ticker/tipo/bruto/líquido), totais por ano, botões CRUD.
class ProventosWidget final : public QWidget {
    Q_OBJECT
public:
    explicit ProventosWidget(QWidget *parent = nullptr);

private slots:
    void onNovo();
    void onEditar();
    void onRemover();
    void onFiltrar();

private:
    void buildUi();
    void recarregarTabela();
    void atualizarTotais();

    QLineEdit    *m_edtTickerFiltro = nullptr;
    QComboBox    *m_cbTipoFiltro    = nullptr;
    QTableWidget *m_tabela          = nullptr;
    QLabel       *m_lblTotais       = nullptr;
    QPushButton  *m_btnNovo         = nullptr;
    QPushButton  *m_btnEditar       = nullptr;
    QPushButton  *m_btnRemover      = nullptr;
};
