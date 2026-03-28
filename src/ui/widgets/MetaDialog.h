#pragma once

#include "models/Meta.h"
#include <QDialog>

class QLineEdit;
class QDateEdit;
class QComboBox;
class CalculadoraEdit;

class MetaDialog final : public QDialog {
    Q_OBJECT
public:
    explicit MetaDialog(const Meta &meta, QWidget *parent = nullptr);

    Meta meta() const;

private:
    void buildUi(const Meta &m);

    QLineEdit       *m_edtNome      = nullptr;
    QComboBox       *m_cbCategoria  = nullptr;
    CalculadoraEdit *m_edtMeta      = nullptr;
    CalculadoraEdit *m_edtAtual     = nullptr;
    QDateEdit       *m_dtInicio     = nullptr;
    QDateEdit       *m_dtPrazo      = nullptr;
    QComboBox       *m_cbStatus     = nullptr;
    QLineEdit       *m_edtObs       = nullptr;

    int m_id = 0;
};
