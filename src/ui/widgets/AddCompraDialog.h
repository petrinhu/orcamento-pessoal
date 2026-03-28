#pragma once

#include "models/Compra.h"
#include "models/Parcela.h"
#include "services/ParcelaService.h"

#include <QDialog>
#include <QList>

class CalculadoraEdit;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QLabel;
class QDateEdit;
class QTabWidget;
class QTableWidget;
class QPushButton;

// Dialog to add a new purchase (compra) with installments.
// Supports all input modes:
//   Tab 1 — Simples:  parcela value + count (no interest)
//   Tab 2 — Total:    total value + count → calculate per-parcela
//   Tab 3 — Juros:    any combo + interest rate → show CET/totals
//
// The dialog always shows a live preview of generated parcelas.
class AddCompraDialog final : public QDialog {
    Q_OBJECT
public:
    explicit AddCompraDialog(const QList<struct Cartao> &cartoes,
                             QWidget *parent = nullptr);

    Compra         compra()   const { return m_compra; }
    QList<Parcela> parcelas() const { return m_parcelas; }

    // Pre-fill from the TVM calculator (Financeira tab → "Enviar para Cartões").
    // Switches to the Juros tab and populates PV, N, i%mês; recalculates PMT.
    void preencherJuros(double pv, int n, double iMensal);

private slots:
    void recalcular();
    void onTabChanged(int idx);

private:
    void buildUi();
    void buildTabSimples(QWidget *tab);
    void buildTabTotal(QWidget *tab);
    void buildTabJuros(QWidget *tab);
    void buildPreview();
    void atualizarPreview();
    void atualizarEstadoCamposJ(); // enable/disable/lock fields per m_solveForJ
    void accept() override;

    // Common fields (all tabs)
    QLineEdit    *m_nome        = nullptr;
    QComboBox    *m_cartao      = nullptr;
    QComboBox    *m_moeda       = nullptr;
    QDateEdit    *m_dataInicio  = nullptr;
    QSpinBox     *m_parcelaIni  = nullptr; // "já é a Xª parcela"
    QSpinBox     *m_numParcelas_s = nullptr; // Simples tab
    QSpinBox     *m_numParcelas_t = nullptr; // Total tab
    QSpinBox     *m_numParcelas_j = nullptr; // Juros tab

    // Simples tab
    CalculadoraEdit *m_valorParcela  = nullptr;

    // Total tab
    CalculadoraEdit *m_valorTotal_t  = nullptr;

    // Juros tab — fields
    CalculadoraEdit *m_valorTotal_j    = nullptr;  // Valor à vista (PV)
    CalculadoraEdit *m_valorParcela_j  = nullptr;  // PMT
    CalculadoraEdit *m_valorTotalPago_j = nullptr; // Total pago = PMT × N
    QDoubleSpinBox *m_jurosMensais    = nullptr;
    QDoubleSpinBox *m_cet_j           = nullptr;
    QLabel         *m_lblJurosTotais  = nullptr;

    // Juros tab — lock buttons (✏️ = input, 🚫 = output/calculated)
    QPushButton *m_lockTotal      = nullptr;
    QPushButton *m_lockN          = nullptr;
    QPushButton *m_lockTaxa       = nullptr;
    QPushButton *m_lockParcela    = nullptr;
    QPushButton *m_lockTotalPago  = nullptr;

    // Which of the 4 Price-formula fields is auto-calculated
    SolveFor m_solveForJ       = SolveFor::Parcela;
    // TotalPago has its own independent lock (not part of SolveFor)
    bool     m_totalPagoIsInput = false;

    // Preview table
    QTableWidget *m_preview    = nullptr;

    QTabWidget   *m_tabs       = nullptr;

    // Result
    Compra         m_compra;
    QList<Parcela> m_parcelas;
    QList<Cartao>  m_cartoes;
};
