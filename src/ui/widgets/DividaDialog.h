#pragma once

#include "models/Divida.h"
#include <QDialog>

class QRadioButton;
class QLineEdit;
class QDoubleSpinBox;
class QDateEdit;
class QComboBox;
class CalculadoraEdit;

class DividaDialog final : public QDialog {
    Q_OBJECT
public:
    explicit DividaDialog(const Divida &divida, QWidget *parent = nullptr);

    Divida divida() const;

private:
    void buildUi(const Divida &d);

    QRadioButton    *m_rDivida      = nullptr;
    QRadioButton    *m_rEmprestimo  = nullptr;
    QLineEdit       *m_edtContraparte = nullptr;
    QLineEdit       *m_edtDescricao   = nullptr;
    CalculadoraEdit *m_edtTotal       = nullptr;
    CalculadoraEdit *m_edtPago        = nullptr;
    QDoubleSpinBox  *m_spnJuros       = nullptr;   // juros a.m. — fonte da verdade
    QDoubleSpinBox  *m_spnJurosAnual  = nullptr;   // juros a.a. — derivado (composto)
    QDateEdit       *m_dtInicio       = nullptr;
    QDateEdit       *m_dtVenc         = nullptr;
    QComboBox       *m_cbStatus       = nullptr;
    QLineEdit       *m_edtObs         = nullptr;

    bool m_sincronizando = false;   // flag-guard contra loop de sinais a.m. <-> a.a.
    int m_id = 0;
};
