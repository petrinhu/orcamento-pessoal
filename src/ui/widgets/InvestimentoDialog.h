#pragma once

#include "models/Categoria.h"
#include "models/LancamentoInvestimento.h"

#include <QDialog>
#include <QList>

class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;
class QRadioButton;
class CalculadoraEdit;
class ReceiptAttachmentWidget;

// Dialog that opens when the user activates a cell in InvestimentosWidget.
// Pre-fills with existing LancamentoInvestimento data (or empty for new entry).
// Returns 1 or N lançamentos (N when recurrence is selected).
class InvestimentoDialog final : public QDialog {
    Q_OBJECT
public:
    explicit InvestimentoDialog(const LancamentoInvestimento &inv,
                                int mes, int ano,
                                const QList<Categoria> &categorias,
                                QWidget *parent = nullptr);

    QList<LancamentoInvestimento> lancamentos() const;

protected:
    void accept() override;

private slots:
    void onRecorrenciaToggled(bool recorrente);

private:
    void buildUi(const QList<Categoria> &categorias);

    // #134: valida o lançamento antes de fechar. Lista vazia = OK.
    // Delega a regra share-based (ticker/qty/preço) ao IInvestActivity (#89)
    // só quando o lançamento é de ativo negociável (ticker preenchido ou
    // tipo Ação/FII); senão valida por valor para não bloquear CDB/Tesouro/etc.
    QStringList validarConsistencia(const LancamentoInvestimento &l) const;

    LancamentoInvestimento m_original;
    int m_mes;
    int m_ano;

    QLineEdit       *m_edtNome      = nullptr;
    QComboBox       *m_cbCategoria  = nullptr;
    QComboBox       *m_cbTipoAtivo  = nullptr;
    QComboBox       *m_cbOperacao   = nullptr;
    CalculadoraEdit *m_edtValor     = nullptr;
    QDateEdit       *m_dateEdit     = nullptr;
    QLineEdit       *m_edtObs       = nullptr;
    QRadioButton    *m_rbSomente    = nullptr;
    QRadioButton    *m_rbRecorrente = nullptr;
    QSpinBox        *m_spnMeses     = nullptr;
    ReceiptAttachmentWidget *m_receipt = nullptr;
    // #48: tracking por ticker
    QLineEdit       *m_edtTicker    = nullptr;
    QDoubleSpinBox  *m_spnQty       = nullptr;
    CalculadoraEdit *m_edtPrecoUnit = nullptr;
};
