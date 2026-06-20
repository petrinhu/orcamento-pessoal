#include "SidebarNav.h"

#include "ui/Theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPalette>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
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
    // objectName necessario para o seletor QWidget#sidebarNav no QSS (#165).
    setObjectName(QStringLiteral("sidebarNav"));

    // O fundo navy e pintado pelo paintEvent (deterministico, independe de
    // QSS/palette/Wayland). Nao usar autoFillBackground=true aqui: com QSS global
    // ativo (Fusion style), o QStyle::polish sobrescreve QPalette::Window com
    // a cor @@BG@@ do tema, anulando o navy. O paintEvent e a unica fonte
    // confiavel do fundo neste contexto.
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);

    // Layout raiz — scroll de itens (top, expande) + footer fixo (bottom).
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(8, 12, 8, 8);
    rootLayout->setSpacing(2);

    // ── Zona de itens: dentro de QScrollArea para rolar quando janela for baixa.
    //
    // Transparencia em cascata: SidebarNav.paintEvent pinta o navy; a QScrollArea
    // e o viewport sao transparentes (WA_NoSystemBackground + autoFillBackground=false);
    // o mainWidget tambem transparente. Assim o navy do paintEvent transpassa ate
    // o conteudo sem ser sobrescrito pelo QSS global (background-color: @@BG@@).
    //
    // Scroll horizontal desligado (sidebar tem largura fixa); vertical as-needed.
    // setWidgetResizable(true) garante que o mainWidget receba o width disponivel
    // (sem isso o sizeHint do widget interno define a largura e pode ficar menor
    // que a sidebar, criando scrollbar horizontal indesejado).
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setObjectName(QStringLiteral("sidebarScrollArea"));
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidgetResizable(true);
    // Transparencia da QScrollArea
    scrollArea->setAttribute(Qt::WA_NoSystemBackground, true);
    scrollArea->setAutoFillBackground(false);
    // Viewport transparente (Qt cria o viewport internamente)
    scrollArea->viewport()->setAttribute(Qt::WA_NoSystemBackground, true);
    scrollArea->viewport()->setAutoFillBackground(false);

    auto *mainWidget = new QWidget(scrollArea);
    mainWidget->setObjectName(QStringLiteral("sidebarMainArea"));
    mainWidget->setAttribute(Qt::WA_NoSystemBackground, true);
    mainWidget->setAutoFillBackground(false);
    m_mainLayout = new QVBoxLayout(mainWidget);
    // Folga lateral 8px: garante que o pill (border-radius:9) mostre os 4 cantos
    // sem encostar na borda esquerda/direita nem no scrollbar (4px, fixo a direita).
    // Sem essa margem o scrollbar sobrepoe o pill quando a area rola (#165 fix).
    m_mainLayout->setContentsMargins(8, 0, 8, 4);
    m_mainLayout->setSpacing(2);
    // Stretch final: mantido permanentemente como ultimo item no m_mainLayout.
    // addItem() usa insertWidget(count()-1) para inserir antes deste stretch,
    // garantindo que os botoes ficam no topo e o espaco vazio expande embaixo.
    // Isso tambem faz o mainWidget nao ser esticado verticalmente pelo QScrollArea
    // quando o conteudo e menor que a viewport.
    m_mainLayout->addStretch(1);

    scrollArea->setWidget(mainWidget);
    rootLayout->addWidget(scrollArea, 1); // stretch=1: ocupa todo o espaco disponivel

    // Separador visual antes do rodape.
    rootLayout->addWidget(criarSeparador(this));

    // Zona de footer: footerItems + bloco de usuario.
    auto *footerWidget = new QWidget(this);
    footerWidget->setObjectName(QStringLiteral("sidebarFooterArea"));
    footerWidget->setAttribute(Qt::WA_NoSystemBackground, true);
    footerWidget->setAutoFillBackground(false);
    m_footerLayout = new QVBoxLayout(footerWidget);
    // Mesma folga lateral do m_mainLayout: pill do footer nao encosta na borda.
    // rootLayout ja tem 8px laterais, entao 0 aqui bastaria; mas como footerWidget
    // e filho direto do rootLayout (nao dentro do scroll area), o rootLayout ja
    // aplica os 8px. Mantemos 0 lateral aqui para nao duplicar — apenas top/bottom.
    m_footerLayout->setContentsMargins(0, 4, 0, 0);
    m_footerLayout->setSpacing(2);
    rootLayout->addWidget(footerWidget);

    // Bloco de usuario (avatar inicial + nome) — fica sempre no fim do footer.
    auto *usuarioWidget = new QWidget(this);
    usuarioWidget->setObjectName(QStringLiteral("sidebarUsuario"));
    usuarioWidget->setAttribute(Qt::WA_NoSystemBackground, true);
    usuarioWidget->setAutoFillBackground(false);
    auto *usuarioLayout = new QHBoxLayout(usuarioWidget);
    usuarioLayout->setContentsMargins(4, 4, 4, 4);
    usuarioLayout->setSpacing(8);

    m_avatarLabel = new QLabel(usuarioWidget);
    m_avatarLabel->setObjectName(QStringLiteral("sidebarAvatar"));
    m_avatarLabel->setFixedSize(28, 28);
    m_avatarLabel->setAlignment(Qt::AlignCenter);
    // Estilo via QSS global (Theme.cpp, seletor QWidget#sidebarNav QLabel#sidebarAvatar).
    // Sem setStyleSheet inline: evita conflito de especificidade com o QSS global (#165).

    m_nomeLabel = new QLabel(usuarioWidget);
    m_nomeLabel->setObjectName(QStringLiteral("sidebarNome"));

    usuarioLayout->addWidget(m_avatarLabel);
    usuarioLayout->addWidget(m_nomeLabel, 1);

    m_footerLayout->addWidget(usuarioWidget);
}

