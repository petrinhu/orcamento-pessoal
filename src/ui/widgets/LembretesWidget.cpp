#include "LembretesWidget.h"
#include "EmptyStateWidget.h"
#include "LembreteDialog.h"
#include "repositories/LembreteRepository.h"
#include "core/commands/UndoCommands.h"
#include "ui/Theme.h"
#include "ui/UiFormatters.h"

#include <QUndoStack>

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

// ── Helpers ───────────────────────────────────────────────────────────────────

// #197 (I18N-01/05): delegava para "R$ "+vírgula e "dd/MM/yyyy" fixos.
static QString fmtCentavos(qint64 c)
{
    if (c == 0) return QStringLiteral("—");
    return uifmt::formatCentavosBRL(c);
}

static QString fmtData(const QString &iso)
{
    if (iso.isEmpty()) return QStringLiteral("—");
    const QDate d = QDate::fromString(iso, Qt::ISODate);
    return d.isValid() ? d.toString(uifmt::dataDisplayFormat()) : iso;
}

// Label dinâmico de prazo para lembrete NÃO resolvido.
//   dias < 0  → "Vencido há N dias"
//   dias == 0 → "Vence hoje"
//   1..3      → "Vence em N dia(s)"
//   > 3       → status base (caller decide)
static QString labelPrazo(int dias)
{
    if (dias < 0) {
        const int n = -dias;
        return n == 1 ? QStringLiteral("Vencido há 1 dia")
                      : QStringLiteral("Vencido há %1 dias").arg(n);
    }
    if (dias == 0) return QStringLiteral("Vence hoje");
    if (dias == 1) return QStringLiteral("Vence em 1 dia");
    return QStringLiteral("Vence em %1 dias").arg(dias);
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

LembretesWidget::LembretesWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void LembretesWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 8, 8, 4);
    toolbar->setSpacing(8);

    auto *btnNovo  = new QPushButton(tr("Novo"), this);
    btnNovo->setIcon(Theme::emojiIcon(QStringLiteral("➕")));
    m_btnEdit      = new QPushButton(tr("Editar"), this);
    m_btnEdit->setIcon(Theme::emojiIcon(QStringLiteral("✏️")));
    m_btnDel       = new QPushButton(tr("Excluir"), this);
    m_btnDel->setIcon(Theme::emojiIcon(QStringLiteral("🗑️")));
    m_btnPagar     = new QPushButton(tr("Pagar"), this);
    m_btnPagar->setIcon(Theme::emojiIcon(QStringLiteral("✅")));
    m_btnNaoDebito = new QPushButton(tr("Não debitado"), this);
    m_btnNaoDebito->setIcon(Theme::emojiIcon(QStringLiteral("⚠️")));
    m_btnIgnorar   = new QPushButton(tr("Ignorar"), this);
    m_btnIgnorar->setIcon(Theme::emojiIcon(QStringLiteral("🚫")));

    m_btnEdit->setEnabled(false);
    m_btnDel->setEnabled(false);
    m_btnPagar->setEnabled(false);
    m_btnNaoDebito->setEnabled(false);
    m_btnIgnorar->setEnabled(false);

    toolbar->addWidget(btnNovo);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnDel);
    toolbar->addWidget(m_btnPagar);
    toolbar->addWidget(m_btnNaoDebito);
    toolbar->addWidget(m_btnIgnorar);
    toolbar->addStretch();

    // Filtro
    auto *lblFiltro = new QLabel(tr("Mostrar:"), this);
    m_cbFiltro      = new QComboBox(this);
    m_cbFiltro->addItem(tr("Todos"),              QStringLiteral("todos"));
    m_cbFiltro->addItem(tr("Pendentes"),          QStringLiteral("pendente"));
    m_cbFiltro->addItem(tr("Não debitados"),      QStringLiteral("nao_pago"));
    m_cbFiltro->addItem(tr("Pagos / Ignorados"),  QStringLiteral("resolvido"));
    toolbar->addWidget(lblFiltro);
    toolbar->addWidget(m_cbFiltro);

    root->addLayout(toolbar);

    // ── Stacked ────────────────────────────────────────────────────────────
    m_stack = new QStackedWidget(this);

    // Empty state guiado (§9.7 manual de estilo, #166 Peça 2).
    // CTA conectado ao handler onNovo da toolbar via slot do widget.
    auto *emptyState = new EmptyStateWidget(
        QStringLiteral("🔔"),
        tr("Nenhum lembrete ainda"),
        tr("Crie alertas para não esquecer contas, vencimentos e qualquer coisa que precisa de atenção."),
        tr("Novo"),
        m_stack);
    connect(emptyState->ctaButton(), &QPushButton::clicked,
            this, &LembretesWidget::onNovo);
    m_stack->addWidget(emptyState);

    m_table = new QTableWidget(0, ColCount, this);
    m_table->setHorizontalHeaderLabels({
        tr("Descrição"), tr("Valor"), tr("Vencimento"),
        tr("Frequência"), tr("Tipo"), tr("Status"), tr("Obs")
    });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(ColDescricao, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(ColValor,     QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColVenc,      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColFreq,      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColTipo,      QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColStatus,    QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(ColObs,       QHeaderView::Stretch);
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
    connect(btnNovo,        &QPushButton::clicked, this, &LembretesWidget::onNovo);
    connect(m_btnEdit,      &QPushButton::clicked, this, &LembretesWidget::onEditar);
    connect(m_btnDel,       &QPushButton::clicked, this, &LembretesWidget::onExcluir);
    connect(m_btnPagar,     &QPushButton::clicked, this, &LembretesWidget::onPagar);
    connect(m_btnNaoDebito, &QPushButton::clicked, this, &LembretesWidget::onNaoDebitado);
    connect(m_btnIgnorar,   &QPushButton::clicked, this, &LembretesWidget::onIgnorar);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &LembretesWidget::onCellDoubleClicked);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &LembretesWidget::onSelectionChanged);
    connect(m_cbFiltro, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LembretesWidget::recarregar);
}

