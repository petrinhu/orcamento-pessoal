#include "ParcelasWidget.h"
#include "AddCompraDialog.h"
#include "CompraDetailPanel.h"
#include "services/CompraService.h"
#include "services/ParcelaService.h"
#include "core/commands/UndoCommands.h"
#include "ui/Theme.h"

#include <QUndoStack>
#include <memory>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QIcon>
#include <QLabel>
#include <QDate>
#include <QDialog>
#include <QDialogButtonBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGroupBox>
#include <QComboBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QMenu>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include <QStackedWidget>
#include <QScrollBar>
#include <QShortcut>
#include <algorithm>

#include "core/settings/AppSettings.h"
#include "services/CurrencyService.h"

#include <QLocale>

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

ParcelasWidget::ParcelasWidget(QWidget *parent)
    : QWidget(parent)
{
    m_currentMonth = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    buildUi();
    recarregar();
}

void ParcelasWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    m_btnNova = new QPushButton(tr("➕ Novo item"), this);
    m_btnNova->setObjectName(QStringLiteral("primary"));
    m_btnExcluir = new QPushButton(tr("🗑️ Excluir"), this);
    m_btnExcluir->setEnabled(false);

    auto *btnOrganizar = new QPushButton(tr("🔃 Organizar por ▾"), this);

    // Month navigation (espelha Dashboard/Planejamento; reusa navegarMes)
    auto *btnPrevMes = new QPushButton(QStringLiteral("◀"), this);
    auto *btnNextMes = new QPushButton(QStringLiteral("▶"), this);
    btnPrevMes->setFixedWidth(28);
    btnNextMes->setFixedWidth(28);
    btnPrevMes->setToolTip(tr("Mês anterior"));
    btnNextMes->setToolTip(tr("Próximo mês"));
    connect(btnPrevMes, &QPushButton::clicked, this, [this]() { navegarMes(-1); });
    connect(btnNextMes, &QPushButton::clicked, this, [this]() { navegarMes(+1); });

    m_lblStatus = new QLabel(this);
    m_lblStatus->setObjectName(QStringLiteral("subtitle"));

    toolbar->addWidget(m_btnNova);
    toolbar->addWidget(m_btnExcluir);
    toolbar->addWidget(btnOrganizar);
    toolbar->addStretch();
    toolbar->addWidget(btnPrevMes);
    toolbar->addWidget(btnNextMes);
    toolbar->addWidget(m_lblStatus);
    root->addLayout(toolbar);

    // ── Stack: empty state (0) + table (1) ────────────────────────────────
    m_stack = new QStackedWidget(this);

    auto *emptyPage = new QWidget(m_stack);
    auto *emptyLay  = new QVBoxLayout(emptyPage);
    emptyLay->setAlignment(Qt::AlignCenter);
    auto *lblEmptyTitle = new QLabel(tr("Nenhuma compra cadastrada."), emptyPage);
    lblEmptyTitle->setAlignment(Qt::AlignCenter);
    QFont fTitle = lblEmptyTitle->font();
    fTitle.setPointSize(14);
    lblEmptyTitle->setFont(fTitle);
    auto *lblEmptyHint = new QLabel(tr("Clique em \"➕ Novo item\" para começar."), emptyPage);
    lblEmptyHint->setObjectName(QStringLiteral("subtitle"));
    lblEmptyHint->setAlignment(Qt::AlignCenter);
    emptyLay->addWidget(lblEmptyTitle);
    emptyLay->addSpacing(4);
    emptyLay->addWidget(lblEmptyHint);
    m_stack->addWidget(emptyPage); // index 0

    m_table = new QTableWidget(m_stack);
    m_table->setAlternatingRowColors(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_stack->addWidget(m_table); // index 1

    root->addWidget(m_stack);

    // ── Detail panel (hidden until a row is selected) ──────────────────────
    m_detailPanel = new CompraDetailPanel(this);
    root->addWidget(m_detailPanel);

    connect(m_btnNova,    &QPushButton::clicked, this, &ParcelasWidget::onNovaCompra);
    connect(m_btnExcluir, &QPushButton::clicked, this, &ParcelasWidget::onExcluir);
    connect(btnOrganizar, &QPushButton::clicked, this, &ParcelasWidget::onOrganizarPor);

    connect(m_table->horizontalHeader(), &QHeaderView::sectionClicked,
            this, &ParcelasWidget::onHeaderClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                const int row = m_table->currentRow();
                const bool validRow = !m_table->selectedItems().isEmpty()
                    && row >= 0 && row < m_compras.size();
                m_btnExcluir->setEnabled(validRow);
                if (validRow) {
                    m_selectedCompraId = m_compras[row].id;
                    m_detailPanel->carregarCompra(m_compras[row]);
                } else {
                    m_selectedCompraId = 0;
                    m_detailPanel->limpar();
                }
            });
    connect(m_table, &QTableWidget::cellChanged, this, &ParcelasWidget::onCellChanged);

    connect(m_detailPanel, &CompraDetailPanel::compraAlterada,
            this, &ParcelasWidget::onCompraAlterada);

    // ── Keyboard navigation (Ctrl+← / Ctrl+→) ─────────────────────────────
    auto *scLeft = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Left),
                                 this, nullptr, nullptr,
                                 Qt::WidgetWithChildrenShortcut);
    connect(scLeft, &QShortcut::activated, this, [this]() { navegarMes(-1); });

    auto *scRight = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Right),
                                  this, nullptr, nullptr,
                                  Qt::WidgetWithChildrenShortcut);
    connect(scRight, &QShortcut::activated, this, [this]() { navegarMes(+1); });
}

