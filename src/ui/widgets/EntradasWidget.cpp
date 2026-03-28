#include "EntradasWidget.h"
#include "LancamentoEntradaDialog.h"
#include "repositories/CategoriaRepository.h"
#include "services/EntradaService.h"
#include "core/settings/AppSettings.h"
#include "core/commands/UndoCommands.h"

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
#include <QApplication>
#include <QClipboard>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QDate>
#include <QFont>
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

static QString formatCentavos(qint64 c)
{
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("R$ %1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

static QTableWidgetItem *readOnlyItem(const QString &text)
{
    auto *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

// ── Construction ──────────────────────────────────────────────────────────────

EntradasWidget::EntradasWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void EntradasWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 8, 8, 4);
    toolbar->setSpacing(8);

    auto *btnNova = new QPushButton(tr("+ Novo lançamento"), this);
    btnNova->setObjectName(QStringLiteral("primary"));
    m_btnDel = new QPushButton(tr("Excluir"), this);
    m_btnDel->setEnabled(false);

    m_btnOrg = new QPushButton(tr("Organizar por ▾"), this);
    m_btnOrg->setToolTip(tr("Ordenar lançamentos dentro de cada mês"));

    toolbar->addWidget(btnNova);
    toolbar->addWidget(m_btnDel);
    toolbar->addWidget(m_btnOrg);
    toolbar->addStretch();
    root->addLayout(toolbar);

    // ── Stack: empty state (0) + table (1) ────────────────────────────────
    m_stack = new QStackedWidget(this);

    auto *emptyPage = new QWidget(m_stack);
    auto *emptyLay  = new QVBoxLayout(emptyPage);
    emptyLay->setAlignment(Qt::AlignCenter);
    auto *lblEmptyTitle = new QLabel(tr("Nenhum lançamento cadastrado."), emptyPage);
    lblEmptyTitle->setAlignment(Qt::AlignCenter);
    QFont fTitle = lblEmptyTitle->font();
    fTitle.setPointSize(14);
    lblEmptyTitle->setFont(fTitle);
    auto *lblEmptyHint = new QLabel(tr("Clique em \"+ Novo lançamento\" para começar."), emptyPage);
    lblEmptyHint->setObjectName(QStringLiteral("subtitle"));
    lblEmptyHint->setAlignment(Qt::AlignCenter);
    emptyLay->addWidget(lblEmptyTitle);
    emptyLay->addSpacing(4);
    emptyLay->addWidget(lblEmptyHint);
    m_stack->addWidget(emptyPage); // index 0

    m_table = new QTableWidget(m_stack);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setVisible(false); // we use row 0 as header
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_stack->addWidget(m_table); // index 1

    root->addWidget(m_stack);

    // ── Connections ────────────────────────────────────────────────────────
    connect(btnNova, &QPushButton::clicked, this, [this]() {
        // Open dialog for new entry in the first visible month
        const QDate targetMonth = m_numMonths > 0 ? m_firstMonth : QDate::currentDate();
        LancamentoEntrada empty;
        LancamentoEntradaDialog dlg(empty, targetMonth.month(), targetMonth.year(),
                                    fontesDisponiveis(),
                                    CategoriaRepository().findByTipo(QStringLiteral("entrada")),
                                    this);
        if (dlg.exec() != QDialog::Accepted) return;
        auto items = std::make_shared<QList<LancamentoEntrada>>();
        EntradaService svc;
        for (auto l : dlg.lancamentos()) {
            svc.salvarComFonte(l);
            items->append(l);
        }
        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(
                tr("Adicionar lançamento(s)"),
                [items]() {
                    for (auto &l : *items) {
                        l.id = 0;
                        EntradaService().salvarComFonte(l);
                    }
                },
                [items]() {
                    for (const auto &l : std::as_const(*items))
                        EntradaService().remover(l.id);
                },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));
        recarregar();
        emit dadosAlterados();
    });

    connect(m_btnDel, &QPushButton::clicked, this, [this]() {
        // Collect unique lançamento IDs from selection (stored in UserRole on Fonte cells)
        QSet<int> ids;
        for (auto *item : m_table->selectedItems()) {
            const int col  = item->column();
            const int row  = item->row();
            if (row < HEADER_ROWS || row == m_table->rowCount() - 1) continue; // headers / total
            const int k       = col / COL_PER_MONTH;
            const int dataRow = row - HEADER_ROWS;
            if (dataRow < 0 || dataRow >= m_maxRows) continue;
            if (k < 0 || k >= m_numMonths) continue;
            const auto &list = m_data[k];
            if (dataRow < list.size() && list[dataRow].id > 0)
                ids.insert(list[dataRow].id);
        }
        if (ids.isEmpty()) return;

        if (QMessageBox::question(this, tr("Excluir"),
                tr("Excluir %1 lançamento(s)?").arg(ids.size()))
            != QMessageBox::Yes) return;

        // Collect items to remove from cached data (for undo)
        auto toRemove = std::make_shared<QList<LancamentoEntrada>>();
        for (const auto &monthList : std::as_const(m_data))
            for (const auto &l : monthList)
                if (ids.contains(l.id)) toRemove->append(l);

        EntradaService svc;
        for (int id : std::as_const(ids))
            svc.remover(id);

        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(
                tr("Excluir lançamento(s)"),
                [toRemove]() {
                    for (const auto &l : std::as_const(*toRemove))
                        EntradaService().remover(l.id);
                },
                [toRemove]() {
                    for (auto &l : *toRemove) {
                        l.id = 0;
                        EntradaService().salvarComFonte(l);
                    }
                },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));

        recarregar();
        emit dadosAlterados();
    });

    connect(m_btnOrg, &QPushButton::clicked,
            this, &EntradasWidget::onOrganizarPor);
    connect(m_table, &QTableWidget::cellClicked,
            this, &EntradasWidget::onCellActivated);

    // ── Keyboard navigation (Ctrl+← / Ctrl+→) ─────────────────────────────
    auto *scLeft = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left),
                                 this, nullptr, nullptr,
                                 Qt::WidgetWithChildrenShortcut);
    connect(scLeft, &QShortcut::activated, this, [this]() { navegarMes(-1); });

    auto *scRight = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right),
                                  this, nullptr, nullptr,
                                  Qt::WidgetWithChildrenShortcut);
    connect(scRight, &QShortcut::activated, this, [this]() { navegarMes(+1); });
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                m_btnDel->setEnabled(!m_table->selectedItems().isEmpty());
            });
}

