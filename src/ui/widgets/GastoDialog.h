#pragma once

#include "models/Categoria.h"
#include "models/GastoAvulso.h"
#include "models/Pagador.h"

#include <QDialog>
#include <QList>
#include <QStringList>

class QComboBox;
class QDateEdit;
class QLineEdit;
class QSpinBox;
class QRadioButton;
class CalculadoraEdit;

// Dialog that opens when the user activates a cell in GastosWidget.
// Pre-fills with existing GastoAvulso data (or empty for new entry).
// Returns 1 or N gastos (N when recurrence is selected).
class GastoDialog final : public QDialog {
    Q_OBJECT
public:
    explicit GastoDialog(const GastoAvulso &gasto,
                         int mes, int ano,
                         const QList<Categoria> &categorias,
                         const QList<Pagador> &pagadores,
                         QWidget *parent = nullptr);

    QList<GastoAvulso> gastos() const;

private slots:
    void onRecorrenciaToggled(bool recorrente);
    void onCriarBeneficiario();
    void onAccept();

private:
    void buildUi(const QList<Categoria> &categorias);
    void popularBeneficiarios();

    GastoAvulso    m_original;
    int            m_mes;
    int            m_ano;
    QList<Pagador> m_pagadores;

    // Resolved on accept
    int     m_resolvedPagadorId    = 0;
    QString m_resolvedPagadorNome;
    int     m_resolvedCategoriaId  = 0;
    QString m_resolvedCategoriaNome;

    QLineEdit      *m_edtDescricao      = nullptr;
    QComboBox      *m_cbCategoria       = nullptr;
    QComboBox      *m_cbTipo            = nullptr;
    CalculadoraEdit*m_edtValor          = nullptr;
    QDateEdit      *m_dateEdit          = nullptr;
    QLineEdit      *m_edtObs            = nullptr;
    QComboBox      *m_cbBeneficiario    = nullptr;
    QRadioButton   *m_rbSomente         = nullptr;
    QRadioButton   *m_rbRecorrente      = nullptr;
    QSpinBox       *m_spnMeses          = nullptr;
};
