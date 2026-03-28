#include "RecorrenciaWidget.h"
#include "RecorrenciaDialog.h"
#include "repositories/CategoriaRepository.h"
#include "repositories/TransacaoRecorrenteRepository.h"
#include "services/EntradaService.h"
#include "services/RecorrenciaService.h"
#include "core/commands/UndoCommands.h"

#include <QUndoStack>
#include <memory>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QDate>
#include <QFont>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString formatCentavos(qint64 c)
{
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("R$ %1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

static QString freqLabel(const QString &f)
{
    if (f == QStringLiteral("semanal"))   return QStringLiteral("Semanal");
    if (f == QStringLiteral("quinzenal")) return QStringLiteral("Quinzenal");
    if (f == QStringLiteral("anual"))     return QStringLiteral("Anual");
    return QStringLiteral("Mensal");
}

static QTableWidgetItem *roItem(const QString &text)
{
    auto *it = new QTableWidgetItem(text);
    it->setFlags(it->flags() & ~Qt::ItemIsEditable);
    return it;
}

// ── Construction ──────────────────────────────────────────────────────────────

RecorrenciaWidget::RecorrenciaWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void RecorrenciaWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(8, 8, 8, 4);
    toolbar->setSpacing(8);

    auto *btnAdd = new QPushButton(tr("+ Adicionar"), this);
    btnAdd->setObjectName(QStringLiteral("primary"));
    m_btnEdit   = new QPushButton(tr("Editar"), this);
    m_btnRem    = new QPushButton(tr("Remover"), this);
    m_btnToggle = new QPushButton(tr("Ativar/Pausar"), this);

    m_btnEdit->setEnabled(false);
    m_btnRem->setEnabled(false);
    m_btnToggle->setEnabled(false);

    toolbar->addWidget(btnAdd);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnRem);
    toolbar->addWidget(m_btnToggle);
    toolbar->addStretch();
    root->addLayout(toolbar);

    // ── Hint label ─────────────────────────────────────────────────────────
    auto *hint = new QLabel(
        tr("Lançamentos são gerados automaticamente ao abrir o app."), this);
    hint->setObjectName(QStringLiteral("subtitle"));
    hint->setContentsMargins(8, 0, 8, 4);
    root->addWidget(hint);

    // ── Table ──────────────────────────────────────────────────────────────
    m_table = new QTableWidget(0, 6, this);
    m_table->setHorizontalHeaderLabels({
        tr("Descrição"), tr("Tipo"), tr("Valor"),
        tr("Frequência"), tr("Próxima"), tr("Ativo")
    });
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    root->addWidget(m_table);

    // ── Connections ────────────────────────────────────────────────────────
    connect(btnAdd,      &QPushButton::clicked, this, &RecorrenciaWidget::onAdicionar);
    connect(m_btnEdit,   &QPushButton::clicked, this, &RecorrenciaWidget::onEditar);
    connect(m_btnRem,    &QPushButton::clicked, this, &RecorrenciaWidget::onRemover);
    connect(m_btnToggle, &QPushButton::clicked, this, &RecorrenciaWidget::onToggleAtivo);

    connect(m_table, &QTableWidget::itemDoubleClicked,
            this, &RecorrenciaWidget::onEditar);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                const bool sel = m_table->currentRow() >= 0;
                m_btnEdit->setEnabled(sel);
                m_btnRem->setEnabled(sel);
                m_btnToggle->setEnabled(sel);
            });
}

// ── Data ──────────────────────────────────────────────────────────────────────

void RecorrenciaWidget::recarregar()
{
    m_itens = TransacaoRecorrenteRepository().findAll();
    popularTabela();
}

void RecorrenciaWidget::popularTabela()
{
    m_table->setRowCount(0);
    for (const auto &t : m_itens) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setRowHeight(row, 28);

        // Col 0: Descrição — store id in UserRole
        auto *itDesc = roItem(t.descricao);
        itDesc->setData(Qt::UserRole, t.id);
        if (!t.ativo) {
            QFont f = itDesc->font();
            f.setItalic(true);
            itDesc->setFont(f);
            itDesc->setForeground(QColor(Qt::gray));
        }
        m_table->setItem(row, 0, itDesc);

        // Col 1: Tipo
        m_table->setItem(row, 1, roItem(t.tipo == QStringLiteral("entrada")
                                        ? tr("Entrada") : tr("Gasto")));

        // Col 2: Valor
        auto *itValor = roItem(formatCentavos(t.valor));
        itValor->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 2, itValor);

        // Col 3: Frequência
        m_table->setItem(row, 3, roItem(freqLabel(t.frequencia)));

        // Col 4: Próxima
        QDate proxima;
        if (t.ativo) {
            proxima = t.ultimoLancamento.isValid()
                      ? RecorrenciaService::proximaApos(t.ultimoLancamento, t.frequencia)
                      : t.dataInicio;
            if (t.dataFim.isValid() && proxima > t.dataFim)
                proxima = QDate();
        }
        m_table->setItem(row, 4, roItem(proxima.isValid()
                                        ? proxima.toString(QStringLiteral("dd/MM/yyyy"))
                                        : tr("—")));

        // Col 5: Ativo
        m_table->setItem(row, 5, roItem(t.ativo ? tr("✓") : tr("✗")));
    }

    m_table->resizeColumnsToContents();
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
}

