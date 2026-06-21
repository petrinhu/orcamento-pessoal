#include "GlossaryLabel.h"

#include "services/GlossarioService.h"
#include "ui/Theme.h"

#include <QColor>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>

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
}

void GlossaryLabel::paintEvent(QPaintEvent *e)
{
    // 1. Pintura normal do QLabel (texto, alinhamento, etc.)
    QLabel::paintEvent(e);

    // 2. Sublinhado pontilhado na baseline do texto.
    //    Condicao: so desenha se o tooltip foi setado (= termo cadastrado).
    if (toolTip().isEmpty())
        return;

    const QString txt = text();
    if (txt.isEmpty())
        return;

    const QFontMetrics fm(font());

    // Largura real do texto renderizado (respeita alinhamento parcial).
    const int txtW = fm.horizontalAdvance(txt);

    // Offset horizontal baseado no alinhamento do label.
    int x0 = contentsMargins().left();
    const Qt::Alignment align = alignment();
    if (align & Qt::AlignHCenter)
        x0 = (width() - txtW) / 2;
    else if (align & Qt::AlignRight)
        x0 = width() - txtW - contentsMargins().right();

    // Baseline: centro vertical do contentsRect + ascent do font.
    const QRect cr = contentsRect();
    const int baseline = cr.top() + (cr.height() + fm.ascent() - fm.descent()) / 2;
    // 1px abaixo da baseline = linha pontilhada classica de sublinhado.
    const int y = baseline + 1;

    // Cor sutil: subtexto do tema com alpha 55% — visivel sem agridir.
    QColor cor(Theme::subtexto());
    cor.setAlpha(140); // ~55% alpha

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false); // pontilhado preciso em pixel-grid
    QPen pen(cor, 1, Qt::DotLine);
    pen.setDashPattern({1.0, 2.0}); // 1px ponto, 2px espaco — classico "dotted"
    p.setPen(pen);
    p.drawLine(x0, y, x0 + txtW, y);
}
