#include "MetasWidget.h"
#include "MetaDialog.h"
#include "EmptyStateWidget.h"
#include "repositories/MetaRepository.h"
#include "core/commands/UndoCommands.h"
#include "ui/Theme.h"

#include <QUndoStack>

#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QProgressBar>
#include <QComboBox>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QInputDialog>
#include <QDate>
#include <QFont>
#include <QBrush>
#include <QColor>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString fmtCentavos(qint64 c)
{
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("R$ %1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
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

MetasWidget::MetasWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void MetasWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 8, 8, 4);
    toolbar->setSpacing(8);

    auto *btnNova = new QPushButton(tr("Nova meta"), this);
    btnNova->setIcon(Theme::emojiIcon(QStringLiteral("➕")));
    m_btnEdit     = new QPushButton(tr("Editar"), this);
    m_btnEdit->setIcon(Theme::emojiIcon(QStringLiteral("✏️")));
    m_btnDel      = new QPushButton(tr("Excluir"), this);
    m_btnDel->setIcon(Theme::emojiIcon(QStringLiteral("🗑️")));
    m_btnAporte   = new QPushButton(tr("Aporte"), this);
    m_btnAporte->setIcon(Theme::emojiIcon(QStringLiteral("💰")));
    m_btnEdit->setEnabled(false);
    m_btnDel->setEnabled(false);
    m_btnAporte->setEnabled(false);

    toolbar->addWidget(btnNova);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDel);
    toolbar->addWidget(m_btnAporte);
    toolbar->addStretch();

    // Filtro de status
    auto *lblFiltro = new QLabel(tr("Status:"), this);
    m_cbFiltro      = new QComboBox(this);
    m_cbFiltro->addItem(tr("Todos"),         QStringLiteral("todos"));
    m_cbFiltro->addItem(tr("Em andamento"),  QStringLiteral("em_andamento"));
    m_cbFiltro->addItem(tr("Concluídas"),    QStringLiteral("concluida"));
    m_cbFiltro->addItem(tr("Canceladas"),    QStringLiteral("cancelada"));
    toolbar->addWidget(lblFiltro);
    toolbar->addWidget(m_cbFiltro);

    root->addLayout(toolbar);

    // ── Stacked (empty state / table) ──────────────────────────────────────
    m_stack = new QStackedWidget(this);

    // Empty state guiado (§9.7 manual de estilo, #166 Peça 2).
    // CTA conectado ao handler onNova via signal→signal do btnNova.
    auto *emptyState = new EmptyStateWidget(
        QStringLiteral("🎯"),
        tr("Nenhuma meta ainda"),
        tr("Defina um objetivo — viagem, reserva, eletrônico — e o PeuBud mostra quanto falta pra chegar lá."),
        tr("Nova meta"),
        m_stack);
    connect(emptyState->ctaButton(), &QPushButton::clicked,
            this, &MetasWidget::onNova);
    m_stack->addWidget(emptyState);

    // Table
    m_table = new QTableWidget(0, ColCount, this);
    m_table->setHorizontalHeaderLabels({
        tr("Nome"), tr("Categoria"), tr("Meta"), tr("Acumulado"),
        tr("Progresso"), tr("Prazo"), tr("Restam"), tr("Status"), tr("Obs")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ColNome,       QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColCategoria,  QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColMeta,       QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColAtual,      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColProgresso,  QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(ColPrazo,      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColRestam,     QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColStatus,     QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColObs,        QHeaderView::Stretch);
    m_table->setColumnWidth(ColProgresso, 120);
    m_table->verticalHeader()->setDefaultSectionSize(36);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_stack->addWidget(m_table);

    root->addWidget(m_stack);

    // ── Connects ───────────────────────────────────────────────────────────
    connect(btnNova,    &QPushButton::clicked, this, &MetasWidget::onNova);
    connect(m_btnEdit,  &QPushButton::clicked, this, &MetasWidget::onEditar);
    connect(m_btnDel,   &QPushButton::clicked, this, &MetasWidget::onExcluir);
    connect(m_btnAporte,&QPushButton::clicked, this, &MetasWidget::onAporte);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &MetasWidget::onCellDoubleClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MetasWidget::onSelectionChanged);
    connect(m_cbFiltro, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MetasWidget::recarregar);
}

// ── Populate ──────────────────────────────────────────────────────────────────

