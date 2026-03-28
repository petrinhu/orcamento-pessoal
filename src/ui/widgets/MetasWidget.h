#pragma once

#include "models/Meta.h"

#include <QWidget>
#include <QList>

class QTableWidget;
class QPushButton;
class QStackedWidget;
class QComboBox;
class QUndoStack;

// Aba de metas financeiras.
// Tabela plana: cada linha = uma meta.
// Colunas: Nome | Categoria | Meta | Acumulado | Progresso | Prazo | Restam | Status | Obs
class MetasWidget final : public QWidget {
    Q_OBJECT
public:
    explicit MetasWidget(QWidget *parent = nullptr);
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
    void onAporte();
    void onCellDoubleClicked(int row, int col);
    void onSelectionChanged();

private:
    void buildUi();
    void populateTable(const QList<Meta> &lista);
    QList<Meta> filteredData() const;

    enum Col {
        ColNome = 0, ColCategoria, ColMeta, ColAtual,
        ColProgresso, ColPrazo, ColRestam, ColStatus, ColObs,
        ColCount
    };

    QStackedWidget *m_stack       = nullptr;
    QTableWidget   *m_table       = nullptr;
    QPushButton    *m_btnEdit     = nullptr;
    QPushButton    *m_btnDel      = nullptr;
    QPushButton    *m_btnAporte   = nullptr;
    QComboBox      *m_cbFiltro    = nullptr;

    QList<Meta> m_data;
    int         m_selectedId = 0;

    QUndoStack *m_undoStack = nullptr;
};
