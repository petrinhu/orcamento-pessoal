#include "RecorrenciaDialog.h"
#include "CalculadoraEdit.h"
#include "models/GastoAvulso.h"
#include "ui/UiFormatters.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCompleter>
#include <QDateEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QStackedWidget>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QDate>
#include <QDebug>
#include <QStandardItemModel>
#include <QStandardItem>

// ── Helpers ───────────────────────────────────────────────────────────────────

static QStandardItemModel *buildGroupedCatModel(const QList<Categoria> &cats,
                                                 QObject *parent)
{
    auto *model = new QStandardItemModel(parent);
    auto *noneItem = new QStandardItem(QObject::tr("— sem categoria —"));
    noneItem->setData(QString(), Qt::UserRole);
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
            header->setData(QString(), Qt::UserRole);
            model->appendRow(header);
            for (const auto &ch : children) {
                if (ch.paiId != root.id) continue;
                auto *child = new QStandardItem(
                    QStringLiteral("  \u2514\u2500 ") + ch.nome);
                child->setData(ch.nome, Qt::UserRole);
                model->appendRow(child);
            }
        } else {
            auto *item = new QStandardItem(root.nome);
            QFont f = item->font(); f.setBold(true); item->setFont(f);
            item->setData(root.nome, Qt::UserRole);
            model->appendRow(item);
        }
    }
    return model;
}

// ── Construction ──────────────────────────────────────────────────────────────

RecorrenciaDialog::RecorrenciaDialog(const TransacaoRecorrente &t,
                                     const QList<FonteRenda> &fontes,
                                     const QList<Categoria> &categorias,
                                     QWidget *parent)
    : QDialog(parent)
    , m_original(t)
    , m_fontes(fontes)
{
    setWindowTitle(t.id == 0 ? tr("Nova recorrência") : tr("Editar recorrência"));
    setMinimumWidth(440);
    buildUi(fontes, categorias);
    populate(t);
}

void RecorrenciaDialog::buildUi(const QList<FonteRenda> &fontes,
                                 const QList<Categoria> &categorias)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(20, 20, 20, 20);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    form->setSpacing(8);

    // Descrição
    m_edtDescricao = new QLineEdit(this);
    m_edtDescricao->setMaxLength(100);
    m_edtDescricao->setPlaceholderText(tr("Ex: Aluguel, Salário GAMA…"));
    form->addRow(tr("Descrição:"), m_edtDescricao);

    // Tipo
    m_cbTipo = new QComboBox(this);
    m_cbTipo->addItem(tr("Gasto"),   QStringLiteral("gasto"));
    m_cbTipo->addItem(tr("Entrada"), QStringLiteral("entrada"));
    form->addRow(tr("Tipo:"), m_cbTipo);

    // Valor
    m_edtValor = new CalculadoraEdit(this);
    m_edtValor->lineEdit()->setPlaceholderText(QStringLiteral("0,00"));
    form->addRow(tr("Valor (R$):"), m_edtValor);

    // Campos dinâmicos por tipo
    m_stackCampos = new QStackedWidget(this);

    // Página 0: Gasto
    auto *pageGasto = new QWidget(m_stackCampos);
    auto *formGasto = new QFormLayout(pageGasto);
    formGasto->setContentsMargins(0, 0, 0, 0);
    formGasto->setSpacing(8);

    m_cbCategoria = new QComboBox(pageGasto);
    m_cbCategoria->setModel(buildGroupedCatModel(categorias, pageGasto));
    formGasto->addRow(tr("Categoria:"), m_cbCategoria);

    m_cbTipoPgto = new QComboBox(pageGasto);
    static const struct { const char *raw; const char *label; } kPgtos[] = {
        { "pix",           "PIX"          },
        { "debito",        "Débito"        },
        { "dinheiro",      "Dinheiro"      },
        { "boleto",        "Boleto"        },
        { "transferencia", "Transferência" },
        { "fixo",          "Fixo"          },
        { "outro",         "Outro"         },
    };
    for (const auto &p : kPgtos)
        m_cbTipoPgto->addItem(tr(p.label), QString::fromLatin1(p.raw));
    formGasto->addRow(tr("Pagamento:"), m_cbTipoPgto);

    m_stackCampos->addWidget(pageGasto); // index 0 = gasto

    // Página 1: Entrada
    auto *pageEntrada = new QWidget(m_stackCampos);
    auto *formEntrada = new QFormLayout(pageEntrada);
    formEntrada->setContentsMargins(0, 0, 0, 0);
    formEntrada->setSpacing(8);

    m_cbFonte = new QComboBox(pageEntrada);
    m_cbFonte->setEditable(true);
    m_cbFonte->setInsertPolicy(QComboBox::NoInsert);
    m_cbFonte->addItem(QString()); // empty = sem fonte
    for (const auto &f : fontes)
        m_cbFonte->addItem(f.nome, f.id);
    if (auto *comp = m_cbFonte->completer()) {
        comp->setFilterMode(Qt::MatchContains);
        comp->setCaseSensitivity(Qt::CaseInsensitive);
    }
    formEntrada->addRow(tr("Fonte de renda:"), m_cbFonte);

    m_stackCampos->addWidget(pageEntrada); // index 1 = entrada

    form->addRow(m_stackCampos);

    // Frequência
    m_cbFrequencia = new QComboBox(this);
    m_cbFrequencia->addItem(tr("Semanal (a cada 7 dias)"),   QStringLiteral("semanal"));
    m_cbFrequencia->addItem(tr("Quinzenal (a cada 14 dias)"), QStringLiteral("quinzenal"));
    m_cbFrequencia->addItem(tr("Mensal"),                     QStringLiteral("mensal"));
    m_cbFrequencia->addItem(tr("Anual"),                      QStringLiteral("anual"));
    m_cbFrequencia->setCurrentIndex(2); // mensal default
    form->addRow(tr("Frequência:"), m_cbFrequencia);

    // Data início
    m_dateInicio = new QDateEdit(QDate::currentDate(), this);
    m_dateInicio->setCalendarPopup(true);
    m_dateInicio->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    form->addRow(tr("Início em:"), m_dateInicio);

    // Data fim
    auto *rowFim = new QHBoxLayout;
    m_chkSemFim  = new QCheckBox(tr("Sem data de fim"), this);
    m_chkSemFim->setChecked(true);
    m_dateFim    = new QDateEdit(QDate::currentDate().addYears(1), this);
    m_dateFim->setCalendarPopup(true);
    m_dateFim->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateFim->setEnabled(false);
    rowFim->addWidget(m_chkSemFim);
    rowFim->addWidget(m_dateFim);
    rowFim->addStretch();
    form->addRow(tr("Termina em:"), rowFim);

    connect(m_chkSemFim, &QCheckBox::toggled, m_dateFim,
            [this](bool checked) { m_dateFim->setEnabled(!checked); });

    // Obs
    m_edtObs = new QLineEdit(this);
    m_edtObs->setPlaceholderText(tr("Observação (opcional)"));
    form->addRow(tr("Obs:"), m_edtObs);

    root->addLayout(form);

    // Buttons
    // Margem interna garante folga para a sombra dos botões (#176).
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->setContentsMargins(0, 6, 10, 10);
    root->addWidget(btns);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Tipo change → switch stack
    connect(m_cbTipo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RecorrenciaDialog::onTipoChanged);
    // Initialize stack to match default tipo (gasto = index 0)
    m_stackCampos->setCurrentIndex(0);
}

