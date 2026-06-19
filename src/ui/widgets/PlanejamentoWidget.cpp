#include "PlanejamentoWidget.h"
#include "ItemOrcamentoDialog.h"
#include "repositories/CategoriaRepository.h"
#include "repositories/FonteRendaRepository.h"
#include "repositories/OrcamentoRepository.h"
#include "repositories/GastoAvulsoRepository.h"
#include "repositories/LancamentoEntradaRepository.h"
#include "services/ResumoService.h"
#include "core/settings/AppSettings.h"
#include "core/commands/UndoCommands.h"
#include "ui/UiFormatters.h"

#include <QUndoStack>
#include <memory>

#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <QFont>
#include <QLocale>
#include <QBrush>
#include <QColor>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QTableWidgetItem *roItem(const QString &text)
{
    auto *it = new QTableWidgetItem(text);
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    return it;
}

static QTableWidgetItem *roItemRight(const QString &text)
{
    auto *it = roItem(text);
    it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return it;
}

// ── Construction ──────────────────────────────────────────────────────────────

PlanejamentoWidget::PlanejamentoWidget(QWidget *parent)
    : QWidget(parent)
    , m_mes(QDate::currentDate().month())
    , m_ano(QDate::currentDate().year())
{
    buildUi();
    recarregar();
}

void PlanejamentoWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 8, 8, 4);
    toolbar->setSpacing(8);

    auto *btnNovo = new QPushButton(tr("+ Novo item"), this);
    btnNovo->setObjectName(QStringLiteral("primary"));
    m_btnDel = new QPushButton(tr("Excluir"), this);
    m_btnDel->setEnabled(false);

    // Month navigation
    auto *btnPrev = new QPushButton(QStringLiteral("◀"), this);
    auto *btnNext = new QPushButton(QStringLiteral("▶"), this);
    btnPrev->setFixedWidth(28);
    btnNext->setFixedWidth(28);
    m_lblMes = new QLabel(this);
    m_lblMes->setMinimumWidth(120);
    m_lblMes->setAlignment(Qt::AlignCenter);
    updateMesLabel();

    toolbar->addWidget(btnNovo);
    toolbar->addWidget(m_btnDel);
    toolbar->addStretch();
    toolbar->addWidget(btnPrev);
    toolbar->addWidget(m_lblMes);
    toolbar->addWidget(btnNext);
    root->addLayout(toolbar);

    // ── Stack ──────────────────────────────────────────────────────────────
    m_stack = new QStackedWidget(this);

    auto *emptyPage = new QWidget(m_stack);
    auto *emptyLay  = new QVBoxLayout(emptyPage);
    emptyLay->setAlignment(Qt::AlignCenter);
    auto *lblTitle = new QLabel(tr("Nenhum item de orçamento cadastrado."), emptyPage);
    lblTitle->setAlignment(Qt::AlignCenter);
    QFont fTitle = lblTitle->font(); fTitle.setPointSize(14);
    lblTitle->setFont(fTitle);
    auto *lblHint = new QLabel(tr("Clique em \"+ Novo item\" para começar."), emptyPage);
    lblHint->setObjectName(QStringLiteral("subtitle"));
    lblHint->setAlignment(Qt::AlignCenter);
    emptyLay->addWidget(lblTitle);
    emptyLay->addSpacing(4);
    emptyLay->addWidget(lblHint);
    m_stack->addWidget(emptyPage); // index 0

    m_table = new QTableWidget(m_stack);
    m_table->setColumnCount(COL_COUNT);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setVisible(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_stack->addWidget(m_table); // index 1

    root->addWidget(m_stack);

    // ── Connections ────────────────────────────────────────────────────────
    connect(btnNovo, &QPushButton::clicked, this, [this]() {
        ItemOrcamento empty;
        auto *dlg = new ItemOrcamentoDialog(empty,
            CategoriaRepository().findAll(),
            FonteRendaRepository().findAll(), this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(dlg, &QDialog::accepted, this, [this, dlg]() {
            auto itemResult = dlg->result();
            if (itemResult.nome.isEmpty()) return;
            OrcamentoRepository().insert(itemResult);
            auto itemPtr = std::make_shared<ItemOrcamento>(itemResult);
            if (m_undoStack)
                m_undoStack->push(new LambdaCommand(
                    tr("Adicionar item de orçamento"),
                    [itemPtr]() { itemPtr->id = 0; OrcamentoRepository().insert(*itemPtr); },
                    [itemPtr]() { OrcamentoRepository().remove(itemPtr->id); },
                    [this]() { recarregar(); emit dadosAlterados(); }
                ));
            recarregar();
            emit dadosAlterados();
        });
        dlg->open();
    });

    connect(m_btnDel, &QPushButton::clicked, this, &PlanejamentoWidget::onExcluir);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &PlanejamentoWidget::onCellDoubleClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                const int row = m_table->currentRow();
                const bool isItem = (row >= 0 && row < m_itemIds.size()
                                     && m_itemIds[row] > 0);
                m_btnDel->setEnabled(isItem);
            });

    connect(btnPrev, &QPushButton::clicked, this, [this]() {
        if (--m_mes < 1) { m_mes = 12; --m_ano; }
        updateMesLabel();
        recarregar();
    });
    connect(btnNext, &QPushButton::clicked, this, [this]() {
        if (++m_mes > 12) { m_mes = 1; ++m_ano; }
        updateMesLabel();
        recarregar();
    });
}

