#include "ReconciliacaoDialog.h"
#include "core/database/DatabaseManager.h"
#include "services/ReconciliacaoAutomatica.h"
#include "ui/UiFormatters.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDate>
#include <QSqlQuery>
#include <QColor>
#include <QBrush>
#include <QRegularExpression>
#include <algorithm>

// ── Helpers ───────────────────────────────────────────────────────────────────

// #197 (I18N-01/05): delegava para "R$ "+vírgula e "dd/MM/yyyy" fixos.
static QString fmtCentavos(qint64 c)
{
    if (c == 0) return QString();
    return uifmt::formatCentavosBRL(c);
}

static QString fmtData(const QString &iso)
{
    const QDate d = QDate::fromString(iso, Qt::ISODate);
    return d.isValid() ? d.toString(uifmt::dataDisplayFormat()) : iso;
}

static QTableWidgetItem *roItem(const QString &text,
                                Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft)
{
    auto *it = new QTableWidgetItem(text);
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    it->setTextAlignment(align);
    return it;
}

// ── Construction ──────────────────────────────────────────────────────────────

ReconciliacaoDialog::ReconciliacaoDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Reconciliar extrato bancário"));
    setMinimumSize(900, 540);
    buildUi();
}

void ReconciliacaoDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(8);

    // ── Controles superiores ───────────────────────────────────────────────
    auto *ctrlRow = new QHBoxLayout;

    auto *lblMes = new QLabel(tr("Mês/ano do extrato:"), this);
    m_cbMes = new QComboBox(this);
    const QStringList meses = {
        tr("Janeiro"), tr("Fevereiro"), tr("Março"),     tr("Abril"),
        tr("Maio"),    tr("Junho"),     tr("Julho"),      tr("Agosto"),
        tr("Setembro"),tr("Outubro"),   tr("Novembro"),   tr("Dezembro")
    };
    for (int i = 0; i < 12; ++i)
        m_cbMes->addItem(meses[i], i + 1);
    m_cbMes->setCurrentIndex(QDate::currentDate().month() - 1);

    m_spnAno = new QSpinBox(this);
    m_spnAno->setRange(2000, 2100);
    m_spnAno->setValue(QDate::currentDate().year());

    m_btnImport = new QPushButton(tr("Importar extrato (CSV/OFX)…"), this);
    m_btnMatch  = new QPushButton(tr("Reconciliar automático"), this);
    m_btnManual = new QPushButton(tr("Marcar conciliado"), this);
    m_btnMatch->setEnabled(false);
    m_btnManual->setEnabled(false);

    ctrlRow->addWidget(lblMes);
    ctrlRow->addWidget(m_cbMes);
    ctrlRow->addWidget(m_spnAno);
    ctrlRow->addSpacing(12);
    ctrlRow->addWidget(m_btnImport);
    ctrlRow->addWidget(m_btnMatch);
    ctrlRow->addWidget(m_btnManual);
    ctrlRow->addStretch();
    root->addLayout(ctrlRow);

    // ── Legenda ────────────────────────────────────────────────────────────
    auto *legendRow = new QHBoxLayout;
    auto makeLegend = [&](const QString &txt, const QColor &cor) {
        auto *lbl = new QLabel(QStringLiteral("  %1  ").arg(txt), this);
        lbl->setAutoFillBackground(true);
        auto pal = lbl->palette();
        pal.setColor(QPalette::Window, cor);
        lbl->setPalette(pal);
        legendRow->addWidget(lbl);
    };
    makeLegend(tr("✓ Conciliado"),   QColor(0xDDFFDD));
    makeLegend(tr("Só no extrato"),  QColor(0xFFFBCC));
    makeLegend(tr("Só no app"),      QColor(0xCCE8FF));
    makeLegend(tr("Divergência"),    QColor(0xFFDDDD));
    legendRow->addStretch();
    root->addLayout(legendRow);

    // ── Tabela ─────────────────────────────────────────────────────────────
    m_table = new QTableWidget(0, ColCount, this);
    m_table->setHorizontalHeaderLabels({
        tr("Status"),
        tr("Data"),
        tr("Descrição (extrato)"),
        tr("Valor (extrato)"),
        tr("Descrição (app)"),
        tr("Valor (app)")
    });
    m_table->horizontalHeader()->setSectionResizeMode(ColStatus,      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColData,        QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColDescricaoExt,QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColValorExt,    QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColDescricaoApp,QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColValorApp,    QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setDefaultSectionSize(32);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    root->addWidget(m_table);

    // ── Resumo ─────────────────────────────────────────────────────────────
    m_lblResumo = new QLabel(this);
    m_lblResumo->setObjectName(QStringLiteral("subtitle"));
    root->addWidget(m_lblResumo);

    // ── Botão fechar ───────────────────────────────────────────────────────
    // Margem interna garante folga para a sombra do botão (#176).
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    btns->setContentsMargins(0, 6, 10, 10);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::close);
    root->addWidget(btns);

    // ── Connects ───────────────────────────────────────────────────────────
    connect(m_btnImport, &QPushButton::clicked, this, &ReconciliacaoDialog::onImportar);
    connect(m_btnMatch,  &QPushButton::clicked, this, &ReconciliacaoDialog::onReconciliar);
    connect(m_btnManual, &QPushButton::clicked, this, &ReconciliacaoDialog::onMarcarManual);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                m_btnManual->setEnabled(
                    !m_table->selectionModel()->selectedRows().isEmpty());
            });
}

