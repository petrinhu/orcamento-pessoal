#pragma once

#include "models/Compra.h"
#include "services/ParcelaService.h"

#include <QWidget>

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

    QComboBox   *m_cmbSolveFor     = nullptr;
    QLineEdit   *m_edtTotal        = nullptr;
    QLineEdit   *m_edtNumParcelas  = nullptr;
    QLineEdit   *m_edtJurosMensais = nullptr;
    QLineEdit   *m_edtCet          = nullptr;
    QLineEdit   *m_edtValorParcela = nullptr;
    QLineEdit   *m_edtJurosTotais  = nullptr;
    QPushButton *m_btnAmortizacao  = nullptr;
};