void ParcelasWidget::selecionarTudo()
{
    m_table->selectAll();
    m_table->setFocus();
}

void ParcelasWidget::copiar()
{
    const auto items = m_table->selectedItems();
    if (items.isEmpty()) return;

    int minRow = items.first()->row(), maxRow = items.first()->row();
    int minCol = items.first()->column(), maxCol = items.first()->column();
    for (const auto *it : items) {
        minRow = qMin(minRow, it->row());
        maxRow = qMax(maxRow, it->row());
        minCol = qMin(minCol, it->column());
        maxCol = qMax(maxCol, it->column());
    }

    QString tsv;
    for (int r = minRow; r <= maxRow; ++r) {
        QStringList row;
        for (int c = minCol; c <= maxCol; ++c) {
            const auto *it = m_table->item(r, c);
            row << (it ? it->text() : QString());
        }
        tsv += row.join(QLatin1Char('\t'));
        tsv += QLatin1Char('\n');
    }
    QApplication::clipboard()->setText(tsv);
}

void ParcelasWidget::recarregar()
{
    m_loading = true;

    CompraService svc;

    // Load cartoes for display
    m_cartoes = svc.cartoes();

    m_compras           = svc.compras();
    m_parcelasPorCompra.clear();
    for (const auto &c : m_compras)
        m_parcelasPorCompra.append(svc.parcelasDeCompra(c.id));

    // Compute month range
    m_firstMonth  = m_currentMonth;
    m_lastMonth   = m_currentMonth.addMonths(3);

    for (int i = 0; i < m_compras.size(); ++i) {
        const auto &parcelas = m_parcelasPorCompra[i];
        if (parcelas.isEmpty()) continue;
        const auto &first = parcelas.first();
        const auto &last  = parcelas.last();
        QDate df = ParcelaService::dataParaParcela(m_compras[i], first.numero);
        QDate dl = ParcelaService::dataParaParcela(m_compras[i], last.numero);
        if (df < m_firstMonth) m_firstMonth = df;
        if (dl > m_lastMonth)  m_lastMonth  = dl;
    }

    m_stack->setCurrentIndex(m_compras.isEmpty() ? 0 : 1);
    populateTable();
    aplicarOrdenacao();
    m_loading = false;

    // Restore selection (and detail panel) after reload
    if (m_selectedCompraId > 0) {
        for (int r = 0; r < m_compras.size(); ++r) {
            if (m_compras[r].id == m_selectedCompraId) {
                m_table->selectRow(r);
                break;
            }
        }
    } else {
        m_detailPanel->limpar();
    }

    // Status bar — total do mês atual
    qint64 totalMes = 0;
    for (int i = 0; i < m_compras.size(); ++i) {
        for (const auto &p : m_parcelasPorCompra[i]) {
            const QDate d = ParcelaService::dataParaParcela(m_compras[i], p.numero);
            if (d.year() == m_currentMonth.year() && d.month() == m_currentMonth.month())
                totalMes += p.valor;
        }
    }
    emit statusAtualizado(tr("%1 compra(s)  •  %2: %3")
        .arg(m_compras.size())
        .arg(m_currentMonth.toString(QStringLiteral("MMM/yy")))
        .arg(ParcelaService::formatarValor(totalMes)));
}

