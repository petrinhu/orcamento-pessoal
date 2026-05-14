#pragma once

#include "models/AjusteCorporativo.h"

#include <QDialog>

class QLineEdit;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;

// AjusteCorporativoDialog (#65) - CRUD de evento corporativo (split,
// inplit, bonificacao). Fator = multiplicador aplicado a qty historica.
class AjusteCorporativoDialog final : public QDialog {
    Q_OBJECT
public:
    explicit AjusteCorporativoDialog(const AjusteCorporativo &original = {},
                                      QWidget *parent = nullptr);

    [[nodiscard]] AjusteCorporativo result() const;

private:
    void buildUi();

    AjusteCorporativo m_original;
    QLineEdit       *m_edtTicker  = nullptr;
    QComboBox       *m_cbTipo     = nullptr;
    QDateEdit       *m_dateEx     = nullptr;
    QDoubleSpinBox  *m_spnFator   = nullptr;
    QLineEdit       *m_edtObs     = nullptr;
};
