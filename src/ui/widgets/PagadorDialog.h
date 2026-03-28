#pragma once

#include "models/Pagador.h"

#include <QDialog>

class QLineEdit;
class QComboBox;

// Dialog to create or edit a Pagador/Recebedor entry.
// Pass an empty Pagador for "new"; pass an existing one for "edit".
class PagadorDialog final : public QDialog {
    Q_OBJECT
public:
    explicit PagadorDialog(const Pagador &pagador, QWidget *parent = nullptr);

    Pagador resultado() const;

private:
    void buildUi();

    Pagador     m_original;
    QLineEdit  *m_edtNome = nullptr;
    QComboBox  *m_cbTipo  = nullptr;
    QLineEdit  *m_edtObs  = nullptr;
};