int RecorrenciaWidget::idDaLinhaSelecionada() const
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_table->rowCount()) return -1;
    const auto *it = m_table->item(row, 0);
    return it ? it->data(Qt::UserRole).toInt() : -1;
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void RecorrenciaWidget::onAdicionar()
{
    TransacaoRecorrente vazia;
    vazia.tipo       = QStringLiteral("gasto");
    vazia.frequencia = QStringLiteral("mensal");
    vazia.dataInicio = QDate::currentDate();
    vazia.ativo      = true;

    EntradaService svc;
    RecorrenciaDialog dlg(vazia, svc.fontes(),
                          CategoriaRepository().findByTipo(QStringLiteral("gasto")), this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto tPtr = std::make_shared<TransacaoRecorrente>(dlg.resultado());
    TransacaoRecorrenteRepository().insert(*tPtr);
    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Adicionar recorrência"),
            [tPtr]() { tPtr->id = 0; TransacaoRecorrenteRepository().insert(*tPtr); },
            [tPtr]() { TransacaoRecorrenteRepository().remove(tPtr->id); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));
    recarregar();
    emit dadosAlterados();
}

void RecorrenciaWidget::onEditar()
{
    const int id = idDaLinhaSelecionada();
    if (id < 0) return;

    auto opt = TransacaoRecorrenteRepository().findById(id);
    if (!opt) return;

    EntradaService svc;
    RecorrenciaDialog dlg(*opt, svc.fontes(),
                          CategoriaRepository().findByTipo(QStringLiteral("gasto")), this);
    if (dlg.exec() != QDialog::Accepted) return;

    auto beforePtr = std::make_shared<TransacaoRecorrente>(*opt);
    auto afterPtr  = std::make_shared<TransacaoRecorrente>(dlg.resultado());
    TransacaoRecorrenteRepository().update(*afterPtr);
    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Editar recorrência"),
            [afterPtr]()  { TransacaoRecorrenteRepository().update(*afterPtr); },
            [beforePtr]() { TransacaoRecorrenteRepository().update(*beforePtr); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));
    recarregar();
    emit dadosAlterados();
}

void RecorrenciaWidget::onRemover()
{
    const int id = idDaLinhaSelecionada();
    if (id < 0) return;

    const int row = m_table->currentRow();
    const QString desc = m_table->item(row, 0)->text();

    if (QMessageBox::question(this,
            tr("Remover recorrência"),
            tr("Remover \"%1\"?\n"
               "Os lançamentos já gerados não serão afetados.").arg(desc))
        != QMessageBox::Yes) return;

    auto opt = TransacaoRecorrenteRepository().findById(id);
    if (!opt) return;
    auto toRemove = std::make_shared<TransacaoRecorrente>(*opt);
    TransacaoRecorrenteRepository().remove(id);
    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            tr("Remover recorrência"),
            [toRemove]() { TransacaoRecorrenteRepository().remove(toRemove->id); },
            [toRemove]() { toRemove->id = 0; TransacaoRecorrenteRepository().insert(*toRemove); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));
    recarregar();
    emit dadosAlterados();
}

void RecorrenciaWidget::onToggleAtivo()
{
    const int id = idDaLinhaSelecionada();
    if (id < 0) return;

    auto opt = TransacaoRecorrenteRepository().findById(id);
    if (!opt) return;

    auto beforePtr = std::make_shared<TransacaoRecorrente>(*opt);
    TransacaoRecorrente t = *opt;
    t.ativo = !t.ativo;
    auto afterPtr = std::make_shared<TransacaoRecorrente>(t);
    TransacaoRecorrenteRepository().update(t);
    if (m_undoStack)
        m_undoStack->push(new LambdaCommand(
            t.ativo ? tr("Ativar recorrência") : tr("Desativar recorrência"),
            [afterPtr]()  { TransacaoRecorrenteRepository().update(*afterPtr); },
            [beforePtr]() { TransacaoRecorrenteRepository().update(*beforePtr); },
            [this]() { recarregar(); emit dadosAlterados(); }
        ));
    recarregar();
    emit dadosAlterados();
}
