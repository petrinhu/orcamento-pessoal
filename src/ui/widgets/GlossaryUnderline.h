#pragma once

class QLabel;
class QPainter;

// GlossaryUnderline (#166 §9.6) — lógica de pintura compartilhada do cue.
//
// Desenha o sublinhado pontilhado (Qt::DotLine, cor Theme::glossarioUnderline())
// sob o texto de um QLabel, posicionado na baseline via fontMetrics e respeitando
// o alinhamento horizontal do label. Não-destrutivo: só pinta, nunca toca text().
//
// Usado por:
//   - GlossaryLabel::paintEvent (pontos curados, ex.: Calculadora)
//   - GlossaryUnderlineFilter::eventFilter (cobertura ampla via varredura)
//
// Mantém a regra de ouro em um único lugar (DRY): cor, posição e padrão do
// pontilhado são idênticos nos dois caminhos.
namespace GlossaryUnderline {

// Pinta o pontilhado sob o texto de `lbl` usando `painter` (já ativo no device
// do label). Assume que o caller decidiu que o label deve ser decorado.
void paint(QLabel *lbl, QPainter *painter);

} // namespace GlossaryUnderline