// ── Data loading ──────────────────────────────────────────────────────────────

void PlanejamentoWidget::recarregar()
{
    const auto items   = OrcamentoRepository().findAll();
    m_stack->setCurrentIndex(items.isEmpty() ? 0 : 1);
    if (items.isEmpty()) return;

    const auto gastos   = GastoAvulsoRepository().findByMesAno(m_mes, m_ano);
    const auto entradas = LancamentoEntradaRepository().findByMesAno(m_mes, m_ano);

    // Totais agregados ("realizado") vêm do serviço canônico (Mid): entradas
    // líquidas, gastos e parcelas do mês inteiro — fonte única de verdade,
    // igual ao Dashboard. O breakdown por categoria/fonte abaixo continua via
    // repositório porque o ResumoService não expõe esse detalhe.
    const ResumoMensal resumo = ResumoService().resumoDoMes(m_mes, m_ano);
    const qint64 parcelasTotalMes = resumo.parcelas;
    const qint64 gastosTotalMes   = resumo.gastos;
    const qint64 entradasTotalMes = resumo.entradasLiquidas;

    // gastos: category → sum (breakdown por categoria, via repositório)
    QMap<QString, qint64> gastosPorCat;
    for (const auto &g : gastos)
        gastosPorCat[g.categoria] += g.valor;

    // entradas: fonte → valorLiquido (breakdown por fonte, via repositório)
    QMap<QString, qint64> entradasPorFonte;
    for (const auto &e : entradas) {
        const qint64 liq = e.valorBruto - e.irpf - e.inss;
        entradasPorFonte[e.fonte] += liq;
    }

    // Compute real for each item
    auto realDoItem = [&](const ItemOrcamento &it) -> qint64 {
        if (it.tipo == QStringLiteral("despesa")) {
            if (it.categoriaRef == QStringLiteral("*"))
                return gastosTotalMes + parcelasTotalMes;
            if (it.categoriaRef.isEmpty())
                return 0;
            return gastosPorCat.value(it.categoriaRef, 0);
        } else {
            if (it.categoriaRef.isEmpty() || it.categoriaRef == QStringLiteral("*"))
                return entradasTotalMes;
            return entradasPorFonte.value(it.categoriaRef, 0);
        }
    };

    populateTable();

    // Rebuild with real values — re-populate now that we have computed values
    // (populateTable is called with member data, so store computed data first)
    // We store item ids in m_itemIds during populateTable(); now fill values.
    // Actually let's do it all in one pass — refactor populateTable to accept computed data.

    // Re-populate the table rows with real values
    // We know the row structure from buildUi layout logic; re-run full populate.
    // Let me redo the approach: populate and fill in one function.

    // Clear and repopulate:
    m_table->blockSignals(true);
    m_itemIds.clear();

    // Separate receitas and despesas
    QList<ItemOrcamento> receitas, despesas;
    for (const auto &it : items) {
        if (it.tipo == QStringLiteral("receita")) receitas.append(it);
        else                                       despesas.append(it);
    }

    // Row count: 1(sub-labels) + 1(Receitas hdr) + R + 1(total) + 1(Despesas hdr) + D + 1(total) + 1(saldo)
    const int nRows = 1 + 1 + receitas.size() + 1 + 1 + despesas.size() + 1 + 1;
    m_table->setRowCount(nRows);
    m_itemIds.resize(nRows, 0);

    // Color palette
    const QColor colGreen(QStringLiteral("#166F4A"));
    const QColor colRed(QStringLiteral("#C94040"));
    const QColor colBg(QStringLiteral("#166F4A"));

    int row = 0;

    // ── Row 0: Sub-labels ──────────────────────────────────────────────────
    {
        const char *labels[COL_COUNT] = {
            QT_TR_NOOP("Nome"), QT_TR_NOOP("Planejado"),
            QT_TR_NOOP("Real"), QT_TR_NOOP("Diferença"), QT_TR_NOOP("%")
        };
        for (int c = 0; c < COL_COUNT; ++c) {
            auto *it = roItem(tr(labels[c]));
            QFont f = it->font(); f.setBold(true); f.setPointSize(11);
            it->setFont(f);
            if (c > 0) it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_table->setItem(row, c, it);
        }
        m_table->setRowHeight(row, 24);
        ++row;
    }

    // Helper: add a section header row
    auto addSectionHeader = [&](const QString &label) {
        auto *it = roItem(QStringLiteral("  ") + label);
        QFont f = it->font(); f.setBold(true);
        it->setFont(f);
        m_table->setItem(row, 0, it);
        m_table->setSpan(row, 0, 1, COL_COUNT);
        m_table->setRowHeight(row, 26);
        m_itemIds[row] = 0;
        ++row;
    };

    // Helper: add a data row
    auto addItemRow = [&](const ItemOrcamento &it, qint64 real) {
        const qint64 plan = it.valorMensal;
        const qint64 diff = real - plan; // negative = spent less (for despesa = good) / earned less (for receita = bad)
        const bool isDespesa = (it.tipo == QStringLiteral("despesa"));

        auto *itNome  = roItem(QStringLiteral("    ") + it.nome);
        itNome->setData(Qt::UserRole, it.id);
        m_table->setItem(row, 0, itNome);
        m_table->setItem(row, 1, roItemRight(uifmt::formatCentavosBRL(plan)));
        m_table->setItem(row, 2, roItemRight(uifmt::formatCentavosBRL(real)));

        auto *itDiff = roItemRight(uifmt::formatCentavosBRL(diff));
        // Color: for despesa diff < 0 = good (spent less); for receita diff > 0 = good (earned more)
        if (diff != 0) {
            const bool good = isDespesa ? (diff < 0) : (diff > 0);
            itDiff->setForeground(QBrush(good ? colGreen : colRed));
        }
        m_table->setItem(row, 3, itDiff);

        QString pctStr = QStringLiteral("—");
        if (plan > 0) {
            const int pct = static_cast<int>(real * 100 / plan);
            pctStr = QStringLiteral("%1%").arg(pct);
        }
        auto *itPct = roItemRight(pctStr);
        if (plan > 0) {
            const bool over = real > plan;
            const bool isGoodOver = !isDespesa; // receita: over = good; despesa: over = bad
            if (real != plan)
                itPct->setForeground(QBrush((over == isGoodOver) ? colGreen : colRed));
        }
        m_table->setItem(row, 4, itPct);

        m_table->setRowHeight(row, 28);
        m_itemIds[row] = it.id;
        ++row;
    };

    // Helper: add a total row
    auto addTotalRow = [&](const QString &label, qint64 planSum, qint64 realSum) {
        const qint64 diff = realSum - planSum;
        auto *itLabel = roItem(QStringLiteral("  ") + label);
        QFont f = itLabel->font(); f.setBold(true);
        itLabel->setFont(f);
        m_table->setItem(row, 0, itLabel);

        auto *itPlan = roItemRight(uifmt::formatCentavosBRL(planSum));
        itPlan->setFont(f);
        m_table->setItem(row, 1, itPlan);

        auto *itReal = roItemRight(uifmt::formatCentavosBRL(realSum));
        itReal->setFont(f);
        m_table->setItem(row, 2, itReal);

        auto *itDiff = roItemRight(uifmt::formatCentavosBRL(diff));
        itDiff->setFont(f);
        m_table->setItem(row, 3, itDiff);

        m_table->setItem(row, 4, roItem({}));
        m_table->setRowHeight(row, 28);
        m_itemIds[row] = 0;
        ++row;
    };

    // ── Receitas ───────────────────────────────────────────────────────────
    addSectionHeader(tr("RECEITAS"));
    qint64 planRec = 0, realRec = 0;
    for (const auto &it : receitas) {
        const qint64 r = realDoItem(it);
        addItemRow(it, r);
        planRec += it.valorMensal;
        realRec += r;
    }
    addTotalRow(tr("Total Receitas"), planRec, realRec);

    // ── Despesas ───────────────────────────────────────────────────────────
    addSectionHeader(tr("DESPESAS"));
    qint64 planDesp = 0, realDesp = 0;
    for (const auto &it : despesas) {
        const qint64 r = realDoItem(it);
        addItemRow(it, r);
        planDesp += it.valorMensal;
        realDesp += r;
    }
    addTotalRow(tr("Total Despesas"), planDesp, realDesp);

    // ── Saldo ──────────────────────────────────────────────────────────────
    {
        const qint64 planSaldo = planRec - planDesp;
        const qint64 realSaldo = realRec  - realDesp;
        auto *itLabel = roItem(tr("  SALDO"));
        QFont f = itLabel->font(); f.setBold(true); f.setPointSize(12);
        itLabel->setFont(f);
        m_table->setItem(row, 0, itLabel);
        m_table->setSpan(row, 0, 1, 1);

        auto addSaldoVal = [&](int col, qint64 val) {
            auto *it = roItemRight(uifmt::formatCentavosBRL(val));
            it->setFont(f);
            it->setForeground(QBrush(val >= 0 ? colGreen : colRed));
            m_table->setItem(row, col, it);
        };
        addSaldoVal(1, planSaldo);
        addSaldoVal(2, realSaldo);
        const qint64 diffSaldo = realSaldo - planSaldo;
        auto *itDiff = roItemRight(uifmt::formatCentavosBRL(diffSaldo));
        itDiff->setFont(f);
        itDiff->setForeground(QBrush(diffSaldo >= 0 ? colGreen : colRed));
        m_table->setItem(row, 3, itDiff);
        m_table->setItem(row, 4, roItem({}));
        m_table->setRowHeight(row, 32);
        m_itemIds[row] = 0;
    }

    // Column widths
    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int c = 1; c < COL_COUNT; ++c)
        m_table->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);

    m_table->blockSignals(false);

    // Status bar
    const qint64 realSaldo = realRec - realDesp;
    emit statusAtualizado(tr("Saldo %1/%2 — planejado: %3 | real: %4")
        .arg(m_mes, 2, 10, QLatin1Char('0'))
        .arg(m_ano)
        .arg(uifmt::formatCentavosBRL(planRec - planDesp))
        .arg(uifmt::formatCentavosBRL(realSaldo)));
}