QDate ParcelasWidget::dateForColumn(int col) const
{
    // col 0..2 = Nome, Cartão, Total
    // col COL_FIRST_MONTH + k = m_firstMonth.addMonths(k)
    if (col < COL_FIRST_MONTH) return QDate();
    return m_firstMonth.addMonths(col - COL_FIRST_MONTH);
}

int ParcelasWidget::columnForDate(const QDate &date) const
{
    int months = m_firstMonth.daysTo(date) > 0
        ? (date.year() - m_firstMonth.year()) * 12 + (date.month() - m_firstMonth.month())
        : -1;
    if (months < 0) return -1;
    return COL_FIRST_MONTH + months;
}

void ParcelasWidget::populateTable()
{
    m_table->blockSignals(true);
    m_table->clearContents();

    // Calculate total months to show
    int totalMonths = (m_lastMonth.year()  - m_firstMonth.year()) * 12
                    + (m_lastMonth.month() - m_firstMonth.month()) + 1;
    totalMonths = qMax(totalMonths, 6);

    const int cols = COL_FIRST_MONTH + totalMonths;
    m_table->setColumnCount(cols);
    m_table->setRowCount(m_compras.size() + 1); // +1 for Total row

    // Headers
    QStringList headers;
    headers << tr("Descrição")
            << tr("Cartão")
            << tr("Total pago");
    for (int k = 0; k < totalMonths; ++k) {
        const QDate d = m_firstMonth.addMonths(k);
        headers << formatMesColuna(d);
    }
    m_table->setHorizontalHeaderLabels(headers);

    // Highlight current month header
    const int curCol = columnForDate(m_currentMonth);
    if (curCol >= 0) {
        auto *hdr = new QTableWidgetItem(
            formatMesColuna(m_currentMonth) + QStringLiteral(" ●"));
        m_table->setHorizontalHeaderItem(curCol, hdr);
    }

    // Fill rows
    QList<qint64> monthTotals(totalMonths, 0LL);

    for (int r = 0; r < m_compras.size(); ++r) {
        const auto &c       = m_compras[r];
        const auto &parcelas = m_parcelasPorCompra[r];

        // Col 0: nome — store compra id in UserRole
        auto *itNome = new QTableWidgetItem(c.nome);
        itNome->setData(Qt::UserRole, c.id);
        m_table->setItem(r, COL_NOME, itNome);

        // Col 1: cartao
        QString cartaoNome;
        for (const auto &ct : m_cartoes)
            if (ct.id == c.cartaoId) { cartaoNome = ct.apelido; break; }
        m_table->setItem(r, COL_CARTAO, new QTableWidgetItem(cartaoNome));

        // Col 2: total (sum of all parcelas)
        qint64 total = 0;
        for (const auto &p : parcelas) total += p.valor;
        auto *itTotal = new QTableWidgetItem(
            ParcelaService::formatarValor(total, c.moeda));
        itTotal->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itTotal->setFlags(itTotal->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(r, COL_TOTAL, itTotal);

        // Month columns
        for (const auto &p : parcelas) {
            const QDate d   = ParcelaService::dataParaParcela(c, p.numero);
            const int   col = columnForDate(d);
            if (col < COL_FIRST_MONTH || col >= cols) continue;

            const int k = col - COL_FIRST_MONTH;
            monthTotals[k] += p.valor;

            auto *cell = new QTableWidgetItem(
                ParcelaService::formatarValor(p.valor, c.moeda));
            cell->setData(Qt::UserRole, p.id);
            cell->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (p.modificada) {
                cell->setForeground(QColor(Theme::acento()));
            }
            m_table->setItem(r, col, cell);
        }
    }

    // Total row
    const int totalRow = m_compras.size();
    auto *lblTotal = new QTableWidgetItem(tr("TOTAL"));
    lblTotal->setFlags(lblTotal->flags() & ~Qt::ItemIsEditable);
    QFont f = lblTotal->font();
    f.setBold(true);
    lblTotal->setFont(f);
    m_table->setItem(totalRow, COL_NOME, lblTotal);

    for (int k = 0; k < totalMonths; ++k) {
        if (monthTotals[k] == 0) continue;
        const QString tgtCur = AppSettings::instance().defaultCurrency();
        qint64 displayVal = monthTotals[k];
        if (tgtCur != QStringLiteral("BRL")) {
            try {
                displayVal = CurrencyService::instance().converter(
                    displayVal, QStringLiteral("BRL"), tgtCur);
            } catch (...) {
                displayVal = monthTotals[k]; // fallback: show BRL on error
            }
        }
        auto *cell = new QTableWidgetItem(
            ParcelaService::formatarValor(displayVal, tgtCur));
        cell->setFlags(cell->flags() & ~Qt::ItemIsEditable);
        cell->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QFont bf = cell->font();
        bf.setBold(true);
        cell->setFont(bf);
        m_table->setItem(totalRow, COL_FIRST_MONTH + k, cell);
    }

    // Scroll to current month
    if (curCol >= 0)
        m_table->scrollTo(m_table->model()->index(0, curCol),
                          QAbstractItemView::PositionAtTop);

    m_table->blockSignals(false);

    // Expande colunas ao conteúdo; nunca encolhe (preserva ajuste manual)
    for (int c = 0; c < m_table->columnCount(); ++c) {
        const int prev = m_table->columnWidth(c);
        m_table->resizeColumnToContents(c);
        if (m_table->columnWidth(c) < prev)
            m_table->setColumnWidth(c, prev);
    }

    m_lblStatus->setText(tr("%1 compra(s)").arg(m_compras.size()));
}

void ParcelasWidget::aplicarOrdenacao()
{
    if (m_sortLevels.isEmpty() && m_lastClickedCol < 0) return;

    const int totalRow = m_compras.size(); // last row = Total, don't sort it
    if (totalRow <= 1) return;

    // Build sort order using QtAlgorithm on row indices
    QList<int> rows;
    for (int i = 0; i < totalRow; ++i) rows.append(i);

    std::stable_sort(rows.begin(), rows.end(), [&](int a, int b) {
        // Apply sort levels in order
        const auto levels = m_sortLevels.isEmpty()
            ? QList<SortLevel>{{m_lastClickedCol, m_sortAsc}}
            : m_sortLevels;

        for (const auto &lvl : levels) {
            if (lvl.coluna < 0) continue;
            auto *ia = m_table->item(a, lvl.coluna);
            auto *ib = m_table->item(b, lvl.coluna);
            const QString va = ia ? ia->text() : QString();
            const QString vb = ib ? ib->text() : QString();
            if (va != vb)
                return lvl.crescente ? va < vb : va > vb;
        }
        return false;
    });

    // Re-order rows in table (swap QTableWidgetItems)
    m_table->blockSignals(true);
    // Save all items
    const int cols = m_table->columnCount();
    QList<QList<QTableWidgetItem*>> saved(totalRow, QList<QTableWidgetItem*>(cols, nullptr));
    for (int r = 0; r < totalRow; ++r)
        for (int c = 0; c < cols; ++c)
            saved[r][c] = m_table->takeItem(r, c);

    for (int newRow = 0; newRow < totalRow; ++newRow)
        for (int c = 0; c < cols; ++c)
            m_table->setItem(newRow, c, saved[rows[newRow]][c]);

    m_table->blockSignals(false);
}

void ParcelasWidget::onNovaCompra()
{
    AddCompraDialog dlg(m_cartoes, this);
    if (dlg.exec() != QDialog::Accepted) return;

    Compra compra = dlg.compra();
    QList<Parcela> parcelas = dlg.parcelas();

    auto compraPtr   = std::make_shared<Compra>(compra);
    auto parcelasPtr = std::make_shared<QList<Parcela>>(parcelas);

    if (!CompraService().inserir(*compraPtr, *parcelasPtr)) {
        QMessageBox::warning(this, tr("Erro"),
                             tr("Não foi possível salvar a compra."));
        return;
    }

    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Adicionar compra"),
            [compraPtr, parcelasPtr]() {
                compraPtr->id = 0;
                CompraService().inserir(*compraPtr, *parcelasPtr);
            },
            [compraPtr]() { CompraService().remover(compraPtr->id); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));

    recarregar();
    emit dadosAlterados();
}

