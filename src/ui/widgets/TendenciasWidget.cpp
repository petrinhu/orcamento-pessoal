#include "TendenciasWidget.h"
#include "EmptyStateWidget.h"
#include "ui/Theme.h"
#include "models/Categoria.h"
#include "models/GastoAvulso.h"
#include "repositories/GastoAvulsoRepository.h"
#include "services/GastoService.h"
#include "services/ParcelaService.h"
#include "services/ResumoService.h"
#include "core/settings/AppSettings.h"
#include "ui/UiFormatters.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateEdit>
#include <QComboBox>
#include <QPushButton>
#include <QTabWidget>
#include <QDate>
#include <QLocale>
#include <QSet>
#include <QPainter>
#include <algorithm>

#include <QStackedWidget>
#include <QChartView>
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QPieSeries>
#include <QPieSlice>
#include <QBarCategoryAxis>
#include <QValueAxis>

// ── Helpers ───────────────────────────────────────────────────────────────────

// Formato curto "Mmm/aa" ou "Mmm/aaaa" respeitando locale do app (#175).
static QString fmtMesAbrev(const QDate &d, const QString &fmt = QStringLiteral("MMM/yy"))
{
    const bool enIntl =
        AppSettings::instance().language() == AppSettings::Language::EnIntl;
    const QLocale loc = enIntl
        ? QLocale(QLocale::English,    QLocale::UnitedStates)
        : QLocale(QLocale::Portuguese, QLocale::Brazil);
    QString s = loc.toString(d, fmt);
    s.remove(u'.');
    if (!s.isEmpty()) s[0] = s[0].toUpper();
    return s;
}

// ── Palette for pie slices ────────────────────────────────────────────────────
namespace {
const QList<QColor> kPaleta = {
    QColor(0x16, 0x6F, 0x4A), QColor(0x4A, 0x90, 0xD9),
    QColor(0xF5, 0xA6, 0x23), QColor(0x9B, 0x59, 0xB6),
    QColor(0x1A, 0xBC, 0x9C), QColor(0xE6, 0x7E, 0x22),
    QColor(0x34, 0x98, 0xDB), QColor(0xC0, 0x39, 0x2B),
    QColor(0x2E, 0x86, 0xC1), QColor(0x22, 0xA3, 0x67),
};
} // namespace

// ─── Construction ─────────────────────────────────────────────────────────────