// ── Populate ──────────────────────────────────────────────────────────────────

void LembretesWidget::populateTable(const QList<Lembrete> &lista)
{
    m_table->setRowCount(lista.size());

    const QColor cNegativo = QColor(Theme::negativo());
    const QColor cPositivo = QColor(Theme::positivo());
    const QColor cAmarelo  = Theme::atencao();

    for (int r = 0; r < lista.size(); ++r) {
        const Lembrete &l = lista[r];
        const int  dias      = l.diasParaVencer();
        const bool resolvido = l.pago() || l.ignorado();

        // ── Estado / aparência ────────────────────────────────────────────
        // Cores: negativo (atrasado / não debitado), branco (vence hoje),
        // amarelo (1..3 dias), positivo (resolvido), sem destaque (futuro).
        QColor bg, fg;
        QString statusTexto = statusLembreteLabel(statusLembreteDeTexto(l.status));

        if (resolvido) {
            bg = cPositivo;
            fg = QColor(Qt::white);
        } else if (l.naoPago() || dias < 0) {
            bg = cNegativo;
            fg = QColor(Qt::white);
            statusTexto = l.naoPago() ? statusTexto : labelPrazo(dias);
        } else if (dias == 0) {
            // Vence hoje → branco, sem fundo forte; texto realçado.
            statusTexto = labelPrazo(dias);
        } else if (dias >= 1 && dias <= 3) {
            bg = cAmarelo;
            statusTexto = labelPrazo(dias);
        }
        // dias > 3 (futuro): sem destaque, status base preservado.

        m_table->setItem(r, ColDescricao, roItem(l.descricao));
        m_table->setItem(r, ColValor,     roItem(fmtCentavos(l.valor),
                                                 Qt::AlignVCenter | Qt::AlignRight));
        m_table->setItem(r, ColVenc,      roItem(fmtData(l.dataVenc)));
        m_table->setItem(r, ColFreq,      roItem(frequenciaLembreteLabel(
            frequenciaLembreteDeTexto(l.frequencia))));
        m_table->setItem(r, ColTipo,      roItem(l.debitoAutomatico
            ? tr("Déb. Auto") : tr("Manual")));
        m_table->setItem(r, ColStatus,    roItem(statusTexto));
        m_table->setItem(r, ColObs,       roItem(l.obs));

        // Guarda id
        if (auto *it = m_table->item(r, ColDescricao))
            it->setData(Qt::UserRole, l.id);

        if (bg.isValid() || fg.isValid()) {
            for (int c = 0; c < ColCount; ++c) {
                if (auto *it = m_table->item(r, c)) {
                    if (bg.isValid()) it->setBackground(bg);
                    if (fg.isValid()) it->setForeground(fg);
                }
            }
        }
    }
}

