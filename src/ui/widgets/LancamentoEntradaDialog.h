#pragma once

#include "models/LancamentoEntrada.h"
#include "models/FonteRenda.h"

#include <QDialog>
#include <QList>

class QComboBox;
class QLineEdit;
class QDateEdit;
class QSpinBox;
class QRadioButton;
class QGroupBox;
class QLabel;

// Dialog that opens whenever the user activates a cell in EntradasWidget.
// Pre-fills with existing LancamentoEntrada data (or empty for new entry).
// Returns a list of LancamentoEntrada (1 or N when recurrent).
class LancamentoEntradaDialog final : public QDialog {
    Q_OBJECT
public:
    explicit LancamentoEntradaDialog(const LancamentoEntrada &lancamento,
                                     int mes, int ano,
                                     const QList<FonteRenda> &fontes,
                                     QWidget *parent = nullptr);

    // Returns 1 entry normally; N entries if recurrence selected.
    QList<LancamentoEntrada> lancamentos() const;

private slots:
    void onValoresChanged();
    void onRecorrenciaToggled(bool recorrente);

private:
    void buildUi(const QList<FonteRenda> &fontes);
    qint64 parseValor(const QString &txt) const;
    QString formatValor(qint64 centavos) const;

    LancamentoEntrada m_original; // id > 0 if editing
    int m_mes;
    int m_ano;

    QComboBox    *m_cbFonte      = nullptr;
    QLineEdit    *m_edtValor     = nullptr;
    QDateEdit    *m_dateEdit     = nullptr;
    QGroupBox    *m_grpImpostos  = nullptr;
    QLineEdit    *m_edtIrpf      = nullptr;
    QLineEdit    *m_edtInss      = nullptr;
    QLabel       *m_lblLiquido   = nullptr;
    QRadioButton *m_rbSomente    = nullptr;
    QRadioButton *m_rbRecorrente = nullptr;
    QSpinBox     *m_spnMeses     = nullptr;
    QLineEdit    *m_edtObs       = nullptr;
};
