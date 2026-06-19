#include "TabShell.h"
#include "ui/TabHelp.h"
#include "core/settings/AppSettings.h"

#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

namespace {
constexpr auto kBlocoDesc  = "desc";
constexpr auto kBlocoInstr = "instr";
}

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

    // ── Bloco Descrição + Bloco Instruções (colapsáveis, independentes) ────
    montarBloco(QString::fromLatin1(kBlocoDesc),  m_description,  m_descToggle,  m_sepDesc,  root);
    montarBloco(QString::fromLatin1(kBlocoInstr), m_instructions, m_instrToggle, m_sepInstr, root);

    // ── Slot de conteúdo (recebe a tabela/widget da aba) ───────────────────
    m_contentSlot = new QVBoxLayout;
    m_contentSlot->setContentsMargins(0, 0, 0, 0);
    root->addLayout(m_contentSlot, 1);
}

void TabShell::montarBloco(const QString &bloco, QLabel *&outLabel, QToolButton *&outToggle,
                           QFrame *&outSep, QVBoxLayout *root)
{
    auto *linha = new QHBoxLayout;
    linha->setContentsMargins(0, 0, 0, 0);
    linha->setSpacing(6);

    outLabel = new QLabel(this);
    outLabel->setObjectName(QStringLiteral("subtitle")); // QSS: 12px / cinza
    outLabel->setWordWrap(true);
    outLabel->setTextFormat(Qt::RichText);
    outLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    linha->addWidget(outLabel, 1);

    // Toggle ▲/▼ alinhado ao topo-direita, na mesma linha do cabeçalho do bloco.
    outToggle = new QToolButton(this);
    outToggle->setAutoRaise(true);
    outToggle->setCursor(Qt::PointingHandCursor);
    outToggle->setFocusPolicy(Qt::TabFocus);
    outToggle->setCheckable(false);
    linha->addWidget(outToggle, 0, Qt::AlignTop | Qt::AlignRight);

    root->addLayout(linha);

    outSep = new QFrame(this);
    outSep->setFrameShape(QFrame::HLine);
    outSep->setFrameShadow(QFrame::Plain);
    root->addWidget(outSep);

    // Estado inicial vem do default (true); o estado persistido real é aplicado
    // quando setDescription/setInstructions têm texto (lá conhecemos a tabKey).
    QObject::connect(outToggle, &QToolButton::clicked, this, [this, bloco, outLabel, outToggle] {
        const bool novo = !outLabel->isVisible();
        aplicarColapso(outLabel, outToggle, novo);
        if (!m_tabKey.isEmpty())
            AppSettings::instance().setTabHelpExpanded(m_tabKey, bloco, novo);
    });

    outLabel->hide();
    outSep->hide();
    outToggle->hide();
}

void TabShell::aplicarColapso(QLabel *label, QToolButton *toggle, bool expandido)
{
    label->setVisible(expandido);
    // ▲ = recolher (está expandido), ▼ = expandir (está recolhido).
    toggle->setText(expandido ? QStringLiteral("▲") : QStringLiteral("▼"));
    toggle->setToolTip(expandido ? tr("Recolher") : tr("Expandir"));
    toggle->setAccessibleName(expandido ? tr("Recolher bloco") : tr("Expandir bloco"));
}

void TabShell::setTitle(const QString &titulo)
{
    m_title->setText(titulo);
}

void TabShell::setTabKey(const QString &tabKey)
{
    m_tabKey = tabKey;
}

void TabShell::setDescription(const QString &texto)
{
    const bool temTexto = !texto.trimmed().isEmpty();
    m_description->setText(texto);
    m_descToggle->setVisible(temTexto);
    m_sepDesc->setVisible(temTexto);
    if (!temTexto) { m_description->hide(); return; }

    const bool expandido = m_tabKey.isEmpty()
        ? true
        : AppSettings::instance().tabHelpExpanded(m_tabKey, QString::fromLatin1(kBlocoDesc));
    aplicarColapso(m_description, m_descToggle, expandido);
}

void TabShell::setInstructions(const QString &textoRich)
{
    const bool temTexto = !textoRich.trimmed().isEmpty();
    m_instructions->setText(textoRich);
    m_instrToggle->setVisible(temTexto);
    m_sepInstr->setVisible(temTexto);
    if (!temTexto) { m_instructions->hide(); return; }

    const bool expandido = m_tabKey.isEmpty()
        ? true
        : AppSettings::instance().tabHelpExpanded(m_tabKey, QString::fromLatin1(kBlocoInstr));
    aplicarColapso(m_instructions, m_instrToggle, expandido);
}

void TabShell::applyHelp()
{
    if (m_tabKey.isEmpty()) return;
    const TabHelpEntry e = tabHelpEntry(m_tabKey);
    if (!e.titulo.isEmpty())
        setTitle(e.titulo);
    setDescription(e.descricao);

    if (e.passos.isEmpty()) {
        setInstructions(QString());
        return;
    }
    QString ol = QStringLiteral("<ol style='margin:0 0 0 16px; padding:0;'>");
    for (const QString &passo : e.passos)
        ol += QStringLiteral("<li style='margin:2px 0;'>%1</li>").arg(passo);
    ol += QStringLiteral("</ol>");
    setInstructions(ol);
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
