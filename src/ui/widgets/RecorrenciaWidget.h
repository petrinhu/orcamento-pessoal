#pragma once

#include "models/TransacaoRecorrente.h"

#include <QWidget>
#include <QList>

class QTableWidget;
class QPushButton;
class QStackedWidget;
class QUndoStack;

class RecorrenciaWidget final : public QWidget {
    Q_OBJECT
public:
    explicit RecorrenciaWidget(QWidget *parent = nullptr);

    void recarregar();
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

signals:
    void dadosAlterados();

private slots:
    void onAdicionar();
    void onEditar();
    void onRemover();
    void onToggleAtivo();

private:
    void buildUi();
    void popularTabela();
    int  idDaLinhaSelecionada() const;

    QStackedWidget *m_stack    = nullptr; // 0=emptyState, 1=table
    QTableWidget   *m_table    = nullptr;
    QPushButton    *m_btnEdit  = nullptr;
    QPushButton    *m_btnRem   = nullptr;
    QPushButton    *m_btnToggle= nullptr;

    QList<TransacaoRecorrente> m_itens;
    QUndoStack *m_undoStack = nullptr;
};
