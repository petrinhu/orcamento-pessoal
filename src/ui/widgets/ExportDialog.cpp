#include "ExportDialog.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>

ExportDialog::ExportDialog(const QStringList &sections,
                           const QString     &initialFormat,
                           QWidget           *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Exportar — Escolher seções"));
    setMinimumWidth(380);
    buildUi(sections, initialFormat);
}

void ExportDialog::buildUi(const QStringList &sections, const QString &initialFormat)
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setSpacing(10);

    // ── Format selector ───────────────────────────────────────────────────────
    auto *fmtGroup = new QGroupBox(tr("Formato"), this);
    auto *fmtLay   = new QHBoxLayout(fmtGroup);
    m_cbFmt = new QComboBox(fmtGroup);

    const QList<QPair<QString,QString>> formats = {
        { tr("CSV (.csv)"),      QStringLiteral("csv")  },
        { tr("Excel (.xlsx)"),   QStringLiteral("xlsx") },
        { tr("JSON (.json)"),    QStringLiteral("json") },
        { tr("Markdown (.md)"),  QStringLiteral("md")   },
        { tr("HTML (.html)"),    QStringLiteral("html") },
        { tr("PDF (.pdf)"),      QStringLiteral("pdf")  },
        { tr("Texto (.txt)"),    QStringLiteral("txt")  },
        { tr("OFX (.ofx)"),      QStringLiteral("ofx")  },
        { tr("QIF (.qif)"),      QStringLiteral("qif")  },
    };
    int initialIdx = 0;
    for (int i = 0; i < formats.size(); ++i) {
        m_cbFmt->addItem(formats[i].first, formats[i].second);
        if (formats[i].second == initialFormat)
            initialIdx = i;
    }
    m_cbFmt->setCurrentIndex(initialIdx);
    fmtLay->addWidget(m_cbFmt, 1);
    vbox->addWidget(fmtGroup);

    // ── Section checkboxes ────────────────────────────────────────────────────
    auto *secGroup = new QGroupBox(tr("Seções a exportar"), this);
    auto *secVbox  = new QVBoxLayout(secGroup);

    m_list = new QListWidget(secGroup);
    m_list->setSelectionMode(QAbstractItemView::NoSelection);
    for (const auto &s : sections) {
        auto *item = new QListWidgetItem(s, m_list);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }
    secVbox->addWidget(m_list);

    auto *btnRow  = new QHBoxLayout;
    auto *btnAll  = new QPushButton(tr("Selecionar tudo"), secGroup);
    auto *btnNone = new QPushButton(tr("Limpar seleção"),  secGroup);
    btnRow->addWidget(btnAll);
    btnRow->addWidget(btnNone);
    btnRow->addStretch();
    secVbox->addLayout(btnRow);
    vbox->addWidget(secGroup, 1);

    // ── Note for OFX/QIF ─────────────────────────────────────────────────────
    auto *lblNote = new QLabel(
        tr("OFX e QIF exportam apenas transações financeiras:\n"
           "Entradas (crédito), Gastos (débito) e Cartões (débito)."),
        this);
    lblNote->setWordWrap(true);
    lblNote->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    vbox->addWidget(lblNote);

    // ── OK / Cancelar ─────────────────────────────────────────────────────────
    auto *bbox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    vbox->addWidget(bbox);

    connect(btnAll,  &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < m_list->count(); ++i)
            m_list->item(i)->setCheckState(Qt::Checked);
    });
    connect(btnNone, &QPushButton::clicked, this, [this]() {
        for (int i = 0; i < m_list->count(); ++i)
            m_list->item(i)->setCheckState(Qt::Unchecked);
    });
    connect(bbox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QStringList ExportDialog::selectedSections() const
{
    QStringList result;
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->checkState() == Qt::Checked)
            result << m_list->item(i)->text();
    }
    return result;
}

QString ExportDialog::selectedFormat() const
{
    return m_cbFmt->currentData().toString();
}
