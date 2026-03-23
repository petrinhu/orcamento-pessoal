#pragma once

#include "models/LancamentoEntrada.h"
#include "models/FonteRenda.h"

#include <QWidget>
#include <QList>
#include <QDate>

class QTableWidget;
class QPushButton;

// Income tab widget — mirrors ParcelasWidget layout.
// Each month = 4 sub-columns: Fonte | Valor líquido | Data | Obs
// Row 0 = month header (merged 4 cols). Last row = TOTAL.
class EntradasWidget final : public QWidget {
    Q_OBJECT
public:
    explicit EntradasWidget(QWidget *parent = nullptr);

signals:
    void dadosAlterados();
    void statusAtualizado(const QString &msg);

public slots:
    void recarregar();
    void copiar();
    void selecionarTudo();

private slots:
    void onCellActivated(int row, int col);

private:
    void buildUi();
    void populateTable();

    // Returns the LancamentoEntrada for (dataRow, monthIndex), or default-constructed if empty.
    LancamentoEntrada lancamentoDaLinha(int dataRow, int monthIndex) const;
    QList<FonteRenda> fontesDisponiveis() const;

    static const int COL_PER_MONTH = 4; // Fonte | Valor | Data | Obs
    static const int HEADER_ROWS   = 2; // row 0 = mês, row 1 = sub-labels

    QTableWidget *m_table   = nullptr;
    QPushButton  *m_btnDel  = nullptr;

    // Data indexed by month: m_data[k] = list of lançamentos for month k
    QList<QList<LancamentoEntrada>> m_data;
    QDate m_firstMonth;
    int   m_numMonths = 0;
    int   m_maxRows   = 0; // data rows (excl. header row 0 and total row)
};