void PlanejamentoWidget::populateTable()
{
    // Intentionally empty — actual logic is inline in recarregar() for single-pass efficiency.
}

// ── Cell interaction ──────────────────────────────────────────────────────────

void PlanejamentoWidget::onCellDoubleClicked(int row, int /*col*/)
{
    if (row < 0 || row >= m_itemIds.size()) return;
    const int id = m_itemIds[row];
    if (id <= 0) return;

    const auto opt = OrcamentoRepository().findById(id);
    if (!opt) return;

    auto *dlg = new ItemOrcamentoDialog(*opt,
                                        CategoriaRepository().findAll(),
                                        FonteRendaRepository().findAll(), this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, this, [this, dlg, opt]() {
        auto itemResult = dlg->result();
        if (itemResult.nome.isEmpty()) return;
        auto beforePtr = std::make_shared<ItemOrcamento>(*opt);
        auto afterPtr  = std::make_shared<ItemOrcamento>(itemResult);
        OrcamentoRepository().update(itemResult);
        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(
                tr("Editar item de orçamento"),
                [afterPtr]()  { OrcamentoRepository().update(*afterPtr); },
                [beforePtr]() { OrcamentoRepository().update(*beforePtr); },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));
        recarregar();
        emit dadosAlterados();
    });
    dlg->open();
}

