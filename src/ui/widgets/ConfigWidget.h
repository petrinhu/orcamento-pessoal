#pragma once

#include "models/Cartao.h"
#include <QWidget>
#include <QList>

class QTableWidget;
class QPushButton;
class QLineEdit;

// Settings widget: manages credit cards (CRUD).
class ConfigWidget final : public QWidget {
    Q_OBJECT
public:
    explicit ConfigWidget(QWidget *parent = nullptr);

    void recarregar();

signals:
    void cartoesAlterados();

private slots:
    void onAdicionar();
    void onRemover();

private:
    void buildUi();

    QTableWidget *m_table   = nullptr;
    QLineEdit    *m_edtNome = nullptr;
    QPushButton  *m_btnAdd  = nullptr;
    QPushButton  *m_btnRem  = nullptr;
    QList<Cartao> m_cartoes;
};
