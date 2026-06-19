#include "TabShell.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

TabShell::TabShell(const QString &titulo, QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    // ── Cabeçalho: título à esquerda, ações à direita ──────────────────────
    auto *header = new QHBoxLayout;
    header->setSpacing(8);

    m_title = new QLabel(titulo, this);
    m_title->setObjectName(QStringLiteral("title")); // QSS: 16px / weight 600
    QFont fTitle = m_title->font();
    fTitle.setPointSize(fTitle.pointSize() + 4);
    fTitle.setBold(true);
    m_title->setFont(fTitle);
    header->addWidget(m_title);

    header->addStretch();

    m_actionsLayout = new QHBoxLayout;
    m_actionsLayout->setContentsMargins(0, 0, 0, 0);
    m_actionsLayout->setSpacing(6);
    header->addLayout(m_actionsLayout);

    root->addLayout(header);

    // ── Separador 1: abaixo do cabeçalho ───────────────────────────────────
    auto *sepHeader = new QFrame(this);
    sepHeader->setFrameShape(QFrame::HLine);
    sepHeader->setFrameShadow(QFrame::Plain);
    root->addWidget(sepHeader);

    // ── Descrição (oculta enquanto vazia) ──────────────────────────────────
    m_description = new QLabel(this);
    m_description->setObjectName(QStringLiteral("subtitle")); // QSS: 12px / cinza
    m_description->setWordWrap(true);
    m_description->setTextFormat(Qt::PlainText);
    m_description->setMaximumHeight(m_description->fontMetrics().lineSpacing() * 3
                                    + 4); // ~3 linhas
    m_description->hide();
    root->addWidget(m_description);

    m_sepDesc = new QFrame(this);
    m_sepDesc->setFrameShape(QFrame::HLine);
    m_sepDesc->setFrameShadow(QFrame::Plain);
    m_sepDesc->hide();
    root->addWidget(m_sepDesc);

    // ── Slot de conteúdo (recebe a tabela/widget da aba) ───────────────────
    m_contentSlot = new QVBoxLayout;
    m_contentSlot->setContentsMargins(0, 0, 0, 0);
    root->addLayout(m_contentSlot, 1);
}

void TabShell::setTitle(const QString &titulo)
{
    m_title->setText(titulo);
}

void TabShell::setDescription(const QString &texto)
{
    m_description->setText(texto);
    const bool temTexto = !texto.trimmed().isEmpty();
    m_description->setVisible(temTexto);
    m_sepDesc->setVisible(temTexto);
}

void TabShell::setContent(QWidget *content)
{
    if (m_content == content) return;
    if (m_content) {
        m_contentSlot->removeWidget(m_content);
        m_content->setParent(nullptr);
    }
    m_content = content;
    if (m_content) {
        m_content->setParent(this);
        m_contentSlot->addWidget(m_content);
    }
}

void TabShell::setActions(QWidget *actions)
{
    if (!actions) return;
    m_actionsLayout->addWidget(actions);
}