void PlanejamentoWidget::onExcluir()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_itemIds.size()) return;
    const int id = m_itemIds[row];
    if (id <= 0) return;

    const auto opt = OrcamentoRepository().findById(id);
    if (!opt) return;

    if (QMessageBox::question(this, tr("Excluir"),
            tr("Excluir \"%1\"?").arg(opt->nome)) != QMessageBox::Yes) return;

    auto toRemove = std::make_shared<ItemOrcamento>(*opt);
    OrcamentoRepository().remove(id);
    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Remover item de orçamento"),
            [toRemove]() { OrcamentoRepository().remove(toRemove->id); },
            [toRemove]() { toRemove->id = 0; OrcamentoRepository().insert(*toRemove); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));
    recarregar();
    emit dadosAlterados();
}

// ── Navigation ────────────────────────────────────────────────────────────────

void PlanejamentoWidget::updateMesLabel()
{
    const bool enIntl =
        AppSettings::instance().language() == AppSettings::Language::EnIntl;
    const QLocale loc = enIntl
        ? QLocale(QLocale::English,    QLocale::UnitedStates)
        : QLocale(QLocale::Portuguese, QLocale::Brazil);
    const QDate d(m_ano, m_mes, 1);
    QString mes = loc.toString(d, QStringLiteral("MMMM"));
    mes.remove(u'.');
    if (!mes.isEmpty()) mes[0] = mes[0].toUpper();
    m_lblMes->setText(QStringLiteral("%1 %2").arg(mes).arg(m_ano));
}