// ── Data loading ──────────────────────────────────────────────────────────────

void EntradasWidget::recarregar()
{
    const auto all = EntradaService().lancamentos();
    m_stack->setCurrentIndex(all.isEmpty() ? 0 : 1);

    // Determine month range
    QDate first = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    QDate last  = first.addMonths(2);
    for (const auto &l : all) {
        const QDate d(l.ano, l.mes, 1);
        if (d < first) first = d;
        if (d > last)  last  = d;
    }

    m_firstMonth = first;
    m_numMonths  = (last.year() - first.year()) * 12
                 + (last.month() - first.month()) + 1;

    // Group by month index
    m_data.clear();
    m_data.resize(m_numMonths);
    for (const auto &l : all) {
        const int k = (l.ano - first.year()) * 12 + (l.mes - first.month());
        if (k >= 0 && k < m_numMonths)
            m_data[k].append(l);
    }

    // Sort within each month if requested
    if (m_sortField != SortFieldEntradas::None) {
        for (auto &list : m_data) {
            std::sort(list.begin(), list.end(),
                [this](const LancamentoEntrada &a, const LancamentoEntrada &b) {
                    bool less = false;
                    switch (m_sortField) {
                    case SortFieldEntradas::Fonte:
                        less = a.fonte.compare(b.fonte, Qt::CaseInsensitive) < 0;
                        break;
                    case SortFieldEntradas::Valor:
                        less = (a.valorBruto - a.irpf - a.inss) < (b.valorBruto - b.irpf - b.inss);
                        break;
                    case SortFieldEntradas::Data:
                        less = QDate(a.ano, a.mes, a.dia) < QDate(b.ano, b.mes, b.dia);
                        break;
                    default:
                        break;
                    }
                    return m_sortAsc ? less : !less;
                });
        }
    }

    // Determine max rows needed
    m_maxRows = 2; // at least 2 empty rows
    for (const auto &list : m_data)
        m_maxRows = qMax(m_maxRows, list.size() + 2);

    populateTable();

    // Status bar — total líquido do mês atual
    const QDate cur(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    const int kCur = (cur.year() - m_firstMonth.year()) * 12
                   + (cur.month() - m_firstMonth.month());
    qint64 totalMes = 0;
    if (kCur >= 0 && kCur < m_numMonths)
        for (const auto &l : m_data[kCur])
            totalMes += l.valorBruto - l.irpf - l.inss;
    emit statusAtualizado(tr("Entradas %1: %2")
        .arg(cur.toString(QStringLiteral("MMM/yy")))
        .arg(formatCentavos(totalMes)));
}

void EntradasWidget::populateTable()
{
    m_table->blockSignals(true);

    const int totalCols = m_numMonths * COL_PER_MONTH;
    const int totalRows = HEADER_ROWS + m_maxRows + 1; // 2 header rows + data rows + total row

    m_table->setRowCount(totalRows);
    m_table->setColumnCount(totalCols);

    // ── Row 0: Month headers (merged 4 cols each) ──────────────────────────
    for (int k = 0; k < m_numMonths; ++k) {
        const QDate d = m_firstMonth.addMonths(k);
        const int baseCol = k * COL_PER_MONTH;

        auto *hdr = readOnlyItem(formatMesColuna(d));
        QFont f = hdr->font();
        f.setBold(true);
        hdr->setFont(f);
        hdr->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(0, baseCol, hdr);
        m_table->setSpan(0, baseCol, 1, COL_PER_MONTH);
    }
    m_table->setRowHeight(0, 28);

    // ── Row 1: Sub-column labels ────────────────────────────────────────────
    static const char* const kSubLabels[COL_PER_MONTH] =
        { QT_TR_NOOP("Fonte"), QT_TR_NOOP("Categoria"), QT_TR_NOOP("Valor"), QT_TR_NOOP("Data"), QT_TR_NOOP("Obs") };
    for (int k = 0; k < m_numMonths; ++k) {
        for (int c = 0; c < COL_PER_MONTH; ++c) {
            auto *it = readOnlyItem(tr(kSubLabels[c]));
            QFont f = it->font();
            f.setBold(true);
            f.setPointSize(11);
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
                const qint64 liq = l.valorBruto - l.irpf - l.inss;
                const QDate  d   = QDate(l.ano, l.mes, l.dia);

                // Col 0: Fonte
                auto *itFonte = readOnlyItem(l.fonte);
                itFonte->setData(Qt::UserRole, l.id);
                m_table->setItem(tableRow, baseCol + 0, itFonte);

                // Col 1: Categoria
                m_table->setItem(tableRow, baseCol + 1,
                    readOnlyItem(l.categoria));

                // Col 2: Valor líquido
                auto *itValor = readOnlyItem(formatCentavos(liq));
                itValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                m_table->setItem(tableRow, baseCol + 2, itValor);

                // Col 3: Data
                m_table->setItem(tableRow, baseCol + 3,
                    readOnlyItem(d.toString(QStringLiteral("dd/MM/yy"))));

                // Col 4: Obs
                auto *itObs = readOnlyItem(l.obs);
                itObs->setToolTip(l.obs);
                m_table->setItem(tableRow, baseCol + 4, itObs);

            } else {
                // Empty cells
                for (int c = 0; c < COL_PER_MONTH; ++c)
                    m_table->setItem(tableRow, baseCol + c, readOnlyItem({}));
            }
        }
    }

    // ── Total row ──────────────────────────────────────────────────────────
    const int totalRow = totalRows - 1;
    m_table->setRowHeight(totalRow, 28);

    for (int k = 0; k < m_numMonths; ++k) {
        const int baseCol = k * COL_PER_MONTH;

        // Col 0: "TOTAL" label
        auto *itLabel = readOnlyItem(tr("TOTAL"));
        QFont f = itLabel->font();
        f.setBold(true);
        itLabel->setFont(f);
        m_table->setItem(totalRow, baseCol + 0, itLabel);

        // Col 1: empty (categoria col)
        m_table->setItem(totalRow, baseCol + 1, readOnlyItem({}));

        // Col 2: sum of líquidos for this month
        qint64 sum = 0;
        for (const auto &l : m_data[k])
            sum += l.valorBruto - l.irpf - l.inss;
        auto *itSum = readOnlyItem(formatCentavos(sum));
        QFont fs = itSum->font();
        fs.setBold(true);
        itSum->setFont(fs);
        itSum->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(totalRow, baseCol + 2, itSum);

        // Cols 3-4: empty
        m_table->setItem(totalRow, baseCol + 3, readOnlyItem({}));
        m_table->setItem(totalRow, baseCol + 4, readOnlyItem({}));
    }

    // Expande colunas ao conteúdo; nunca encolhe (preserva ajuste manual)
    for (int c = 0; c < m_table->columnCount(); ++c) {
        const int prev = m_table->columnWidth(c);
        m_table->resizeColumnToContents(c);
        if (m_table->columnWidth(c) < prev)
            m_table->setColumnWidth(c, prev);
    }

    m_table->blockSignals(false);
}

