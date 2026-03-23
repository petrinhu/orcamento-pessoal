#include "CompraDetailPanel.h"
#include "AmortizacaoDialog.h"
#include "CalculadoraEdit.h"
#include "services/ParcelaService.h"

#include <QRegularExpression>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QFrame>

// ─── helpers ─────────────────────────────────────────────────────────────────

QString CompraDetailPanel::formatPct(double pct)
{
    // e.g. 1.5 → "1,50%"
    return QStringLiteral("%1%").arg(
        QString::number(pct, 'f', 4)
            .replace(QLatin1Char('.'), QLatin1Char(','))
            .remove(QRegularExpression(QStringLiteral("0+$")))
            .remove(QRegularExpression(QStringLiteral(",$")))
    );
}

double CompraDetailPanel::parsePct(const QString &s)
{
    QString clean = s.trimmed();
    clean.remove(QLatin1Char('%')).replace(QLatin1Char(','), QLatin1Char('.'));
    bool ok = false;
    double v = clean.toDouble(&ok);
    return ok ? v : 0.0;
}

// ─── construction ────────────────────────────────────────────────────────────

CompraDetailPanel::CompraDetailPanel(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    hide();
}

void CompraDetailPanel::buildUi()
{
    // ── separator line at top ──────────────────────────────────────────────
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    outer->addWidget(separator);

    auto *inner = new QVBoxLayout;
    inner->setContentsMargins(16, 8, 16, 8);
    inner->setSpacing(6);
    outer->addLayout(inner);

    // ── control bar ───────────────────────────────────────────────────────
    auto *ctrlBar = new QHBoxLayout;
    ctrlBar->setSpacing(8);

    auto *lblCalc = new QLabel(tr("Calcular:"), this);
    m_cmbSolveFor = new QComboBox(this);
    m_cmbSolveFor->addItem(tr("Valor parcela"),   static_cast<int>(SolveFor::Parcela));
    m_cmbSolveFor->addItem(tr("Total"),            static_cast<int>(SolveFor::Total));
    m_cmbSolveFor->addItem(tr("Taxa de juros"),    static_cast<int>(SolveFor::Taxa));
    m_cmbSolveFor->addItem(tr("Nº de parcelas"),   static_cast<int>(SolveFor::NumParcelas));

    m_btnAmortizacao = new QPushButton(tr("Tabela de amortização…"), this);
    m_btnAmortizacao->setEnabled(false);

    ctrlBar->addWidget(lblCalc);
    ctrlBar->addWidget(m_cmbSolveFor);
    ctrlBar->addStretch();
    ctrlBar->addWidget(m_btnAmortizacao);
    inner->addLayout(ctrlBar);

    // ── fields row ────────────────────────────────────────────────────────
    auto *fieldsRow = new QHBoxLayout;
    fieldsRow->setSpacing(12);

    // Helper: create a labelled column and add it to fieldsRow.
    // Two variants: monetary (CalculadoraEdit — shows ⊞ button) and plain (QLineEdit).
    auto makeFieldCalc = [&](const QString &label, CalculadoraEdit *&edt) {
        auto *col = new QVBoxLayout;
        col->setSpacing(2);
        auto *lbl = new QLabel(label, this);
        QFont f = lbl->font();
        f.setPointSize(f.pointSize() - 1);
        lbl->setFont(f);
        lbl->setObjectName(QStringLiteral("subtitle"));
        edt = new CalculadoraEdit(this);
        edt->setMinimumWidth(90);
        col->addWidget(lbl);
        col->addWidget(edt);
        fieldsRow->addLayout(col);
    };
    auto makeField = [&](const QString &label, QLineEdit *&edt) {
        auto *col = new QVBoxLayout;
        col->setSpacing(2);
        auto *lbl = new QLabel(label, this);
        QFont f = lbl->font();
        f.setPointSize(f.pointSize() - 1);
        lbl->setFont(f);
        lbl->setObjectName(QStringLiteral("subtitle"));
        edt = new QLineEdit(this);
        edt->setMinimumWidth(90);
        col->addWidget(lbl);
        col->addWidget(edt);
        fieldsRow->addLayout(col);
    };

    makeFieldCalc(tr("Total"),          m_edtTotal);
    makeField    (tr("Parcelas"),       m_edtNumParcelas);
    makeField    (tr("Juros a.m."),     m_edtJurosMensais);
    makeField    (tr("CET a.a."),       m_edtCet);
    makeFieldCalc(tr("Valor parcela"),  m_edtValorParcela);
    makeFieldCalc(tr("Juros totais"),   m_edtJurosTotais);
    m_edtJurosTotais->setReadOnly(true); // always read-only → button auto-hidden

    fieldsRow->addStretch();
    inner->addLayout(fieldsRow);

    // ── connections ───────────────────────────────────────────────────────
    connect(m_cmbSolveFor, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CompraDetailPanel::onSolveForChanged);

    connect(m_edtTotal,        &QLineEdit::editingFinished, this, &CompraDetailPanel::onValorTotalEdited);
    connect(m_edtNumParcelas,  &QLineEdit::editingFinished, this, &CompraDetailPanel::onNumParcelasEdited);
    connect(m_edtJurosMensais, &QLineEdit::editingFinished, this, &CompraDetailPanel::onJurosMensaisEdited);
    connect(m_edtCet,          &QLineEdit::editingFinished, this, &CompraDetailPanel::onCetEdited);
    connect(m_edtValorParcela, &QLineEdit::editingFinished, this, &CompraDetailPanel::onValorParcelaEdited);

    connect(m_btnAmortizacao, &QPushButton::clicked, this, &CompraDetailPanel::onAbrirAmortizacao);
}