void RecorrenciaDialog::populate(const TransacaoRecorrente &tr)
{
    m_edtDescricao->setText(tr.descricao);

    // Tipo
    const int tipoIdx = (tr.tipo == QStringLiteral("entrada")) ? 1 : 0;
    m_cbTipo->setCurrentIndex(tipoIdx);
    m_stackCampos->setCurrentIndex(tipoIdx == 1 ? 1 : 0);

    // Valor
    if (tr.valor > 0)
        m_edtValor->lineEdit()->setText(uifmt::formatCentavos(tr.valor));

    // Frequência
    for (int i = 0; i < m_cbFrequencia->count(); ++i) {
        if (m_cbFrequencia->itemData(i).toString() == tr.frequencia) {
            m_cbFrequencia->setCurrentIndex(i);
            break;
        }
    }

    // Datas
    if (tr.dataInicio.isValid())
        m_dateInicio->setDate(tr.dataInicio);
    if (tr.dataFim.isValid()) {
        m_chkSemFim->setChecked(false);
        m_dateFim->setEnabled(true);
        m_dateFim->setDate(tr.dataFim);
    }

    // Obs
    m_edtObs->setText(tr.obs);

    // Gasto fields: pre-select category by text (UserRole)
    if (!tr.categoria.isEmpty()) {
        for (int i = 0; i < m_cbCategoria->model()->rowCount(); ++i) {
            if (m_cbCategoria->model()->index(i, 0)
                    .data(Qt::UserRole).toString()
                    .compare(tr.categoria, Qt::CaseInsensitive) == 0) {
                m_cbCategoria->setCurrentIndex(i);
                break;
            }
        }
    }
    for (int i = 0; i < m_cbTipoPgto->count(); ++i) {
        if (m_cbTipoPgto->itemData(i).toString() == tr.tipoPgto) {
            m_cbTipoPgto->setCurrentIndex(i);
            break;
        }
    }

    // Entrada fields
    if (!tr.fonte.isEmpty()) {
        m_cbFonte->setCurrentText(tr.fonte);
    }
}

void RecorrenciaDialog::onTipoChanged(int index)
{
    // index 0 = gasto, index 1 = entrada (combo order)
    // stack: 0 = gasto page, 1 = entrada page
    m_stackCampos->setCurrentIndex(index == 1 ? 1 : 0);
}

TransacaoRecorrente RecorrenciaDialog::resultado() const
{
    TransacaoRecorrente t = m_original;
    t.descricao  = m_edtDescricao->text().trimmed();
    t.tipo       = m_cbTipo->currentData().toString();
    t.valor      = uifmt::parseCentavos(m_edtValor->lineEdit()->text());
    t.frequencia = m_cbFrequencia->currentData().toString();
    t.dataInicio = m_dateInicio->date();
    t.dataFim    = m_chkSemFim->isChecked() ? QDate() : m_dateFim->date();
    t.obs        = m_edtObs->text().trimmed();

    if (t.tipo == QStringLiteral("gasto")) {
        t.categoria = m_cbCategoria->model()
            ->index(m_cbCategoria->currentIndex(), 0)
            .data(Qt::UserRole).toString();
        t.tipoPgto  = m_cbTipoPgto->currentData().toString();
        t.fonte.clear();
        t.fonteId = 0;
    } else {
        const QString fonteNome = m_cbFonte->currentText().trimmed();
        t.fonte = fonteNome;
        // Resolve fonteId from list
        t.fonteId = 0;
        for (const auto &f : m_fontes) {
            if (f.nome == fonteNome) { t.fonteId = f.id; break; }
        }
        t.categoria.clear();
        t.tipoPgto.clear();
    }

    return t;
}
