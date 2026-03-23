#include "EntradasWidget.h"
#include "LancamentoEntradaDialog.h"
#include "repositories/FonteRendaRepository.h"
#include "repositories/LancamentoEntradaRepository.h"
#include "core/settings/AppSettings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QApplication>
#include <QClipboard>
#include <QLocale>
#include <QMessageBox>
#include <QDate>
#include <QFont>

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

    toolbar->addWidget(btnNova);
    toolbar->addWidget(m_btnDel);
    toolbar->addStretch();
    root->addLayout(toolbar);

    // ── Table ──────────────────────────────────────────────────────────────
    m_table = new QTableWidget(this);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setVisible(false); // we use row 0 as header
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    root->addWidget(m_table);

    // ── Connections ────────────────────────────────────────────────────────
    connect(btnNova, &QPushButton::clicked, this, [this]() {
        // Open dialog for new entry in the first visible month
        const QDate targetMonth = m_numMonths > 0 ? m_firstMonth : QDate::currentDate();
        LancamentoEntrada empty;
        LancamentoEntradaDialog dlg(empty, targetMonth.month(), targetMonth.year(),
                                    fontesDisponiveis(), this);
        if (dlg.exec() != QDialog::Accepted) return;
        LancamentoEntradaRepository repo;
        for (auto l : dlg.lancamentos()) {
            if (l.id > 0) repo.update(l);
            else          repo.insert(l);
        }
        // Save new fontes if typed inline
        const auto lancados = dlg.lancamentos();
        if (!lancados.isEmpty() && !lancados.first().fonte.isEmpty())
            FonteRendaRepository().findOrCreate(lancados.first().fonte);
        recarregar();
        emit dadosAlterados();
    });

    connect(m_btnDel, &QPushButton::clicked, this, [this]() {
        // Collect unique lançamento IDs from selection (stored in UserRole on Fonte cells)
        QSet<int> ids;
        for (auto *item : m_table->selectedItems()) {
            const int col  = item->column();
            const int row  = item->row();
            if (row == 0 || row == m_table->rowCount() - 1) continue; // header / total
            const int k    = col / COL_PER_MONTH;
            const int dataRow = row - 1; // row 0 is header
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

        LancamentoEntradaRepository repo;
        for (int id : std::as_const(ids))
            repo.remove(id);
        recarregar();
        emit dadosAlterados();
    });

    connect(m_table, &QTableWidget::cellActivated,
            this, &EntradasWidget::onCellActivated);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                m_btnDel->setEnabled(!m_table->selectedItems().isEmpty());
            });
}

// ── Data loading ──────────────────────────────────────────────────────────────

void EntradasWidget::recarregar()
{
    const auto all = LancamentoEntradaRepository().findAll();

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

    // Determine max rows needed
    m_maxRows = 2; // at least 2 empty rows
    for (const auto &list : m_data)
        m_maxRows = qMax(m_maxRows, list.size() + 2);

    populateTable();
}

void EntradasWidget::populateTable()
{
    m_table->blockSignals(true);

    const int totalCols = m_numMonths * COL_PER_MONTH;
    const int totalRows = 1 + m_maxRows + 1; // header row + data rows + total row

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

        // Sub-column headers as tooltips on row 0 cols (visual only)
        // The actual sub-header labels are encoded in column widths / tooltips
    }
    m_table->setRowHeight(0, 28);

    // ── Data rows ──────────────────────────────────────────────────────────
    for (int dataRow = 0; dataRow < m_maxRows; ++dataRow) {
        const int tableRow = dataRow + 1;
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

                // Col 1: Valor líquido
                auto *itValor = readOnlyItem(formatCentavos(liq));
                itValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                m_table->setItem(tableRow, baseCol + 1, itValor);

                // Col 2: Data
                m_table->setItem(tableRow, baseCol + 2,
                    readOnlyItem(d.toString(QStringLiteral("dd/MM/yy"))));

                // Col 3: Obs
                auto *itObs = readOnlyItem(l.obs);
                itObs->setToolTip(l.obs);
                m_table->setItem(tableRow, baseCol + 3, itObs);

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

        // Col 1: sum of líquidos for this month
        qint64 sum = 0;
        for (const auto &l : m_data[k])
            sum += l.valorBruto - l.irpf - l.inss;
        auto *itSum = readOnlyItem(formatCentavos(sum));
        QFont fs = itSum->font();
        fs.setBold(true);
        itSum->setFont(fs);
        itSum->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(totalRow, baseCol + 1, itSum);

        // Cols 2-3: empty
        m_table->setItem(totalRow, baseCol + 2, readOnlyItem({}));
        m_table->setItem(totalRow, baseCol + 3, readOnlyItem({}));
    }

    // ── Column widths ──────────────────────────────────────────────────────
    for (int k = 0; k < m_numMonths; ++k) {
        const int base = k * COL_PER_MONTH;
        m_table->setColumnWidth(base + 0, 140); // Fonte
        m_table->setColumnWidth(base + 1, 110); // Valor
        m_table->setColumnWidth(base + 2,  80); // Data
        m_table->setColumnWidth(base + 3, 140); // Obs
    }

    m_table->blockSignals(false);
}

// ── Cell interaction ──────────────────────────────────────────────────────────

void EntradasWidget::onCellActivated(int row, int col)
{
    if (row == 0) return;                      // month header row
    if (col < 0 || col >= m_table->columnCount()) return;

    const int k       = col / COL_PER_MONTH;
    const int dataRow = row - 1;               // row 0 is header
    const int totalRow = m_table->rowCount() - 1;

    if (row == totalRow) return;               // TOTAL row

    const QDate monthDate = m_firstMonth.addMonths(k);
    LancamentoEntrada existing = lancamentoDaLinha(dataRow, k);

    LancamentoEntradaDialog dlg(existing, monthDate.month(), monthDate.year(),
                                fontesDisponiveis(), this);
    if (dlg.exec() != QDialog::Accepted) return;

    LancamentoEntradaRepository repo;
    FonteRendaRepository fonteRepo;
    for (auto l : dlg.lancamentos()) {
        // Ensure fonte exists
        if (!l.fonte.isEmpty()) {
            const auto f = fonteRepo.findOrCreate(l.fonte);
            l.fonteId = f.id;
        }
        if (l.id > 0) repo.update(l);
        else          repo.insert(l);
    }

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
    return FonteRendaRepository().findAll();
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
