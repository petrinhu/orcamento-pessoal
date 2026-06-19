#include "InvestimentosWidget.h"
#include "InvestimentoDialog.h"
#include "repositories/CategoriaRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"
#include "core/settings/AppSettings.h"
#include "core/commands/UndoCommands.h"
#include "ui/UiFormatters.h"

#include <QUndoStack>
#include <memory>

#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <QChartView>
#include <QChart>
#include <QPieSeries>
#include <QPieSlice>
#include <QBarSeries>
#include <QBarSet>
#include <QBarCategoryAxis>
#include <QValueAxis>
#include <QPainter>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QDate>
#include <QFont>
#include <QScrollBar>
#include <QShortcut>
#include <algorithm>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString formatMesColuna(QDate d)
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

static QTableWidgetItem *readOnlyItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

// ── Construction ──────────────────────────────────────────────────────────────

InvestimentosWidget::InvestimentosWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void InvestimentosWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 8, 8, 4);
    toolbar->setSpacing(8);

    auto *btnNovo = new QPushButton(tr("➕ Novo investimento"), this);
    btnNovo->setObjectName(QStringLiteral("primary"));
    m_btnDel = new QPushButton(tr("🗑️ Excluir"), this);
    m_btnDel->setEnabled(false);
    m_btnOrg = new QPushButton(tr("🔃 Organizar por ▾"), this);
    m_btnOrg->setToolTip(tr("Ordenar lançamentos dentro de cada mês"));
    m_btnGrafico = new QPushButton(tr("📊 Gráfico ▾"), this);
    m_btnGrafico->setCheckable(true);
    m_btnGrafico->setToolTip(tr("Mostrar/ocultar painel de gráficos"));

    toolbar->addWidget(btnNovo);
    toolbar->addWidget(m_btnDel);
    toolbar->addWidget(m_btnOrg);
    toolbar->addStretch();
    toolbar->addWidget(m_btnGrafico);
    root->addLayout(toolbar);

    // ── Stack: empty state (0) + table (1) ────────────────────────────────
    m_stack = new QStackedWidget(this);

    auto *emptyPage = new QWidget(m_stack);
    auto *emptyLay  = new QVBoxLayout(emptyPage);
    emptyLay->setAlignment(Qt::AlignCenter);
    auto *lblTitle = new QLabel(tr("Nenhum investimento cadastrado."), emptyPage);
    lblTitle->setAlignment(Qt::AlignCenter);
    QFont fTitle = lblTitle->font();
    fTitle.setPointSize(14);
    lblTitle->setFont(fTitle);
    auto *lblHint = new QLabel(tr("Clique em \"➕ Novo investimento\" para começar."), emptyPage);
    lblHint->setObjectName(QStringLiteral("subtitle"));
    lblHint->setAlignment(Qt::AlignCenter);
    emptyLay->addWidget(lblTitle);
    emptyLay->addSpacing(4);
    emptyLay->addWidget(lblHint);
    m_stack->addWidget(emptyPage); // index 0

    m_table = new QTableWidget(m_stack);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setVisible(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_stack->addWidget(m_table); // index 1

    root->addWidget(m_stack);

    // ── Chart panel (hidden by default) ───────────────────────────────────
    buildChartPanel();
    root->addWidget(m_chartPanel);

    // ── Connections ────────────────────────────────────────────────────────
    connect(m_btnGrafico, &QPushButton::toggled, this, [this](bool on) {
        m_chartPanel->setVisible(on);
        if (on) atualizarGraficos();
    });

    connect(btnNovo, &QPushButton::clicked, this, [this]() {
        const QDate targetMonth = m_numMonths > 0
            ? m_firstMonth : QDate::currentDate();
        LancamentoInvestimento empty;
        auto *dlg = new InvestimentoDialog(empty,
                                           targetMonth.month(), targetMonth.year(),
                                           CategoriaRepository().findByTipo(QStringLiteral("investimento")),
                                           this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        connect(dlg, &QDialog::accepted, this, [this, dlg]() {
            auto items = std::make_shared<QList<LancamentoInvestimento>>();
            LancamentoInvestimentoRepository repo;
            for (auto l : dlg->lancamentos()) {
                repo.insert(l);
                items->append(l);
            }
            if (m_undoStack)
                m_undoStack->push(new LambdaCommand(
                    tr("Adicionar investimento(s)"),
                    [items]() {
                        for (auto &l : *items) {
                            l.id = 0;
                            LancamentoInvestimentoRepository().insert(l);
                        }
                    },
                    [items]() {
                        for (const auto &l : std::as_const(*items))
                            LancamentoInvestimentoRepository().remove(l.id);
                    },
                    [this]() { recarregar(); emit dadosAlterados(); }
                ));
            recarregar();
            emit dadosAlterados();
        });
        dlg->open();
    });

    connect(m_btnDel, &QPushButton::clicked, this, [this]() {
        QSet<int> ids;
        for (auto *item : m_table->selectedItems()) {
            const int row = item->row();
            const int col = item->column();
            if (row < HEADER_ROWS || row == m_table->rowCount() - 1) continue;
            const int k       = col / COL_PER_MONTH;
            const int dataRow = row - HEADER_ROWS;
            if (k < 0 || k >= m_numMonths) continue;
            const auto &list = m_data[k];
            if (dataRow < list.size() && list[dataRow].id > 0)
                ids.insert(list[dataRow].id);
        }
        if (ids.isEmpty()) return;
        if (QMessageBox::question(this, tr("Excluir"),
                tr("Excluir %1 investimento(s)?").arg(ids.size()))
            != QMessageBox::Yes) return;
        auto toRemove = std::make_shared<QList<LancamentoInvestimento>>();
        for (const auto &monthList : std::as_const(m_data))
            for (const auto &l : monthList)
                if (ids.contains(l.id)) toRemove->append(l);

        LancamentoInvestimentoRepository repo;
        for (int id : std::as_const(ids))
            repo.remove(id);

        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(
                tr("Excluir %1 investimento(s)").arg(ids.size()),
                [toRemove]() {
                    for (const auto &l : std::as_const(*toRemove))
                        LancamentoInvestimentoRepository().remove(l.id);
                },
                [toRemove]() {
                    for (auto &l : *toRemove) {
                        l.id = 0;
                        LancamentoInvestimentoRepository().insert(l);
                    }
                },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));

        recarregar();
        emit dadosAlterados();
    });

    connect(m_btnOrg, &QPushButton::clicked, this, &InvestimentosWidget::onOrganizarPor);
    connect(m_table, &QTableWidget::cellClicked, this, &InvestimentosWidget::onCellClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                m_btnDel->setEnabled(!m_table->selectedItems().isEmpty());
            });

    // ── Keyboard navigation ────────────────────────────────────────────────
    auto *scLeft = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left),
                                 this, nullptr, nullptr,
                                 Qt::WidgetWithChildrenShortcut);
    connect(scLeft, &QShortcut::activated, this, [this]() { navegarMes(-1); });

    auto *scRight = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right),
                                  this, nullptr, nullptr,
                                  Qt::WidgetWithChildrenShortcut);
    connect(scRight, &QShortcut::activated, this, [this]() { navegarMes(+1); });
}

