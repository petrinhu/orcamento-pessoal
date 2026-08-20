#include "GlossarioDialog.h"
#include "services/GlossarioService.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <algorithm>

GlossarioDialog::GlossarioDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Glossário financeiro"));
    resize(720, 480);
    buildUi();
    popularLista();
}

void GlossarioDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    auto *hint = new QLabel(tr("Termos técnicos usados no PeuBud, em pt-BR plain. Digite para filtrar."), this);
    hint->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    root->addWidget(hint);

    m_edtBusca = new QLineEdit(this);
    m_edtBusca->setPlaceholderText(tr("Buscar termo (ex: CET, FIRE, IRPF)…"));
    m_edtBusca->setClearButtonEnabled(true);
    root->addWidget(m_edtBusca);

    auto *body = new QHBoxLayout;
    m_lista = new QListWidget(this);
    m_lista->setMaximumWidth(220);
    m_lista->setAlternatingRowColors(true);
    body->addWidget(m_lista);

    m_texto = new QTextBrowser(this);
    m_texto->setOpenExternalLinks(true);
    body->addWidget(m_texto, 1);
    root->addLayout(body);

    auto *footer = new QHBoxLayout;
    auto *lblFonte = new QLabel(
        tr("<i>Sugerir termo faltante: abra issue em github.com/petrinhu/orcamento-pessoal</i>"), this);
    lblFonte->setOpenExternalLinks(true);
    footer->addWidget(lblFonte, 1);
    auto *btnClose = new QPushButton(tr("Fechar"), this);
    btnClose->setDefault(true);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(btnClose);
    root->addLayout(footer);

    connect(m_edtBusca, &QLineEdit::textChanged, this, &GlossarioDialog::onFiltrar);
    connect(m_lista, &QListWidget::currentRowChanged,
            this, [this](int) { onSelecaoChanged(); });
}

void GlossarioDialog::popularLista()
{
    auto termos = GlossarioService::termos();
    std::sort(termos.begin(), termos.end());
    m_lista->clear();
    m_lista->addItems(termos);
    if (m_lista->count() > 0) m_lista->setCurrentRow(0);
}

void GlossarioDialog::onFiltrar(const QString &texto)
{
    const QString needle = texto.trimmed().toUpper();
    for (int i = 0; i < m_lista->count(); ++i) {
        auto *it = m_lista->item(i);
        const bool match = needle.isEmpty()
            || it->text().toUpper().contains(needle)
            || GlossarioService::definicao(it->text()).toUpper().contains(needle);
        it->setHidden(!match);
    }
    // Move foco para primeiro visivel
    for (int i = 0; i < m_lista->count(); ++i) {
        if (!m_lista->item(i)->isHidden()) {
            m_lista->setCurrentRow(i);
            break;
        }
    }
}

void GlossarioDialog::onSelecaoChanged()
{
    auto *it = m_lista->currentItem();
    if (!it) {
        m_texto->clear();
        return;
    }
    const QString termo = it->text();
    const QString def = GlossarioService::definicao(termo);
    const QString micro = GlossarioService::microcopy(termo);
    const QString coloq = GlossarioService::labelColoquial(termo);

    QString html;
    html += QStringLiteral("<h2>%1</h2>").arg(termo);
    if (!coloq.isEmpty()) {
        html += QStringLiteral("<p><b>Em pt-BR plain:</b> %1</p>").arg(coloq);
    }
    html += QStringLiteral("<p>%1</p>").arg(def);
    if (!micro.isEmpty()) {
        html += QStringLiteral("<p style='color: gray; font-size: 11px;'>%1</p>").arg(micro);
    }
    m_texto->setHtml(html);
}

void GlossarioDialog::selecionarTermo(const QString &termo)
{
    const auto items = m_lista->findItems(termo.toUpper(), Qt::MatchExactly);
    if (!items.isEmpty()) m_lista->setCurrentItem(items.first());
}
