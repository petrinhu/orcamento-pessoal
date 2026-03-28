#pragma once

#include <QDialog>
#include <QList>

class QTableWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QSpinBox;

struct ItemExtrato {
    QString data;        // ISO yyyy-MM-dd
    QString descricao;
    qint64  valor = 0;   // centavos; negativo = débito
};

// Dialog de reconciliação bancária — compara extrato importado vs. lançamentos do app.
// Sem persistência (sessão única, in-memory).
class ReconciliacaoDialog final : public QDialog {
    Q_OBJECT
public:
    explicit ReconciliacaoDialog(QWidget *parent = nullptr);

private slots:
    void onImportar();
    void onReconciliar();
    void onMarcarManual();

private:
    void buildUi();
    void carregarAppEntries(int mes, int ano);
    void executarMatching();
    void atualizarResumo();

    // Parser mínimo CSV: data, descrição, valor
    QList<ItemExtrato> parseCsv(const QString &path) const;
    // Parser OFX: lê tags <STMTTRN>
    QList<ItemExtrato> parseOfx(const QString &path) const;

    // ── Colunas da tabela combinada ──────────────────────────────────────
    enum Col {
        ColStatus = 0,      // "✓ Conciliado" / "Só extrato" / "Só app" / "Divergência"
        ColData, ColDescricaoExt, ColValorExt,
        ColDescricaoApp, ColValorApp,
        ColCount
    };

    QTableWidget *m_table     = nullptr;
    QPushButton  *m_btnImport = nullptr;
    QPushButton  *m_btnMatch  = nullptr;
    QPushButton  *m_btnManual = nullptr;
    QComboBox    *m_cbMes     = nullptr;
    QSpinBox     *m_spnAno    = nullptr;
    QLabel       *m_lblResumo = nullptr;

    QList<ItemExtrato> m_extrato;  // linhas do arquivo importado
    // App entries: par {data ISO, descricao, valor}
    QList<ItemExtrato> m_appItems;

    // Status: 0=nao_conciliado 1=conciliado 2=manual
    QList<int> m_status;
};