TendenciasWidget::TendenciasWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void TendenciasWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // ── Controls bar ──────────────────────────────────────────────────────────
    auto *ctrlBar = new QHBoxLayout;
    ctrlBar->setSpacing(6);

    auto *lblDe = new QLabel(tr("De:"), this);
    m_dtInicio = new QDateEdit(QDate::currentDate().addMonths(-11), this);
    m_dtInicio->setDisplayFormat(uifmt::mesAnoDisplayFormat());
    m_dtInicio->setCalendarPopup(true);

    auto *lblAte = new QLabel(tr("Até:"), this);
    m_dtFim = new QDateEdit(QDate::currentDate(), this);
    m_dtFim->setDisplayFormat(uifmt::mesAnoDisplayFormat());
    m_dtFim->setCalendarPopup(true);

    auto *lblCat = new QLabel(tr("Categoria:"), this);
    m_cbCategoria = new QComboBox(this);
    m_cbCategoria->addItem(tr("Todas"));
    m_cbCategoria->setMinimumWidth(160);

    auto *btnAplicar = new QPushButton(tr("Aplicar"), this);
    btnAplicar->setIcon(Theme::emojiIcon(QStringLiteral("🔄")));
    btnAplicar->setObjectName(QStringLiteral("primary"));

    ctrlBar->addWidget(lblDe);
    ctrlBar->addWidget(m_dtInicio);
    ctrlBar->addWidget(lblAte);
    ctrlBar->addWidget(m_dtFim);
    ctrlBar->addWidget(lblCat);
    ctrlBar->addWidget(m_cbCategoria);
    ctrlBar->addWidget(btnAplicar);
    ctrlBar->addStretch();
    root->addLayout(ctrlBar);

    // ── Stack: empty state (0) + charts (1) ──────────────────────────────────
    // Sem CTA: Tendências é read-only, dados vêm de outros widgets (#166 Peça 2).
    m_stack = new QStackedWidget(this);

    auto *emptyState = new EmptyStateWidget(
        QStringLiteral("📊"),
        tr("Ainda sem dados para analisar"),
        tr("Os gráficos aparecem aqui conforme você registra gastos. Comece adicionando suas despesas."),
        QString(), // sem CTA
        m_stack);
    m_stack->addWidget(emptyState); // index 0

    // ── Chart tabs ────────────────────────────────────────────────────────────
    m_chartTabs = new QTabWidget(m_stack);

    auto makeView = [this]() {
        auto *v = new QChartView(new QChart, m_stack);
        v->setRenderHint(QPainter::Antialiasing);
        v->setFrameShape(QFrame::NoFrame);
        return v;
    };

    m_viewMensal  = makeView();
    m_viewCat     = makeView();
    m_viewTipo    = makeView();
    m_viewFixoVar = makeView();

    m_chartTabs->addTab(m_viewMensal,  tr("Mensal"));
    m_chartTabs->addTab(m_viewCat,     tr("Por Categoria"));
    m_chartTabs->addTab(m_viewTipo,    tr("Por Tipo de Pagamento"));
    m_chartTabs->addTab(m_viewFixoVar, tr("Fixo × Variável"));
    m_stack->addWidget(m_chartTabs); // index 1

    root->addWidget(m_stack, 1);

    // ── Summary strip ─────────────────────────────────────────────────────────
    auto *sumBar = new QHBoxLayout;
    sumBar->setSpacing(12);

    auto mkLbl = [this](QLayout *lay) -> QLabel* {
        auto *l = new QLabel(this);
        l->setObjectName(QStringLiteral("subtitle"));
        lay->addWidget(l);
        return l;
    };

    m_lblTotal    = mkLbl(sumBar);
    sumBar->addStretch();
    m_lblMedia    = mkLbl(sumBar);
    sumBar->addStretch();
    m_lblPico     = mkLbl(sumBar);
    sumBar->addStretch();
    m_lblCatPrinc = mkLbl(sumBar);
    root->addLayout(sumBar);

    connect(btnAplicar, &QPushButton::clicked, this, &TendenciasWidget::recarregar);
}

// ─── Data loading ─────────────────────────────────────────────────────────────

void TendenciasWidget::recarregar()
{
    const QDate inicio(m_dtInicio->date().year(), m_dtInicio->date().month(), 1);
    const QDate fim(m_dtFim->date().year(), m_dtFim->date().month(), 1);
    if (inicio > fim) return;

    // Detecta se há dados no período antes de construir os gráficos.
    // A alternância do stack para o empty state é feita no final,
    // após buildGraficoMensal verificar m_meses.

    const QString catSel = m_cbCategoria->currentIndex() == 0
                           ? QString() : m_cbCategoria->currentText();

    // Gastos avulsos do período: alimentam o breakdown por categoria (filtrado)
    // e por tipo de pagamento. O filtro de categoria afeta APENAS o gráfico
    // "Por Categoria" (#149 decisão 1A) — não o Mensal, nem Tipo, nem Fixo×Var.
    const auto todos = GastoAvulsoRepository().findAll();
    QList<GastoAvulso> noPeriodo;
    QSet<QString> cats;

    for (const auto &g : todos) {
        const QDate d(g.ano, g.mes, 1);
        if (d < inicio || d > fim) continue;
        const QString cat = g.categoria.isEmpty() ? tr("Sem categoria") : g.categoria;
        cats.insert(cat);
        noPeriodo.append(g);
    }

    reconstruirComboCategorias(QStringList(cats.begin(), cats.end()), catSel);

    // Build ordered month list and zero-init the map
    m_meses.clear();
    m_porMes.clear();
    QDate cur = inicio;
    while (cur <= fim) {
        const QPair<int,int> key{cur.year(), cur.month()};
        m_meses.append(key);
        m_porMes[key] = 0;
        cur = cur.addMonths(1);
    }

    // Gráfico Mensal: total de despesas do mês inteiro (gastos + parcelas) via
    // serviço canônico (Mid), igual ao Dashboard. Inclui parcelas de cartão que
    // o agregado de GastoAvulso omitia (#149 decisão 1A).
    {
        ResumoService resumoSvc;
        for (const auto &m : m_meses) {
            const ResumoMensal r = resumoSvc.resumoDoMes(m.second, m.first);
            m_porMes[m] = r.gastos + r.parcelas;
        }
    }

    // Fixo × Variável agregado do período via GastoService (eixo TipoDespesa da
    // categoria, canônico #118) — resolve a classificação correta de "fixo".
    m_totalFixos = 0;
    m_totalVariaveis = 0;
    {
        GastoService gastoSvc;
        for (const auto &m : m_meses) {
            const ResumoGastos rg = gastoSvc.resumoDoMes(m.second, m.first);
            m_totalFixos     += rg.totalFixos;
            m_totalVariaveis += rg.totalVariaveis;
        }
    }

    m_porCat.clear();
    m_porTipo.clear();

    for (const auto &g : noPeriodo) {
        const QString cat  = g.categoria.isEmpty() ? tr("Sem categoria") : g.categoria;
        const QString tipo = tipoPgtoLabel(g.tipoPgto);
        // Filtro de categoria aplica só ao breakdown por categoria.
        if (catSel.isEmpty() || g.categoria == catSel)
            m_porCat[cat] += g.valor;
        m_porTipo[tipo] += g.valor;
    }

    buildGraficoMensal();
    buildGraficoCategoria();
    buildGraficoTipo();
    buildGraficoFixoVariavel();

    // Compute summary stats
    qint64 total   = 0;
    qint64 picoVal = 0;
    int    picoAno = 0, picoMes = 0;
    for (const auto &m : m_meses) {
        const qint64 v = m_porMes.value(m, 0);
        total += v;
        if (v > picoVal) { picoVal = v; picoAno = m.first; picoMes = m.second; }
    }

    QString catPrincipal;
    qint64  catMax = 0;
    for (auto it = m_porCat.cbegin(); it != m_porCat.cend(); ++it) {
        if (it.value() > catMax) { catMax = it.value(); catPrincipal = it.key(); }
    }

    atualizarResumo(total, m_meses.size(), picoAno, picoMes, picoVal, catPrincipal);

    // Alterna stack: empty state (0) quando não há dados agregados, charts (1) caso contrário.
    const bool temDados = !m_meses.isEmpty() && total > 0;
    m_stack->setCurrentIndex(temDados ? 1 : 0);
}

