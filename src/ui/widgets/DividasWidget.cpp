#include "DividasWidget.h"
#include "EmptyStateWidget.h"
#include "ui/Theme.h"
#include "DividaDialog.h"
#include "repositories/DividaRepository.h"
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
#include <QComboBox>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QDate>
#include <QFont>
#include <QBrush>
#include <QColor>

#include <cmath>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString fmtCentavos(qint64 c)
{
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("R$ %1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

static QString fmtJuros(int jurosMensal)
{
    if (jurosMensal == 0) return QStringLiteral("—");
    const double mensal = jurosMensal / 10000.0;
    // a.a. composto = (1 + i_m)^12 - 1
    const double anual = (std::pow(1.0 + mensal / 100.0, 12.0) - 1.0) * 100.0;
    return QStringLiteral("%1% a.m. (%2% a.a.)")
        .arg(mensal, 0, 'f', 2)
        .arg(anual,  0, 'f', 2);
}

static QString fmtData(const QString &iso)
{
    if (iso.isEmpty()) return QStringLiteral("—");
    const QDate d = QDate::fromString(iso, Qt::ISODate);
    return d.isValid() ? d.toString(QStringLiteral("dd/MM/yyyy")) : iso;
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

DividasWidget::DividasWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void DividasWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 8, 8, 4);
    toolbar->setSpacing(8);

    auto *btnNova = new QPushButton(tr("Nova dívida"), this);
    btnNova->setIcon(Theme::emojiIcon(QStringLiteral("➕")));
    btnNova->setObjectName(QStringLiteral("primary"));
    m_btnEdit = new QPushButton(tr("Editar"), this);
    m_btnEdit->setIcon(Theme::emojiIcon(QStringLiteral("✏️")));
    m_btnEdit->setEnabled(false);
    m_btnDel  = new QPushButton(tr("Excluir"), this);
    m_btnDel->setIcon(Theme::emojiIcon(QStringLiteral("🗑️")));
    m_btnDel->setEnabled(false);
    m_btnQuit = new QPushButton(tr("Quitar"), this);
    m_btnQuit->setIcon(Theme::emojiIcon(QStringLiteral("✅")));
    m_btnQuit->setEnabled(false);
    m_btnQuit->setToolTip(tr("Marcar como quitado"));

    // Filtros
    auto *lblFiltroTipo = new QLabel(tr("Tipo:"), this);
    m_cbFiltroTipo = new QComboBox(this);
    m_cbFiltroTipo->addItem(tr("Todos"),         QStringLiteral("todos"));
    m_cbFiltroTipo->addItem(tr("Dívidas"),        QStringLiteral("divida"));
    m_cbFiltroTipo->addItem(tr("Empréstimos"),    QStringLiteral("emprestimo"));

    auto *lblFiltroStatus = new QLabel(tr("Status:"), this);
    m_cbFiltroStatus = new QComboBox(this);
    m_cbFiltroStatus->addItem(tr("Todos"),         QStringLiteral("todos"));
    m_cbFiltroStatus->addItem(tr("Em andamento"),  QStringLiteral("em_andamento"));
    m_cbFiltroStatus->addItem(tr("Quitado"),       QStringLiteral("quitado"));
    m_cbFiltroStatus->addItem(tr("Atrasado"),      QStringLiteral("atrasado"));

    toolbar->addWidget(btnNova);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDel);
    toolbar->addWidget(m_btnQuit);
    toolbar->addStretch();
    toolbar->addWidget(lblFiltroTipo);
    toolbar->addWidget(m_cbFiltroTipo);
    toolbar->addSpacing(12);
    toolbar->addWidget(lblFiltroStatus);
    toolbar->addWidget(m_cbFiltroStatus);
    root->addLayout(toolbar);

    // ── Stack: empty (0) + table (1) ──────────────────────────────────────
    m_stack = new QStackedWidget(this);

    // Empty state guiado (§9.7 manual de estilo, #166 Peça 2).
    // CTA conectado ao handler onNova da toolbar via slot do widget.
    auto *emptyState = new EmptyStateWidget(
        QStringLiteral("💸"),
        tr("Nenhuma dívida registrada"),
        tr("Anote o que você deve — empréstimo, cartão atrasado — para acompanhar e planejar o pagamento."),
        tr("Nova dívida"),
        m_stack);
    connect(emptyState->ctaButton(), &QPushButton::clicked,
            this, &DividasWidget::onNova);
    m_stack->addWidget(emptyState); // index 0

    m_table = new QTableWidget(m_stack);
    m_table->setColumnCount(ColCount);
    m_table->setHorizontalHeaderLabels({
        tr("Tipo"), tr("Contraparte"), tr("Descrição"),
        tr("Total"), tr("Pago"), tr("Saldo"),
        tr("Juros"), tr("Vencimento"), tr("Status"), tr("Obs")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ColContraparte, QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(ColDescricao,   QHeaderView::Interactive);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setAlternatingRowColors(true);
    m_stack->addWidget(m_table); // index 1

    root->addWidget(m_stack);

    // ── Connections ────────────────────────────────────────────────────────
    connect(btnNova,    &QPushButton::clicked, this, &DividasWidget::onNova);
    connect(m_btnEdit,  &QPushButton::clicked, this, &DividasWidget::onEditar);
    connect(m_btnDel,   &QPushButton::clicked, this, &DividasWidget::onExcluir);
    connect(m_btnQuit,  &QPushButton::clicked, this, &DividasWidget::onQuitar);
    connect(m_table, &QTableWidget::itemDoubleClicked, this,
            [this](QTableWidgetItem *it) { onCellDoubleClicked(it->row(), it->column()); });
    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &DividasWidget::onSelectionChanged);
    connect(m_cbFiltroTipo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { populateTable(filteredData()); });
    connect(m_cbFiltroStatus, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { populateTable(filteredData()); });
}

// ── Data ──────────────────────────────────────────────────────────────────────

QList<Divida> DividasWidget::filteredData() const
{
    const QString fTipo   = m_cbFiltroTipo->currentData().toString();
    const QString fStatus = m_cbFiltroStatus->currentData().toString();

    QList<Divida> result;
    for (const auto &d : m_data) {
        if (fTipo   != QStringLiteral("todos") && d.tipo   != fTipo)   continue;
        if (fStatus != QStringLiteral("todos") && d.status != fStatus) continue;
        result << d;
    }
    return result;
}

void DividasWidget::populateTable(const QList<Divida> &lista)
{
    m_table->blockSignals(true);
    m_table->clearContents();

    const int dataRows = lista.size();
    m_table->setRowCount(dataRows + 1); // +1 para linha de totais

    // Cores de status
    static const QColor kAtrasado(0xFF, 0xEE, 0xEE);
    static const QColor kQuitado (0xEE, 0xFF, 0xEE);

    qint64 saldoDividas = 0, saldoEmprestimos = 0;

    for (int r = 0; r < dataRows; ++r) {
        const Divida &d = lista[r];
        m_table->setRowHeight(r, 36);

        const StatusDivida st = statusDividaDeTexto(d.status);
        const TipoDivida   tp = tipoDividaDeTexto(d.tipo);

        // Background por status
        QColor bg;
        if (st == StatusDivida::Atrasado) bg = kAtrasado;
        else if (st == StatusDivida::Quitado) bg = kQuitado;

        auto setCell = [&](int col, const QString &txt,
                           Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft) {
            auto *it = roItem(txt, align);
            it->setData(Qt::UserRole, d.id);
            if (bg.isValid()) it->setBackground(bg);
            m_table->setItem(r, col, it);
        };

        setCell(ColTipo,        tipoDividaLabel(tp));
        setCell(ColContraparte, d.contraparte);
        setCell(ColDescricao,   d.descricao);
        setCell(ColTotal,  fmtCentavos(d.valorTotal),  Qt::AlignVCenter | Qt::AlignRight);
        setCell(ColPago,   fmtCentavos(d.valorPago),   Qt::AlignVCenter | Qt::AlignRight);
        setCell(ColSaldo,  fmtCentavos(d.saldo()),     Qt::AlignVCenter | Qt::AlignRight);
        setCell(ColJuros,  fmtJuros(d.jurosMensal),    Qt::AlignVCenter | Qt::AlignRight);
        setCell(ColVenc,   fmtData(d.dataVenc));
        setCell(ColStatus, statusDividaLabel(st));
        setCell(ColObs,    d.obs);

        if (tp == TipoDivida::Divida)
            saldoDividas += d.saldo();
        else
            saldoEmprestimos += d.saldo();
    }

    // ── Linha de totais ────────────────────────────────────────────────────
    const int totalRow = dataRows;
    m_table->setRowHeight(totalRow, 36);

    auto totalItem = [&](int col, const QString &txt,
                         Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft) {
        auto *it = roItem(txt, align);
        it->setData(Qt::UserRole, -1); // id=-1 → linha total
        QFont f = it->font(); f.setBold(true); it->setFont(f);
        m_table->setItem(totalRow, col, it);
    };

    totalItem(ColTipo,        QStringLiteral("TOTAL"));
    totalItem(ColContraparte, QStringLiteral("Dívidas: ") + fmtCentavos(saldoDividas));
    totalItem(ColDescricao,   QStringLiteral("Empréstimos: ") + fmtCentavos(saldoEmprestimos));
    for (int c = ColTotal; c < ColCount; ++c)
        totalItem(c, QString());

    m_table->blockSignals(false);

    // Stack: show table if data exists globally (not filtered)
    m_stack->setCurrentIndex(m_data.isEmpty() ? 0 : 1);

    // Status bar
    emit statusAtualizado(
        tr("Dívidas: %1   Empréstimos: %2  ")
        .arg(fmtCentavos(saldoDividas))
        .arg(fmtCentavos(saldoEmprestimos)));
}

void DividasWidget::recarregar()
{
    m_data = DividaRepository().findAll();
    populateTable(filteredData());

    // Restore selection
    if (m_selectedId > 0) {
        const auto filtered = filteredData();
        for (int r = 0; r < filtered.size(); ++r) {
            if (filtered[r].id == m_selectedId) {
                m_table->selectRow(r);
                break;
            }
        }
    }
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void DividasWidget::onSelectionChanged()
{
    const auto rows = m_table->selectionModel()->selectedRows();
    bool hasReal = false;
    bool isQuitado = false;
    if (!rows.isEmpty()) {
        const int id = m_table->item(rows.first().row(), 0)
                       ->data(Qt::UserRole).toInt();
        hasReal = id > 0;
        if (hasReal) {
            m_selectedId = id;
            const auto &filtered = filteredData();
            for (const auto &d : filtered) {
                if (d.id == id) {
                    isQuitado = (d.status == QStringLiteral("quitado"));
                    break;
                }
            }
        }
    }
    m_btnEdit->setEnabled(hasReal);
    m_btnDel->setEnabled(hasReal);
    m_btnQuit->setEnabled(hasReal && !isQuitado);
}

void DividasWidget::onCellDoubleClicked(int row, int /*col*/)
{
    const auto *it = m_table->item(row, 0);
    if (!it) return;
    const int id = it->data(Qt::UserRole).toInt();
    if (id <= 0) return;

    const auto opt = DividaRepository().findById(id);
    if (!opt) return;

    auto *dlg = new DividaDialog(*opt, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, this, [this, dlg, id]() {
        Divida updated = dlg->divida();
        updated.id = id;
        const Divida original = *DividaRepository().findById(id);
        DividaRepository().update(updated);
        if (m_undoStack) {
            m_undoStack->push(new LambdaCommand(
                tr("Editar dívida"),
                [updated]()  { DividaRepository().update(updated); },
                [original]() { DividaRepository().update(original); },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));
        }
        recarregar();
        emit dadosAlterados();
    });
    dlg->open();
}

void DividasWidget::onNova()
{
    Divida empty;
    empty.status = QStringLiteral("em_andamento");
    empty.tipo   = QStringLiteral("divida");
    empty.dataInicio = QDate::currentDate().toString(Qt::ISODate);

    auto *dlg = new DividaDialog(empty, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        auto item = std::make_shared<Divida>(dlg->divida());
        DividaRepository().insert(*item);
        if (m_undoStack) {
            m_undoStack->push(new LambdaCommand(
                tr("Nova dívida"),
                [item]() {
                    item->id = 0;
                    DividaRepository().insert(*item);
                },
                [item]() { DividaRepository().remove(item->id); },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));
        }
        m_selectedId = item->id;
        recarregar();
        emit dadosAlterados();
    });
    dlg->open();
}

void DividasWidget::onEditar()
{
    if (m_selectedId <= 0) return;
    const auto opt = DividaRepository().findById(m_selectedId);
    if (!opt) return;

    auto *dlg = new DividaDialog(*opt, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        Divida updated = dlg->divida();
        updated.id = m_selectedId;
        const auto origOpt = DividaRepository().findById(m_selectedId);
        if (!origOpt) return;
        const Divida original = *origOpt;
        DividaRepository().update(updated);
        if (m_undoStack) {
            m_undoStack->push(new LambdaCommand(
                tr("Editar dívida"),
                [updated]()  { DividaRepository().update(updated); },
                [original]() { DividaRepository().update(original); },
                [this]() { recarregar(); emit dadosAlterados(); }
            ));
        }
        recarregar();
        emit dadosAlterados();
    });
    dlg->open();
}

