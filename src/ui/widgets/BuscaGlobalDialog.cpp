#include "BuscaGlobalDialog.h"
#include "services/BuscaService.h"
#include "ui/UiFormatters.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QDate>
#include <QBrush>
#include <QColor>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QString fmtCentavos(qint64 c)
{
    const bool neg = c < 0;
    const qint64 abs_c = neg ? -c : c;
    return QStringLiteral("%1R$ %2,%3")
        .arg(neg ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
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

BuscaGlobalDialog::BuscaGlobalDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Busca global de transações"));
    setMinimumSize(820, 480);
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);
    buildUi();
}

void BuscaGlobalDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(8);

    // ── Barra de busca ─────────────────────────────────────────────────────
    auto *searchRow = new QHBoxLayout;
    m_edtBusca = new QLineEdit(this);
    m_edtBusca->setPlaceholderText(tr("Descrição, categoria, obs…"));
    m_edtBusca->setClearButtonEnabled(true);

    m_cbPeriodo = new QComboBox(this);
    m_cbPeriodo->addItem(tr("Este mês"),           QStringLiteral("mes"));
    m_cbPeriodo->addItem(tr("Últimos 3 meses"),    QStringLiteral("3meses"));
    m_cbPeriodo->addItem(tr("Últimos 6 meses"),    QStringLiteral("6meses"));
    m_cbPeriodo->addItem(tr("Último ano"),         QStringLiteral("ano"));
    m_cbPeriodo->addItem(tr("Todo o período"),     QStringLiteral("tudo"));

    m_cbTipo = new QComboBox(this);
    m_cbTipo->addItem(tr("Todos os tipos"),    QStringLiteral("todos"));
    m_cbTipo->addItem(tr("Entradas"),          QStringLiteral("entradas"));
    m_cbTipo->addItem(tr("Gastos"),            QStringLiteral("gastos"));
    m_cbTipo->addItem(tr("Investimentos"),     QStringLiteral("investimentos"));
    m_cbTipo->addItem(tr("Dívidas"),           QStringLiteral("dividas"));
    m_cbTipo->addItem(tr("Cartões"),           QStringLiteral("cartoes"));

    m_btnBuscar = new QPushButton(tr("Buscar"), this);
    m_btnBuscar->setDefault(true);

    searchRow->addWidget(m_edtBusca, 3);
    searchRow->addWidget(m_cbPeriodo);
    searchRow->addWidget(m_cbTipo);
    searchRow->addWidget(m_btnBuscar);
    root->addLayout(searchRow);

    // ── Tabela de resultados ───────────────────────────────────────────────
    m_table = new QTableWidget(0, 5, this);
    m_table->setHorizontalHeaderLabels({
        tr("Tipo"), tr("Data"), tr("Descrição"), tr("Valor"), tr("Categoria / Obs")
    });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_table->verticalHeader()->setDefaultSectionSize(32);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    root->addWidget(m_table);

    // Hint
    auto *lblHint = new QLabel(
        tr("Duplo-clique em um resultado para navegar à aba correspondente."), this);
    lblHint->setObjectName(QStringLiteral("subtitle"));
    root->addWidget(lblHint);

    // Botão fechar
    // Margem interna garante folga para a sombra do botão (#176).
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    btns->setContentsMargins(0, 6, 10, 10);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::close);
    root->addWidget(btns);

    // ── Connects ───────────────────────────────────────────────────────────
    connect(m_btnBuscar, &QPushButton::clicked, this, &BuscaGlobalDialog::onBuscar);
    connect(m_edtBusca, &QLineEdit::returnPressed, this, &BuscaGlobalDialog::onBuscar);
    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &BuscaGlobalDialog::onCellDoubleClicked);
}

// ── Busca ─────────────────────────────────────────────────────────────────────

void BuscaGlobalDialog::onBuscar()
{
    const QString termo   = m_edtBusca->text().trimmed();
    const QString periodo = m_cbPeriodo->currentData().toString();
    const QString tipo    = m_cbTipo->currentData().toString();
    popularResultados(termo, periodo, tipo);
}

void BuscaGlobalDialog::popularResultados(const QString &termo,
                                          const QString &periodo,
                                          const QString &tipo)
{
    m_table->setRowCount(0);
    m_tabIndices.clear();

    const QList<BuscaService::Resultado> resultados =
        BuscaService::buscar(termo, periodo, tipo);

    m_table->setRowCount(resultados.size());
    m_tabIndices.resize(resultados.size());

    for (int i = 0; i < resultados.size(); ++i) {
        const auto &r = resultados[i];
        m_tabIndices[i] = r.tabIndex;

        // Rótulo traduzido + cor de destaque: presentation concern, fica na
        // UI (o Service devolve só a categoria estável não-traduzida).
        QString label;
        QColor  cor;
        if (r.categoria == QLatin1String(BuscaService::kCategoriaEntrada)) {
            label = tr("Entrada");
            cor   = QColor(0xE8F5ED);
        } else if (r.categoria == QLatin1String(BuscaService::kCategoriaGasto)) {
            label = tr("Gasto");
            cor   = QColor(0xFFF3E8);
        } else if (r.categoria == QLatin1String(BuscaService::kCategoriaInvestimento)) {
            label = tr("Investimento");
            cor   = QColor(0xE8EEFF);
        } else if (r.categoria == QLatin1String(BuscaService::kCategoriaDivida)) {
            label = tr("Dívida");
            cor   = QColor(0xFFEEFF);
        } else if (r.categoria == QLatin1String(BuscaService::kCategoriaCartao)) {
            label = tr("Cartão");
            cor   = QColor(0xFFF8E8);
        }

        m_table->setItem(i, 0, roItem(label));
        m_table->setItem(i, 1, roItem(fmtData(r.data)));
        m_table->setItem(i, 2, roItem(r.descricao));
        m_table->setItem(i, 3, roItem(fmtCentavos(r.valor),
                                      Qt::AlignVCenter | Qt::AlignRight));
        m_table->setItem(i, 4, roItem(r.categoriaObs));

        if (cor.isValid()) {
            for (int c = 0; c < 5; ++c)
                if (auto *it = m_table->item(i, c))
                    it->setBackground(cor);
        }
    }

    // Ajusta título com contagem
    setWindowTitle(tr("Busca global — %1 resultado(s)").arg(resultados.size()));
}

void BuscaGlobalDialog::onCellDoubleClicked(int row, int /*col*/)
{
    if (row >= 0 && row < m_tabIndices.size()) {
        emit navegarParaAba(m_tabIndices[row]);
        // Não fecha — dialog não-modal; usuário pode continuar buscando
    }
}
