#include "TabShell.h"
#include "ui/TabHelp.h"
#include "ui/Theme.h"
#include "core/settings/AppSettings.h"

#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QSizePolicy>

namespace {
constexpr auto kBlocoDesc  = "desc";
constexpr auto kBlocoInstr = "instr";

// Eyebrow fixos (rótulos de metadado, não fazem parte da microcopy de TabHelp).
// Usando u8"..." para garantir codificação UTF-8 correta do "É" (U+00C9).
const QString kEyebrowDescStr  = QStringLiteral("O QUE É ESTA ABA");
const QString kEyebrowInstrStr = QStringLiteral("COMO USAR, PASSO A PASSO");

// Dot de acento antes do eyebrow (ponto 5px, âncora visual).
// Em Qt rich-text não há suporte a ::before CSS. Usamos um prefixo inline.
// O QLabel#helpEyebrow usa cor @@HELP_EYEBROW@@ via QSS.
static QString eyebrowTexto(const QString &rotulo)
{
    // U+25CF BLACK CIRCLE + espaço + rótulo
    // O QSS define a cor e peso; aqui só a composição textual.
    return QStringLiteral("● ") + rotulo;
}

// Gera o texto rich-text da Descrição.
// A spec pede 1ª frase/oração em Medium (500).
// Como QLabel/Qt rich-text não suporta font-weight: 500 inline,
// usamos <b> (bold) apenas na 1ª frase e deixamos o QSS em 400 pra o corpo.
// "1ª frase" = tudo antes do 1º " — " (m-dash da microcopy) ou antes do 1º ponto final.
static QString formatarDescricaoRich(const QString &texto)
{
    if (texto.isEmpty()) return texto;

    // Detecta ponto de quebra: " — " (m-dash com espaços) preferido;
    // senão, primeiro ". " (fim de frase); senão, texto inteiro em Medium.
    const QString separador = QStringLiteral(" — "); // " — "
    int pos = texto.indexOf(separador);

    QString primeiraOracao;
    QString resto;

    if (pos != -1) {
        primeiraOracao = texto.left(pos);
        resto = texto.mid(pos); // inclui o " — "
    } else {
        // Tenta ". " como alternativa
        pos = texto.indexOf(QStringLiteral(". "));
        if (pos != -1) {
            primeiraOracao = texto.left(pos + 1); // inclui o ponto
            resto = texto.mid(pos + 1);
        } else {
            // Texto todo em ênfase
            primeiraOracao = texto;
            resto.clear();
        }
    }

    // Sanitização: escapar < > & para não quebrar o rich-text do Qt
    // (os emojis e texto das microcopy são texto plano; apenas <b> existente
    // na microcopy de tab_cartoes/tab_entradas/... já vem como HTML da TabHelp).
    // Como os textos de TabHelp NÃO contêm HTML na descrição (só nos passos),
    // aplicamos escape manual aqui para a 1ª oração + resto:
    auto esc = [](const QString &s) -> QString {
        QString out = s;
        out.replace(QStringLiteral("&"),  QStringLiteral("&amp;"));
        out.replace(QStringLiteral("<"),  QStringLiteral("&lt;"));
        out.replace(QStringLiteral(">"),  QStringLiteral("&gt;"));
        return out;
    };

    // Constrói rich-text:
    // - <span style="font-weight:600"> para 1ª oração (o QSS define 400 no corpo;
    //   usamos 600 inline para a âncora porque QLabel suporta inline style via Qt HTML)
    // - resto em peso normal (herda do QSS helpDescBody = 400)
    QString rt = QStringLiteral("<html><body style=\"margin:0;padding:0;\">"
                                "<span style=\"font-weight:600;\">%1</span>%2"
                                "</body></html>")
                     .arg(esc(primeiraOracao), esc(resto));
    return rt;
}

} // namespace

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
    m_title->setProperty("glossarioIgnore", true);   // ajuda nao e rotulo de campo
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

    // ── Callout Descrição ───────────────────────────────────────────────────
    {
        QWidget *bodyWidget = nullptr;
        m_descBox = montarCallout(
            QString::fromLatin1(kBlocoDesc),
            m_descEyebrow, m_descToggle,
            bodyWidget, m_sepDesc, root);
        // montarCallout seta m_descBody internamente para a Descrição
        (void)bodyWidget;
    }

    // ── Callout Instruções ──────────────────────────────────────────────────
    {
        QWidget *bodyWidget = nullptr;
        m_instrBox = montarCallout(
            QString::fromLatin1(kBlocoInstr),
            m_instrEyebrow, m_instrToggle,
            bodyWidget, m_sepInstr, root);
        // montarCallout seta m_instrBody internamente para as Instruções
        (void)bodyWidget;
    }

    // ── Slot de conteúdo (#188): envolto em QScrollArea para que a janela
    // possa ser reduzida sem clipar o conteúdo — rola em vez de cortar.
    // Apenas o conteúdo da aba entra no scroll; os callouts ficam acima.
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame); // sem borda extra
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_scrollContent = new QWidget;
    m_scrollContent->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_contentSlot = new QVBoxLayout(m_scrollContent);
    m_contentSlot->setContentsMargins(0, 0, 0, 0);
    m_contentSlot->setSpacing(0);

    m_scrollArea->setWidget(m_scrollContent);
    root->addWidget(m_scrollArea, 1);
}