// ── Data loading ──────────────────────────────────────────────────────────────

void InvestimentosWidget::recarregar()
{
    const auto all = LancamentoInvestimentoRepository().findAll();
    m_stack->setCurrentIndex(all.isEmpty() ? 0 : 1);

    QDate first = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    QDate last  = first.addMonths(2);
    for (const auto &l : all) {
        const QDate d(l.ano, l.mes, 1);
        if (d < first) first = d;
        if (d > last)  last  = d;
    }

    m_firstMonth = first;
    m_numMonths  = (last.year()  - first.year())  * 12
                 + (last.month() - first.month()) + 1;

    m_data.clear();
    m_data.resize(m_numMonths);
    for (const auto &l : all) {
        const int k = (l.ano - first.year()) * 12 + (l.mes - first.month());
        if (k >= 0 && k < m_numMonths)
            m_data[k].append(l);
    }

    // Sort within each month
    if (m_sortField != SortFieldInv::None) {
        for (auto &list : m_data) {
            std::sort(list.begin(), list.end(),
                [this](const LancamentoInvestimento &a, const LancamentoInvestimento &b) {
                    bool less = false;
                    switch (m_sortField) {
                    case SortFieldInv::Nome:
                        less = a.nome.compare(b.nome, Qt::CaseInsensitive) < 0; break;
                    case SortFieldInv::TipoAtivo:
                        less = a.tipoAtivo.compare(b.tipoAtivo, Qt::CaseInsensitive) < 0; break;
                    case SortFieldInv::Operacao:
                        less = a.tipoOperacao.compare(b.tipoOperacao, Qt::CaseInsensitive) < 0; break;
                    case SortFieldInv::Valor:
                        less = a.valor < b.valor; break;
                    case SortFieldInv::Data:
                        less = QDate(a.ano, a.mes, a.dia) < QDate(b.ano, b.mes, b.dia); break;
                    default: break;
                    }
                    return m_sortAsc ? less : !less;
                });
        }
    }

    m_maxRows = 2;
    for (const auto &list : m_data)
        m_maxRows = qMax(m_maxRows, list.size() + 2);

    populateTable();

    // Status bar — saldo líquido do mês atual
    const QDate cur(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    const int kCur = (cur.year() - m_firstMonth.year()) * 12
                   + (cur.month() - m_firstMonth.month());
    qint64 saldo = 0;
    if (kCur >= 0 && kCur < m_numMonths)
        for (const auto &l : m_data[kCur])
            saldo += valorSignadoInv(l);
    emit statusAtualizado(tr("Investimentos %1: %2")
        .arg(cur.toString(QStringLiteral("MMM/yy")))
        .arg(uifmt::formatCentavosBRL(saldo)));

    if (m_chartPanel && m_chartPanel->isVisible())
        atualizarGraficos();
}

void InvestimentosWidget::populateTable()
{
    m_table->blockSignals(true);

    const int totalCols = m_numMonths * COL_PER_MONTH;
    const int totalRows = HEADER_ROWS + m_maxRows + 1;

    m_table->setRowCount(totalRows);
    m_table->setColumnCount(totalCols);

    // ── Row 0: Month headers ───────────────────────────────────────────────
    for (int k = 0; k < m_numMonths; ++k) {
        const QDate d = m_firstMonth.addMonths(k);
        const int baseCol = k * COL_PER_MONTH;
        auto *hdr = readOnlyItem(formatMesColuna(d));
        QFont f = hdr->font(); f.setBold(true);
        hdr->setFont(f);
        hdr->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(0, baseCol, hdr);
        m_table->setSpan(0, baseCol, 1, COL_PER_MONTH);
    }
    m_table->setRowHeight(0, 28);

    // ── Row 1: Sub-column labels ───────────────────────────────────────────
    static const char* const kSubLabels[COL_PER_MONTH] = {
        QT_TR_NOOP("Nome"), QT_TR_NOOP("Categoria"), QT_TR_NOOP("Tipo"),
        QT_TR_NOOP("Operação"), QT_TR_NOOP("Valor"),
        QT_TR_NOOP("Data"),  QT_TR_NOOP("Obs")
    };
    for (int k = 0; k < m_numMonths; ++k) {
        for (int c = 0; c < COL_PER_MONTH; ++c) {
            auto *it = readOnlyItem(tr(kSubLabels[c]));
            QFont f = it->font(); f.setBold(true); f.setPointSize(11);
            it->setFont(f);
            m_table->setItem(1, k * COL_PER_MONTH + c, it);
        }
    }
    m_table->setRowHeight(1, 24);

    // ── Data rows ──────────────────────────────────────────────────────────
    for (int dataRow = 0; dataRow < m_maxRows; ++dataRow) {
        const int tableRow = dataRow + HEADER_ROWS;
        m_table->setRowHeight(tableRow, 28);

        for (int k = 0; k < m_numMonths; ++k) {
            const int baseCol = k * COL_PER_MONTH;
            const auto &list  = m_data[k];

            if (dataRow < list.size()) {
                const auto &l = list[dataRow];
                const QDate d(l.ano, l.mes, l.dia);

                // Col 0: Nome (stores id in UserRole)
                auto *itNome = readOnlyItem(l.nome);
                itNome->setData(Qt::UserRole, l.id);
                m_table->setItem(tableRow, baseCol + 0, itNome);

                // Col 1: Categoria
                m_table->setItem(tableRow, baseCol + 1,
                    readOnlyItem(l.categoria));

                // Col 2: Tipo Ativo
                m_table->setItem(tableRow, baseCol + 2,
                    readOnlyItem(tipoAtivoLabel(l.tipoAtivo)));

                // Col 3: Operação
                m_table->setItem(tableRow, baseCol + 3,
                    readOnlyItem(tipoOpInvLabel(l.tipoOperacao)));

                // Col 4: Valor
                auto *itValor = readOnlyItem(uifmt::formatCentavosBRL(l.valor));
                itValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                m_table->setItem(tableRow, baseCol + 4, itValor);

                // Col 5: Data
                m_table->setItem(tableRow, baseCol + 5,
                    readOnlyItem(d.toString(QStringLiteral("dd/MM/yy"))));

                // Col 6: Obs
                auto *itObs = readOnlyItem(l.obs);
                itObs->setToolTip(l.obs);
                m_table->setItem(tableRow, baseCol + 6, itObs);

            } else {
                for (int c = 0; c < COL_PER_MONTH; ++c)
                    m_table->setItem(tableRow, baseCol + c, readOnlyItem({}));
            }
        }
    }

    // ── Total row (saldo líquido) ──────────────────────────────────────────
    const int totalRow = totalRows - 1;
    m_table->setRowHeight(totalRow, 28);

    for (int k = 0; k < m_numMonths; ++k) {
        const int baseCol = k * COL_PER_MONTH;

        auto *itLabel = readOnlyItem(tr("SALDO"));
        QFont f = itLabel->font(); f.setBold(true);
        itLabel->setFont(f);
        m_table->setItem(totalRow, baseCol + 0, itLabel);

        qint64 saldo = 0;
        for (const auto &l : m_data[k]) saldo += valorSignadoInv(l);
        auto *itSaldo = readOnlyItem(uifmt::formatCentavosBRL(saldo));
        QFont fs = itSaldo->font(); fs.setBold(true);
        itSaldo->setFont(fs);
        itSaldo->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(totalRow, baseCol + 4, itSaldo);

        for (int c : {1, 2, 3, 5, 6})
            m_table->setItem(totalRow, baseCol + c, readOnlyItem({}));
    }

    // Auto-expand columns
    for (int c = 0; c < m_table->columnCount(); ++c) {
        const int prev = m_table->columnWidth(c);
        m_table->resizeColumnToContents(c);
        if (m_table->columnWidth(c) < prev)
            m_table->setColumnWidth(c, prev);
    }

    m_table->blockSignals(false);
}

// ── Cell interaction ──────────────────────────────────────────────────────────

void InvestimentosWidget::onCellClicked(int row, int col)
{
    if (row < HEADER_ROWS) return;
    if (col < 0 || col >= m_table->columnCount()) return;
    const int totalRow = m_table->rowCount() - 1;
    if (row == totalRow) return;

    const int k       = col / COL_PER_MONTH;
    const int dataRow = row - HEADER_ROWS;
    const QDate monthDate = m_firstMonth.addMonths(k);
    LancamentoInvestimento existing = lancamentoDaLinha(dataRow, k);

    auto *dlg = new InvestimentoDialog(existing, monthDate.month(), monthDate.year(),
                                       CategoriaRepository().findByTipo(QStringLiteral("investimento")),
                                       this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        auto beforeItems   = std::make_shared<QList<LancamentoInvestimento>>();
        auto updatedItems  = std::make_shared<QList<LancamentoInvestimento>>();
        auto insertedItems = std::make_shared<QList<LancamentoInvestimento>>();

        LancamentoInvestimentoRepository repo;
        for (auto l : dlg->lancamentos()) {
            if (l.id > 0) {
                auto opt = LancamentoInvestimentoRepository().findById(l.id);
                if (opt) beforeItems->append(*opt);
                updatedItems->append(l);
                repo.update(l);
            } else {
                repo.insert(l);
                insertedItems->append(l);
            }
        }

        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(
                tr("Editar investimento"),
                [updatedItems, insertedItems]() {
                    for (const auto &l : std::as_const(*updatedItems))
                        LancamentoInvestimentoRepository().update(l);
                    for (auto &l : *insertedItems) {
                        l.id = 0;
                        LancamentoInvestimentoRepository().insert(l);
                    }
                },
                [beforeItems, insertedItems]() {
                    for (const auto &l : std::as_const(*beforeItems))
                        LancamentoInvestimentoRepository().update(l);
                    for (const auto &l : std::as_const(*insertedItems))
                        LancamentoInvestimentoRepository().remove(l.id);
                },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));

        recarregar();
        emit dadosAlterados();
    });
    dlg->open();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

