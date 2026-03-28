#include "InvestimentoDialog.h"
#include "CalculadoraEdit.h"

#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDate>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QStandardItemModel *buildGroupedCatModel(const QList<Categoria> &cats,
                                                 QObject *parent)
{
    auto *model = new QStandardItemModel(parent);
    auto *noneItem = new QStandardItem(QObject::tr("— sem categoria —"));
    noneItem->setData(0,         Qt::UserRole);
    noneItem->setData(QString(), Qt::UserRole + 1);
    model->appendRow(noneItem);

    QList<Categoria> roots, children;
    for (const auto &c : cats) {
        if (c.paiId == 0) roots.append(c);
        else              children.append(c);
    }
    for (const auto &root : roots) {
        bool hasChild = false;
        for (const auto &ch : children)
            if (ch.paiId == root.id) { hasChild = true; break; }

        if (hasChild) {
            auto *header = new QStandardItem(root.nome);
            QFont f = header->font(); f.setBold(true); header->setFont(f);
            header->setFlags(Qt::ItemIsEnabled);
            header->setData(-root.id, Qt::UserRole);
            header->setData(QString(),  Qt::UserRole + 1);
            model->appendRow(header);
            for (const auto &ch : children) {
                if (ch.paiId != root.id) continue;
                auto *child = new QStandardItem(
                    QStringLiteral("  \u2514\u2500 ") + ch.nome);
                child->setData(ch.id,   Qt::UserRole);
                child->setData(ch.nome, Qt::UserRole + 1);
                model->appendRow(child);
            }
        } else {
            auto *item = new QStandardItem(root.nome);
            QFont f = item->font(); f.setBold(true); item->setFont(f);
            item->setData(root.id,   Qt::UserRole);
            item->setData(root.nome, Qt::UserRole + 1);
            model->appendRow(item);
        }
    }
    return model;
}

