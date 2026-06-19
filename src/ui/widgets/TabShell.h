#pragma once

#include <QWidget>

class QLabel;
class QHBoxLayout;
class QVBoxLayout;
class QFrame;

// Invólucro de layout padrão para uma aba do PeuBud.
//
// De cima para baixo:
//   [ Título ............................. (Barra de ações) ]
//   ─────────────────────────────────────────────────────────  (separador)
//   Descrição curta (≤ 3 linhas, cinza, com quebra de linha)
//   ─────────────────────────────────────────────────────────  (separador)
//   [ Conteúdo — a tabela/widget principal da aba ]
//
// Uso:
//   auto *shell = new TabShell(tr("Cartões"));
//   shell->setDescription(tr("Texto curto p/ iniciante…"));
//   shell->setContent(parcelasWidget);           // assume ownership via layout
//   shell->actionsLayout()->addWidget(btnNovo);  // ou setActions(container)
//
// Estilo segue o Theme atual: usa os objectNames "title"/"subtitle" do QSS.
class TabShell final : public QWidget {
    Q_OBJECT
public:
    explicit TabShell(const QString &titulo, QWidget *parent = nullptr);

    void setTitle(const QString &titulo);

    // Descrição curta para iniciantes. Vazia = linha oculta (sem separador extra).
    void setDescription(const QString &texto);

    // Conteúdo principal (tabela/widget existente). Substitui o anterior.
    void setContent(QWidget *content);

    // Barra de ações: a aba adiciona seus botões diretamente neste layout.
    QHBoxLayout *actionsLayout() const { return m_actionsLayout; }

    // Alternativa a actionsLayout(): injeta um container já montado.
    void setActions(QWidget *actions);

private:
    QLabel      *m_title       = nullptr;
    QLabel      *m_description  = nullptr;
    QHBoxLayout *m_actionsLayout = nullptr;
    QFrame      *m_sepDesc       = nullptr; // separador acima da descrição
    QVBoxLayout *m_contentSlot   = nullptr; // recebe o widget de conteúdo
    QWidget     *m_content       = nullptr;
};
