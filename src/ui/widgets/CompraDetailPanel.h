#pragma once

#include "models/Compra.h"
#include "services/ParcelaService.h"

#include <QWidget>

class CalculadoraEdit;
class QLineEdit;
class QComboBox;
class QPushButton;

// Bottom panel shown when a compra row is selected in ParcelasWidget.
// Displays: Total | Nº Parcelas | Juros a.m. | CET | Valor parcela | Juros totais
// The "Calcular:" combo controls which field is the output (disabled/gray).
// Editing any enabled field + pressing Enter/Tab triggers recalculation.
// Emits compraAlterada(Compra) when the user confirms changes.
class CompraDetailPanel final : public QWidget {
    Q_OBJECT
public:
    explicit CompraDetailPanel(QWidget *parent = nullptr);

    // Load a compra into the panel (makes panel visible if hidden).
    void carregarCompra(const Compra &c);

    // Clear panel (hides it).
    void limpar();

signals:
    void compraAlterada(Compra compra);

private slots:
    void onSolveForChanged(int index);
    void onValorTotalEdited();
    void onNumParcelasEdited();
    void onJurosMensaisEdited();
    void onCetEdited();
    void onValorParcelaEdited();
    void onAbrirAmortizacao();

private:
    void buildUi();
    void atualizarCampos();    // write m_dados → UI fields
    void readFromFields();     // read UI fields → m_dados (skip disabled ones)
    void recalcularEEmitir();  // recalcular + aplicar + emit
    void atualizarEstadoCampos(); // enable/disable fields based on m_solveFor

    static QString formatPct(double pct);  // "1,50%"
    static double  parsePct(const QString &s); // "1,50%" → 1.5

    Compra           m_compra;
    DadosFinanceiros m_dados;
    SolveFor         m_solveFor = SolveFor::Parcela;

    QComboBox       *m_cmbSolveFor     = nullptr;
    CalculadoraEdit *m_edtTotal        = nullptr;  // monetary
    QLineEdit       *m_edtNumParcelas  = nullptr;  // count — plain QLineEdit
    QLineEdit       *m_edtJurosMensais = nullptr;  // % — plain QLineEdit
    QLineEdit       *m_edtCet          = nullptr;  // % — plain QLineEdit
    CalculadoraEdit *m_edtValorParcela = nullptr;  // monetary
    CalculadoraEdit *m_edtJurosTotais  = nullptr;  // monetary (always read-only → button hidden)
    QPushButton *m_btnAmortizacao  = nullptr;
};