// ─── public API ──────────────────────────────────────────────────────────────

void CompraDetailPanel::carregarCompra(const Compra &c)
{
    m_compra  = c;
    m_dados   = ParcelaService::dadosFromCompra(c);
    atualizarCampos();
    atualizarEstadoCampos();
    m_btnAmortizacao->setEnabled(c.numParcelas > 0 && c.valorTotal > 0);
    show();
}

void CompraDetailPanel::limpar()
{
    m_edtTotal->clear();
    m_edtNumParcelas->clear();
    m_edtJurosMensais->clear();
    m_edtCet->clear();
    m_edtValorParcela->clear();
    m_edtJurosTotais->clear();
    m_btnAmortizacao->setEnabled(false);
    hide();
}

// ─── private helpers ─────────────────────────────────────────────────────────

void CompraDetailPanel::atualizarCampos()
{
    m_edtTotal->setText(ParcelaService::formatarValor(m_dados.valorTotal, m_compra.moeda));
    m_edtNumParcelas->setText(QString::number(m_dados.numParcelas));
    m_edtJurosMensais->setText(formatPct(m_dados.jurosMensaisPct));
    m_edtCet->setText(formatPct(m_dados.cetPct));
    m_edtValorParcela->setText(ParcelaService::formatarValor(m_dados.valorParcela, m_compra.moeda));
    m_edtJurosTotais->setText(ParcelaService::formatarValor(m_dados.jurosTotais, m_compra.moeda));
}

void CompraDetailPanel::atualizarEstadoCampos()
{
    // All editable by default
    m_edtTotal->setReadOnly(false);
    m_edtNumParcelas->setReadOnly(false);
    m_edtJurosMensais->setReadOnly(false);
    m_edtCet->setReadOnly(false);
    m_edtValorParcela->setReadOnly(false);
    // jurosTotais always read-only (never editable)
    m_edtJurosTotais->setReadOnly(true);

    // The field being solved is the output → read-only
    switch (m_solveFor) {
    case SolveFor::Total:
        m_edtTotal->setReadOnly(true);
        break;
    case SolveFor::NumParcelas:
        m_edtNumParcelas->setReadOnly(true);
        break;
    case SolveFor::Taxa:
        m_edtJurosMensais->setReadOnly(true);
        m_edtCet->setReadOnly(true);
        break;
    case SolveFor::Parcela:
        m_edtValorParcela->setReadOnly(true);
        break;
    }
}

void CompraDetailPanel::readFromFields()
{
    if (!m_edtTotal->isReadOnly())
        m_dados.valorTotal = ParcelaService::parsearValor(m_edtTotal->text());

    if (!m_edtNumParcelas->isReadOnly()) {
        bool ok = false;
        int n = m_edtNumParcelas->text().toInt(&ok);
        if (ok && n > 0) m_dados.numParcelas = n;
    }

    if (!m_edtJurosMensais->isReadOnly())
        m_dados.jurosMensaisPct = parsePct(m_edtJurosMensais->text());

    if (!m_edtCet->isReadOnly())
        m_dados.cetPct = parsePct(m_edtCet->text());

    if (!m_edtValorParcela->isReadOnly())
        m_dados.valorParcela = ParcelaService::parsearValor(m_edtValorParcela->text());
}

void CompraDetailPanel::recalcularEEmitir()
{
    m_dados = ParcelaService::recalcular(m_dados, m_solveFor);
    atualizarCampos();
    m_btnAmortizacao->setEnabled(m_dados.numParcelas > 0 && m_dados.valorTotal > 0);

    ParcelaService::aplicarDadosACompra(m_compra, m_dados);
    emit compraAlterada(m_compra);
}

// ─── slots ───────────────────────────────────────────────────────────────────

void CompraDetailPanel::onSolveForChanged(int index)
{
    m_solveFor = static_cast<SolveFor>(m_cmbSolveFor->itemData(index).toInt());
    atualizarEstadoCampos();
    // Trigger a recalculation right away so the new output field is computed
    readFromFields();
    recalcularEEmitir();
}

void CompraDetailPanel::onValorTotalEdited()
{
    readFromFields();
    recalcularEEmitir();
}

void CompraDetailPanel::onNumParcelasEdited()
{
    readFromFields();
    recalcularEEmitir();
}

void CompraDetailPanel::onJurosMensaisEdited()
{
    // Sync CET from jurosMensais before recalculating
    m_dados.jurosMensaisPct = parsePct(m_edtJurosMensais->text());
    m_dados.cetPct = ParcelaService::cetFromJurosMensais(m_dados.jurosMensaisPct);
    readFromFields();
    recalcularEEmitir();
}

void CompraDetailPanel::onCetEdited()
{
    // Sync jurosMensais from CET before recalculating
    m_dados.cetPct = parsePct(m_edtCet->text());
    m_dados.jurosMensaisPct = ParcelaService::jurosMensaisFromCet(m_dados.cetPct);
    readFromFields();
    recalcularEEmitir();
}

void CompraDetailPanel::onValorParcelaEdited()
{
    readFromFields();
    recalcularEEmitir();
}

void CompraDetailPanel::onAbrirAmortizacao()
{
    auto *dlg = new AmortizacaoDialog(m_compra, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->exec();
}