// ── recarregar ────────────────────────────────────────────────────────────────

void LembretesWidget::recarregar()
{
    m_data = LembreteRepository().findAll();
    const QList<Lembrete> filtered = filteredData();

    m_stack->setCurrentIndex(filtered.isEmpty() ? 0 : 1);
    if (!filtered.isEmpty())
        populateTable(filtered);

    m_selectedId = 0;
    m_btnEdit->setEnabled(false);
    m_btnDel->setEnabled(false);
    m_btnPagar->setEnabled(false);
    m_btnNaoDebito->setEnabled(false);
    m_btnIgnorar->setEnabled(false);

    int pendentes = 0, urgentes = 0;
    for (const Lembrete &l : m_data) {
        if (l.pendente()) ++pendentes;
        if (l.vencido() || l.naoPago()) ++urgentes;
    }
    emit statusAtualizado(
        tr("%1 lembrete(s)  •  %2 pendente(s)  •  %3 urgente(s)  ")
        .arg(m_data.size()).arg(pendentes).arg(urgentes));
}

QList<Lembrete> LembretesWidget::filteredData() const
{
    const QString filtro = m_cbFiltro->currentData().toString();
    if (filtro == QStringLiteral("todos"))
        return m_data;
    QList<Lembrete> result;
    for (const Lembrete &l : m_data) {
        if (filtro == QStringLiteral("resolvido")) {
            if (l.pago() || l.ignorado()) result << l;
        } else if (l.status == filtro) {
            result << l;
        }
    }
    return result;
}

// ── contarUrgentes ─────────────────────────────────────────────────────────────

int LembretesWidget::contarUrgentes()
{
    const auto lista = LembreteRepository().findAtivos();
    int count = 0;
    for (const Lembrete &l : lista)
        if (l.vencido() || l.naoPago())
            ++count;
    return count;
}

// ── marcarStatus ──────────────────────────────────────────────────────────────

void LembretesWidget::marcarStatus(int id, const QString &novoStatus)
{
    auto opt = LembreteRepository().findById(id);
    if (!opt) return;

    const Lembrete before = *opt;
    Lembrete updated = *opt;
    updated.status = novoStatus;

    // Gera próxima ocorrência para frequências recorrentes ao pagar
    auto gerarProxima = [](const Lembrete &l) {
        const auto freq = frequenciaLembreteDeTexto(l.frequencia);
        if (freq == FrequenciaLembrete::Unico) return;

        QDate proxData = QDate::fromString(l.dataVenc, Qt::ISODate);
        if (!proxData.isValid()) return;

        if (freq == FrequenciaLembrete::Mensal)
            proxData = proxData.addMonths(1);
        else // Anual
            proxData = proxData.addYears(1);

        Lembrete proxima = l;
        proxima.id     = 0;
        proxima.status = QStringLiteral("pendente");
        proxima.dataVenc = proxData.toString(Qt::ISODate);
        LembreteRepository().insert(proxima);
    };

    auto undo = [this, before]() {
        LembreteRepository().update(before);
        emit dadosAlterados();
    };
    auto redo = [this, updated, novoStatus, gerarProxima]() mutable {
        LembreteRepository().update(updated);
        // Próxima ocorrência só ao pagar (não ao marcar nao_pago/ignorado)
        if (novoStatus == QStringLiteral("pago"))
            gerarProxima(updated);
        emit dadosAlterados();
    };

    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(tr("Alterar status de lembrete"), redo, undo));
    else
        redo();
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void LembretesWidget::onSelectionChanged()
{
    const auto rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        m_selectedId = 0;
        m_btnEdit->setEnabled(false);
        m_btnDel->setEnabled(false);
        m_btnPagar->setEnabled(false);
        m_btnNaoDebito->setEnabled(false);
        m_btnIgnorar->setEnabled(false);
        return;
    }
    const int id = m_table->item(rows.first().row(), ColDescricao)
                   ->data(Qt::UserRole).toInt();
    if (id <= 0) {
        m_selectedId = 0;
        m_btnEdit->setEnabled(false);
        m_btnDel->setEnabled(false);
        m_btnPagar->setEnabled(false);
        m_btnNaoDebito->setEnabled(false);
        m_btnIgnorar->setEnabled(false);
        return;
    }
    m_selectedId = id;

    // Busca o lembrete atual para determinar quais botões habilitar
    const auto filtered = filteredData();
    const Lembrete *found = nullptr;
    for (const Lembrete &l : filtered)
        if (l.id == id) { found = &l; break; }

    m_btnEdit->setEnabled(true);
    m_btnDel->setEnabled(true);

    if (found) {
        const bool ativo   = found->pendente() || found->naoPago();
        const bool ehDebAuto = found->debitoAutomatico;
        // Pagar: disponível para pendentes (manuais e debitoAuto) e nao_pago
        m_btnPagar->setEnabled(ativo);
        // Não debitado: só para débito automático que está pendente ou pago
        m_btnNaoDebito->setEnabled(ehDebAuto && !found->naoPago());
        // Ignorar: disponível para pendentes
        m_btnIgnorar->setEnabled(found->pendente() || found->naoPago());
    } else {
        m_btnPagar->setEnabled(false);
        m_btnNaoDebito->setEnabled(false);
        m_btnIgnorar->setEnabled(false);
    }
}

