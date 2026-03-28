#pragma once

#include "models/Categoria.h"
#include "models/FonteRenda.h"
#include "models/ItemOrcamento.h"

#include <QDialog>
#include <QList>

class QComboBox;
class QLabel;
class QLineEdit;
class CalculadoraEdit;

// Dialog for creating/editing an ItemOrcamento (budget line item).
class ItemOrcamentoDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ItemOrcamentoDialog(const ItemOrcamento &item,
                                 const QList<Categoria> &categorias,
                                 const QList<FonteRenda> &fontes,
                                 QWidget *parent = nullptr);

    ItemOrcamento result() const;

private slots:
    void onTipoChanged(int index);

private:
    void buildUi();
    void repopulateCatRef();

    ItemOrcamento      m_original;
    QList<Categoria>   m_categorias;
    QList<FonteRenda>  m_fontes;

    QLineEdit       *m_edtNome      = nullptr;
    QComboBox       *m_cbTipo       = nullptr;
    CalculadoraEdit *m_edtValor     = nullptr;
    QComboBox       *m_cbCatRef     = nullptr;
    QLabel          *m_lblCatHint   = nullptr;
};
