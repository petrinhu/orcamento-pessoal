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
        tr("<b>Licença: GNU Affero General Public License v3.0 (AGPL-3.0)</b><br>"
           "✓ Uso pessoal, distribuição e modificação: <b>permitidos</b>.<br>"
           "✗ Uso comercial ou venda: <b>proibidos</b>."), this);
    lblLic->setWordWrap(true);
    lblLic->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblLic);

    layout->addSpacing(14);

    // ── Third-party licenses (DIST-05) ──────────────────────────────────────
    auto *sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep2);

    layout->addSpacing(14);

    auto *lblTerceiros = new QLabel(
        tr("<b>Bibliotecas e recursos de terceiros</b><br>"
           "Fonte Inter — SIL Open Font License 1.1<br>"
           "Ícones Noto Color Emoji — SIL Open Font License 1.1<br>"
           "QXlsx — Licença MIT<br>"
           "libsodium — Licença ISC"), this);
    lblTerceiros->setObjectName(QStringLiteral("subtitle"));
    lblTerceiros->setWordWrap(true);
    lblTerceiros->setAlignment(Qt::AlignCenter);
    layout->addWidget(lblTerceiros);

    layout->addSpacing(20);

    // ── Close button ──────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(btns);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::accept);
}
