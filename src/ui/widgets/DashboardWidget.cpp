#include "DashboardWidget.h"
#include "services/EntradaService.h"
#include "services/CompraService.h"
#include "services/ParcelaService.h"
#include "repositories/GastoAvulsoRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"
#include "models/LancamentoInvestimento.h"
#include "core/settings/AppSettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QFont>
#include <QLocale>
#include <QDate>
#include <QEvent>
#include <QMouseEvent>
#include <QComboBox>
#include <QPainter>

#include <QChartView>
#include <QChart>
#include <QBarSeries>
#include <QBarSet>
#include <QLineSeries>
#include <QBarCategoryAxis>
#include <QValueAxis>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString fmtCentavos(qint64 c)
{
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("R$ %1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

static QString nomeMes(const QDate &d)
{
    const bool enIntl =
        AppSettings::instance().language() == AppSettings::Language::EnIntl;
    const QLocale loc = enIntl
        ? QLocale(QLocale::English,    QLocale::UnitedStates)
        : QLocale(QLocale::Portuguese, QLocale::Brazil);
    QString s = loc.toString(d, QStringLiteral("MMMM / yyyy"));
    if (!s.isEmpty()) s[0] = s[0].toUpper();
    return s;
}

// ── Construction ──────────────────────────────────────────────────────────────

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent)
{
    m_mes = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    buildUi();
    atualizarDados();
}

QWidget *DashboardWidget::makeCard(const QString &title,
                                   QLabel **outValue,
                                   QLabel **outSub,
                                   int tabIndex)
{
    auto *frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Plain);
    frame->setObjectName(QStringLiteral("dashCard"));
    frame->setMinimumWidth(160);

    if (tabIndex >= 0) {
        frame->setCursor(Qt::PointingHandCursor);
        frame->installEventFilter(this);
        frame->setProperty("tabIndex", tabIndex);
    }

    auto *lay = new QVBoxLayout(frame);
    lay->setContentsMargins(16, 12, 16, 12);
    lay->setSpacing(4);

    auto *lblTitle = new QLabel(title, frame);
    lblTitle->setObjectName(QStringLiteral("subtitle"));
    QFont ft = lblTitle->font();
    ft.setPointSize(10);
    lblTitle->setFont(ft);
    lay->addWidget(lblTitle);

    // Arrow hint for clickable cards
    if (tabIndex >= 0) {
        auto *rowTitle = new QHBoxLayout;
        rowTitle->setContentsMargins(0, 0, 0, 0);
        rowTitle->setSpacing(4);
        lay->removeWidget(lblTitle); // re-add in row
        rowTitle->addWidget(lblTitle);
        auto *lblArrow = new QLabel(QStringLiteral("→"), frame);
        lblArrow->setObjectName(QStringLiteral("subtitle"));
        QFont fa = lblArrow->font(); fa.setPointSize(10);
        lblArrow->setFont(fa);
        rowTitle->addWidget(lblArrow);
        rowTitle->addStretch();
        lay->addLayout(rowTitle);
    }

    auto *lblValue = new QLabel(QStringLiteral("R$ 0,00"), frame);
    QFont fv = lblValue->font();
    fv.setPointSize(18);
    fv.setBold(true);
    lblValue->setFont(fv);
    lay->addWidget(lblValue);

    if (outSub) {
        auto *lblSub = new QLabel(frame);
        lblSub->setObjectName(QStringLiteral("subtitle"));
        QFont fs = lblSub->font();
        fs.setPointSize(9);
        lblSub->setFont(fs);
        lay->addWidget(lblSub);
        *outSub = lblSub;
    }

    *outValue = lblValue;
    return frame;
}

void DashboardWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Barra de navegação de mês ───────────────────────────────────────────
    auto *navBar = new QHBoxLayout;
    navBar->setContentsMargins(16, 12, 16, 8);

    auto *btnPrev = new QPushButton(QStringLiteral("◀"), this);
    auto *btnNext = new QPushButton(QStringLiteral("▶"), this);
    btnPrev->setFixedWidth(32);
    btnNext->setFixedWidth(32);

    m_lblMes = new QLabel(this);
    QFont fmes = m_lblMes->font();
    fmes.setPointSize(14);
    fmes.setBold(true);
    m_lblMes->setFont(fmes);
    m_lblMes->setAlignment(Qt::AlignCenter);

    navBar->addWidget(btnPrev);
    navBar->addWidget(m_lblMes, 1);
    navBar->addWidget(btnNext);
    root->addLayout(navBar);

    connect(btnPrev, &QPushButton::clicked, this, [this]() { navegarMes(-1); });
    connect(btnNext, &QPushButton::clicked, this, [this]() { navegarMes(+1); });

    // ── Scroll area ────────────────────────────────────────────────────────
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    root->addWidget(scroll);

    auto *content = new QWidget(scroll);
    scroll->setWidget(content);
    auto *lay = new QVBoxLayout(content);
    lay->setContentsMargins(16, 8, 16, 24);
    lay->setSpacing(20);

    // ── Seção: Este mês ────────────────────────────────────────────────────
    auto *lblSecMes = new QLabel(tr("Este mês"), content);
    QFont fsec = lblSecMes->font();
    fsec.setPointSize(11);
    fsec.setBold(true);
    lblSecMes->setFont(fsec);
    lay->addWidget(lblSecMes);

    // Linha 1: Entradas | Gastos | Saldo | Parcelas
    // tabIndex: Entradas=1, Gastos=2, Saldo=sem aba, Cartões=10
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(12);
    row1->addWidget(makeCard(tr("Entradas"),  &m_valEntradas, &m_subEntradas, 1));
    row1->addWidget(makeCard(tr("Gastos"),    &m_valGastos,   &m_subGastos,   2));
    row1->addWidget(makeCard(tr("Saldo"),     &m_valSaldo));
    row1->addWidget(makeCard(tr("Parcelas"),  &m_valParcelas, &m_subParcelas, 10));
    lay->addLayout(row1);

    // Linha 2: Investimentos (net) | Saldo líquido
    // tabIndex: Investimentos=4
    auto *row2 = new QHBoxLayout;
    row2->setSpacing(12);
    row2->addWidget(makeCard(tr("Investimentos (net)"), &m_valInvNet, &m_subInvNet, 4));
    row2->addWidget(makeCard(tr("Saldo líquido total"), &m_valSaldoLiq));
    row2->addStretch();
    lay->addLayout(row2);

    lay->addSpacing(8);

    // ── Seção: Acumulado no ano ────────────────────────────────────────────
    auto *lblSecAno = new QLabel(tr("Acumulado no ano"), content);
    lblSecAno->setFont(fsec);
    lay->addWidget(lblSecAno);

    auto *row3 = new QHBoxLayout;
    row3->setSpacing(12);
    row3->addWidget(makeCard(tr("Entradas"),  &m_valAnualEntradas));
    row3->addWidget(makeCard(tr("Gastos"),    &m_valAnualGastos));
    row3->addWidget(makeCard(tr("Saldo"),     &m_valAnualSaldo));
    row3->addStretch();
    lay->addLayout(row3);

    lay->addSpacing(8);

    // ── Seção: Gráfico mensal ──────────────────────────────────────────────
    auto *lblSecGrafico = new QLabel(tr("Entradas × Gastos"), content);
    lblSecGrafico->setFont(fsec);
    lay->addWidget(lblSecGrafico);

    // Período selector
    auto *rowPeriodo = new QHBoxLayout;
    auto *lblPer = new QLabel(tr("Período:"), content);
    m_cbPeriodo = new QComboBox(content);
    m_cbPeriodo->addItem(tr("3 meses"),  3);
    m_cbPeriodo->addItem(tr("6 meses"),  6);
    m_cbPeriodo->addItem(tr("12 meses"), 12);
    m_cbPeriodo->addItem(tr("24 meses"), 24);
    m_cbPeriodo->setCurrentIndex(1); // 6 meses padrão
    rowPeriodo->addWidget(lblPer);
    rowPeriodo->addWidget(m_cbPeriodo);
    rowPeriodo->addStretch();
    lay->addLayout(rowPeriodo);

    auto *chart = new QChart;
    chart->setMargins(QMargins(4, 4, 4, 4));
    chart->legend()->setAlignment(Qt::AlignBottom);
    m_chartView = new QChartView(chart, content);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(260);
    m_chartView->setFrameShape(QFrame::NoFrame);
    lay->addWidget(m_chartView);

    connect(m_cbPeriodo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DashboardWidget::atualizarGrafico);

    lay->addStretch();
}