QFrame *TabShell::montarCallout(const QString &bloco,
                                QLabel *&outEyebrow,
                                QToolButton *&outToggle,
                                QWidget *&outBody,
                                QFrame *&outSep,
                                QVBoxLayout *root)
{
    // ── Callout: QFrame com objectName helpDescBox ou helpInstrBox ──────────
    const bool ehDesc = (bloco == QLatin1String(kBlocoDesc));
    auto *box = new QFrame(this);
    box->setObjectName(ehDesc ? QStringLiteral("helpDescBox")
                              : QStringLiteral("helpInstrBox"));
    box->setProperty("glossarioIgnore", true);

    // (#201, FE-01) Sem setStyleSheet() inline: o QSS global do Theme já cobre
    // #helpDescBox/#helpInstrBox (Theme.cpp) com os mesmos tokens de cor. O
    // inline anterior era avaliado 1× aqui no construtor e MATAVA a cascata —
    // ao trocar de tema (Theme::setMode + qApp->setStyleSheet), os callouts
    // ficavam presos na cor do boot. Deixar o QSS global assumir.

    auto *boxLayout = new QVBoxLayout(box);
    boxLayout->setContentsMargins(13, 10, 13, 12);
    boxLayout->setSpacing(6);

    // ── Barra do eyebrow: rótulo à esquerda, toggle à direita ───────────────
    auto *barraEyebrow = new QHBoxLayout;
    barraEyebrow->setContentsMargins(0, 0, 0, 0);
    barraEyebrow->setSpacing(6);

    outEyebrow = new QLabel(
        eyebrowTexto(ehDesc ? kEyebrowDescStr : kEyebrowInstrStr),
        box);
    outEyebrow->setObjectName(QStringLiteral("helpEyebrow"));
    outEyebrow->setProperty("glossarioIgnore", true);
    // (#201, FE-01) Cor via QSS global (seletor #helpEyebrow) — sem inline
    // (ver comentário acima, mesmo motivo: preservar a cascata de tema).
    // Font: 11px SemiBold (QSS letter-spacing não funciona em Qt; peso via QFont).
    QFont fEyebrow = outEyebrow->font();
    fEyebrow.setPixelSize(11);
    fEyebrow.setWeight(QFont::DemiBold); // 600
    outEyebrow->setFont(fEyebrow);
    barraEyebrow->addWidget(outEyebrow, 1);

    outToggle = new QToolButton(box);
    outToggle->setObjectName(QStringLiteral("iconButton")); // QSS: sem borda
    outToggle->setAutoRaise(true);
    outToggle->setCursor(Qt::PointingHandCursor);
    outToggle->setFocusPolicy(Qt::TabFocus);
    outToggle->setCheckable(false);
    barraEyebrow->addWidget(outToggle, 0, Qt::AlignRight | Qt::AlignVCenter);

    boxLayout->addLayout(barraEyebrow);

    // ── Corpo: criado de forma diferente para Descrição vs Instruções ────────
    if (bloco == QLatin1String(kBlocoDesc)) {
        // Corpo da Descrição: QLabel rich-text
        auto *bodyLabel = new QLabel(box);
        bodyLabel->setObjectName(QStringLiteral("helpDescBody"));
        bodyLabel->setProperty("glossarioIgnore", true);
        bodyLabel->setWordWrap(true);
        bodyLabel->setTextFormat(Qt::RichText);
        bodyLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        bodyLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        // font-size 15px via QFont (QSS define cor e family via seletor)
        QFont fBody = bodyLabel->font();
        fBody.setPixelSize(15);
        bodyLabel->setFont(fBody);
        boxLayout->addWidget(bodyLabel);
        outBody = bodyLabel;
        m_descBody = bodyLabel; // cache tipado
    } else {
        // Corpo das Instruções: QWidget contêiner de passos-pílula
        auto *passosWidget = new QWidget(box);
        passosWidget->setProperty("glossarioIgnore", true);
        m_instrPassosLayout = new QVBoxLayout(passosWidget);
        m_instrPassosLayout->setContentsMargins(0, 4, 0, 2);
        m_instrPassosLayout->setSpacing(0);
        boxLayout->addWidget(passosWidget);
        outBody = passosWidget;
        m_instrBody = passosWidget;
    }

    root->addWidget(box);

    // ── Separador abaixo do callout ─────────────────────────────────────────
    outSep = new QFrame(this);
    outSep->setFrameShape(QFrame::HLine);
    outSep->setFrameShadow(QFrame::Plain);
    root->addWidget(outSep);

    // Estado inicial: oculto até texto ser definido
    box->hide();
    outSep->hide();
    outToggle->hide();

    // Conecta toggle ao colapso do corpo
    QObject::connect(outToggle, &QToolButton::clicked, this, [this, bloco, outBody = outBody, outToggle] {
        const bool novo = !outBody->isVisible();
        aplicarColapso(outBody, outToggle, novo);
        if (!m_tabKey.isEmpty())
            AppSettings::instance().setTabHelpExpanded(m_tabKey, bloco, novo);
    });

    return box;
}