// ─── Chart builders ───────────────────────────────────────────────────────────

void TendenciasWidget::buildGraficoMensal()
{
    auto *chart = new QChart;
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setMargins(QMargins(4, 4, 4, 4));

    if (m_meses.isEmpty()) {
        chart->setTitle(tr("Despesas mensais — sem dados"));
        m_viewMensal->setChart(chart);
        return;
    }

    auto *barSet = new QBarSet(tr("Despesa (R$)"));
    barSet->setColor(QColor(0xC9, 0x40, 0x40));

    QStringList labels;
    for (const auto &m : m_meses) {
        barSet->append(m_porMes.value(m, 0) / 100.0);
        labels << fmtMesAbrev(QDate(m.first, m.second, 1));
    }

    auto *series = new QBarSeries;
    series->append(barSet);
    chart->addSeries(series);
    chart->setTitle(tr("Despesas mensais (gastos + parcelas)"));

    auto *axisX = new QBarCategoryAxis;
    axisX->append(labels);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    auto *axisY = new QValueAxis;
    axisY->setLabelFormat(QStringLiteral("%.2f"));
    axisY->setTitleText(uifmt::simboloMoedaAtiva()); // #197 I18N-01
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->hide();
    m_viewMensal->setChart(chart);
}

void TendenciasWidget::buildGraficoCategoria()
{
    auto *chart = new QChart;
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setMargins(QMargins(4, 4, 4, 4));
    chart->setTitle(tr("Gastos por categoria"));

    if (m_porCat.isEmpty()) {
        m_viewCat->setChart(chart);
        return;
    }

    qint64 totalCat = 0;
    for (qint64 v : m_porCat) totalCat += v;

    auto *series = new QPieSeries;
    series->setHoleSize(0.35);

    int idx = 0;
    for (auto it = m_porCat.cbegin(); it != m_porCat.cend(); ++it, ++idx) {
        const double pct = totalCat > 0 ? it.value() * 100.0 / totalCat : 0.0;
        const QString lbl = QStringLiteral("%1 (%2 %)")
            .arg(it.key()).arg(pct, 0, 'f', 1);
        auto *slice = series->append(lbl, it.value() / 100.0);
        slice->setColor(kPaleta.at(idx % kPaleta.size()));
    }

    chart->addSeries(series);
    chart->legend()->setAlignment(Qt::AlignRight);
    m_viewCat->setChart(chart);
}

