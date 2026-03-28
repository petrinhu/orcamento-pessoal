#pragma once

#include "models/Divida.h"

#include <QWidget>
#include <QList>

class QTableWidget;
class QPushButton;
class QStackedWidget;
class QComboBox;
class QUndoStack;

// Dívidas e empréstimos tab widget.
// Tabela plana (não por mês): cada linha = uma dívida ou empréstimo.
// Colunas: Tipo | Contraparte | Descrição | Total | Pago | Saldo | Juros a.m. | Vencimento | Status | Obs
// Última linha = TOTAL (id == -1 por convenção).
class DividasWidget final : public QWidget {
    Q_OBJECT
public:
    explicit DividasWidget(QWidget *parent = nullptr);
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

signals:
    void dadosAlterados();
    void statusAtualizado(const QString &msg);

public slots:
    void recarregar();
    void copiar();
    void selecionarTudo();

private slots:
    void onNova();
    void onEditar();
    void onExcluir();
    void onQuitar();
    void onCellDoubleClicked(int row, int col);
    void onSelectionChanged();

private:
    void buildUi();
    void populateTable(const QList<Divida> &lista);
    QList<Divida> filteredData() const;

    // Columns
    enum Col {
        ColTipo = 0, ColContraparte, ColDescricao,
        ColTotal, ColPago, ColSaldo,
        ColJuros, ColVenc, ColStatus, ColObs,
        ColCount
    };

    QStackedWidget *m_stack   = nullptr;
    QTableWidget   *m_table   = nullptr;
    QPushButton    *m_btnEdit = nullptr;
    QPushButton    *m_btnDel  = nullptr;
    QPushButton    *m_btnQuit = nullptr;
    QComboBox      *m_cbFiltroStatus = nullptr;
    QComboBox      *m_cbFiltroTipo   = nullptr;

    QList<Divida> m_data;       // todos os registros (sem filtro)
    int           m_selectedId = 0;

    QUndoStack *m_undoStack = nullptr;
};
