#pragma once

#include <QWidget>
#include <QDate>

class QLabel;
class QScrollArea;
class QComboBox;
class QChartView;

class DashboardWidget final : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);

    void recarregar();

    // Print/Preview (#94) - renderiza o widget via QPrintPreviewDialog.
    // Acessível pelo botão 🖨 na navBar; pode ser chamado externamente.
    void imprimir();

    // Fechamento mensal (#88) - simula -> confirma -> close().
    // Se mes ja fechado: oferece reopen().
    void onFecharMes();

signals:
    void statusAtualizado(const QString &msg);
    // Emitted when the user clicks a card; index matches QTabWidget tab order.
    void navegarParaAba(int tabIndex);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    void buildUi();
    void atualizarDados();
    void atualizarGrafico();
    void navegarMes(int delta);

    // Creates a styled card frame; sets *outValue to the value label.
    // tabIndex >= 0 makes the card clickable (emits navegarParaAba).
    // Returns the widget to add to a layout.
    QWidget *makeCard(const QString &title, QLabel **outValue,
                      QLabel **outSub = nullptr, int tabIndex = -1);

    QDate   m_mes; // first day of displayed month

    QLabel *m_lblMes = nullptr;

    // Mês — linha 1
    QLabel *m_valEntradas = nullptr;
    QLabel *m_valGastos   = nullptr;
    QLabel *m_valSaldo    = nullptr;
    QLabel *m_valParcelas = nullptr;

    // Mês — linha 2
    QLabel *m_valInvNet   = nullptr;
    QLabel *m_valSaldoLiq = nullptr;
    QLabel *m_valProventos = nullptr; // #66 dividendos/JCP/rendimentos
    QLabel *m_subProventos = nullptr;

    // Subs
    QLabel *m_subEntradas = nullptr;
    QLabel *m_subGastos   = nullptr;
    QLabel *m_subParcelas = nullptr;
    QLabel *m_subInvNet   = nullptr;

    // Anuais
    QLabel *m_valAnualEntradas = nullptr;
    QLabel *m_valAnualGastos   = nullptr;
    QLabel *m_valAnualSaldo    = nullptr;

    // Gráfico mensal
    QComboBox  *m_cbPeriodo  = nullptr;
    QChartView *m_chartView  = nullptr;
};