void TendenciasWidget::buildGraficoTipo()
{
    auto *chart = new QChart;
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setMargins(QMargins(4, 4, 4, 4));
    chart->setTitle(tr("Gastos por tipo de pagamento"));

    if (m_porTipo.isEmpty()) {
        m_viewTipo->setChart(chart);
        return;
    }

    qint64 totalTipo = 0;
    for (qint64 v : m_porTipo) totalTipo += v;

    auto *series = new QPieSeries;
    series->setHoleSize(0.35);

    int idx = 0;
    for (auto it = m_porTipo.cbegin(); it != m_porTipo.cend(); ++it, ++idx) {
        const double pct = totalTipo > 0 ? it.value() * 100.0 / totalTipo : 0.0;
        const QString lbl = QStringLiteral("%1 (%2 %)")
            .arg(it.key()).arg(pct, 0, 'f', 1);
        auto *slice = series->append(lbl, it.value() / 100.0);
        slice->setColor(kPaleta.at(idx % kPaleta.size()));
    }

    chart->addSeries(series);
    chart->legend()->setAlignment(Qt::AlignRight);
    m_viewTipo->setChart(chart);
}

void TendenciasWidget::buildGraficoFixoVariavel()
{
    auto *chart = new QChart;
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setMargins(QMargins(4, 4, 4, 4));
    chart->setTitle(tr("Gastos fixos × variáveis"));

    const qint64 total = m_totalFixos + m_totalVariaveis;
    if (total <= 0) {
        m_viewFixoVar->setChart(chart);
        return;
    }

    auto *series = new QPieSeries;
    series->setHoleSize(0.35);

    struct Fatia { qint64 valor; QString label; QColor cor; };
    const Fatia fatias[] = {
        { m_totalFixos,     tipoDespesaLabel(TipoDespesa::Fixo),     kPaleta.at(0) },
        { m_totalVariaveis, tipoDespesaLabel(TipoDespesa::Variavel), kPaleta.at(2) },
    };

    for (const auto &f : fatias) {
        if (f.valor <= 0) continue;
        const double pct = f.valor * 100.0 / total;
        const QString lbl = QStringLiteral("%1 (%2 %)").arg(f.label).arg(pct, 0, 'f', 1);
        auto *slice = series->append(lbl, f.valor / 100.0);
        slice->setColor(f.cor);
    }

    chart->addSeries(series);
    chart->legend()->setAlignment(Qt::AlignRight);
    m_viewFixoVar->setChart(chart);
}

// ─── Helpers ──────────────────────────────────────────────────────────────────

void TendenciasWidget::reconstruirComboCategorias(const QStringList &cats, const QString &sel)
{
    QStringList sorted = cats;
    std::sort(sorted.begin(), sorted.end());

    m_cbCategoria->blockSignals(true);
    m_cbCategoria->clear();
    m_cbCategoria->addItem(tr("Todas"));
    m_cbCategoria->addItems(sorted);
    if (!sel.isEmpty()) {
        const int idx = m_cbCategoria->findText(sel);
        if (idx >= 0) m_cbCategoria->setCurrentIndex(idx);
    }
    m_cbCategoria->blockSignals(false);
}

void TendenciasWidget::atualizarResumo(qint64 total, int numMeses,
                                        int picoAno, int picoMes, qint64 picoVal,
                                        const QString &catPrincipal)
{
    m_lblTotal->setText(tr("Total: %1").arg(ParcelaService::formatarValor(total)));
    m_lblMedia->setText(tr("Média/mês: %1").arg(
        ParcelaService::formatarValor(numMeses > 0 ? total / numMeses : 0)));

    if (picoAno > 0) {
        const QString mNome = fmtMesAbrev(QDate(picoAno, picoMes, 1), QStringLiteral("MMM/yyyy"));
        m_lblPico->setText(tr("Pico: %1 (%2)").arg(mNome, ParcelaService::formatarValor(picoVal)));
    } else {
        m_lblPico->setText(tr("Pico: —"));
    }

    m_lblCatPrinc->setText(tr("Maior categoria: %1")
        .arg(catPrincipal.isEmpty() ? QStringLiteral("—") : catPrincipal));
}