void TabShell::aplicarColapso(QWidget *body, QToolButton *toggle, bool expandido)
{
    body->setVisible(expandido);
    // 🔼 = recolher (está expandido), 🔽 = expandir (está recolhido).
    toggle->setText(QString());
    toggle->setIcon(Theme::emojiIcon(expandido ? QStringLiteral("🔼")
                                                : QStringLiteral("🔽")));
    toggle->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toggle->setToolTip(expandido ? tr("Recolher") : tr("Expandir"));
    toggle->setAccessibleName(expandido ? tr("Recolher bloco") : tr("Expandir bloco"));
}

void TabShell::reconstruirPassos(const QStringList &passos)
{
    if (!m_instrPassosLayout) return;

    // Remove widgets existentes do layout de passos
    QLayoutItem *item;
    while ((item = m_instrPassosLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    for (int i = 0; i < passos.size(); ++i) {
        // ── Divisória tracejada entre passos (não antes do 1º) ─────────────
        if (i > 0) {
            auto *sep = new QFrame(m_instrBody);
            sep->setObjectName(QStringLiteral("helpStepSep"));
            sep->setFrameShape(QFrame::HLine);
            sep->setFrameShadow(QFrame::Plain);
            // A divisória tracejada é feita via QFrame + estilo inline.
            // QSS QFrame#helpStepSep define max-height:1px e background.
            // Para o estilo "dashed", usamos customização pelo setLineWidth:
            sep->setLineWidth(0);
            sep->setMidLineWidth(1);
            m_instrPassosLayout->addWidget(sep);
        }

        // ── Linha do passo: pílula numerada + texto ─────────────────────────
        auto *stepRow = new QWidget(m_instrBody);
        stepRow->setProperty("glossarioIgnore", true);
        auto *rowLayout = new QHBoxLayout(stepRow);
        rowLayout->setContentsMargins(0, 5, 0, 5);
        rowLayout->setSpacing(10);

        // Pílula: QLabel circular com número.
        // Estilo inline garante border-radius no Fusion style (QSS global pode
        // ser sobrescrito por subcontrol em QLabel sem autoFillBackground).
        const QString pillNumBg = Theme::helpNumBg();
        const QString pillNumFg = Theme::helpNumFg();
        auto *pill = new QLabel(QString::number(i + 1), stepRow);
        pill->setObjectName(QStringLiteral("helpPill"));
        pill->setProperty("glossarioIgnore", true);
        pill->setAlignment(Qt::AlignCenter);
        pill->setFixedSize(22, 22);
        pill->setStyleSheet(
            QStringLiteral("QLabel {"
                           "  background-color: %1;"
                           "  color: %2;"
                           "  border-radius: 11px;"
                           "  font-size: 12px;"
                           "  font-weight: 600;"
                           "}")
                .arg(pillNumBg, pillNumFg));
        rowLayout->addWidget(pill, 0, Qt::AlignVCenter | Qt::AlignLeft);

        // Texto do passo: rich-text para suportar <b> já presentes na microcopy
        auto *stepLabel = new QLabel(passos[i], stepRow);
        stepLabel->setObjectName(QStringLiteral("helpInstrBody"));
        stepLabel->setProperty("glossarioIgnore", true);
        stepLabel->setWordWrap(true);
        stepLabel->setTextFormat(Qt::RichText);
        stepLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        stepLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        QFont fStep = stepLabel->font();
        fStep.setPixelSize(14);
        stepLabel->setFont(fStep);
        rowLayout->addWidget(stepLabel, 1);

        m_instrPassosLayout->addWidget(stepRow);
    }
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
    m_descBox->setVisible(temTexto);
    m_descToggle->setVisible(temTexto);
    m_sepDesc->setVisible(temTexto);
    // m_descBody é sempre criado no construtor pelo montarCallout (Desc); o check
    // defensivo abaixo satisfaz o cppcheck e documenta a invariante.
    if (!m_descBody) return;

    if (!temTexto) {
        m_descBody->hide();
        return;
    }

    m_descBody->setText(formatarDescricaoRich(texto));

    const bool expandido = m_tabKey.isEmpty()
        ? true
        : AppSettings::instance().tabHelpExpanded(m_tabKey, QString::fromLatin1(kBlocoDesc));
    aplicarColapso(m_descBody, m_descToggle, expandido);
}

void TabShell::setInstructions(const QStringList &passos)
{
    const bool temPassos = !passos.isEmpty();
    m_instrBox->setVisible(temPassos);
    m_instrToggle->setVisible(temPassos);
    m_sepInstr->setVisible(temPassos);
    // m_instrBody é sempre criado no construtor pelo montarCallout (Instr).
    if (!m_instrBody) return;

    if (!temPassos) {
        m_instrBody->hide();
        return;
    }

    reconstruirPassos(passos);

    const bool expandido = m_tabKey.isEmpty()
        ? true
        : AppSettings::instance().tabHelpExpanded(m_tabKey, QString::fromLatin1(kBlocoInstr));
    aplicarColapso(m_instrBody, m_instrToggle, expandido);
}

void TabShell::applyHelp()
{
    if (m_tabKey.isEmpty()) return;
    const TabHelpEntry e = tabHelpEntry(m_tabKey);
    if (!e.titulo.isEmpty())
        setTitle(e.titulo);
    setDescription(e.descricao);
    setInstructions(e.passos);
}

void TabShell::setContent(QWidget *content)
{
    if (m_content == content) return;
    if (m_content) {
        m_contentSlot->removeWidget(m_content);
        m_content->setParent(nullptr);
        // (#201, FE-03) setParent(nullptr) sozinho não destrói o widget —
        // se setContent for reusado (contrato público prevê), o conteúdo
        // antigo vazava. deleteLater() agenda a destruição no próximo ciclo
        // do event loop (seguro mesmo se algum sinal ainda estiver em voo).
        m_content->deleteLater();
    }
    m_content = content;
    if (m_content) {
        // #188: parent é m_scrollContent (widget interno do QScrollArea),
        // não o TabShell diretamente — assim o layout dentro do scroll o gerencia.
        m_content->setParent(m_scrollContent);
        m_contentSlot->addWidget(m_content);
    }
}

void TabShell::setActions(QWidget *actions)
{
    if (!actions) return;
    m_actionsLayout->addWidget(actions);
}
