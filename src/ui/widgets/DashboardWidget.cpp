#include "DashboardWidget.h"
#include "ui/Theme.h"
#include "services/ResumoService.h"
#include "services/GastoService.h"
#include "services/FechamentoMensalService.h"
#include "core/database/DatabaseManager.h"
#include "core/session/UserSession.h"
#include "core/settings/AppSettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
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

#include <QPrinter>
#include <QPrintPreviewDialog>

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

// Formato curto "Mmm/aa" (ex.: "Jan/25") respeitando locale do app (#175).
static QString nomeMesAbrev(const QDate &d)
{
    const bool enIntl =
        AppSettings::instance().language() == AppSettings::Language::EnIntl;
    const QLocale loc = enIntl
        ? QLocale(QLocale::English,    QLocale::UnitedStates)
        : QLocale(QLocale::Portuguese, QLocale::Brazil);
    QString s = loc.toString(d, QStringLiteral("MMM/yy"));
    s.remove(u'.');
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
        // ➡️ emoji laranja via emojiIcon — consistente com padrão de ícones do app (#165).
        auto *lblArrow = new QLabel(frame);
        lblArrow->setPixmap(Theme::emojiIcon(QStringLiteral("➡️")).pixmap(14, 14));
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

    // ◀️ ▶️ com VS16 (U+FE0F) — emoji laranja via emojiIcon, padrão do app (#165).
    // Botão transparente via #iconButton (sem box/borda). Mesmo estilo dos
    // toggles 🔼/🔽 do TabShell.
    auto *btnPrev = new QPushButton(this);
    auto *btnNext = new QPushButton(this);
    btnPrev->setIcon(Theme::emojiIcon(QStringLiteral("◀️")));
    btnNext->setIcon(Theme::emojiIcon(QStringLiteral("▶️")));
    btnPrev->setObjectName(QStringLiteral("iconButton"));
    btnNext->setObjectName(QStringLiteral("iconButton"));

    m_lblMes = new QLabel(this);
    QFont fmes = m_lblMes->font();
    fmes.setPointSize(14);
    fmes.setBold(true);
    m_lblMes->setFont(fmes);
    m_lblMes->setAlignment(Qt::AlignCenter);

    // Botão Imprimir (#94) — icon-only, sem borda (#165).
    auto *btnPrint = new QPushButton(this);
    btnPrint->setIcon(Theme::emojiIcon(QStringLiteral("🖨️")));
    btnPrint->setObjectName(QStringLiteral("iconButton"));
    btnPrint->setToolTip(tr("Imprimir Dashboard (visualização prévia)"));

    // Botão Fechar mês (#88) — icon-only, sem borda (#165).
    auto *btnFechar = new QPushButton(this);
    btnFechar->setIcon(Theme::emojiIcon(QStringLiteral("🔒")));
    btnFechar->setObjectName(QStringLiteral("iconButton"));
    btnFechar->setToolTip(tr("Fechar mês contábil (snapshot imutável)"));

    navBar->addWidget(btnPrev);
    navBar->addWidget(m_lblMes, 1);
    navBar->addWidget(btnNext);
    navBar->addWidget(btnFechar);
    navBar->addWidget(btnPrint);
    root->addLayout(navBar);

    connect(btnPrev,   &QPushButton::clicked, this, [this]() { navegarMes(-1); });
    connect(btnNext,   &QPushButton::clicked, this, [this]() { navegarMes(+1); });
    connect(btnPrint,  &QPushButton::clicked, this, &DashboardWidget::imprimir);
    connect(btnFechar, &QPushButton::clicked, this, &DashboardWidget::onFecharMes);

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
    // tabIndex (ordem default de MainWindow): Entradas=1, Gastos=2, Cartões=3.
    // NOTA: indices fixos quebram se o usuario reordenar as abas (drag & drop);
    // refactor recomendado para navegacao por chave estavel (objectName).
    auto *row1 = new QHBoxLayout;
    row1->setSpacing(12);
    row1->addWidget(makeCard(tr("Entradas"),  &m_valEntradas, &m_subEntradas, 1));
    row1->addWidget(makeCard(tr("Gastos"),    &m_valGastos,   &m_subGastos,   2));
    row1->addWidget(makeCard(tr("Saldo"),     &m_valSaldo));
    row1->addWidget(makeCard(tr("Parcelas"),  &m_valParcelas, &m_subParcelas, 3));
    lay->addLayout(row1);

    // Linha 2: Investimentos (net) | Saldo líquido
    // tabIndex (ordem default): Investimentos=9, Proventos=10, Gastos=2.
    auto *row2 = new QHBoxLayout;
    row2->setSpacing(12);
    row2->addWidget(makeCard(tr("Investimentos (net)"), &m_valInvNet, &m_subInvNet, 9));
    row2->addWidget(makeCard(tr("Saldo líquido total"), &m_valSaldoLiq));
    row2->addWidget(makeCard(tr("Proventos (ano)"),     &m_valProventos, &m_subProventos, 10)); // #66
    // #121: gastos fixos × variáveis do mês (aba Gastos = 2)
    row2->addWidget(makeCard(tr("Gastos fixos vs variáveis"),
                             &m_valFixosVar, &m_subFixosVar, 2));
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

    // ── Agregação centralizada (camada Mid) ────────────────────────────────
    // Toda a leitura/soma de Entradas, Gastos, Parcelas, Investimentos e
    // Proventos vive agora em ResumoService (testado por test_resumo.cpp).
    // Este widget apenas formata os campos pré-calculados.
    ResumoService svc;
    const ResumoMensal m = svc.resumoDoMes(mes, ano);
    const ResumoAnual  a = svc.resumoDoAno(ano);

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

    setValor(m_valEntradas,  m.entradasLiquidas);
    setValor(m_valGastos,    m.gastos);
    setValor(m_valSaldo,     m.saldoMes,         true);
    setValor(m_valParcelas,  m.parcelas);
    setValor(m_valInvNet,    m.investimentosNet, true);
    setValor(m_valSaldoLiq,  m.saldoLiquido,     true);

    setValor(m_valAnualEntradas, a.entradasLiquidas);
    setValor(m_valAnualGastos,   a.gastos);
    setValor(m_valAnualSaldo,    a.saldo,            true);

    // Sub-labels
    m_subEntradas->setText(tr("%n lançamento(s)", nullptr, m.cntEntradas));
    m_subGastos->setText(tr("%n lançamento(s)", nullptr, m.cntGastos));
    m_subParcelas->setText(tr("%n parcela(s)", nullptr, m.cntParcelas));
    m_subInvNet->setText(tr("Aportes: %1  Resgates: %2")
        .arg(fmtCentavos(m.aportes))
        .arg(fmtCentavos(m.resgates)));

    // #66: card Proventos (acumulado ano corrente)
    setValor(m_valProventos, a.proventosLiquidos);
    m_subProventos->setText(tr("%n evento(s) em %1", nullptr, a.cntProventos)
        .arg(ano));

    // #121: card Gastos fixos × variáveis (mês exibido)
    const ResumoGastos rg = GastoService().resumoDoMes(mes, ano);
    const double pctFixos = rg.pctFixosBps / 100.0;
    m_valFixosVar->setText(tr("Fixos %1").arg(fmtCentavos(rg.totalFixos)));
    m_valFixosVar->setStyleSheet(QString());
    m_subFixosVar->setText(tr("Variáveis: %1  •  Fixos: %2%")
        .arg(fmtCentavos(rg.totalVariaveis))
        .arg(QString::number(pctFixos, 'f', 1)));

    emit statusAtualizado(tr("Saldo %1: %2  ")
        .arg(nomeMesAbrev(m_mes))
        .arg(fmtCentavos(m.saldoMes)));

    atualizarGrafico();
}