void DividasWidget::onExcluir()
{
    if (m_selectedId <= 0) return;
    const auto opt = DividaRepository().findById(m_selectedId);
    if (!opt) return;

    const int res = QMessageBox::question(this, tr("Excluir"),
        tr("Excluir \"%1\"?").arg(opt->descricao.isEmpty()
            ? opt->contraparte : opt->descricao),
        QMessageBox::Yes | QMessageBox::No);
    if (res != QMessageBox::Yes) return;

    const Divida snap = *opt;
    DividaRepository().remove(m_selectedId);
    if (m_undoStack) {
        m_undoStack->push(new LambdaCommand(
            tr("Excluir dívida"),
            [snap]() {
                Divida d = snap;
                d.id = 0;
                DividaRepository().insert(d);
            },
            [snap]() { DividaRepository().remove(snap.id); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));
    }
    m_selectedId = 0;
    recarregar();
    emit dadosAlterados();
}

void DividasWidget::onQuitar()
{
    if (m_selectedId <= 0) return;
    const auto opt = DividaRepository().findById(m_selectedId);
    if (!opt) return;

    Divida updated = *opt;
    const Divida original = *opt;
    updated.status  = QStringLiteral("quitado");
    updated.valorPago = updated.valorTotal;
    DividaRepository().update(updated);

    if (m_undoStack) {
        m_undoStack->push(new LambdaCommand(
            tr("Quitar dívida"),
            [updated]()  { DividaRepository().update(updated); },
            [original]() { DividaRepository().update(original); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));
    }
    recarregar();
    emit dadosAlterados();
}

// ── Clipboard ─────────────────────────────────────────────────────────────────

void DividasWidget::copiar()
{
    const auto items = m_table->selectedItems();
    if (items.isEmpty()) return;
    QStringList cols;
    for (auto *it : items)
        cols << it->text();
    QApplication::clipboard()->setText(cols.join(QLatin1Char('\t')));
}

void DividasWidget::selecionarTudo()
{
    m_table->selectAll();
}
