#pragma once

#include "models/Categoria.h"
#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;

class ConfigCategoriasWidget final : public QWidget {
    Q_OBJECT
public:
    explicit ConfigCategoriasWidget(QWidget *parent = nullptr);

    void recarregar();

signals:
    void categoriasAlteradas();

private slots:
    void onAdicionar();
    void onEditar();
    void onRemover();
    void onItemDoubleClicked(QTreeWidgetItem *item, int col);
    void onSelectionChanged();

private:
    void buildUi();
    Categoria categoriaFromItem(QTreeWidgetItem *item) const;

    QTreeWidget *m_tree     = nullptr;
    QPushButton *m_btnAdd   = nullptr;
    QPushButton *m_btnEdit  = nullptr;
    QPushButton *m_btnRem   = nullptr;
};
