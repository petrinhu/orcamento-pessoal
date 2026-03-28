#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;

// CRUD widget for managing Pagadores/Recebedores.
// Lives in the Preferences tab widget.
class ConfigPagadoresWidget final : public QWidget {
    Q_OBJECT
public:
    explicit ConfigPagadoresWidget(QWidget *parent = nullptr);

signals:
    void pagadoresAlterados();

private slots:
    void onAdicionar();
    void onEditar();
    void onRemover();

private:
    void buildUi();
    void recarregar();

    QTableWidget *m_table   = nullptr;
    QPushButton  *m_btnEdit = nullptr;
    QPushButton  *m_btnRem  = nullptr;
};