void MetasWidget::populateTable(const QList<Meta> &lista)
{
    const int dataRows = lista.size();
    // +1 para linha de total
    m_table->setRowCount(dataRows + 1);

    qint64 sumMeta  = 0;
    qint64 sumAtual = 0;

    for (int r = 0; r < dataRows; ++r) {
        const Meta &m = lista[r];
        sumMeta  += m.valorMeta;
        sumAtual += m.valorAtual;

        m_table->setItem(r, ColNome,       roItem(m.nome));
        m_table->setItem(r, ColCategoria,  roItem(categoriaMetaLabel(
            categoriaMetaDeTexto(m.categoria))));
        m_table->setItem(r, ColMeta,       roItem(fmtCentavos(m.valorMeta),
                                                   Qt::AlignVCenter | Qt::AlignRight));
        m_table->setItem(r, ColAtual,      roItem(fmtCentavos(m.valorAtual),
                                                   Qt::AlignVCenter | Qt::AlignRight));

        // Progress bar embutida
        auto *bar = new QProgressBar(m_table);
        const int pct = m.progresso();
        bar->setRange(0, 100);
        bar->setValue(qMin(pct, 100));
        bar->setFormat(QStringLiteral("%1 %").arg(pct));
        bar->setTextVisible(true);
        bar->setAlignment(Qt::AlignCenter);
        m_table->setCellWidget(r, ColProgresso, bar);

        // Cor da barra por progresso
        QString barColor;
        if (m.status == QStringLiteral("concluida") || pct >= 100)
            barColor = Theme::positivo();
        else if (pct >= 50)
            barColor = Theme::acento();
        else
            barColor = Theme::atencao().name();
        bar->setStyleSheet(
            QStringLiteral("QProgressBar::chunk { background: %1; }").arg(barColor));

        // Prazo e dias restantes
        m_table->setItem(r, ColPrazo, roItem(fmtData(m.dataPrazo)));
        if (!m.dataPrazo.isEmpty()) {
            const int dias = m.diasRestantes();
            QString diasTxt;
            if (dias < 0)       diasTxt = tr("%1d atrasada").arg(-dias);
            else if (dias == 0) diasTxt = tr("Hoje");
            else                diasTxt = tr("%1d").arg(dias);
            auto *diasItem = roItem(diasTxt, Qt::AlignVCenter | Qt::AlignRight);
            if (dias < 0) diasItem->setForeground(QColor(Theme::negativo()));
            m_table->setItem(r, ColRestam, diasItem);
        } else {
            m_table->setItem(r, ColRestam, roItem(QStringLiteral("—"),
                             Qt::AlignVCenter | Qt::AlignCenter));
        }

        m_table->setItem(r, ColStatus, roItem(statusMetaLabel(
            statusMetaDeTexto(m.status))));
        m_table->setItem(r, ColObs,    roItem(m.obs));

        // Cor de fundo por status
        QColor bg;
        if (m.status == QStringLiteral("concluida"))
            bg = QColor(0xEEFFEE);
        else if (m.status == QStringLiteral("cancelada"))
            bg = QColor(0xF5F5F5);

        if (bg.isValid()) {
            for (int c = 0; c < ColCount; ++c) {
                if (auto *it = m_table->item(r, c))
                    it->setBackground(bg);
            }
        }

        // Guarda id na primeira coluna
        if (auto *it = m_table->item(r, ColNome))
            it->setData(Qt::UserRole, m.id);
    }

    // ── Linha de total ─────────────────────────────────────────────────────
    const int totalRow = dataRows;
    m_table->setItem(totalRow, ColNome, roItem(tr("TOTAL")));
    m_table->setItem(totalRow, ColMeta,  roItem(fmtCentavos(sumMeta),
                                                Qt::AlignVCenter | Qt::AlignRight));
    m_table->setItem(totalRow, ColAtual, roItem(fmtCentavos(sumAtual),
                                                Qt::AlignVCenter | Qt::AlignRight));
    const int totalPct = sumMeta > 0
        ? static_cast<int>(sumAtual * 100 / sumMeta) : 0;
    m_table->setItem(totalRow, ColProgresso,
                     roItem(QStringLiteral("%1 %").arg(totalPct),
                            Qt::AlignVCenter | Qt::AlignCenter));
    m_table->setItem(totalRow, ColStatus, roItem(QString()));

    QFont bold;
    bold.setBold(true);
    for (int c = 0; c < ColCount; ++c) {
        if (auto *it = m_table->item(totalRow, c)) {
            it->setFont(bold);
            it->setData(Qt::UserRole, -1);
        }
    }
}

// ── recarregar ────────────────────────────────────────────────────────────────

void MetasWidget::recarregar()
{
    m_data = MetaRepository().findAll();
    const QList<Meta> filtered = filteredData();

    m_stack->setCurrentIndex(filtered.isEmpty() ? 0 : 1);
    if (!filtered.isEmpty())
        populateTable(filtered);

    // Restaura seleção
    m_selectedId = 0;
    m_btnEdit->setEnabled(false);
    m_btnDel->setEnabled(false);
    m_btnAporte->setEnabled(false);

    const int total  = filtered.size();
    const int em_and = std::count_if(filtered.begin(), filtered.end(),
        [](const Meta &m){ return m.status == QStringLiteral("em_andamento"); });
    emit statusAtualizado(tr("%1 meta(s)  •  %2 em andamento  ").arg(total).arg(em_and));
}

