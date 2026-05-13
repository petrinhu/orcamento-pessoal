#include "AuditLogViewer.h"

#include "core/database/DatabaseManager.h"
#include "services/AuditLogService.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDateTime>
#include <QDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

AuditLogViewer::AuditLogViewer(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    carregarOpcoesTabela();
    onFiltrar(); // carrega ultimas 500 entradas no inicio
}

void AuditLogViewer::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // ── Filtros ────────────────────────────────────────────────────────────
    auto *filtros = new QHBoxLayout;
    filtros->setSpacing(8);

    m_cbTabela = new QComboBox(this);
    m_cbTabela->setMinimumWidth(180);

    m_cbOp = new QComboBox(this);
    m_cbOp->addItem(tr("(todas)"),  QString());
    m_cbOp->addItem(QStringLiteral("INSERT"), QStringLiteral("INSERT"));
    m_cbOp->addItem(QStringLiteral("UPDATE"), QStringLiteral("UPDATE"));
    m_cbOp->addItem(QStringLiteral("DELETE"), QStringLiteral("DELETE"));

    m_dateDe = new QDateEdit(QDate::currentDate().addMonths(-1), this);
    m_dateDe->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateDe->setCalendarPopup(true);

    m_dateAte = new QDateEdit(QDate::currentDate(), this);
    m_dateAte->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateAte->setCalendarPopup(true);

    m_spnLimite = new QSpinBox(this);
    m_spnLimite->setRange(50, 10000);
    m_spnLimite->setSingleStep(50);
    m_spnLimite->setValue(500);

    m_btnFiltrar = new QPushButton(tr("Filtrar"), this);
    m_btnFiltrar->setObjectName(QStringLiteral("primary"));

    filtros->addWidget(new QLabel(tr("Tabela:"), this));
    filtros->addWidget(m_cbTabela);
    filtros->addSpacing(8);
    filtros->addWidget(new QLabel(tr("Op:"), this));
    filtros->addWidget(m_cbOp);
    filtros->addSpacing(8);
    filtros->addWidget(new QLabel(tr("De:"), this));
    filtros->addWidget(m_dateDe);
    filtros->addWidget(new QLabel(tr("Até:"), this));
    filtros->addWidget(m_dateAte);
    filtros->addSpacing(8);
    filtros->addWidget(new QLabel(tr("Limite:"), this));
    filtros->addWidget(m_spnLimite);
    filtros->addStretch();
    filtros->addWidget(m_btnFiltrar);

    root->addLayout(filtros);

    connect(m_btnFiltrar, &QPushButton::clicked,
            this, &AuditLogViewer::onFiltrar);

    // ── Tabela ─────────────────────────────────────────────────────────────
    m_tabela = new QTableWidget(this);
    m_tabela->setColumnCount(6);
    m_tabela->setHorizontalHeaderLabels({
        tr("Timestamp"), tr("Tabela"), tr("Op"), tr("Registro"),
        tr("Antes"), tr("Depois")
    });
    m_tabela->horizontalHeader()->setStretchLastSection(true);
    m_tabela->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tabela->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tabela->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tabela->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tabela->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive);
    m_tabela->verticalHeader()->setVisible(false);
    m_tabela->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabela->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabela->setAlternatingRowColors(true);

    connect(m_tabela, &QTableWidget::cellDoubleClicked,
            this, &AuditLogViewer::onLinhaSelecionada);

    root->addWidget(m_tabela);
}

void AuditLogViewer::carregarOpcoesTabela()
{
    m_cbTabela->clear();
    m_cbTabela->addItem(tr("(todas)"), QString());
    // Lista alinhada com tabelas cobertas pelos triggers em
    // AuditLogService::ensureSchema (compra/lancamentos*/cartoes/etc).
    for (const QString &t : {
            QStringLiteral("lancamentos_entradas"),
            QStringLiteral("lancamentos_gastos"),
            QStringLiteral("lancamentos_investimentos"),
            QStringLiteral("compras"),
            QStringLiteral("parcelas"),
            QStringLiteral("cartoes"),
            QStringLiteral("fontes_renda"),
         }) {
        m_cbTabela->addItem(t, t);
    }
}

