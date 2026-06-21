#pragma once

#include <QObject>

class QLabel;

// GlossaryUnderlineFilter (#166 §9.6) — cue estático para cobertura AMPLA.
//
// Permite adicionar o sublinhado pontilhado a um QLabel JÁ EXISTENTE sem trocar
// sua classe (impossível em runtime) e sem mexer no layout. Instala-se como
// event filter no próprio label e, no QEvent::Paint, pinta o pontilhado por cima
// do conteúdo já desenhado — exatamente o que GlossaryLabel faz no paintEvent,
// mas aplicável a qualquer QLabel da varredura do GlossarioDecorator.
//
// Robustez:
//   - Singleton stateless instalado por-label (não em qApp): só intercepta o
//     paint dos labels efetivamente decorados — custo zero no resto do app.
//   - Idempotente: decorar() marca o label com a propriedade dinâmica
//     "glossarioUnderlined"; segunda chamada é no-op (não duplica o filtro).
//   - Não-destrutivo: nunca toca text(); só desenha após QLabel::paintEvent.
//   - Filtra DEPOIS do paint do label (retorna false): a pintura base sempre
//     acontece; o pontilhado é uma camada adicional segura.
class GlossaryUnderlineFilter : public QObject {
    Q_OBJECT
public:
    // Instala o cue no label (idempotente). Caller já validou que o label contém
    // um termo do glossário. Marca "glossarioUnderlined"=true.
    static void decorar(QLabel *lbl);

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    using QObject::QObject;

    // Instância única compartilhada: pinta sempre da mesma forma.
    static GlossaryUnderlineFilter *instance();

    // Guarda de reentrância para o re-despacho síncrono do Paint (single-thread,
    // GUI). Evita recursão infinita ao reenviar o evento ao próprio label.
    bool m_reentrant = false;
};
