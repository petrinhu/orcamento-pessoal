#include "EmptyStateWidget.h"
#include "ui/Theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

// ── Construction ──────────────────────────────────────────────────────────────

EmptyStateWidget::EmptyStateWidget(const QString &emoji,
                                   const QString &titulo,
                                   const QString &frase,
                                   const QString &ctaLabel,
                                   QWidget       *parent)
    : QWidget(parent)
{
    // Layout raiz: espaçamento generoso ao centro (H+V)
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 48, 32, 48);
    root->setSpacing(0);
    root->addStretch();

    // ── Emoji grande 64px ─────────────────────────────────────────────────────
    m_lblEmoji = new QLabel(this);
    m_lblEmoji->setObjectName(QStringLiteral("emptyStateEmoji"));
    m_lblEmoji->setPixmap(Theme::emojiIcon(emoji, 64).pixmap(64, 64));
    m_lblEmoji->setAlignment(Qt::AlignHCenter);
    root->addWidget(m_lblEmoji);
    root->addSpacing(16);

    // ── Título — Inter SemiBold 16px, cor @@TXT@@ ────────────────────────────
    m_lblTitulo = new QLabel(titulo, this);
    m_lblTitulo->setObjectName(QStringLiteral("emptyStateTitle"));
    m_lblTitulo->setAlignment(Qt::AlignHCenter);
    // QSS globalinject via objectName — Inter SemiBold 16px, cor text (@@TXT@@)
    // é controlado pelo QSS do Theme.cpp (seletor QLabel#emptyStateTitle).
    root->addWidget(m_lblTitulo);
    root->addSpacing(8);

    // ── Frase de apoio — Inter Regular 13px, cor @@SUB@@, wrap ───────────────
    m_lblFrase = new QLabel(frase, this);
    m_lblFrase->setObjectName(QStringLiteral("emptyStateFrase"));
    m_lblFrase->setAlignment(Qt::AlignHCenter);
    m_lblFrase->setWordWrap(true);
    m_lblFrase->setMaximumWidth(480); // largura de leitura confortável ~60-65ch
    // Estilo via objectName: Regular 13px, cor subtext (@@SUB@@).
    root->addWidget(m_lblFrase, 0, Qt::AlignHCenter);

    // ── CTA opcional — botão #primary ─────────────────────────────────────────
    if (!ctaLabel.isEmpty()) {
        root->addSpacing(20);
        auto *row = new QHBoxLayout;
        row->setContentsMargins(0, 0, 0, 0);
        m_btnCta = new QPushButton(ctaLabel, this);
        m_btnCta->setObjectName(QStringLiteral("primary"));
        m_btnCta->setIcon(Theme::emojiIcon(QStringLiteral("➕")));
        // Tamanho razoável — não stretch: CTA deve parecer convidativo, não dominante
        m_btnCta->setMinimumWidth(160);
        row->addStretch();
        row->addWidget(m_btnCta);
        row->addStretch();
        root->addLayout(row);
    }

    root->addStretch();
}