// ── Data ──────────────────────────────────────────────────────────────────────

void DashboardWidget::atualizarDados()
{
    m_lblMes->setText(nomeMes(m_mes));

    const int mes = m_mes.month();
    const int ano = m_mes.year();

    // ── Entradas do mês ────────────────────────────────────────────────────
    qint64 totalEntradas = 0;
    int    cntEntradas   = 0;
    {
        const auto todos = EntradaService().lancamentos();
        for (const auto &l : todos) {
            if (l.mes == mes && l.ano == ano) {
                totalEntradas += l.valorBruto - l.irpf - l.inss;
                ++cntEntradas;
            }
        }
    }

    // ── Gastos do mês ──────────────────────────────────────────────────────
    qint64 totalGastos = 0;
    int    cntGastos   = 0;
    {
        const auto gastos = GastoAvulsoRepository().findByMesAno(mes, ano);
        for (const auto &g : gastos) {
            totalGastos += g.valor;
            ++cntGastos;
        }
    }

    // ── Parcelas do mês ────────────────────────────────────────────────────
    qint64 totalParcelas = 0;
    int    cntParcelas   = 0;
    {
        CompraService svc;
        const auto compras = svc.compras();
        for (const auto &c : compras) {
            for (const auto &p : svc.parcelasDeCompra(c.id)) {
                const QDate d = ParcelaService::dataParaParcela(c, p.numero);
                if (d.year() == ano && d.month() == mes) {
                    totalParcelas += p.valor;
                    ++cntParcelas;
                }
            }
        }
    }

    // ── Investimentos do mês ───────────────────────────────────────────────
    qint64 totalAportes  = 0;
    qint64 totalResgates = 0;
    {
        const auto invs = LancamentoInvestimentoRepository().findByMesAno(mes, ano);
        for (const auto &inv : invs) {
            if (inv.tipoOperacao == QStringLiteral("aporte"))
                totalAportes += inv.valor;
            else
                totalResgates += inv.valor;
        }
    }
    const qint64 invNet = totalResgates - totalAportes;

    // ── Totais anuais ──────────────────────────────────────────────────────
    qint64 anualEntradas = 0, anualGastos = 0;
    {
        const auto todos = EntradaService().lancamentos();
        for (const auto &l : todos)
            if (l.ano == ano)
                anualEntradas += l.valorBruto - l.irpf - l.inss;

        const auto gastos = GastoAvulsoRepository().findAll();
        for (const auto &g : gastos)
            if (g.ano == ano)
                anualGastos += g.valor;
    }

    // ── Saldos derivados ───────────────────────────────────────────────────
    const qint64 saldoMes = totalEntradas - totalGastos;
    const qint64 saldoLiq = totalEntradas - totalGastos - totalParcelas;
    const qint64 saldoAno = anualEntradas - anualGastos;

    // ── Atualiza labels ────────────────────────────────────────────────────
    auto setValor = [](QLabel *lbl, qint64 v, bool colored = false) {
        lbl->setText(fmtCentavos(v));
        if (colored) {
            const QString cor = v >= 0
                ? QStringLiteral("#166F4A")  // positivo
                : QStringLiteral("#C94040"); // negativo
            lbl->setStyleSheet(QStringLiteral("color: %1;").arg(cor));
        } else {
            lbl->setStyleSheet(QString());
        }
    };

    setValor(m_valEntradas,  totalEntradas);
    setValor(m_valGastos,    totalGastos);
    setValor(m_valSaldo,     saldoMes,  true);
    setValor(m_valParcelas,  totalParcelas);
    setValor(m_valInvNet,    invNet,    true);
    setValor(m_valSaldoLiq,  saldoLiq,  true);

    setValor(m_valAnualEntradas, anualEntradas);
    setValor(m_valAnualGastos,   anualGastos);
    setValor(m_valAnualSaldo,    saldoAno,  true);

    // Sub-labels
    m_subEntradas->setText(tr("%n lançamento(s)", nullptr, cntEntradas));
    m_subGastos->setText(tr("%n lançamento(s)", nullptr, cntGastos));
    m_subParcelas->setText(tr("%n parcela(s)", nullptr, cntParcelas));
    m_subInvNet->setText(tr("Aportes: %1  Resgates: %2")
        .arg(fmtCentavos(totalAportes))
        .arg(fmtCentavos(totalResgates)));

    emit statusAtualizado(tr("Saldo %1: %2  ")
        .arg(m_mes.toString(QStringLiteral("MMM/yy")))
        .arg(fmtCentavos(saldoMes)));

    atualizarGrafico();
}

