#pragma once

#include "models/Provento.h"

#include <QDialog>

class QLineEdit;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class CalculadoraEdit;

// ProventoDialog (#66) - CRUD de Provento (dividendo/JCP/rendimento/bonus).
// Snapshot do ticker preserva historico. Calcula valor liquido a partir do
// bruto e aliquota IR (JCP retém 15%, demais isentos PF).
class ProventoDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ProventoDialog(const Provento &original = {},
                            QWidget *parent = nullptr);

    [[nodiscard]] Provento result() const;

private slots:
    void recalcularLiquido();

private:
    void buildUi();

    Provento m_original;

    QLineEdit       *m_edtTicker  = nullptr;
    QComboBox       *m_cbTipo     = nullptr;
    QDateEdit       *m_dateEvento = nullptr;
    CalculadoraEdit *m_edtBruto   = nullptr;
    CalculadoraEdit *m_edtLiquido = nullptr;
    QDoubleSpinBox  *m_spnQty     = nullptr;
    QDoubleSpinBox  *m_spnPorAcao = nullptr;
    QLineEdit       *m_edtObs     = nullptr;
};
