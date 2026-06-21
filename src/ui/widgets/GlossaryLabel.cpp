#include "GlossaryLabel.h"

#include "GlossaryUnderline.h"
#include "services/GlossarioService.h"

#include <QPaintEvent>
#include <QPainter>

GlossaryLabel::GlossaryLabel(const QString &termo, QWidget *parent)
    : QLabel(termo, parent)
{
    aplicarCue(termo);
}

void GlossaryLabel::aplicarCue(const QString &termo)
{
    // Remove ":" final para lookup (ex.: "CET:" -> "CET").
    QString chave = termo;
    if (chave.endsWith(QLatin1Char(':')))
        chave.chop(1);
    chave = chave.trimmed().toUpper();

    const QString def = GlossarioService::definicao(chave);
    if (def.isEmpty()) {
        // Termo nao cadastrado: sem cue (nao quebrar; apenas nao decorar).
        return;
    }

    setToolTip(def);
    setCursor(Qt::WhatsThisCursor);

    // Marca como já-sublinhado para que GlossaryUnderlineFilter::decorar (chamado
    // por aplicarTooltips numa varredura posterior) NÃO instale um segundo cue —
    // este label já pinta o pontilhado no próprio paintEvent. Evita pintura dupla.
    setProperty("glossarioUnderlined", true);
}

void GlossaryLabel::paintEvent(QPaintEvent *e)
{
    // 1. Pintura normal do QLabel (texto, alinhamento, etc.)
    QLabel::paintEvent(e);

    // 2. Sublinhado pontilhado — so quando o termo foi reconhecido (tooltip setado).
    //    Pintura delegada ao helper compartilhado (DRY: mesma cor/posicao do filtro).
    if (toolTip().isEmpty())
        return;

    QPainter p(this);
    GlossaryUnderline::paint(this, &p);
}
