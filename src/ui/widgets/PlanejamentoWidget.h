#pragma once

#include <QWidget>
#include <QDate>

class QTableWidget;
class QPushButton;
class QStackedWidget;
class QLabel;
class QUndoStack;

// Budget/planning tab — single-month view with ◀ month ▶ navigation.
// Rows = budget items (grouped: Receitas then Despesas).
// Columns: Nome | Planejado | Real | Diferença | %
// "Real" values are auto-computed from lancamentos_gastos/entradas for the selected month.
class PlanejamentoWidget final : public QWidget {
    Q_OBJECT
public:
    explicit PlanejamentoWidget(QWidget *parent = nullptr);
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

signals:
    void dadosAlterados();
    void statusAtualizado(const QString &msg);

public slots:
    void recarregar();

private slots:
    void onCellDoubleClicked(int row, int col);
    void onExcluir();

private:
    void buildUi();
    void populateTable();
    void updateMesLabel();

    static const int COL_COUNT = 5; // Nome | Planejado | Real | Diferença | %

    QStackedWidget *m_stack  = nullptr;
    QTableWidget   *m_table  = nullptr;
    QPushButton    *m_btnDel = nullptr;
    QLabel         *m_lblMes = nullptr;

    int m_mes;
    int m_ano;

    // Row index of each budget item (for double-click editing); -1 = not an item row
    QList<int> m_itemIds; // parallel to table rows: itemId or 0 if section/total

    QUndoStack *m_undoStack = nullptr;
};