// ── Importar ──────────────────────────────────────────────────────────────────

void ReconciliacaoDialog::onImportar()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Selecionar extrato"),
        QString(),
        tr("Extratos (*.csv *.ofx *.qfx);;CSV (*.csv);;OFX/QFX (*.ofx *.qfx);;Todos (*)"));
    if (path.isEmpty()) return;

    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == QStringLiteral("ofx") || ext == QStringLiteral("qfx"))
        m_extrato = parseOfx(path);
    else
        m_extrato = parseCsv(path);

    if (m_extrato.isEmpty()) {
        QMessageBox::warning(this, tr("Importar extrato"),
            tr("Nenhuma transação encontrada no arquivo.\n"
               "Verifique se o formato é CSV (data, descrição, valor) "
               "ou OFX/QFX padrão."));
        return;
    }

    carregarAppEntries(m_cbMes->currentData().toInt(), m_spnAno->value());
    executarMatching();
    m_btnMatch->setEnabled(true);
}

void ReconciliacaoDialog::carregarAppEntries(int mes, int ano)
{
    m_appItems.clear();
    auto db = DatabaseManager::instance().db();

    // Entradas (valor positivo)
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "SELECT dia, mes, ano, fonte, (valor_bruto - irpf - inss) "
            "FROM lancamentos_entradas WHERE mes = :mes AND ano = :ano"));
        q.bindValue(QStringLiteral(":mes"), mes);
        q.bindValue(QStringLiteral(":ano"), ano);
        q.exec();
        while (q.next()) {
            ItemExtrato it;
            it.data      = QDate(q.value(2).toInt(), q.value(1).toInt(),
                                 q.value(0).toInt()).toString(Qt::ISODate);
            it.descricao = q.value(3).toString();
            it.valor     = q.value(4).toLongLong();
            m_appItems << it;
        }
    }

    // Gastos (valor negativo)
    {
        QSqlQuery q(db);
        q.prepare(QStringLiteral(
            "SELECT dia, mes, ano, descricao, valor "
            "FROM lancamentos_gastos WHERE mes = :mes AND ano = :ano"));
        q.bindValue(QStringLiteral(":mes"), mes);
        q.bindValue(QStringLiteral(":ano"), ano);
        q.exec();
        while (q.next()) {
            ItemExtrato it;
            it.data      = QDate(q.value(2).toInt(), q.value(1).toInt(),
                                 q.value(0).toInt()).toString(Qt::ISODate);
            it.descricao = q.value(3).toString();
            it.valor     = -q.value(4).toLongLong();
            m_appItems << it;
        }
    }
}

// ── Matching automático ───────────────────────────────────────────────────────

