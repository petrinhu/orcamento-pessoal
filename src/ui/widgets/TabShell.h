#pragma once

#include <QWidget>

class QLabel;
class QToolButton;
class QHBoxLayout;
class QVBoxLayout;
class QFrame;

// Invólucro de layout padrão para uma aba do PeuBud.
//
// De cima para baixo:
//   [ Título ............................. (Barra de ações) ]
//   ───────────────────────────────────────────────────────  (separador)
//   ┌─ Callout Descrição (eyebrow + corpo) ────── [▲/▼] ─┐
//   │ faixa de acento 3px │ "O QUE É ESTA ABA"           │
//   │                     │ texto 15px / 1ª oração Medium  │
//   └────────────────────────────────────────────────────-─┘
//   ───────────────────────────────────────────────────────  (separador)
//   ┌─ Callout Instruções (eyebrow + passos-pílula) ─ [▲/▼] ─┐
//   │ "COMO USAR, PASSO A PASSO"                               │
//   │ 1 passo (pílula azul + texto 14px / divisória tracejada)  │
//   └──────────────────────────────────────────────────────────┘
//   ───────────────────────────────────────────────────────  (separador)
//   [ Conteúdo — a tabela/widget principal da aba ]
//
// Os dois callouts colapsam de forma independente; o toggle (no eyebrow)
// permanece visível mesmo recolhido. Estado persistido por aba + bloco em
// AppSettings (chaves ui/tabhelp/<tabKey>/desc | instr).
//
// Uso típico:
//   auto *shell = new TabShell(tr("Cartões"));
//   shell->setTabKey("tab_cartoes");
//   shell->setContent(parcelasWidget);
//   shell->actionsLayout()->addWidget(btnNovo);
//   shell->applyHelp();   // lê descrição+instruções de tabHelp()
//
// Estilo via QSS — objectNames: helpDescBox, helpInstrBox, helpEyebrow,
// helpDescBody, helpInstrBody, helpPill, helpStepSep (#166).
class TabShell final : public QWidget {
    Q_OBJECT
public:
    explicit TabShell(const QString &titulo, QWidget *parent = nullptr);

    void setTitle(const QString &titulo);

    // Chave estável da aba para persistir o colapso por bloco. Deve ser
    // definida antes de applyHelp()/setDescription()/setInstructions() para o
    // estado restaurar corretamente.
    void setTabKey(const QString &tabKey);
    QString tabKey() const { return m_tabKey; }

    // Descrição (texto corrido, rich-text para ênfases inline). Vazia = bloco oculto.
    void setDescription(const QString &texto);

    // Instruções passo a passo: lista de strings (cada item = 1 passo).
    // Vazia = bloco oculto. Substitui a assinatura antiga de rich-text <ol>.
    void setInstructions(const QStringList &passos);

    // Atalho DRY: preenche título + descrição + instruções a partir do
    // registro central tabHelp() usando o tabKey atual.
    // No-op se a key não existir no registro.
    void applyHelp();

    // Conteúdo principal (tabela/widget existente). Substitui o anterior.
    void setContent(QWidget *content);

    // Barra de ações: a aba adiciona seus botões diretamente neste layout.
    QHBoxLayout *actionsLayout() const { return m_actionsLayout; }

    // Alternativa a actionsLayout(): injeta um container já montado.
    void setActions(QWidget *actions);

private:
    // Monta o contêiner callout de um bloco (eyebrow + toggle + corpo colapsável).
    // bloco = "desc" | "instr" (sufixo da chave QSettings).
    // Retorna o QFrame callout (helpDescBox ou helpInstrBox).
    QFrame *montarCallout(const QString &bloco,
                          QLabel *&outEyebrow,
                          QToolButton *&outToggle,
                          QWidget *&outBody,
                          QFrame *&outSep,
                          QVBoxLayout *root);

    void aplicarColapso(QWidget *body, QToolButton *toggle, bool expandido);

    // Reconstrói o conteúdo do callout de Instruções com a lista de passos.
    void reconstruirPassos(const QStringList &passos);

    QString      m_tabKey;

    QLabel      *m_title         = nullptr;
    QHBoxLayout *m_actionsLayout = nullptr;

    // ── Bloco Descrição ──────────────────────────────────────────────────────
    QFrame      *m_descBox       = nullptr;
    QLabel      *m_descEyebrow   = nullptr;
    QLabel      *m_descBody      = nullptr; // QLabel rich-text (15px)
    QToolButton *m_descToggle    = nullptr;
    QFrame      *m_sepDesc       = nullptr;

    // ── Bloco Instruções ─────────────────────────────────────────────────────
    QFrame      *m_instrBox      = nullptr;
    QLabel      *m_instrEyebrow  = nullptr;
    QWidget     *m_instrBody     = nullptr; // contêiner dos passos-pílula
    QVBoxLayout *m_instrPassosLayout = nullptr;
    QToolButton *m_instrToggle   = nullptr;
    QFrame      *m_sepInstr      = nullptr;

    QVBoxLayout *m_contentSlot   = nullptr;
    QWidget     *m_content       = nullptr;
};
