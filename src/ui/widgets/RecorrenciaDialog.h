#pragma once

#include "models/Categoria.h"
#include "models/FonteRenda.h"
#include "models/TransacaoRecorrente.h"

#include <QDialog>
#include <QList>

class QComboBox;
class QDateEdit;
class QLineEdit;
class QCheckBox;
class QLabel;
class QStackedWidget;
class CalculadoraEdit;

class RecorrenciaDialog final : public QDialog {
    Q_OBJECT
public:
    explicit RecorrenciaDialog(const TransacaoRecorrente &tr,
                               const QList<FonteRenda> &fontes,
                               const QList<Categoria> &categorias,
                               QWidget *parent = nullptr);

    TransacaoRecorrente resultado() const;

private slots:
    void onTipoChanged(int index);

private:
    void buildUi(const QList<FonteRenda> &fontes, const QList<Categoria> &categorias);
    void populate(const TransacaoRecorrente &tr);

    TransacaoRecorrente m_original;

    QLineEdit       *m_edtDescricao  = nullptr;
    QComboBox       *m_cbTipo        = nullptr;
    CalculadoraEdit *m_edtValor      = nullptr;
    QComboBox       *m_cbFrequencia  = nullptr;
    QDateEdit       *m_dateInicio    = nullptr;
    QCheckBox       *m_chkSemFim     = nullptr;
    QDateEdit       *m_dateFim       = nullptr;
    QLineEdit       *m_edtObs        = nullptr;

    // Campos dinâmicos por tipo
    QStackedWidget  *m_stackCampos   = nullptr;
    // página 0 = entrada
    QComboBox       *m_cbFonte       = nullptr;
    // página 1 = gasto
    QComboBox       *m_cbCategoria   = nullptr;
    QComboBox       *m_cbTipoPgto    = nullptr;

    QList<FonteRenda> m_fontes;
};