void DashboardWidget::atualizarGrafico()
{
    const int numMeses = m_cbPeriodo->currentData().toInt();

    // Collect aggregated data for the last N months ending at m_mes
    QStringList labels;
    QList<double> valsEntradas, valsGastos, valsSaldo;

    const EntradaService entSvc;
    const auto todosLanc = entSvc.lancamentos();
    const auto todosGastos = GastoAvulsoRepository().findAll();

    for (int i = numMeses - 1; i >= 0; --i) {
        const QDate d = m_mes.addMonths(-i);
        const int   m = d.month();
        const int   y = d.year();

        qint64 entradas = 0;
        for (const auto &l : todosLanc)
            if (l.mes == m && l.ano == y)
                entradas += l.valorBruto - l.irpf - l.inss;

        qint64 gastos = 0;
        for (const auto &g : todosGastos)
            if (g.mes == m && g.ano == y)
                gastos += g.valor;

        labels << d.toString(QStringLiteral("MMM/yy"));
        valsEntradas << entradas / 100.0;
        valsGastos   << gastos   / 100.0;
        valsSaldo    << (entradas - gastos) / 100.0;
    }

    // ── Bar sets ───────────────────────────────────────────────────────────
    auto *setEnt = new QBarSet(tr("Entradas"));
    setEnt->setColor(QColor(0x16, 0x6F, 0x4A));
    setEnt->setBorderColor(QColor(0x16, 0x6F, 0x4A));

    auto *setGas = new QBarSet(tr("Gastos"));
    setGas->setColor(QColor(0xC9, 0x40, 0x40));
    setGas->setBorderColor(QColor(0xC9, 0x40, 0x40));

    for (int i = 0; i < numMeses; ++i) {
        *setEnt << valsEntradas[i];
        *setGas << valsGastos[i];
    }

    auto *barSeries = new QBarSeries;
    barSeries->append(setEnt);
    barSeries->append(setGas);

    // ── Line series (Saldo) ────────────────────────────────────────────────
    auto *lineSeries = new QLineSeries;
    lineSeries->setName(tr("Saldo"));
    lineSeries->setColor(QColor(0x4A, 0x90, 0xD9));
    QPen pen(QColor(0x4A, 0x90, 0xD9));
    pen.setWidth(2);
    lineSeries->setPen(pen);
    for (int i = 0; i < numMeses; ++i)
        lineSeries->append(i, valsSaldo[i]);

    // ── Axes ───────────────────────────────────────────────────────────────
    auto *axisX = new QBarCategoryAxis;
    axisX->append(labels);

    auto *axisY = new QValueAxis;
    axisY->setTitleText(QStringLiteral("R$"));
    axisY->setLabelFormat(QStringLiteral("%.0f"));

    // ── Chart ──────────────────────────────────────────────────────────────
    auto *chart = new QChart;
    chart->addSeries(barSeries);
    chart->addSeries(lineSeries);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisX);
    barSeries->attachAxis(axisY);
    lineSeries->attachAxis(axisX);
    lineSeries->attachAxis(axisY);
    chart->setMargins(QMargins(4, 4, 4, 4));
    chart->legend()->setAlignment(Qt::AlignBottom);

    m_chartView->setChart(chart);
}

void DashboardWidget::recarregar()
{
    atualizarDados();
}

bool DashboardWidget::eventFilter(QObject *obj, QEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        auto *frame = qobject_cast<QFrame *>(obj);
        if (frame) {
            const QVariant v = frame->property("tabIndex");
            if (v.isValid())
                emit navegarParaAba(v.toInt());
        }
    }
    return QWidget::eventFilter(obj, ev);
}

void DashboardWidget::navegarMes(int delta)
{
    m_mes = m_mes.addMonths(delta);
    atualizarDados();
}