LancamentoInvestimento InvestimentosWidget::lancamentoDaLinha(int dataRow, int monthIndex) const
{
    if (monthIndex < 0 || monthIndex >= m_numMonths) return {};
    const auto &list = m_data[monthIndex];
    if (dataRow < 0 || dataRow >= list.size()) return {};
    return list[dataRow];
}

// ── Sort ──────────────────────────────────────────────────────────────────────

void InvestimentosWidget::onOrganizarPor()
{
    QMenu menu(this);

    auto addOpt = [&](const QString &label, SortFieldInv field, bool asc) {
        auto *act = menu.addAction(label);
        act->setCheckable(true);
        act->setChecked(m_sortField == field && m_sortAsc == asc);
        connect(act, &QAction::triggered, this, [this, field, asc]() {
            m_sortField = field;
            m_sortAsc   = asc;
            recarregar();
        });
    };

    addOpt(tr("Nome A→Z"),      SortFieldInv::Nome,      true);
    addOpt(tr("Nome Z→A"),      SortFieldInv::Nome,      false);
    menu.addSeparator();
    addOpt(tr("Tipo A→Z"),      SortFieldInv::TipoAtivo, true);
    addOpt(tr("Tipo Z→A"),      SortFieldInv::TipoAtivo, false);
    menu.addSeparator();
    addOpt(tr("Operação A→Z"),  SortFieldInv::Operacao,  true);
    addOpt(tr("Operação Z→A"),  SortFieldInv::Operacao,  false);
    menu.addSeparator();
    addOpt(tr("Valor ↑"),       SortFieldInv::Valor,     true);
    addOpt(tr("Valor ↓"),       SortFieldInv::Valor,     false);
    menu.addSeparator();
    addOpt(tr("Data ↑"),        SortFieldInv::Data,      true);
    addOpt(tr("Data ↓"),        SortFieldInv::Data,      false);
    menu.addSeparator();
    auto *actNone = menu.addAction(tr("Sem ordenação"));
    actNone->setCheckable(true);
    actNone->setChecked(m_sortField == SortFieldInv::None);
    connect(actNone, &QAction::triggered, this, [this]() {
        m_sortField = SortFieldInv::None;
        recarregar();
    });

    menu.exec(m_btnOrg->mapToGlobal(QPoint(0, m_btnOrg->height())));
}