void DashboardWidget::atualizarGrafico()
{
    const int numMeses = m_cbPeriodo->currentData().toInt();

    // Collect aggregated data for the last N months ending at m_mes.
    // A agregação mês a mês vem de ResumoService::serie (camada Mid, testada).
    QStringList labels;
    QList<double> valsEntradas, valsGastos, valsSaldo;

    const QDate ini = m_mes.addMonths(-(numMeses - 1));
    const QDate fim = m_mes;
    ResumoService svc;
    const auto serie = svc.serie(ini, fim);

    for (const auto &r : serie) {
        const QDate d(r.ano, r.mes, 1);
        labels << nomeMesAbrev(d);
        valsEntradas << r.entradasLiquidas / 100.0;
        valsGastos   << r.gastos           / 100.0;
        valsSaldo    << r.saldoMes         / 100.0;
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

// Print/Preview (#94)
// Abre QPrintPreviewDialog que renderiza este widget via QWidget::render.
// Usuario escolhe impressora ou exporta PDF na propria janela de preview.
void DashboardWidget::imprimir()
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setPageOrientation(QPageLayout::Portrait);
    printer.setDocName(QStringLiteral("PeuBud - Dashboard %1").arg(nomeMes(m_mes)));

    QPrintPreviewDialog preview(&printer, this);
    preview.setWindowTitle(tr("Imprimir Dashboard"));
    preview.resize(900, 700);
    connect(&preview, &QPrintPreviewDialog::paintRequested,
            this, [this](QPrinter *p) {
                QPainter painter(p);
                const QRect pageRect = p->pageLayout().paintRectPixels(p->resolution());
                const QSize widgetSize = this->size();
                const qreal sx = qreal(pageRect.width())  / qreal(widgetSize.width());
                const qreal sy = qreal(pageRect.height()) / qreal(widgetSize.height());
                const qreal s  = qMin(sx, sy);
                painter.scale(s, s);
                this->render(&painter, QPoint(), QRegion(),
                             QWidget::DrawWindowBackground | QWidget::DrawChildren);
            });
    preview.exec();
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

// ── Fechamento mensal (#88) ───────────────────────────────────────────────────

void DashboardWidget::onFecharMes()
{
    auto db = DatabaseManager::instance().db();
    const QDate periodo(m_mes.year(), m_mes.month(), 1);

    // Ja fechado? Oferecer reabrir.
    const auto existente = FechamentoMensalService::findByPeriod(db, periodo);
    if (existente.has_value()) {
        const auto resposta = QMessageBox::question(
            this, tr("Mês já fechado"),
            tr("O mês %1 já foi fechado em %2 por %3.\n\n"
               "Deseja reabri-lo? O snapshot será descartado;\n"
               "os lançamentos originais permanecem intactos.")
                .arg(m_lblMes->text(),
                     existente->criadoEm.toString(QStringLiteral("dd/MM/yyyy HH:mm")),
                     existente->fechadoPor.isEmpty() ? tr("desconhecido") : existente->fechadoPor),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (resposta != QMessageBox::Yes) return;

        if (FechamentoMensalService::reopen(db, periodo)) {
            QMessageBox::information(this, tr("Reaberto"),
                tr("Mês %1 reaberto. Edições nos lançamentos voltam a refletir aqui.")
                    .arg(m_lblMes->text()));
            emit statusAtualizado(tr("Mês %1 reaberto").arg(m_lblMes->text()));
        } else {
            QMessageBox::warning(this, tr("Reabrir"),
                tr("Falha ao reabrir o mês."));
        }
        return;
    }

    // Preview via simulate (nao persiste)
    const auto resumo = FechamentoMensalService::simulate(db, periodo);
    auto centavos = [](qint64 c) {
        return QLocale(QLocale::Portuguese, QLocale::Brazil)
            .toString(c / 100.0, 'f', 2);
    };
    const QString detalhes = tr(
        "Resumo do mês %1:\n\n"
        "  • Entradas:        R$ %2\n"
        "  • Gastos:          R$ %3\n"
        "  • Parcelas:        R$ %4\n"
        "  • Investimentos:   R$ %5\n"
        "  • Saldo líquido:   R$ %6\n"
        "  • Lançamentos:     %7\n\n"
        "Após o fechamento, edições nos lançamentos deste mês\n"
        "NÃO alterarão estes valores. Para refazer, reabra o mês.\n\n"
        "Confirmar fechamento?")
        .arg(m_lblMes->text(),
             centavos(resumo.entradasCentavos),
             centavos(resumo.gastosCentavos),
             centavos(resumo.parcelasCentavos),
             centavos(resumo.investimentosCentavos),
             centavos(resumo.saldoLiquidoCentavos),
             QString::number(resumo.numLancamentos));

    const auto resposta = QMessageBox::question(
        this, tr("Fechar mês"), detalhes,
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (resposta != QMessageBox::Yes) return;

    const QString usuario = UserSession::instance().name();
    const auto snap = FechamentoMensalService::close(db, periodo, usuario);
    if (snap.has_value()) {
        QMessageBox::information(this, tr("Mês fechado"),
            tr("Mês %1 fechado com sucesso.\nSaldo líquido consolidado: R$ %2")
                .arg(m_lblMes->text(),
                     centavos(resumo.saldoLiquidoCentavos)));
        emit statusAtualizado(tr("Mês %1 fechado").arg(m_lblMes->text()));
    } else {
        QMessageBox::warning(this, tr("Fechar mês"),
            tr("Não foi possível fechar o mês (verifique se já existe um snapshot)."));
    }
}
