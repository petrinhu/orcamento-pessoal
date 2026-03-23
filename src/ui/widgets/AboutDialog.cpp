#include "AboutDialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFrame>
#include <QFont>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Sobre PeuBud"));
    setMinimumWidth(420);
    setModal(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 28, 32, 24);
    layout->setSpacing(0);

    // ── App name ─────────────────────────────────────────────────────────
    auto *lblNome = new QLabel(QStringLiteral("PeuBud"), this);
    QFont fNome = lblNome->font();
    fNome.setPointSize(20);
    fNome.setBold(true);
    lblNome->setFont(fNome);
    lblNome->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblNome);

    auto *lblSub = new QLabel(tr("Orçamento Pessoal"), this);
    lblSub->setObjectName(QStringLiteral("subtitle"));
    lblSub->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblSub);

    layout->addSpacing(4);

    auto *lblVer = new QLabel(tr("Versão 0.1.0"), this);
    lblVer->setObjectName(QStringLiteral("subtitle"));
    lblVer->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblVer);

    layout->addSpacing(20);

    // ── Description ──────────────────────────────────────────────────────
    auto *lblDesc = new QLabel(
        tr("Desenvolvido por <b>Petrus Silva Costa</b> — © 2026<br><br>"
           "Aplicativo de orçamento pessoal em C++20/Qt6.<br>"
           "Banco de dados criptografado com AES-256-CBC."), this);
    lblDesc->setWordWrap(true);
    lblDesc->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblDesc);

    layout->addSpacing(8);

    auto *lblRepo = new QLabel(
        tr("Repositório: codeberg.org/petrinhu/orcamento_pessoal"), this);
    lblRepo->setObjectName(QStringLiteral("subtitle"));
    lblRepo->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblRepo);

    layout->addSpacing(20);

    // ── Separator ─────────────────────────────────────────────────────────
    auto *sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep);

    layout->addSpacing(14);

    // ── License ───────────────────────────────────────────────────────────
    auto *lblLic = new QLabel(
        tr("<b>Licença: PolyForm Noncommercial License 1.0.0</b><br>"
           "✓ Uso pessoal, distribuição e modificação: <b>permitidos</b>.<br>"
           "✗ Uso comercial ou venda: <b>proibidos</b>."), this);
    lblLic->setWordWrap(true);
    lblLic->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblLic);

    layout->addSpacing(20);

    // ── Close button ──────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::accept);
}
