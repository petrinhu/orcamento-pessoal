#include "BuscaGlobalDialog.h"
#include "core/database/DatabaseManager.h"
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
#include <QSqlQuery>
#include <QBrush>
#include <QColor>

// ── Tab indices (em sincronia com a ordem default de MainWindow) ───────────────
// 0=Dashboard 1=Entradas 2=Gastos 3=Cartões 4=Recorrências 5=Dívidas 6=Metas
// 7=Planejamento 8=Tendências 9=Investimentos 10=Proventos 11=Bolsa 12=Lembretes
// ATENCAO: indices fixos sao fragueis se o usuario reordenar as abas (drag &
// drop). Refactor recomendado: navegar por chave estavel (objectName). Ver
// nota de follow-up no retorno da tarefa de abas reordenaveis.
static constexpr int kTabEntradas      = 1;
static constexpr int kTabGastos        = 2;
static constexpr int kTabInvestimentos = 9;
static constexpr int kTabDividas       = 5;
static constexpr int kTabCartoes       = 3;

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

static QString buildIsoDate(int day, int month, int year)
{
    return QDate(year, month, day).toString(Qt::ISODate);
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

static QPair<QString, QString> periodoParaDatas(const QString &periodo)
{
    const QDate hoje  = QDate::currentDate();
    QString dataInicio, dataFim;
    dataFim = hoje.toString(Qt::ISODate);

    if (periodo == QStringLiteral("mes")) {
        dataInicio = QDate(hoje.year(), hoje.month(), 1).toString(Qt::ISODate);
    } else if (periodo == QStringLiteral("3meses")) {
        dataInicio = hoje.addMonths(-3).toString(Qt::ISODate);
    } else if (periodo == QStringLiteral("6meses")) {
        dataInicio = hoje.addMonths(-6).toString(Qt::ISODate);
    } else if (periodo == QStringLiteral("ano")) {
        dataInicio = hoje.addYears(-1).toString(Qt::ISODate);
    } else {
        dataInicio.clear(); // sem filtro de data
    }
    return {dataInicio, dataFim};
}

struct ResultadoBusca {
    QString tipo;
    QString data;      // ISO yyyy-MM-dd
    QString descricao;
    qint64  valor = 0;
    QString categoriaObs;
    int     tabIndex = 0;
    QColor  cor;
};

void BuscaGlobalDialog::popularResultados(const QString &termo,
                                          const QString &periodo,
                                          const QString &tipo)
{
    m_table->setRowCount(0);
    m_tabIndices.clear();

    const auto [dataInicio, dataFim] = periodoParaDatas(periodo);
    const QString like = QStringLiteral("%%1%").arg(termo);

    auto db = DatabaseManager::instance().db();
    QList<ResultadoBusca> resultados;

    // ── Entradas ──────────────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("entradas")) {
        QSqlQuery q(db);
        const bool filtrarData = !dataInicio.isEmpty();
        const QString sql = filtrarData
            ? QStringLiteral(
                "SELECT ano, mes, dia, fonte, valor_bruto, irpf, inss, obs "
                "FROM lancamentos_entradas "
                "WHERE date(ano||'-'||printf('%02d',mes)||'-'||printf('%02d',dia)) "
                "      BETWEEN :data_ini AND :data_fim "
                "  AND (fonte LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500")
            : QStringLiteral(
                "SELECT ano, mes, dia, fonte, valor_bruto, irpf, inss, obs "
                "FROM lancamentos_entradas "
                "WHERE (fonte LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500");
        q.prepare(sql);
        if (filtrarData) {
            q.bindValue(QStringLiteral(":data_ini"), dataInicio);
            q.bindValue(QStringLiteral(":data_fim"), dataFim);
        }
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            ResultadoBusca r;
            r.tipo         = tr("Entrada");
            r.data         = buildIsoDate(q.value(2).toInt(),
                                          q.value(1).toInt(),
                                          q.value(0).toInt());
            r.descricao    = q.value(3).toString();
            const qint64 bruto = q.value(4).toLongLong();
            const qint64 irpf  = q.value(5).toLongLong();
            const qint64 inss  = q.value(6).toLongLong();
            r.valor        = bruto - irpf - inss;
            r.categoriaObs = q.value(7).toString();
            r.tabIndex     = kTabEntradas;
            r.cor          = QColor(0xE8F5ED);
            resultados << r;
        }
    }

    // ── Gastos ────────────────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("gastos")) {
        QSqlQuery q(db);
        const bool filtrarData = !dataInicio.isEmpty();
        const QString sql = filtrarData
            ? QStringLiteral(
                "SELECT ano, mes, dia, descricao, valor, categoria, obs "
                "FROM lancamentos_gastos "
                "WHERE date(ano||'-'||printf('%02d',mes)||'-'||printf('%02d',dia)) "
                "      BETWEEN :data_ini AND :data_fim "
                "  AND (descricao LIKE :like OR categoria LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500")
            : QStringLiteral(
                "SELECT ano, mes, dia, descricao, valor, categoria, obs "
                "FROM lancamentos_gastos "
                "WHERE (descricao LIKE :like OR categoria LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500");
        q.prepare(sql);
        if (filtrarData) {
            q.bindValue(QStringLiteral(":data_ini"), dataInicio);
            q.bindValue(QStringLiteral(":data_fim"), dataFim);
        }
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            ResultadoBusca r;
            r.tipo         = tr("Gasto");
            r.data         = buildIsoDate(q.value(2).toInt(),
                                          q.value(1).toInt(),
                                          q.value(0).toInt());
            r.descricao    = q.value(3).toString();
            r.valor        = -q.value(4).toLongLong();
            r.categoriaObs = q.value(5).toString();
            if (!q.value(6).toString().isEmpty())
                r.categoriaObs += QStringLiteral(" — ") + q.value(6).toString();
            r.tabIndex     = kTabGastos;
            r.cor          = QColor(0xFFF3E8);
            resultados << r;
        }
    }

    // ── Investimentos ─────────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("investimentos")) {
        QSqlQuery q(db);
        const bool filtrarData = !dataInicio.isEmpty();
        const QString sql = filtrarData
            ? QStringLiteral(
                "SELECT ano, mes, dia, nome, valor, tipo_ativo, obs "
                "FROM lancamentos_investimentos "
                "WHERE date(ano||'-'||printf('%02d',mes)||'-'||printf('%02d',dia)) "
                "      BETWEEN :data_ini AND :data_fim "
                "  AND (nome LIKE :like OR tipo_ativo LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500")
            : QStringLiteral(
                "SELECT ano, mes, dia, nome, valor, tipo_ativo, obs "
                "FROM lancamentos_investimentos "
                "WHERE (nome LIKE :like OR tipo_ativo LIKE :like OR obs LIKE :like) "
                "ORDER BY ano DESC, mes DESC, dia DESC LIMIT 500");
        q.prepare(sql);
        if (filtrarData) {
            q.bindValue(QStringLiteral(":data_ini"), dataInicio);
            q.bindValue(QStringLiteral(":data_fim"), dataFim);
        }
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            ResultadoBusca r;
            r.tipo         = tr("Investimento");
            r.data         = buildIsoDate(q.value(2).toInt(),
                                          q.value(1).toInt(),
                                          q.value(0).toInt());
            r.descricao    = q.value(3).toString();
            r.valor        = q.value(4).toLongLong();
            r.categoriaObs = q.value(5).toString();
            if (!q.value(6).toString().isEmpty())
                r.categoriaObs += QStringLiteral(" — ") + q.value(6).toString();
            r.tabIndex     = kTabInvestimentos;
            r.cor          = QColor(0xE8EEFF);
            resultados << r;
        }
    }

    // ── Dívidas ───────────────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("dividas")) {
        QSqlQuery q(db);
        const QString sql = QStringLiteral(
            "SELECT data_inicio, contraparte, descricao, valor_total, obs "
            "FROM dividas "
            "WHERE (contraparte LIKE :like OR descricao LIKE :like OR obs LIKE :like) "
            "ORDER BY data_inicio DESC LIMIT 200");
        q.prepare(sql);
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            ResultadoBusca r;
            r.tipo         = tr("Dívida");
            r.data         = q.value(0).toString();
            r.descricao    = q.value(1).toString();
            if (!q.value(2).toString().isEmpty())
                r.descricao += QStringLiteral(": ") + q.value(2).toString();
            r.valor        = q.value(3).toLongLong();
            r.categoriaObs = q.value(4).toString();
            r.tabIndex     = kTabDividas;
            r.cor          = QColor(0xFFEEFF);
            resultados << r;
        }
    }

    // ── Cartões (compras) ─────────────────────────────────────────────────
    if (tipo == QStringLiteral("todos") || tipo == QStringLiteral("cartoes")) {
        QSqlQuery q(db);
        const QString sql = QStringLiteral(
            "SELECT c.criado_em, c.nome, c.valor_total, ca.apelido "
            "FROM compras c "
            "LEFT JOIN cartoes ca ON ca.id = c.cartao_id "
            "WHERE (c.nome LIKE :like OR ca.apelido LIKE :like) "
            "ORDER BY c.criado_em DESC LIMIT 200");
        q.prepare(sql);
        q.bindValue(QStringLiteral(":like"), like);
        q.exec();
        while (q.next()) {
            ResultadoBusca r;
            r.tipo         = tr("Cartão");
            r.data         = q.value(0).toString().left(10);
            r.descricao    = q.value(1).toString();
            r.valor        = q.value(2).toLongLong();
            r.categoriaObs = q.value(3).toString();
            r.tabIndex     = kTabCartoes;
            r.cor          = QColor(0xFFF8E8);
            resultados << r;
        }
    }

    // ── Popular tabela ────────────────────────────────────────────────────
    m_table->setRowCount(resultados.size());
    m_tabIndices.resize(resultados.size());

    for (int i = 0; i < resultados.size(); ++i) {
        const auto &r = resultados[i];
        m_tabIndices[i] = r.tabIndex;

        m_table->setItem(i, 0, roItem(r.tipo));
        m_table->setItem(i, 1, roItem(fmtData(r.data)));
        m_table->setItem(i, 2, roItem(r.descricao));
        m_table->setItem(i, 3, roItem(fmtCentavos(r.valor),
                                      Qt::AlignVCenter | Qt::AlignRight));
        m_table->setItem(i, 4, roItem(r.categoriaObs));

        if (r.cor.isValid()) {
            for (int c = 0; c < 5; ++c)
                if (auto *it = m_table->item(i, c))
                    it->setBackground(r.cor);
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
