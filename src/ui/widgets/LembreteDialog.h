#pragma once

#include "models/Lembrete.h"
#include <QDialog>

class QLineEdit;
class QDateEdit;
class QComboBox;
class QCheckBox;
class CalculadoraEdit;

class LembreteDialog final : public QDialog {
    Q_OBJECT
public:
    explicit LembreteDialog(const Lembrete &lembrete, QWidget *parent = nullptr);

    Lembrete lembrete() const;

private:
    void buildUi(const Lembrete &l);
    void onDebitoToggled(bool checked);

    QLineEdit       *m_edtDescricao = nullptr;
    CalculadoraEdit *m_edtValor     = nullptr;
    QDateEdit       *m_dtVenc       = nullptr;
    QComboBox       *m_cbFreq       = nullptr;
    QCheckBox       *m_chkDebito    = nullptr;
    QComboBox       *m_cbStatus     = nullptr;
    QLineEdit       *m_edtObs       = nullptr;

    int m_id = 0;
};
