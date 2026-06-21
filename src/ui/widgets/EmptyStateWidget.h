#pragma once

// EmptyStateWidget — estado vazio guiado (§9.7 manual de estilo, #166 Peça 2)
//
// Estrutura centralizada:
//   [emoji ~64px via Theme::emojiIcon]
//   [título  — Inter SemiBold 16px, @@TXT@@]
//   [frase   — Inter Regular 13px, @@SUB@@, wrap]
//   [CTA     — QPushButton #primary, opcional]
//
// O CTA opcional deve ser conectado ao MESMO handler do botão de adicionar da
// toolbar — não duplicar a lógica de criação. Passar nullptr para omitir.

#include <QWidget>

class QLabel;
class QPushButton;

class EmptyStateWidget final : public QWidget {
    Q_OBJECT
public:
    // Cria o empty state.
    //   emoji       — glifo Unicode, ex.: "💵"   (via Theme::emojiIcon, 64px)
    //   titulo      — texto curto, SemiBold 16px
    //   frase       — texto de apoio, Regular 13px, multiline ok
    //   ctaLabel    — rótulo do botão CTA; QString() ou vazio = sem botão
    //   parent      — parent widget usual
    explicit EmptyStateWidget(const QString &emoji,
                              const QString &titulo,
                              const QString &frase,
                              const QString &ctaLabel = QString(),
                              QWidget       *parent   = nullptr);

    // Retorna o botão CTA (pode ser nullptr se criado sem CTA).
    // Usar para conectar ao handler existente na toolbar:
    //   connect(m_emptyState->ctaButton(), &QPushButton::clicked,
    //           this, &MeuWidget::onAdicionar);
    QPushButton *ctaButton() const { return m_btnCta; }

    // Acessors para testes de regressão (objectNames definidos no .cpp).
    QLabel      *emojiLabel()  const { return m_lblEmoji; }
    QLabel      *titleLabel()  const { return m_lblTitulo; }
    QLabel      *phraseLabel() const { return m_lblFrase; }

private:
    QLabel      *m_lblEmoji  = nullptr;
    QLabel      *m_lblTitulo = nullptr;
    QLabel      *m_lblFrase  = nullptr;
    QPushButton *m_btnCta    = nullptr;
};