void AuditLogViewer::onFiltrar()
{
    auto db = DatabaseManager::instance().db();
    const QString  tabela = m_cbTabela->currentData().toString();
    const QString  op     = m_cbOp->currentData().toString();
    const QDateTime ini   = QDateTime(m_dateDe->date(),  QTime(0, 0, 0));
    const QDateTime fim   = QDateTime(m_dateAte->date(), QTime(23, 59, 59));
    const int      lim    = m_spnLimite->value();

    auto entradas = AuditLogService::query(db, tabela, ini, fim, lim);

    // Filtro op client-side (AuditLogService::query nao expoe filtro por op)
    QList<AuditLogEntry> filtradas;
    filtradas.reserve(entradas.size());
    for (const auto &e : entradas) {
        if (op.isEmpty() || e.op == op) filtradas.append(e);
    }

    m_tabela->setRowCount(filtradas.size());
    auto resumir = [](const QString &json) -> QString {
        if (json.isEmpty()) return QStringLiteral("—");
        return json.left(80) + (json.size() > 80 ? QStringLiteral("…") : QString());
    };
    for (int row = 0; row < filtradas.size(); ++row) {
        const auto &e = filtradas[row];
        m_tabela->setItem(row, 0, new QTableWidgetItem(
            e.timestamp.toString(QStringLiteral("dd/MM/yyyy HH:mm:ss"))));
        m_tabela->setItem(row, 1, new QTableWidgetItem(e.tabela));
        m_tabela->setItem(row, 2, new QTableWidgetItem(e.op));
        m_tabela->setItem(row, 3, new QTableWidgetItem(QString::number(e.registroId)));

        auto *itAntes  = new QTableWidgetItem(resumir(e.dadosAntigos));
        itAntes->setData(Qt::UserRole, e.dadosAntigos);
        auto *itDepois = new QTableWidgetItem(resumir(e.dadosNovos));
        itDepois->setData(Qt::UserRole, e.dadosNovos);
        m_tabela->setItem(row, 4, itAntes);
        m_tabela->setItem(row, 5, itDepois);
    }
    m_tabela->resizeColumnsToContents();
}

void AuditLogViewer::onLinhaSelecionada(int row, int /*column*/)
{
    if (row < 0 || row >= m_tabela->rowCount()) return;

    auto *dlg = new QDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowModality(Qt::ApplicationModal);
    dlg->setWindowTitle(tr("Auditoria — detalhe"));
    dlg->resize(700, 500);

    auto *v = new QVBoxLayout(dlg);
    auto *info = new QLabel(tr("<b>%1</b> · %2 · registro #%3 · %4")
        .arg(m_tabela->item(row, 1)->text(),
             m_tabela->item(row, 2)->text(),
             m_tabela->item(row, 3)->text(),
             m_tabela->item(row, 0)->text()), dlg);
    v->addWidget(info);

    auto *form = new QFormLayout;

    auto fmt = [](const QString &raw) -> QString {
        if (raw.isEmpty()) return tr("(vazio)");
        const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
        if (doc.isObject() || doc.isArray()) {
            return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
        }
        return raw;
    };

    auto *txtAntes = new QTextEdit(dlg);
    txtAntes->setReadOnly(true);
    txtAntes->setPlainText(fmt(m_tabela->item(row, 4)->data(Qt::UserRole).toString()));
    txtAntes->setFontFamily(QStringLiteral("monospace"));

    auto *txtDepois = new QTextEdit(dlg);
    txtDepois->setReadOnly(true);
    txtDepois->setPlainText(fmt(m_tabela->item(row, 5)->data(Qt::UserRole).toString()));
    txtDepois->setFontFamily(QStringLiteral("monospace"));

    form->addRow(tr("Antes:"),  txtAntes);
    form->addRow(tr("Depois:"), txtDepois);
    v->addLayout(form);

    auto *btn = new QPushButton(tr("Fechar"), dlg);
    connect(btn, &QPushButton::clicked, dlg, &QDialog::accept);
    v->addWidget(btn);
    dlg->show();
}