// ── Cell interaction ──────────────────────────────────────────────────────────

void EntradasWidget::onCellActivated(int row, int col)
{
    if (row < HEADER_ROWS) return;             // month header or sub-label row
    if (col < 0 || col >= m_table->columnCount()) return;

    const int k        = col / COL_PER_MONTH;
    const int dataRow  = row - HEADER_ROWS;
    const int totalRow = m_table->rowCount() - 1;

    if (row == totalRow) return;               // TOTAL row

    const QDate monthDate = m_firstMonth.addMonths(k);
    LancamentoEntrada existing = lancamentoDaLinha(dataRow, k);

    LancamentoEntradaDialog dlg(existing, monthDate.month(), monthDate.year(),
                                fontesDisponiveis(),
                                CategoriaRepository().findByTipo(QStringLiteral("entrada")),
                                this);
    if (dlg.exec() != QDialog::Accepted) return;

    const bool isUpdate = (existing.id > 0);
    auto beforeItem = std::make_shared<LancamentoEntrada>(existing);
    auto afterItems = std::make_shared<QList<LancamentoEntrada>>();

    EntradaService svc;
    for (auto l : dlg.lancamentos()) {
        svc.salvarComFonte(l);
        afterItems->append(l);
    }

    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            isUpdate ? tr("Editar lançamento") : tr("Adicionar lançamento(s)"),
            [afterItems, isUpdate]() {
                for (auto &l : *afterItems) {
                    if (!isUpdate) l.id = 0;
                    EntradaService().salvarComFonte(l);
                }
            },
            [beforeItem, afterItems, isUpdate]() {
                if (isUpdate)
                    EntradaService().salvarComFonte(*beforeItem);
                else
                    for (const auto &l : std::as_const(*afterItems))
                        EntradaService().remover(l.id);
            },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));

    recarregar();
    emit dadosAlterados();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

