#pragma once

#include "models/GastoAvulso.h"

#include <QWidget>
#include <QList>
#include <QDate>

class QTableWidget;
class QPushButton;
class QStackedWidget;

enum class SortFieldGastos { None, Descricao, Categoria, Tipo, Valor, Data };

// Gastos tab widget — mirrors EntradasWidget layout.
// Each month = 6 sub-columns: Descrição | Categoria | Tipo | Valor | Data | Obs
// Row 0 = month header (merged 6 cols). Row 1 = sub-labels. Last row = TOTAL.
class GastosWidget final : public QWidget {
    Q_OBJECT
public:
    explicit GastosWidget(QWidget *parent = nullptr);

signals:
    void dadosAlterados();
    void statusAtualizado(const QString &msg);

public slots:
    void recarregar();
    void copiar();
    void selecionarTudo();
    void scrollParaMesAtual();
    void navegarMes(int delta);

private slots:
    void onCellClicked(int row, int col);
    void onOrganizarPor();

private:
    void buildUi();
    void populateTable();

    GastoAvulso gastoDaLinha(int dataRow, int monthIndex) const;

    static const int COL_PER_MONTH = 6; // Descrição | Categoria | Tipo | Valor | Data | Obs
    static const int HEADER_ROWS   = 2; // row 0 = mês, row 1 = sub-labels

    QStackedWidget *m_stack  = nullptr;
    QTableWidget   *m_table  = nullptr;
    QPushButton    *m_btnDel = nullptr;
    QPushButton    *m_btnOrg = nullptr;

    SortFieldGastos m_sortField = SortFieldGastos::None;
    bool            m_sortAsc   = true;

    QList<QList<GastoAvulso>> m_data;
    QDate m_firstMonth;
    int   m_numMonths = 0;
    int   m_maxRows   = 0;
};