// ── paintEvent ───────────────────────────────────────────────────────────────
//
// Pinta o gradiente navy diretamente com QPainter. Esta abordagem e
// deterministica e funciona sob qualquer plataforma/compositor/style:
//   - QSS background em QWidget customizado e instavel no Fusion/Wayland
//     quando ha stylesheet global ativo (o style re-pinta sobre o QSS).
//   - QPalette::Window tambem falha quando o QStyle::polish sobrescreve
//     a palette com os tokens do tema (@@BG@@).
//   - paintEvent contorna ambos: e chamado pelo framework APOS o style
//     terminar, e o resultado e o pixel final na tela.
//
// Filhos com WA_NoSystemBackground + autoFillBackground=false nao solicitam
// pintura propria, entao o navy aqui pintado transparece ate eles.

void SidebarNav::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QLinearGradient g(0, 0, 0, height());
    g.setColorAt(0.0, QColor(Theme::sidebar1()));
    g.setColorAt(1.0, QColor(Theme::sidebar2()));
    p.fillRect(rect(), g);
}

// ── API publica ───────────────────────────────────────────────────────────────

void SidebarNav::addGrupo(const QString &titulo)
{
    auto *label = new QLabel(titulo.toUpper(), this);
    label->setObjectName(QStringLiteral("grupo"));
    // Margem superior separa o grupo do item anterior.
    label->setContentsMargins(6, 10, 6, 2);
    // Insere antes do stretch final (count()-1) para manter o stretch como
    // ultimo item — isso garante que os grupos ficam empilhados no topo.
    m_mainLayout->insertWidget(m_mainLayout->count() - 1, label);
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
    registrarItem(chave, emoji, titulo, m_footerLayout, posAntesDaUsuario);
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

QString SidebarNav::avatarTexto() const
{
    return m_avatarLabel ? m_avatarLabel->text() : QString();
}

// ── registrarItem (privado) ───────────────────────────────────────────────────

void SidebarNav::registrarItem(const QString &chave, const QString &emoji,
                               const QString &titulo, QVBoxLayout *layout,
                               int indice)
{
    auto *btn = new QPushButton(titulo, this);
    btn->setObjectName(QStringLiteral("navItem"));
    btn->setIcon(Theme::emojiIcon(emoji));
    btn->setFlat(true);
    btn->setProperty("ativo", false);
    btn->setCursor(Qt::PointingHandCursor);

    m_itens.insert(chave, btn);

    if (indice >= 0) {
        // Posicao explicita: usada pelo addFooterItem para inserir antes do usuarioWidget.
        layout->insertWidget(indice, btn);
    } else if (layout == m_mainLayout) {
        // Zona de itens: inserir antes do stretch final (count()-1).
        // O stretch permanece sempre o ultimo item; os botoes empilham acima dele.
        layout->insertWidget(layout->count() - 1, btn);
    } else {
        // Footer layout ou outro: append simples (sem stretch a preservar).
        layout->addWidget(btn);
    }

    const QString chaveCapturada = chave;
    connect(btn, &QPushButton::clicked, this, [this, chaveCapturada]() {
        setAtivo(chaveCapturada);
        emit navegou(chaveCapturada);
    });
}
