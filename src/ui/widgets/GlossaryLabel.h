#pragma once

#include <QLabel>

// GlossaryLabel (#166 Peca 3 §9.6) — cue estatico de glossario.
//
// QLabel especializado que exibe um termo financeiro com:
//   1. Sublinhado pontilhado desenhado no paintEvent (Qt::DotLine via QPainter).
//      Cor: Theme::subtexto() com alpha 60% — sutil nos 2 temas, nao sobrepoe o
//      texto. Linha na baseline do texto (descida = fontMetrics().descent() - 1px).
//   2. Tooltip escuro setado a partir de GlossarioService::definicao(termo).
//   3. Cursor Qt::WhatsThisCursor (simbolo "?") para indicar "ha informacao aqui".
//
// Garantias:
//   - text() NUNCA e alterado. O widget e nao-destrutivo.
//   - Idempotente: multiplas chamadas ao construtor nao duplicam efeito.
//   - Funciona com QT_QPA_PLATFORM=offscreen (paintEvent usa QPainter, nao
//     efeitos de compositor).
//
// Uso:
//   auto *lbl = new GlossaryLabel(QStringLiteral("CET"), parent);
//   // Substitui: auto *lbl = new QLabel(QStringLiteral("CET:"), parent);
//   // Nota: o ":" pode ser passado como sufixo ou dentro do texto — o widget
//   // busca a definicao pelo texto (stripado de ":" para lookup).
//
// Onde NAO usar:
//   - Texto em prosa/frases longas: usar GlossarioDecorator::aplicarTooltips().
//   - Dentro de QFormLayout como field (campo de entrada): manter QLabel comum +
//     aplicarTooltips() ou aplicarMicrocopies() conforme o caso.
class GlossaryLabel : public QLabel
{
    Q_OBJECT
public:
    // Constroi o label com o termo como texto + cue estatico automatico.
    // termo    = texto exibido (ex.: "CET" ou "CET:").
    //            Para lookup no glossario, ":" final e removido automaticamente.
    // parent   = owner (Qt ownership).
    explicit GlossaryLabel(const QString &termo, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    void aplicarCue(const QString &termo);
};