// (#139/#85) Matching automático via ReconciliacaoAutomatica::resolver
// (subset-sum/knapsack, já testado em tests/test_reconciliacao.cpp). Para
// cada linha do extrato, monta os candidatos = itens do app ainda não usados,
// mesmo sinal (débito casa só com débito) e dentro de ±3 dias da data do
// extrato; resolve o subconjunto cuja soma bate exatamente com |valor do
// extrato| — permite conciliar 1 linha do extrato contra N lançamentos do
// app (ex.: um PIX dividido em duas parcelas no app), o que o match
// item-a-item anterior não fazia.
//
// Invariante de timeout (#85): resolver() tem timeout default de 2s por
// chamada (uma chamada por linha do extrato). Se o timeout dispara para uma
// linha, NÃO travamos a UI tentando de novo — a linha cai no fallback
// manual (status "Só extrato"/"Divergência"), e o usuário resolve via botão
// "Marcar conciliado" (m_btnManual).
void ReconciliacaoDialog::executarMatching()
{
    // Constrói conjunto de todos os itens únicos (extrato + app)
    // Matching: subset-sum (data ± 3 dias, mesmo sinal) → conciliado
    //           só no extrato: amarelo; só no app: azul

    struct LinhaTabela {
        int     statusIdx;   // 0=nao_concil 1=concil 2=manual 3=soextrato 4=soapp 5=divergencia
        QString data;
        QString descExt, valExt;
        QString descApp, valApp;
        qint64  valorExtCents = 0;
        qint64  valorAppCents = 0;
    };

    QList<LinhaTabela> linhas;
    QList<bool> usadoApp(m_appItems.size(), false);

    for (const ItemExtrato &ext : m_extrato) {
        LinhaTabela linha;
        linha.data    = ext.data;
        linha.descExt = ext.descricao;
        linha.valExt  = fmtCentavos(ext.valor);
        linha.valorExtCents = ext.valor;

        const QDate dExt = QDate::fromString(ext.data, Qt::ISODate);

        // Candidatos: itens do app não usados, mesmo sinal do lançamento do
        // extrato, dentro da janela de ±3 dias.
        QList<ReconciliacaoAutomatica::Lancamento> candidatos;
        QList<int> origemDoCandidato; // id sequencial do candidato -> índice em m_appItems
        for (int i = 0; i < m_appItems.size(); ++i) {
            if (usadoApp[i]) continue;
            const ItemExtrato &app = m_appItems[i];
            if ((app.valor > 0) != (ext.valor > 0)) continue; // sinais diferentes
            const QDate dApp = QDate::fromString(app.data, Qt::ISODate);
            if (!(dExt.isValid() && dApp.isValid() && qAbs(dExt.daysTo(dApp)) <= 3))
                continue;
            candidatos.append({static_cast<qint64>(origemDoCandidato.size()),
                                qAbs(app.valor), app.descricao});
            origemDoCandidato.append(i);
        }

        bool matched = false;
        if (ext.valor != 0 && !candidatos.isEmpty()) {
            const auto resultado = ReconciliacaoAutomatica::resolver(
                candidatos, qAbs(ext.valor));
            if (resultado.encontrado && !resultado.timeoutAtingido
                && !resultado.ids.isEmpty())
            {
                matched = true;
                QStringList descs;
                qint64 somaApp = 0;
                for (qint64 candId : resultado.ids) {
                    const int origIdx = origemDoCandidato[static_cast<int>(candId)];
                    usadoApp[origIdx] = true;
                    descs << m_appItems[origIdx].descricao;
                    somaApp += m_appItems[origIdx].valor;
                }
                linha.statusIdx     = 1; // conciliado
                linha.descApp       = descs.join(QStringLiteral(" + "));
                linha.valApp        = fmtCentavos(somaApp);
                linha.valorAppCents = somaApp;
            }
        }

        if (!matched) {
            // Fallback manual: mesmo comportamento anterior à #139 — sinaliza
            // divergência com o item mais próximo por data (só data, sem
            // exigir valor), senão marca "só no extrato".
            for (int i = 0; i < m_appItems.size(); ++i) {
                if (usadoApp[i]) continue;
                const QDate dApp = QDate::fromString(m_appItems[i].data, Qt::ISODate);
                if (dExt.isValid() && dApp.isValid()
                    && qAbs(dExt.daysTo(dApp)) <= 3)
                {
                    usadoApp[i] = true;
                    linha.statusIdx = 5; // divergência
                    linha.descApp   = m_appItems[i].descricao;
                    linha.valApp    = fmtCentavos(m_appItems[i].valor);
                    linha.valorAppCents = m_appItems[i].valor;
                    break;
                }
            }
            if (linha.statusIdx == 0)
                linha.statusIdx = 3; // só no extrato
        }

        linhas << linha;
    }

    // Itens do app não encontrados no extrato
    for (int i = 0; i < m_appItems.size(); ++i) {
        if (usadoApp[i]) continue;
        LinhaTabela linha;
        linha.statusIdx = 4; // só no app
        linha.data      = m_appItems[i].data;
        linha.descApp   = m_appItems[i].descricao;
        linha.valApp    = fmtCentavos(m_appItems[i].valor);
        linha.valorAppCents = m_appItems[i].valor;
        linhas << linha;
    }

    // Ordenar por data
    std::sort(linhas.begin(), linhas.end(), [](const LinhaTabela &a, const LinhaTabela &b) {
        return a.data < b.data;
    });

    m_status.resize(linhas.size());
    m_table->setRowCount(linhas.size());

    static const QColor kConciliado(0xDDFFDD);
    static const QColor kSoExtrato(0xFFFBCC);
    static const QColor kSoApp(0xCCE8FF);
    static const QColor kDivergencia(0xFFDDDD);

    const QStringList statusLabels = {
        tr("Pendente"), tr("✓ Conciliado"), tr("✓ Manual"),
        tr("Só extrato"), tr("Só app"), tr("Divergência")
    };
    const QList<QColor> statusCores = {
        QColor(), kConciliado, kConciliado, kSoExtrato, kSoApp, kDivergencia
    };

    for (int r = 0; r < linhas.size(); ++r) {
        const auto &l = linhas[r];
        m_status[r] = l.statusIdx;

        m_table->setItem(r, ColStatus,       roItem(statusLabels[l.statusIdx]));
        m_table->setItem(r, ColData,         roItem(fmtData(l.data)));
        m_table->setItem(r, ColDescricaoExt, roItem(l.descExt));
        m_table->setItem(r, ColValorExt,     roItem(l.valExt,
                                              Qt::AlignVCenter | Qt::AlignRight));
        m_table->setItem(r, ColDescricaoApp, roItem(l.descApp));
        m_table->setItem(r, ColValorApp,     roItem(l.valApp,
                                              Qt::AlignVCenter | Qt::AlignRight));

        const QColor &cor = statusCores[l.statusIdx];
        if (cor.isValid()) {
            for (int c = 0; c < ColCount; ++c)
                if (auto *it = m_table->item(r, c))
                    it->setBackground(cor);
        }
    }

    atualizarResumo();
}

