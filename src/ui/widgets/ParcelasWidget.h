#pragma once

#include "models/Compra.h"
#include "models/Parcela.h"
#include "models/Cartao.h"

#include <QWidget>
#include <QList>
#include <QDate>

class QTableWidget;
class QPushButton;
class QLabel;
class QHeaderView;
class QStackedWidget;
class CompraDetailPanel;
class QUndoStack;

// Main view: matrix of compras (rows) × months (columns).
// Current month is always the first visible column.
// Past months scroll left; future months scroll right.
// Bottom row is always "Total".
//
// Features:
//  - Click column header to sort (desc → asc toggle)
//  - "Organizar por" menu: up to 3-level sort
//  - Direct cell editing with recalc dialog
//  - "Nova compra" / "Excluir" buttons
class ParcelasWidget final : public QWidget {
    Q_OBJECT
public:
    explicit ParcelasWidget(QWidget *parent = nullptr);

    void recarregar();
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

public slots:
    void copiar();
    void selecionarTudo();
    void scrollParaMesAtual();
    void navegarMes(int delta); // delta = ±N meses

    // Open AddCompraDialog pre-filled with TVM data from the Financeira calculator.
    void novaCompraDeCalculadora(double pv, int n, double iMensal, double pmt);

signals:
    void dadosAlterados();
    void statusAtualizado(const QString &msg);

private slots:
    void onNovaCompra();
    void onExcluir();
    void onHeaderClicked(int coluna);
    void onOrganizarPor();
    void onCellChanged(int row, int col);
    void onCompraAlterada(const Compra &c);

private:
    void buildUi();
    void populateTable();
    void aplicarOrdenacao();

    // Returns the QDate for column index (0 = current month)
    QDate dateForColumn(int col) const;
    int   columnForDate(const QDate &date) const;

    // Sort state
    struct SortLevel {
        int  coluna   = -1;
        bool crescente = true;
    };
    QList<SortLevel> m_sortLevels; // up to 3
    int  m_lastClickedCol = -1;
    bool m_sortAsc        = true;

    // Data
    QList<Compra>         m_compras;
    QList<QList<Parcela>> m_parcelasPorCompra; // index matches m_compras
    QList<Cartao>         m_cartoes;

    // UI
    QStackedWidget    *m_stack        = nullptr;
    QTableWidget      *m_table        = nullptr;
    QPushButton       *m_btnNova      = nullptr;
    QPushButton       *m_btnExcluir   = nullptr;
    QLabel            *m_lblStatus    = nullptr;
    CompraDetailPanel *m_detailPanel  = nullptr;

    int m_selectedCompraId = 0; // 0 = none selected

    bool m_loading = false; // guard against recursive itemChanged

    QUndoStack *m_undoStack = nullptr;

    // Month range shown: from m_firstMonth to m_lastMonth
    QDate m_firstMonth; // earliest parcela date among all compras
    QDate m_lastMonth;  // latest parcela date + 1 extra
    QDate m_currentMonth;

    // Fixed column indices
    static constexpr int COL_NOME   = 0;
    static constexpr int COL_CARTAO = 1;
    static constexpr int COL_TOTAL  = 2;
    static constexpr int COL_FIRST_MONTH = 3; // months start here
};