QList<Meta> MetasWidget::filteredData() const
{
    const QString filtro = m_cbFiltro->currentData().toString();
    if (filtro == QStringLiteral("todos"))
        return m_data;
    QList<Meta> result;
    for (const Meta &m : m_data)
        if (m.status == filtro)
            result << m;
    return result;
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void MetasWidget::onSelectionChanged()
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        m_selectedId = 0;
        m_btnEdit->setEnabled(false);
        m_btnDel->setEnabled(false);
        m_btnAporte->setEnabled(false);
        return;
    }
    const int id = m_table->item(rows.first().row(), ColNome)
                   ->data(Qt::UserRole).toInt();
    const bool valid = id > 0;
    m_selectedId = valid ? id : 0;
    m_btnEdit->setEnabled(valid);
    m_btnDel->setEnabled(valid);
    m_btnAporte->setEnabled(valid);
}

void MetasWidget::onCellDoubleClicked(int row, int /*col*/)
{
    if (!m_table->item(row, ColNome)) return;
    const int id = m_table->item(row, ColNome)->data(Qt::UserRole).toInt();
    if (id <= 0) return;
    m_selectedId = id;
    onEditar();
}

void MetasWidget::onNova()
{
    Meta blank;
    blank.dataInicio = QDate::currentDate().toString(Qt::ISODate);
    blank.status     = QStringLiteral("em_andamento");

    auto *dlg = new MetaDialog(blank, this);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        Meta m = dlg->meta();
        if (m.nome.isEmpty()) return;

        const Meta saved = m;
        auto undo = [this, saved]() {
            MetaRepository().remove(saved.id);
            emit dadosAlterados();
        };
        auto redo = [this, m]() mutable {
            MetaRepository().insert(m);
            emit dadosAlterados();
        };
        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(tr("Nova meta"), redo, undo));
        else
            redo();
    });
    dlg->open();
}

void MetasWidget::onEditar()
{
    if (m_selectedId <= 0) return;
    auto opt = MetaRepository().findById(m_selectedId);
    if (!opt) return;

    auto *dlg = new MetaDialog(*opt, this);
    connect(dlg, &QDialog::accepted, this, [this, dlg, before = *opt]() {
        Meta m = dlg->meta();
        if (m.nome.isEmpty()) return;

        auto undo = [this, before]() {
            MetaRepository().update(before);
            emit dadosAlterados();
        };
        auto redo = [this, m]() {
            MetaRepository().update(m);
            emit dadosAlterados();
        };
        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(tr("Editar meta"), redo, undo));
        else
            redo();
    });
    dlg->open();
}

void MetasWidget::onExcluir()
{
    if (m_selectedId <= 0) return;
    auto opt = MetaRepository().findById(m_selectedId);
    if (!opt) return;

    if (QMessageBox::question(this, tr("Excluir meta"),
            tr("Excluir a meta \"%1\"?").arg(opt->nome),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    const Meta backup = *opt;
    auto undo = [this, backup]() mutable {
        MetaRepository().insert(const_cast<Meta&>(backup));
        emit dadosAlterados();
    };
    auto redo = [this, id = backup.id]() {
        MetaRepository().remove(id);
        emit dadosAlterados();
    };
    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(tr("Excluir meta"), redo, undo));
    else
        redo();
}

void MetasWidget::onAporte()
{
    if (m_selectedId <= 0) return;
    auto opt = MetaRepository().findById(m_selectedId);
    if (!opt) return;

    bool ok = false;
    const QString txt = QInputDialog::getText(
        this, tr("Registrar aporte"),
        tr("Valor do aporte para \"%1\" (R$):").arg(opt->nome),
        QLineEdit::Normal, QString(), &ok);
    if (!ok || txt.trimmed().isEmpty()) return;

    // Parseia valor
    QString s = txt.trimmed();
    s.remove(QStringLiteral("R$")).remove(QLatin1Char(' '));
    if (s.contains(QLatin1Char(',')) && s.contains(QLatin1Char('.')))
        s.remove(QLatin1Char('.'));
    s.replace(QLatin1Char(','), QLatin1Char('.'));
    const double val = s.toDouble(&ok);
    if (!ok || val <= 0) return;

    const qint64 aporte = static_cast<qint64>(qRound(val * 100.0));
    const qint64 anterior = opt->valorAtual;

    Meta updated = *opt;
    updated.valorAtual += aporte;
    // Auto-concluir se atingiu a meta
    if (updated.valorAtual >= updated.valorMeta
        && updated.status == QStringLiteral("em_andamento"))
    {
        updated.status = QStringLiteral("concluida");
    }

    auto undo = [this, before = *opt]() {
        MetaRepository().update(before);
        emit dadosAlterados();
    };
    auto redo = [this, updated]() {
        MetaRepository().update(updated);
        emit dadosAlterados();
    };
    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Aporte em meta"), redo, undo));
    else
        redo();

    Q_UNUSED(anterior)
}

void MetasWidget::copiar()
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) { selecionarTudo(); }

    QStringList lines;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QStringList cols;
        for (int c = 0; c < ColCount; ++c) {
            if (auto *it = m_table->item(r, c))
                cols << it->text();
            else
                cols << QString();
        }
        lines << cols.join(QLatin1Char('\t'));
    }
    QApplication::clipboard()->setText(lines.join(QLatin1Char('\n')));
}

void MetasWidget::selecionarTudo()
{
    m_table->selectAll();
}