void ParcelasWidget::novaCompraDeCalculadora(double pv, int n, double iMensal, double pmt)
{
    Q_UNUSED(pmt) // PMT is recalculated by AddCompraDialog from PV/N/i
    AddCompraDialog dlg(m_cartoes, this);
    dlg.preencherJuros(pv, n, iMensal);
    if (dlg.exec() != QDialog::Accepted) return;

    auto compraPtr2   = std::make_shared<Compra>(dlg.compra());
    auto parcelasPtr2 = std::make_shared<QList<Parcela>>(dlg.parcelas());

    if (!CompraService().inserir(*compraPtr2, *parcelasPtr2)) {
        QMessageBox::warning(this, tr("Erro"),
                             tr("Não foi possível salvar a compra."));
        return;
    }

    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Adicionar compra"),
            [compraPtr2, parcelasPtr2]() {
                compraPtr2->id = 0;
                CompraService().inserir(*compraPtr2, *parcelasPtr2);
            },
            [compraPtr2]() { CompraService().remover(compraPtr2->id); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));

    recarregar();
    emit dadosAlterados();
}

void ParcelasWidget::onExcluir()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_compras.size()) return;

    const int id   = m_compras[row].id;
    const QString nome = m_compras[row].nome;

    if (QMessageBox::question(this,
            tr("Confirmar exclusão"),
            tr("Excluir \"%1\" e todas as suas parcelas?").arg(nome))
        != QMessageBox::Yes) return;

    auto compraSnap   = std::make_shared<Compra>(m_compras[row]);
    auto parcelasSnap = std::make_shared<QList<Parcela>>(m_parcelasPorCompra[row]);

    CompraService().remover(id); // CASCADE removes parcelas

    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Remover compra"),
            [compraSnap]() { CompraService().remover(compraSnap->id); },
            [compraSnap, parcelasSnap]() {
                compraSnap->id = 0;
                CompraService().inserir(*compraSnap, *parcelasSnap);
            },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));

    recarregar();
    emit dadosAlterados();
}