void LembretesWidget::onCellDoubleClicked(int row, int /*col*/)
{
    if (!m_table->item(row, ColDescricao)) return;
    const int id = m_table->item(row, ColDescricao)->data(Qt::UserRole).toInt();
    if (id <= 0) return;
    m_selectedId = id;
    onEditar();
}

void LembretesWidget::onNovo()
{
    Lembrete blank;
    blank.dataVenc   = QDate::currentDate().toString(Qt::ISODate);
    blank.frequencia = QStringLiteral("unico");
    blank.status     = QStringLiteral("pendente");

    auto *dlg = new LembreteDialog(blank, this);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        Lembrete l = dlg->lembrete();
        if (l.descricao.isEmpty()) return;

        const Lembrete saved = l;
        auto undo = [this, saved]() {
            LembreteRepository().remove(saved.id);
            emit dadosAlterados();
        };
        auto redo = [this, l]() mutable {
            LembreteRepository().insert(l);
            emit dadosAlterados();
        };
        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(tr("Novo lembrete"), redo, undo));
        else
            redo();
    });
    dlg->open();
}

void LembretesWidget::onEditar()
{
    if (m_selectedId <= 0) return;
    auto opt = LembreteRepository().findById(m_selectedId);
    if (!opt) return;

    auto *dlg = new LembreteDialog(*opt, this);
    connect(dlg, &QDialog::accepted, this, [this, dlg, before = *opt]() {
        Lembrete l = dlg->lembrete();
        if (l.descricao.isEmpty()) return;

        auto undo = [this, before]() {
            LembreteRepository().update(before);
            emit dadosAlterados();
        };
        auto redo = [this, l]() {
            LembreteRepository().update(l);
            emit dadosAlterados();
        };
        if (m_undoStack)
            m_undoStack->push(new LambdaCommand(tr("Editar lembrete"), redo, undo));
        else
            redo();
    });
    dlg->open();
}

void LembretesWidget::onExcluir()
{
    if (m_selectedId <= 0) return;
    auto opt = LembreteRepository().findById(m_selectedId);
    if (!opt) return;

    if (QMessageBox::question(this, tr("Excluir lembrete"),
            tr("Excluir o lembrete \"%1\"?").arg(opt->descricao),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    const Lembrete backup = *opt;
    auto undo = [this, backup]() mutable {
        LembreteRepository().insert(const_cast<Lembrete&>(backup));
        emit dadosAlterados();
    };
    auto redo = [this, id = backup.id]() {
        LembreteRepository().remove(id);
        emit dadosAlterados();
    };
    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(tr("Excluir lembrete"), redo, undo));
    else
        redo();
}

void LembretesWidget::onPagar()
{
    if (m_selectedId > 0)
        marcarStatus(m_selectedId, QStringLiteral("pago"));
}

void LembretesWidget::onNaoDebitado()
{
    if (m_selectedId > 0)
        marcarStatus(m_selectedId, QStringLiteral("nao_pago"));
}

void LembretesWidget::onIgnorar()
{
    if (m_selectedId > 0)
        marcarStatus(m_selectedId, QStringLiteral("ignorado"));
}

void LembretesWidget::copiar()
{
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

void LembretesWidget::selecionarTudo()
{
    m_table->selectAll();
}