void ReconciliacaoDialog::atualizarResumo()
{
    int concil = 0, soExt = 0, soApp = 0, diverg = 0;
    for (int s : m_status) {
        if (s == 1 || s == 2) ++concil;
        else if (s == 3)      ++soExt;
        else if (s == 4)      ++soApp;
        else if (s == 5)      ++diverg;
    }

    qint64 sumExtrato = 0, sumApp = 0;
    for (const ItemExtrato &e : m_extrato) sumExtrato += e.valor;
    for (const ItemExtrato &a : m_appItems) sumApp += a.valor;

    m_lblResumo->setText(tr(
        "✓ Conciliados: %1  |  Só extrato: %2  |  Só app: %3  |  Divergências: %4"
        "    —    Saldo extrato: %5  |  Saldo app: %6  |  Diferença: %7")
        .arg(concil).arg(soExt).arg(soApp).arg(diverg)
        .arg(fmtCentavos(sumExtrato))
        .arg(fmtCentavos(sumApp))
        .arg(fmtCentavos(sumExtrato - sumApp)));
}

void ReconciliacaoDialog::onReconciliar()
{
    // Re-executa o matching com os dados já carregados
    if (!m_extrato.isEmpty())
        executarMatching();
}

void ReconciliacaoDialog::onMarcarManual()
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return;
    const int r = rows.first().row();
    if (r < 0 || r >= m_status.size()) return;

    m_status[r] = 2; // conciliado manual
    if (auto *it = m_table->item(r, ColStatus))
        it->setText(tr("✓ Manual"));
    for (int c = 0; c < ColCount; ++c)
        if (auto *it = m_table->item(r, c))
            it->setBackground(QColor(0xDDFFDD));

    atualizarResumo();
}

// ── Parsers ───────────────────────────────────────────────────────────────────

