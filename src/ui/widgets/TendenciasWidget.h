#pragma once

#include <QWidget>
#include <QList>
#include <QMap>
#include <QPair>
#include <QtTypes>

class QChartView;
class QDateEdit;
class QComboBox;
class QLabel;
class QStackedWidget;
class QTabWidget;

// Spending-trends tab — four charts (monthly bar of total despesas via
// ResumoService, by-category donut, by-payment-type donut, fixo×variável donut
// via GastoService) plus a summary strip.
// Read-only: no undo/redo needed.
class TendenciasWidget final : public QWidget {
    Q_OBJECT
public:
    explicit TendenciasWidget(QWidget *parent = nullptr);
    void recarregar();

private:
    void buildUi();
    void reconstruirComboCategorias(const QStringList &cats, const QString &sel);
    void buildGraficoMensal();
    void buildGraficoCategoria();
    void buildGraficoTipo();
    void buildGraficoFixoVariavel();
    void atualizarResumo(qint64 total, int numMeses,
                         int picoAno, int picoMes, qint64 picoVal,
                         const QString &catPrincipal);

    // ── Controls ──────────────────────────────────────────────────────────────
    QDateEdit  *m_dtInicio    = nullptr;
    QDateEdit  *m_dtFim       = nullptr;
    QComboBox  *m_cbCategoria = nullptr;

    // ── Stack: 0=emptyState (sem dados), 1=charts ─────────────────────────────
    QStackedWidget *m_stack    = nullptr;

    // ── Charts ────────────────────────────────────────────────────────────────
    QTabWidget *m_chartTabs  = nullptr;
    QChartView *m_viewMensal  = nullptr;
    QChartView *m_viewCat     = nullptr;
    QChartView *m_viewTipo     = nullptr;
    QChartView *m_viewFixoVar = nullptr;

    // ── Summary labels ────────────────────────────────────────────────────────
    QLabel *m_lblTotal    = nullptr;
    QLabel *m_lblMedia    = nullptr;
    QLabel *m_lblPico     = nullptr;
    QLabel *m_lblCatPrinc = nullptr;

    // ── Aggregated data (rebuilt on each recarregar) ──────────────────────────
    QList<QPair<int,int>>        m_meses;   // ordered (ano, mes) in range
    QMap<QPair<int,int>, qint64> m_porMes;  // total despesas (gastos+parcelas) centavos per month — via ResumoService
    QMap<QString, qint64>        m_porCat;  // total centavos per category (filtro de categoria aplica aqui)
    QMap<QString, qint64>        m_porTipo; // total centavos per payment type
    qint64                       m_totalFixos     = 0; // Σ período — via GastoService (#118)
    qint64                       m_totalVariaveis = 0; // Σ período — via GastoService (#118)
};