void ParcelasWidget::onHeaderClicked(int col)
{
    if (col < COL_NOME) return;
    if (m_lastClickedCol == col) {
        m_sortAsc = !m_sortAsc;
    } else {
        m_lastClickedCol = col;
        m_sortAsc = false; // first click = descending
    }
    m_sortLevels.clear(); // header click overrides multi-sort
    aplicarOrdenacao();
}

void ParcelasWidget::onOrganizarPor()
{
    // Dialog with 3 combo boxes for sort levels
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Organizar por"));
    auto *form = new QFormLayout(&dlg);
    form->setContentsMargins(20, 20, 20, 20);
    form->setSpacing(12);

    QStringList colNames;
    colNames << QStringLiteral("—") << tr("Descrição")
             << tr("Cartão") << tr("Total");
    for (int k = 0; k < m_table->columnCount() - COL_FIRST_MONTH; ++k)
        colNames << formatMesColuna(m_firstMonth.addMonths(k));

    auto makeCombo = [&]() {
        auto *cb = new QComboBox(&dlg);
        cb->addItems(colNames);
        return cb;
    };

    auto *cb1 = makeCombo(); form->addRow(tr("Primeiro por"), cb1);
    auto *cb2 = makeCombo(); form->addRow(tr("Segundo por"),  cb2);
    auto *cb3 = makeCombo(); form->addRow(tr("Terceiro por"), cb3);

    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    auto idxToCol = [&](int comboIdx) -> int {
        // 0=—, 1=Descrição(0), 2=Cartão(1), 3=Total(2), 4+=months
        if (comboIdx == 0) return -1;
        return comboIdx - 1;
    };

    m_sortLevels.clear();
    for (auto *cb : {cb1, cb2, cb3}) {
        const int col = idxToCol(cb->currentIndex());
        if (col >= 0)
            m_sortLevels.append({col, true});
    }
    m_lastClickedCol = -1;
    aplicarOrdenacao();
}

