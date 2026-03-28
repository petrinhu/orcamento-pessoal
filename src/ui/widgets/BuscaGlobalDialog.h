#pragma once

#include <QDialog>

class QLineEdit;
class QComboBox;
class QTableWidget;
class QPushButton;

// Busca/filtro global de transações em todas as abas.
// Dialog não-modal — pode ficar aberto enquanto o usuário navega.
// Double-click em um resultado emite navegarParaAba(idx).
class BuscaGlobalDialog final : public QDialog {
    Q_OBJECT
public:
    explicit BuscaGlobalDialog(QWidget *parent = nullptr);

signals:
    // Emitido quando o usuário dá double-click em um resultado.
    // tabIndex = índice da aba no QTabWidget do MainWindow.
    void navegarParaAba(int tabIndex);

private slots:
    void onBuscar();
    void onCellDoubleClicked(int row, int col);

private:
    void buildUi();
    void popularResultados(const QString &termo,
                           const QString &periodo,
                           const QString &tipo);

    QLineEdit    *m_edtBusca    = nullptr;
    QComboBox    *m_cbPeriodo   = nullptr;
    QComboBox    *m_cbTipo      = nullptr;
    QTableWidget *m_table       = nullptr;
    QPushButton  *m_btnBuscar   = nullptr;

    // Mapeamento linha → tabIndex da aba
    QList<int> m_tabIndices;
};
