#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;
class QLineEdit;

// AjustesCorporativosWidget (#65) - CRUD de splits/inplits/bonificacoes.
// Aba "Ajustes" em PrefsWidget. Aplica retroativamente em precos_ativos
// quando ticker tem historico (delegado para AjusteCorporativoService).
class AjustesCorporativosWidget final : public QWidget {
    Q_OBJECT
public:
    explicit AjustesCorporativosWidget(QWidget *parent = nullptr);

private slots:
    void onNovo();
    void onRemover();

private:
    void buildUi();
    void recarregar();

    QLineEdit    *m_edtFiltro = nullptr;
    QTableWidget *m_tabela    = nullptr;
    QPushButton  *m_btnNovo   = nullptr;
    QPushButton  *m_btnRemover = nullptr;
};