void ParcelasWidget::onCellChanged(int row, int col)
{
    if (m_loading) return;
    if (col < COL_FIRST_MONTH) return;
    if (row >= m_compras.size()) return; // Total row

    auto *cell = m_table->item(row, col);
    if (!cell) return;

    const qint64 novoValor = ParcelaService::parsearValor(cell->text());

    // Find which parcela this cell represents
    const auto &compra   = m_compras[row];
    auto        parcelas = m_parcelasPorCompra[row];
    const QDate cellDate = dateForColumn(col);

    int parcelaIndex = -1;
    for (int k = 0; k < parcelas.size(); ++k) {
        if (ParcelaService::dataParaParcela(compra, parcelas[k].numero) == cellDate) {
            parcelaIndex = k;
            break;
        }
    }
    if (parcelaIndex < 0) return;

    // Ask how to recalculate
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Atualizar parcelas"));
    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(12);
    layout->addWidget(new QLabel(
        QStringLiteral("Como deseja atualizar as demais parcelas?"), &dlg));

    auto *grp = new QGroupBox(&dlg);
    auto *vl  = new QVBoxLayout(grp);
    auto *rSo      = new QRadioButton(QStringLiteral("Alterar só esta parcela"), &dlg);
    auto *rRedist  = new QRadioButton(QStringLiteral("Redistribuir diferença nas demais"), &dlg);
    auto *rIgualar = new QRadioButton(
        QStringLiteral("Igualar todas as parcelas a este valor (recalcula total)"), &dlg);
    auto *rJuros   = new QRadioButton(QStringLiteral("Recalcular juros a partir daqui"), &dlg);
    rSo->setChecked(true);
    vl->addWidget(rSo);
    vl->addWidget(rRedist);
    vl->addWidget(rIgualar);
    vl->addWidget(rJuros);
    layout->addWidget(grp);

    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) {
        recarregar(); // revert
        return;
    }

    auto modo = ParcelaService::RecalcMode::SoEsta;
    if (rRedist->isChecked())  modo = ParcelaService::RecalcMode::RedistribuirDiff;
    if (rIgualar->isChecked()) modo = ParcelaService::RecalcMode::IgualarTodas;
    if (rJuros->isChecked())   modo = ParcelaService::RecalcMode::RecalcJuros;

    const QList<Parcela> novas = ParcelaService::recalcularAposEdicao(
        parcelas, parcelaIndex, novoValor, modo, compra.valorTotal);

    auto parcelasAntes = std::make_shared<QList<Parcela>>(parcelas);
    auto parcelasDepois = std::make_shared<QList<Parcela>>(novas);
    auto compraAntes  = std::make_shared<Compra>(compra);

    CompraService svc;
    if (modo == ParcelaService::RecalcMode::IgualarTodas) {
        Compra updCompra = compra;
        updCompra.valorTotal = novoValor * static_cast<qint64>(novas.size());
        auto compraDepois = std::make_shared<Compra>(updCompra);
        svc.atualizarComParcelas(updCompra, novas);
        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(
                tr("Editar parcelas"),
                [compraDepois, parcelasDepois]() {
                    CompraService().atualizarComParcelas(*compraDepois, *parcelasDepois);
                },
                [compraAntes, parcelasAntes]() {
                    CompraService().atualizarComParcelas(*compraAntes, *parcelasAntes);
                },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));
    } else {
        svc.substituirParcelas(compra.id, novas);
        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(
                tr("Editar parcelas"),
                [compraAntes, parcelasDepois]() {
                    CompraService().substituirParcelas(compraAntes->id, *parcelasDepois);
                },
                [compraAntes, parcelasAntes]() {
                    CompraService().substituirParcelas(compraAntes->id, *parcelasAntes);
                },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));
    }

    recarregar();
    emit dadosAlterados();
}