// ── Clipboard ─────────────────────────────────────────────────────────────────

void InvestimentosWidget::copiar()
{
    const auto items = m_table->selectedItems();
    if (items.isEmpty()) return;
    int minRow = items.first()->row(), maxRow = items.first()->row();
    int minCol = items.first()->column(), maxCol = items.first()->column();
    for (auto *it : items) {
        minRow = qMin(minRow, it->row()); maxRow = qMax(maxRow, it->row());
        minCol = qMin(minCol, it->column()); maxCol = qMax(maxCol, it->column());
    }
    QString tsv;
    for (int r = minRow; r <= maxRow; ++r) {
        QStringList cols;
        for (int c = minCol; c <= maxCol; ++c) {
            auto *it = m_table->item(r, c);
            cols << (it ? it->text() : QString());
        }
        tsv += cols.join(QLatin1Char('\t')) + QLatin1Char('\n');
    }
    QApplication::clipboard()->setText(tsv);
}

void InvestimentosWidget::selecionarTudo()
{
    m_table->selectAll();
}

// ── Navigation ────────────────────────────────────────────────────────────────

void InvestimentosWidget::scrollParaMesAtual()
{
    if (!m_table || m_numMonths == 0) return;
    const QDate cur(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    int k = (cur.year() - m_firstMonth.year()) * 12 + (cur.month() - m_firstMonth.month());
    k = qBound(0, k, m_numMonths - 1);
    m_table->horizontalScrollBar()->setValue(
        m_table->horizontalHeader()->sectionPosition(k * COL_PER_MONTH));
}

void InvestimentosWidget::navegarMes(int delta)
{
    if (!m_table || m_numMonths == 0) return;
    const QHeaderView *hh = m_table->horizontalHeader();
    const int currentX = m_table->horizontalScrollBar()->value();
    int visibleMonth = 0;
    for (int k = 0; k < m_numMonths; ++k) {
        if (hh->sectionPosition(k * COL_PER_MONTH) >= currentX) {
            visibleMonth = k;
            break;
        }
    }
    const int targetMonth = qBound(0, visibleMonth + delta, m_numMonths - 1);
    m_table->horizontalScrollBar()->setValue(
        hh->sectionPosition(targetMonth * COL_PER_MONTH));
}

// ── Charts ────────────────────────────────────────────────────────────────────

void InvestimentosWidget::buildChartPanel()
{
    m_chartPanel = new QWidget(this);
    m_chartPanel->setFixedHeight(260);
    m_chartPanel->setVisible(false);

    auto *lay = new QHBoxLayout(m_chartPanel);
    lay->setContentsMargins(8, 4, 8, 4);
    lay->setSpacing(12);

    m_viewTipoAtivo = new QChartView(new QChart, m_chartPanel);
    m_viewTipoAtivo->setRenderHint(QPainter::Antialiasing);
    m_viewTipoAtivo->setFrameShape(QFrame::NoFrame);

    m_viewEvolucao = new QChartView(new QChart, m_chartPanel);
    m_viewEvolucao->setRenderHint(QPainter::Antialiasing);
    m_viewEvolucao->setFrameShape(QFrame::NoFrame);

    lay->addWidget(m_viewTipoAtivo);
    lay->addWidget(m_viewEvolucao);
}

void InvestimentosWidget::atualizarGraficos()
{
    const auto all = LancamentoInvestimentoRepository().findAll();

    // ── Donut: aportes líquidos por tipo de ativo ──────────────────────────
    QMap<QString, qint64> porTipo;
    for (const auto &l : all) {
        const qint64 v = (l.tipoOperacao == QStringLiteral("aporte"))
                         ? l.valor : -l.valor;
        porTipo[l.tipoAtivo.isEmpty() ? tr("(sem tipo)") : l.tipoAtivo] += v;
    }

    const QList<QColor> paleta = {
        QColor(0x16, 0x6F, 0x4A), QColor(0x4A, 0x90, 0xD9),
        QColor(0xC9, 0x40, 0x40), QColor(0xE8, 0xA8, 0x30),
        QColor(0x7B, 0x5E, 0xA7), QColor(0x2E, 0x8B, 0x57),
        QColor(0xD2, 0x69, 0x1E), QColor(0x20, 0x8F, 0xBD),
    };

    auto *pieSeries = new QPieSeries;
    pieSeries->setHoleSize(0.35);
    int colorIdx = 0;
    for (auto it = porTipo.constBegin(); it != porTipo.constEnd(); ++it) {
        if (it.value() <= 0) continue;
        auto *slice = pieSeries->append(it.key(), it.value() / 100.0);
        slice->setColor(paleta[colorIdx % paleta.size()]);
        ++colorIdx;
    }
    // Show label % for slices > 5 %
    for (auto *slice : pieSeries->slices()) {
        if (slice->percentage() > 0.05)
            slice->setLabelVisible(true);
        slice->setLabel(QStringLiteral("%1 (%2 %)")
            .arg(slice->label())
            .arg(qRound(slice->percentage() * 100)));
    }

    auto *chartDonut = new QChart;
    chartDonut->addSeries(pieSeries);
    chartDonut->setTitle(tr("Por tipo de ativo (aportes líquidos)"));
    chartDonut->legend()->setAlignment(Qt::AlignRight);
    chartDonut->setMargins(QMargins(4, 4, 4, 4));
    m_viewTipoAtivo->setChart(chartDonut);

    // ── Barras: evolução mensal líquida (últimos 12 meses) ─────────────────
    const QDate hoje = QDate::currentDate();
    const int N = 12;
    QStringList labels;
    auto *barSet = new QBarSet(tr("Líquido"));
    barSet->setColor(QColor(0x4A, 0x90, 0xD9));
    barSet->setBorderColor(QColor(0x4A, 0x90, 0xD9));

    for (int i = N - 1; i >= 0; --i) {
        const QDate d = QDate(hoje.year(), hoje.month(), 1).addMonths(-i);
        qint64 net = 0;
        for (const auto &l : all) {
            if (l.mes == d.month() && l.ano == d.year())
                net += (l.tipoOperacao == QStringLiteral("aporte"))
                       ? l.valor : -l.valor;
        }
        labels << d.toString(QStringLiteral("MMM/yy"));
        *barSet << net / 100.0;
    }

    auto *barSeries = new QBarSeries;
    barSeries->append(barSet);

    auto *axisX = new QBarCategoryAxis;
    axisX->append(labels);
    auto *axisY = new QValueAxis;
    axisY->setTitleText(QStringLiteral("R$"));
    axisY->setLabelFormat(QStringLiteral("%.0f"));

    auto *chartBar = new QChart;
    chartBar->addSeries(barSeries);
    chartBar->setTitle(tr("Evolução mensal líquida (12 meses)"));
    chartBar->addAxis(axisX, Qt::AlignBottom);
    chartBar->addAxis(axisY, Qt::AlignLeft);
    barSeries->attachAxis(axisX);
    barSeries->attachAxis(axisY);
    chartBar->legend()->hide();
    chartBar->setMargins(QMargins(4, 4, 4, 4));
    m_viewEvolucao->setChart(chartBar);
}