LancamentoEntrada EntradasWidget::lancamentoDaLinha(int dataRow, int monthIndex) const
{
    if (monthIndex < 0 || monthIndex >= m_numMonths) return {};
    const auto &list = m_data[monthIndex];
    if (dataRow < 0 || dataRow >= list.size()) return {};
    return list[dataRow];
}

QList<FonteRenda> EntradasWidget::fontesDisponiveis() const
{
    return EntradaService().fontes();
}

// ── Sort ──────────────────────────────────────────────────────────────────────

void EntradasWidget::onOrganizarPor()
{
    QMenu menu(this);

    // Helper: add a checkable action that sets sort field + direction
    auto addOpt = [&](const QString &label, SortFieldEntradas field, bool asc) {
        auto *act = menu.addAction(label);
        act->setCheckable(true);
        act->setChecked(m_sortField == field && m_sortAsc == asc);
        connect(act, &QAction::triggered, this, [this, field, asc]() {
            m_sortField = field;
            m_sortAsc   = asc;
            recarregar();
        });
    };

    addOpt(tr("Fonte A → Z"),  SortFieldEntradas::Fonte, true);
    addOpt(tr("Fonte Z → A"),  SortFieldEntradas::Fonte, false);
    menu.addSeparator();
    addOpt(tr("Valor ↑"),      SortFieldEntradas::Valor, true);
    addOpt(tr("Valor ↓"),      SortFieldEntradas::Valor, false);
    menu.addSeparator();
    addOpt(tr("Data ↑"),       SortFieldEntradas::Data, true);
    addOpt(tr("Data ↓"),       SortFieldEntradas::Data, false);
    menu.addSeparator();

    auto *actNenhum = menu.addAction(tr("Sem ordenação"));
    actNenhum->setCheckable(true);
    actNenhum->setChecked(m_sortField == SortFieldEntradas::None);
    connect(actNenhum, &QAction::triggered, this, [this]() {
        m_sortField = SortFieldEntradas::None;
        recarregar();
    });

    // Show menu below the button
    menu.exec(m_btnOrg->mapToGlobal(QPoint(0, m_btnOrg->height())));
}