static qint64 parseCentavos(const QString &txt)
{
    QString s = txt.trimmed();
    s.remove(QStringLiteral("R$")).remove(QLatin1Char(' '));
    if (s.contains(QLatin1Char(',')) && s.contains(QLatin1Char('.'))) {
        s.remove(QLatin1Char('.'));
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    } else if (s.contains(QLatin1Char(','))) {
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    bool ok = false;
    const double val = s.toDouble(&ok);
    return ok ? static_cast<qint64>(qRound(val * 100.0)) : 0LL;
}

static QString formatCentavos(qint64 c)
{
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("%1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

// ── TipoAtivo entries ─────────────────────────────────────────────────────────

struct TipoAtivoEntry { TipoAtivo tipo; const char *label; };
static const TipoAtivoEntry kTiposAtivo[] = {
    { TipoAtivo::Acao,     "Ação"     },
    { TipoAtivo::Fii,      "FII"      },
    { TipoAtivo::Cdb,      "CDB"      },
    { TipoAtivo::LciLca,   "LCI/LCA"  },
    { TipoAtivo::Tesouro,  "Tesouro"  },
    { TipoAtivo::Fundo,    "Fundo"    },
    { TipoAtivo::Poupanca, "Poupança" },
    { TipoAtivo::Outro,    "Outro"    },
};

struct TipoOpEntry { TipoOperacaoInv op; const char *label; };
static const TipoOpEntry kTiposOp[] = {
    { TipoOperacaoInv::Aporte,     "Aporte"     },
    { TipoOperacaoInv::Resgate,    "Resgate"    },
    { TipoOperacaoInv::Rendimento, "Rendimento" },
    { TipoOperacaoInv::Dividendo,  "Dividendo"  },
};

// ── Constructor ───────────────────────────────────────────────────────────────

InvestimentoDialog::InvestimentoDialog(const LancamentoInvestimento &inv,
                                       int mes, int ano,
                                       const QList<Categoria> &categorias,
                                       QWidget *parent)
    : QDialog(parent)
    , m_original(inv)
    , m_mes(mes)
    , m_ano(ano)
{
    setWindowTitle(inv.id > 0 ? tr("Editar investimento") : tr("Novo investimento"));
    buildUi(categorias);
}

void InvestimentoDialog::buildUi(const QList<Categoria> &categorias)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    form->setSpacing(8);

    // ── Nome ───────────────────────────────────────────────────────────────
    m_edtNome = new QLineEdit(this);
    m_edtNome->setPlaceholderText(tr("Ex: PETR4, CDB Nubank, Tesouro IPCA+"));
    m_edtNome->setMaxLength(200);
    m_edtNome->setText(m_original.nome);
    form->addRow(tr("Nome / Ativo:"), m_edtNome);

    // ── Categoria ──────────────────────────────────────────────────────────
    m_cbCategoria = new QComboBox(this);
    m_cbCategoria->setModel(buildGroupedCatModel(categorias, this));

    int catSelectIdx = 0;
    if (m_original.categoriaId > 0) {
        for (int i = 0; i < m_cbCategoria->model()->rowCount(); ++i) {
            if (m_cbCategoria->model()->index(i, 0)
                    .data(Qt::UserRole).toInt() == m_original.categoriaId) {
                catSelectIdx = i; break;
            }
        }
    } else if (!m_original.categoria.isEmpty()) {
        for (int i = 1; i < m_cbCategoria->model()->rowCount(); ++i) {
            if (m_cbCategoria->model()->index(i, 0)
                    .data(Qt::UserRole + 1).toString()
                    .compare(m_original.categoria, Qt::CaseInsensitive) == 0) {
                catSelectIdx = i; break;
            }
        }
    }
    m_cbCategoria->setCurrentIndex(catSelectIdx);
    form->addRow(tr("Categoria:"), m_cbCategoria);

    // ── Tipo de ativo ──────────────────────────────────────────────────────
    m_cbTipoAtivo = new QComboBox(this);
    const TipoAtivo curAtivo = tipoAtivoDeTexto(m_original.tipoAtivo);
    int defaultAtivo = 0;
    for (int i = 0; i < static_cast<int>(std::size(kTiposAtivo)); ++i) {
        m_cbTipoAtivo->addItem(tr(kTiposAtivo[i].label),
                               tipoAtivoParaTexto(kTiposAtivo[i].tipo));
        if (kTiposAtivo[i].tipo == curAtivo)
            defaultAtivo = i;
    }
    m_cbTipoAtivo->setCurrentIndex(defaultAtivo);
    form->addRow(tr("Tipo de ativo:"), m_cbTipoAtivo);

    // ── Operação ───────────────────────────────────────────────────────────
    m_cbOperacao = new QComboBox(this);
    const TipoOperacaoInv curOp = tipoOpInvDeTexto(m_original.tipoOperacao);
    int defaultOp = 0;
    for (int i = 0; i < static_cast<int>(std::size(kTiposOp)); ++i) {
        m_cbOperacao->addItem(tr(kTiposOp[i].label),
                              tipoOpInvParaTexto(kTiposOp[i].op));
        if (kTiposOp[i].op == curOp)
            defaultOp = i;
    }
    m_cbOperacao->setCurrentIndex(defaultOp);
    form->addRow(tr("Operação:"), m_cbOperacao);

    // ── Valor ──────────────────────────────────────────────────────────────
    m_edtValor = new CalculadoraEdit(this);
    m_edtValor->setPlaceholderText(QStringLiteral("0,00"));
    if (m_original.valor > 0)
        m_edtValor->setText(formatCentavos(m_original.valor));
    form->addRow(tr("Valor:"), m_edtValor);

    // ── Data ───────────────────────────────────────────────────────────────
    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateEdit->setCalendarPopup(true);
    const QDate defaultDate = (m_original.dia > 0)
        ? QDate(m_ano, m_mes, m_original.dia)
        : QDate(m_ano, m_mes, 1);
    m_dateEdit->setDate(defaultDate);
    form->addRow(tr("Data:"), m_dateEdit);

    // ── Obs ────────────────────────────────────────────────────────────────
    m_edtObs = new QLineEdit(this);
    m_edtObs->setPlaceholderText(tr("Observação (opcional)"));
    m_edtObs->setMaxLength(200);
    m_edtObs->setText(m_original.obs);
    form->addRow(tr("Obs:"), m_edtObs);

    root->addLayout(form);

    // ── Recorrência ────────────────────────────────────────────────────────
    auto *grpRec = new QGroupBox(tr("Recorrência"), this);
    auto *layRec = new QVBoxLayout(grpRec);
    layRec->setSpacing(4);

    m_rbSomente    = new QRadioButton(tr("Somente este mês"), grpRec);
    m_rbRecorrente = new QRadioButton(tr("Repetir por"), grpRec);
    m_rbSomente->setChecked(true);

    auto *rowMeses = new QHBoxLayout;
    m_spnMeses = new QSpinBox(grpRec);
    m_spnMeses->setRange(2, 120);
    m_spnMeses->setValue(12);
    m_spnMeses->setSuffix(tr(" meses"));
    m_spnMeses->setEnabled(false);
    rowMeses->addWidget(m_rbRecorrente);
    rowMeses->addWidget(m_spnMeses);
    rowMeses->addStretch();

    layRec->addWidget(m_rbSomente);
    layRec->addLayout(rowMeses);

    connect(m_rbRecorrente, &QRadioButton::toggled,
            this, &InvestimentoDialog::onRecorrenciaToggled);
    root->addWidget(grpRec);

    // ── Buttons ────────────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Save)->setText(tr("Salvar"));
    btns->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);

    setMinimumWidth(400);
    m_edtNome->setFocus();
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void InvestimentoDialog::onRecorrenciaToggled(bool recorrente)
{
    m_spnMeses->setEnabled(recorrente);
}

// ── Result ────────────────────────────────────────────────────────────────────

QList<LancamentoInvestimento> InvestimentoDialog::lancamentos() const
{
    const QString nome      = m_edtNome->text().trimmed();
    const QString tipoAtivo = m_cbTipoAtivo->currentData().toString();
    const QString tipoOp    = m_cbOperacao->currentData().toString();
    const qint64  valor     = parseCentavos(m_edtValor->text());
    const QString obs       = m_edtObs->text().trimmed();
    const QDate   baseDate  = m_dateEdit->date();
    const int     nMeses    = m_rbRecorrente->isChecked() ? m_spnMeses->value() : 1;

    const int catIdx = m_cbCategoria->currentIndex();
    const QModelIndex mi = m_cbCategoria->model()->index(catIdx, 0);
    int     catId   = mi.data(Qt::UserRole).toInt();
    QString catNome = mi.data(Qt::UserRole + 1).toString();
    if (catId < 0) { catId = 0; catNome.clear(); }

    QList<LancamentoInvestimento> result;
    for (int i = 0; i < nMeses; ++i) {
        const QDate d = baseDate.addMonths(i);
        LancamentoInvestimento l;
        l.id           = (i == 0 && m_original.id > 0) ? m_original.id : 0;
        l.mes          = d.month();
        l.ano          = d.year();
        l.dia          = d.day();
        l.nome         = nome;
        l.tipoAtivo    = tipoAtivo;
        l.tipoOperacao = tipoOp;
        l.valor        = valor;
        l.categoria    = catNome;
        l.categoriaId  = catId;
        l.obs          = obs;
        result.append(l);
    }
    return result;
}
