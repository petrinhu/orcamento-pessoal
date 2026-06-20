#include "SidebarNav.h"

#include "ui/Theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

// ── helpers internos ──────────────────────────────────────────────────────────

static QFrame *criarSeparador(QWidget *parent)
{
    auto *sep = new QFrame(parent);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Plain);
    sep->setObjectName(QStringLiteral("sidebarSep"));
    return sep;
}

// ── SidebarNav ────────────────────────────────────────────────────────────────

SidebarNav::SidebarNav(QWidget *parent)
    : QWidget(parent)
{
    // Layout raiz — empilha itens (top), stretch e footer (bottom).
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 12, 8, 8);
    rootLayout->setSpacing(2);

    // Zona de itens: grupos + navItems na ordem de insercao.
    auto *mainWidget = new QWidget(this);
    m_mainLayout = new QVBoxLayout(mainWidget);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(2);
    rootLayout->addWidget(mainWidget);

    // Espaco flexivel empurra o footer para baixo.
    rootLayout->addStretch();

    // Separador visual antes do rodape.
    rootLayout->addWidget(criarSeparador(this));

    // Zona de footer: footerItems + bloco de usuario.
    auto *footerWidget = new QWidget(this);
    m_footerLayout = new QVBoxLayout(footerWidget);
    m_footerLayout->setContentsMargins(0, 4, 0, 0);
    m_footerLayout->setSpacing(2);
    rootLayout->addWidget(footerWidget);

    // Bloco de usuario (avatar inicial + nome) — fica sempre no fim do footer.
    auto *usuarioWidget = new QWidget(this);
    auto *usuarioLayout = new QHBoxLayout(usuarioWidget);
    usuarioLayout->setContentsMargins(4, 4, 4, 4);
    usuarioLayout->setSpacing(8);

    m_avatarLabel = new QLabel(this);
    m_avatarLabel->setObjectName(QStringLiteral("sidebarAvatar"));
    m_avatarLabel->setFixedSize(28, 28);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    // Estilo basico do circulo — sera refinado pelo QSS navy na task futura.
    m_avatarLabel->setStyleSheet(
        QStringLiteral("background:#3B82F6; color:white; border-radius:14px; font-weight:600;"));

    m_nomeLabel = new QLabel(this);
    m_nomeLabel->setObjectName(QStringLiteral("sidebarNome"));

    usuarioLayout->addWidget(m_avatarLabel);
    usuarioLayout->addWidget(m_nomeLabel, 1);

    m_footerLayout->addWidget(usuarioWidget);
}

// ── API publica ───────────────────────────────────────────────────────────────

void SidebarNav::addGrupo(const QString &titulo)
{
    auto *label = new QLabel(titulo.toUpper(), this);
    label->setObjectName(QStringLiteral("grupo"));
    // Margem superior separa o grupo do item anterior.
    label->setContentsMargins(6, 10, 6, 2);
    m_mainLayout->addWidget(label);
}

void SidebarNav::addItem(const QString &chave, const QString &emoji,
                         const QString &titulo)
{
    registrarItem(chave, emoji, titulo, m_mainLayout);
}

void SidebarNav::addFooterItem(const QString &chave, const QString &emoji,
                               const QString &titulo)
{
    // Footer items ficam acima do bloco de usuario; insere antes do ultimo widget
    // (que e o usuarioWidget). Count() - 1 = indice do usuarioWidget.
    const int posAntesDaUsuario = m_footerLayout->count() - 1;

    auto *btn = new QPushButton(titulo, this);
    btn->setObjectName(QStringLiteral("navItem"));
    btn->setIcon(Theme::emojiIcon(emoji));
    btn->setFlat(true);
    btn->setProperty("ativo", false);
    btn->setCursor(Qt::PointingHandCursor);

    m_itens.insert(chave, btn);

    // Posicao fixada antes do bloco de usuario.
    m_footerLayout->insertWidget(posAntesDaUsuario, btn);

    const QString chaveCapturada = chave;
    connect(btn, &QPushButton::clicked, this, [this, chaveCapturada]() {
        setAtivo(chaveCapturada);
        emit navegou(chaveCapturada);
    });
}

void SidebarNav::setUsuario(const QString &nome)
{
    if (nome.isEmpty()) {
        m_avatarLabel->setText(QStringLiteral("?"));
        m_nomeLabel->setText(QString());
    } else {
        m_avatarLabel->setText(nome.at(0).toUpper());
        m_nomeLabel->setText(nome);
    }
}

void SidebarNav::setAtivo(const QString &chave)
{
    for (auto it = m_itens.begin(); it != m_itens.end(); ++it) {
        const bool ativo = (it.key() == chave);
        it.value()->setProperty("ativo", ativo);
        // Forca o QSS a reprocessar a propriedade dinamica.
        it.value()->style()->polish(it.value());
    }
}

QPushButton *SidebarNav::itemParaTeste(const QString &chave) const
{
    return m_itens.value(chave, nullptr);
}

// ── registrarItem (privado) ───────────────────────────────────────────────────

void SidebarNav::registrarItem(const QString &chave, const QString &emoji,
                               const QString &titulo, QVBoxLayout *layout)
{
    auto *btn = new QPushButton(titulo, this);
    btn->setObjectName(QStringLiteral("navItem"));
    btn->setIcon(Theme::emojiIcon(emoji));
    btn->setFlat(true);
    btn->setProperty("ativo", false);
    btn->setCursor(Qt::PointingHandCursor);

    m_itens.insert(chave, btn);
    layout->addWidget(btn);

    const QString chaveCapturada = chave;
    connect(btn, &QPushButton::clicked, this, [this, chaveCapturada]() {
        setAtivo(chaveCapturada);
        emit navegou(chaveCapturada);
    });
}
