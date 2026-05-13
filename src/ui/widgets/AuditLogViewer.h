#pragma once

#include <QWidget>

class QComboBox;
class QDateEdit;
class QPushButton;
class QTableWidget;
class QSpinBox;

// AuditLogViewer (#92) - UI para inspecionar trilha de auditoria gerada
// pelos triggers SQLite registrados em AuditLogService::ensureSchema().
//
// Filtros disponiveis:
//   - tabela (combo, vazio = todas)
//   - operacao (combo: INSERT/UPDATE/DELETE/vazio)
//   - intervalo de datas (de/ate)
//   - limite (spinbox, default 500)
//
// Tabela mostra ts, tabela, op, registro_id, dados_antigos e dados_novos.
// Clique em linha expande JSON em dialog separado (read-only).
class AuditLogViewer final : public QWidget {
    Q_OBJECT
public:
    explicit AuditLogViewer(QWidget *parent = nullptr);

private slots:
    void onFiltrar();
    void onLinhaSelecionada(int row, int column);

private:
    void buildUi();
    void carregarOpcoesTabela();

    QComboBox    *m_cbTabela  = nullptr;
    QComboBox    *m_cbOp      = nullptr;
    QDateEdit    *m_dateDe    = nullptr;
    QDateEdit    *m_dateAte   = nullptr;
    QSpinBox     *m_spnLimite = nullptr;
    QPushButton  *m_btnFiltrar = nullptr;
    QTableWidget *m_tabela    = nullptr;
};
