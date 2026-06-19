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
//   Descrição (word-wrap, cinza) ....................... [▲/▼]  ← toggle direita
//   ───────────────────────────────────────────────────────  (separador)
//   Instruções (passo a passo) ......................... [▲/▼]  ← toggle direita
//   ───────────────────────────────────────────────────────  (separador)
//   [ Conteúdo — a tabela/widget principal da aba ]
//
// Os dois blocos (Descrição e Instruções) colapsam de forma independente; o
// toggle permanece visível mesmo recolhido. O estado é persistido por aba e
// por bloco em AppSettings (chaves ui/tabhelp/<tabKey>/desc | instr).
//
// Uso típico (texto vem do registro central TabHelp):
//   auto *shell = new TabShell(tr("Cartões"));
//   shell->setTabKey("tab_cartoes");
//   shell->setContent(parcelasWidget);
//   shell->actionsLayout()->addWidget(btnNovo);
//   shell->applyHelp();   // lê descrição+instruções de tabHelp()
//
// Estilo segue o Theme atual: usa os objectNames "title"/"subtitle" do QSS.
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

    // Descrição (texto corrido, rich-text permitido). Vazia = bloco oculto.
    void setDescription(const QString &texto);

    // Instruções passo a passo (rich-text; ex.: "<ol><li>…</li></ol>" ou texto
    // com <br>). Vazia = bloco oculto.
    void setInstructions(const QString &textoRich);

    // Atalho DRY: preenche título + descrição + instruções a partir do
    // registro central tabHelp() usando o tabKey atual. Formata os passos como
    // lista ordenada <ol>. No-op se a key não existir no registro.
    void applyHelp();

    // Conteúdo principal (tabela/widget existente). Substitui o anterior.
    void setContent(QWidget *content);

    // Barra de ações: a aba adiciona seus botões diretamente neste layout.
    QHBoxLayout *actionsLayout() const { return m_actionsLayout; }

    // Alternativa a actionsLayout(): injeta um container já montado.
    void setActions(QWidget *actions);

private:
    // Cria um bloco colapsável (texto + toggle ▲/▼ alinhado à direita + um
    // separador abaixo). bloco = "desc" | "instr" (sufixo da chave QSettings).
    void montarBloco(const QString &bloco, QLabel *&outLabel, QToolButton *&outToggle,
                     QFrame *&outSep, QVBoxLayout *root);
    void aplicarColapso(QLabel *label, QToolButton *toggle, bool expandido);

    QString      m_tabKey;

    QLabel      *m_title        = nullptr;
    QHBoxLayout *m_actionsLayout = nullptr;

    QLabel      *m_description   = nullptr;
    QToolButton *m_descToggle    = nullptr;
    QFrame      *m_sepDesc       = nullptr;

    QLabel      *m_instructions  = nullptr;
    QToolButton *m_instrToggle   = nullptr;
    QFrame      *m_sepInstr      = nullptr;

    QVBoxLayout *m_contentSlot   = nullptr;
    QWidget     *m_content       = nullptr;
};