// ── Public slots ──────────────────────────────────────────────────────────────

void EntradasWidget::copiar()
{
    const auto items = m_table->selectedItems();
    if (items.isEmpty()) return;

    int minRow = items.first()->row(), maxRow = items.first()->row();
    int minCol = items.first()->column(), maxCol = items.first()->column();
    for (auto *it : items) {
        minRow = qMin(minRow, it->row());
        maxRow = qMax(maxRow, it->row());
        minCol = qMin(minCol, it->column());
        maxCol = qMax(maxCol, it->column());
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

void EntradasWidget::selecionarTudo()
{
    m_table->selectAll();
}

// ── Navigation ────────────────────────────────────────────────────────────────

void EntradasWidget::scrollParaMesAtual()
{
    if (!m_table || m_numMonths == 0) return;
    // Find the month index for current date
    const QDate cur(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    int k = (cur.year() - m_firstMonth.year()) * 12 + (cur.month() - m_firstMonth.month());
    k = qBound(0, k, m_numMonths - 1);
    const int col = k * COL_PER_MONTH;
    m_table->horizontalScrollBar()->setValue(
        m_table->horizontalHeader()->sectionPosition(col));
}

void EntradasWidget::navegarMes(int delta)
{
    if (!m_table || m_numMonths == 0) return;
    const QHeaderView *hh = m_table->horizontalHeader();
    const int currentX = m_table->horizontalScrollBar()->value();
    // Determine currently leftmost visible month
    int visibleMonth = 0;
    for (int k = 0; k < m_numMonths; ++k) {
        const int col = k * COL_PER_MONTH;
        if (hh->sectionPosition(col) >= currentX) {
            visibleMonth = k;
            break;
        }
    }
    const int targetMonth = qBound(0, visibleMonth + delta, m_numMonths - 1);
    m_table->horizontalScrollBar()->setValue(
        hh->sectionPosition(targetMonth * COL_PER_MONTH));
}
