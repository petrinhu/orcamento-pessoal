#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;
class QLineEdit;

// Settings widget: manages income sources (Fontes de Renda) CRUD.
class ConfigFontesWidget final : public QWidget {
    Q_OBJECT
public:
    explicit ConfigFontesWidget(QWidget *parent = nullptr);

    void recarregar();

signals:
    void fontesAlteradas();

private slots:
    void onAdicionar();
    void onRemover();

private:
    void buildUi();

    QTableWidget *m_table   = nullptr;
    QLineEdit    *m_edtNome = nullptr;
    QPushButton  *m_btnAdd  = nullptr;
    QPushButton  *m_btnRem  = nullptr;
};
