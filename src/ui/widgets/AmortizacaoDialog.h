#pragma once

#include "models/Compra.h"

#include <QDialog>

class QTableWidget;

// Shows the Price (SAF) amortization schedule for a compra.
// Columns: Nº | Data | Saldo devedor | Juros | Amortização | Parcela
// Read-only view; no editing.
class AmortizacaoDialog final : public QDialog {
    Q_OBJECT
public:
    explicit AmortizacaoDialog(const Compra &compra, QWidget *parent = nullptr);

private:
    void buildUi(const Compra &compra);

    QTableWidget *m_table = nullptr;
};
