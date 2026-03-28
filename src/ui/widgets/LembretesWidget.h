#pragma once

#include "models/Lembrete.h"

#include <QWidget>
#include <QList>

class QTableWidget;
class QPushButton;
class QStackedWidget;
class QComboBox;
class QUndoStack;

// Aba de lembretes e alertas.
// Tabela plana ordenada por data de vencimento.
// Colunas: Descrição | Valor | Vencimento | Frequência | Tipo | Status | Obs
class LembretesWidget final : public QWidget {
    Q_OBJECT
public:
    explicit LembretesWidget(QWidget *parent = nullptr);
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

    // Retorna número de alertas urgentes (vencidos + não debitados).
    // Usado pelo MainWindow ao iniciar o app.
    static int contarUrgentes();

signals:
    void dadosAlterados();
    void statusAtualizado(const QString &msg);

public slots:
    void recarregar();
    void copiar();
    void selecionarTudo();

private slots:
    void onNovo();
    void onEditar();
    void onExcluir();
    void onPagar();
    void onNaoDebitado();
    void onIgnorar();
    void onCellDoubleClicked(int row, int col);
    void onSelectionChanged();

private:
    void buildUi();
    void populateTable(const QList<Lembrete> &lista);
    QList<Lembrete> filteredData() const;

    // Marca status e gera próxima ocorrência se frequência != único.
    void marcarStatus(int id, const QString &novoStatus);

    enum Col {
        ColDescricao = 0, ColValor, ColVenc, ColFreq,
        ColTipo, ColStatus, ColObs,
        ColCount
    };

    QStackedWidget *m_stack         = nullptr;
    QTableWidget   *m_table         = nullptr;
    QPushButton    *m_btnEdit       = nullptr;
    QPushButton    *m_btnDel        = nullptr;
    QPushButton    *m_btnPagar      = nullptr;
    QPushButton    *m_btnNaoDebito  = nullptr;
    QPushButton    *m_btnIgnorar    = nullptr;
    QComboBox      *m_cbFiltro      = nullptr;

    QList<Lembrete> m_data;
    int             m_selectedId = 0;

    QUndoStack *m_undoStack = nullptr;
};