QList<ItemExtrato> ReconciliacaoDialog::parseCsv(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream in(&f);
    QList<ItemExtrato> result;
    bool firstLine = true;
    int colData = 0, colDesc = 1, colValor = 2; // heurística padrão

    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // Detecta separador
        const QChar sep = line.contains(QLatin1Char(';')) ? QLatin1Char(';') : QLatin1Char(',');
        const QStringList cols = line.split(sep);
        if (cols.size() < 2) continue;

        if (firstLine) {
            firstLine = false;
            // Detecta colunas pelo header
            for (int i = 0; i < cols.size(); ++i) {
                const QString h = cols[i].toLower().trimmed().remove(QLatin1Char('"'));
                if (h.contains(QStringLiteral("data")) || h.contains(QStringLiteral("date")))
                    colData = i;
                else if (h.contains(QStringLiteral("hist")) || h.contains(QStringLiteral("desc"))
                         || h.contains(QStringLiteral("memo")) || h.contains(QStringLiteral("narr")))
                    colDesc = i;
                else if (h.contains(QStringLiteral("valor")) || h.contains(QStringLiteral("value"))
                         || h.contains(QStringLiteral("amount")) || h.contains(QStringLiteral("montant")))
                    colValor = i;
            }
            continue;
        }

        if (cols.size() <= std::max({colData, colDesc, colValor})) continue;

        auto clean = [](const QString &s) {
            return s.trimmed().remove(QLatin1Char('"'));
        };

        // Parseia data — tenta formatos comuns
        QString dataRaw = clean(cols[colData]);
        QDate d;
        for (const QString &fmt : {
                QStringLiteral("yyyy-MM-dd"), QStringLiteral("dd/MM/yyyy"),
                QStringLiteral("MM/dd/yyyy"), QStringLiteral("dd-MM-yyyy")}) {
            d = QDate::fromString(dataRaw, fmt);
            if (d.isValid()) break;
        }
        if (!d.isValid()) continue;

        // Parseia valor
        QString valStr = clean(cols[colValor]);
        valStr.remove(QRegularExpression(QStringLiteral("[R$\\s]")));
        if (valStr.contains(QLatin1Char(',')) && valStr.contains(QLatin1Char('.')))
            valStr.remove(QLatin1Char('.'));
        valStr.replace(QLatin1Char(','), QLatin1Char('.'));
        bool ok = false;
        const double val = valStr.toDouble(&ok);
        if (!ok) continue;

        ItemExtrato it;
        it.data      = d.toString(Qt::ISODate);
        it.descricao = clean(cols[colDesc]);
        it.valor     = static_cast<qint64>(qRound(val * 100.0));
        result << it;
    }
    return result;
}

QList<ItemExtrato> ReconciliacaoDialog::parseOfx(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    const QString content = QTextStream(&f).readAll();
    QList<ItemExtrato> result;

    // Regex para blocos <STMTTRN>…</STMTTRN> (OFX SGML e XML)
    static const QRegularExpression reTrn(
        QStringLiteral("<STMTTRN>(.+?)</STMTTRN>"),
        QRegularExpression::DotMatchesEverythingOption
        | QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reDtPosted(
        QStringLiteral("<DTPOSTED>\\s*(\\d{8})"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reTrnAmt(
        QStringLiteral("<TRNAMT>\\s*([\\-0-9\\.]+)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reMemo(
        QStringLiteral("<MEMO>\\s*([^<]+)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression reName(
        QStringLiteral("<NAME>\\s*([^<]+)"),
        QRegularExpression::CaseInsensitiveOption);

    auto it = reTrn.globalMatch(content);
    while (it.hasNext()) {
        const auto m = it.next();
        const QString blk = m.captured(1);

        // Data
        QDate d;
        auto mDate = reDtPosted.match(blk);
        if (mDate.hasMatch()) {
            const QString ds = mDate.captured(1);
            d = QDate::fromString(ds.left(8), QStringLiteral("yyyyMMdd"));
        }
        if (!d.isValid()) continue;

        // Valor
        auto mAmt = reTrnAmt.match(blk);
        if (!mAmt.hasMatch()) continue;
        bool ok = false;
        const double val = mAmt.captured(1).toDouble(&ok);
        if (!ok) continue;

        // Descrição: prefere MEMO, fallback NAME
        QString desc;
        auto mMemo = reMemo.match(blk);
        if (mMemo.hasMatch()) desc = mMemo.captured(1).trimmed();
        if (desc.isEmpty()) {
            auto mName = reName.match(blk);
            if (mName.hasMatch()) desc = mName.captured(1).trimmed();
        }

        ItemExtrato item;
        item.data      = d.toString(Qt::ISODate);
        item.descricao = desc;
        item.valor     = static_cast<qint64>(qRound(val * 100.0));
        result << item;
    }
    return result;
}