// ─── Detail panel ─────────────────────────────────────────────────────────────

void ParcelasWidget::onCompraAlterada(const Compra &c)
{
    // Regenerate parcelas from the new parameters
    ResultadoFinanceiro res;
    const double juros = c.jurosMensais / 10000.0;
    if (juros > 1e-9)
        res = ParcelaService::comJurosMensais(c, juros);
    else
        res = ParcelaService::semJuros(c);

    QList<Parcela> novas = res.parcelas;
    for (auto &p : novas)
        p.compraId = c.id;

    // Save before state for undo
    const Compra *origPtr = nullptr;
    for (const auto &comp : std::as_const(m_compras))
        if (comp.id == c.id) { origPtr = &comp; break; }
    auto compraAntes   = std::make_shared<Compra>(origPtr ? *origPtr : c);
    auto parcelasAntes = std::make_shared<QList<Parcela>>(
        CompraService().parcelasDeCompra(c.id));
    auto compraDepois   = std::make_shared<Compra>(c);
    auto parcelasDepois = std::make_shared<QList<Parcela>>(novas);

    CompraService().atualizarComParcelas(c, novas);

    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Editar compra"),
            [compraDepois, parcelasDepois]() {
                CompraService().atualizarComParcelas(*compraDepois, *parcelasDepois);
            },
            [compraAntes, parcelasAntes]() {
                CompraService().atualizarComParcelas(*compraAntes, *parcelasAntes);
            },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));

    recarregar(); // restores selection via m_selectedCompraId
    emit dadosAlterados();
}

// ── Navigation ────────────────────────────────────────────────────────────────

void ParcelasWidget::scrollParaMesAtual()
{
    const int col = columnForDate(m_currentMonth);
    if (col < 0 || !m_table) return;
    m_table->scrollTo(m_table->model()->index(0, col), QAbstractItemView::PositionAtTop);
    m_table->horizontalScrollBar()->setValue(
        m_table->horizontalHeader()->sectionPosition(col));
}

void ParcelasWidget::navegarMes(int delta)
{
    if (!m_table) return;
    const int currentX = m_table->horizontalScrollBar()->value();
    // Find the first fully visible month column and shift by delta months
    const QHeaderView *hh = m_table->horizontalHeader();
    int visibleCol = COL_FIRST_MONTH;
    for (int c = COL_FIRST_MONTH; c < m_table->columnCount(); ++c) {
        if (hh->sectionPosition(c) >= currentX) {
            visibleCol = c;
            break;
        }
    }
    const int targetCol = qBound(COL_FIRST_MONTH,
                                 visibleCol + delta,
                                 m_table->columnCount() - 1);
    m_table->horizontalScrollBar()->setValue(hh->sectionPosition(targetCol));
}
