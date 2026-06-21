#include "GlossaryUnderline.h"

#include "ui/Theme.h"

#include <QColor>
#include <QFontMetrics>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QRect>

namespace GlossaryUnderline {

void paint(QLabel *lbl, QPainter *painter)
{
    if (!lbl || !painter)
        return;

    const QString txt = lbl->text();
    if (txt.isEmpty())
        return;

    // Rich-text (HTML) nao tem baseline unica trivial — nao decorar para nao
    // riscar no meio do conteudo formatado. Cobertura ampla foca em texto plano.
    if (Qt::mightBeRichText(txt))
        return;

    const QFontMetrics fm(lbl->font());
    const int txtW = fm.horizontalAdvance(txt);
    if (txtW <= 0)
        return;

    // Offset horizontal conforme alinhamento do label.
    const QRect cr = lbl->contentsRect();
    int x0 = cr.left();
    const Qt::Alignment align = lbl->alignment();
    if (align & Qt::AlignHCenter)
        x0 = cr.left() + (cr.width() - txtW) / 2;
    else if (align & Qt::AlignRight)
        x0 = cr.right() - txtW;

    // Clampa o inicio para nao sangrar fora do contentsRect.
    if (x0 < cr.left())
        x0 = cr.left();

    // Baseline vertical: centro do contentsRect + meia-altura tipografica.
    const int baseline = cr.top() + (cr.height() + fm.ascent() - fm.descent()) / 2;
    const int y = baseline + 1; // 1px abaixo da baseline = sublinhado classico

    QPen pen(Theme::glossarioUnderline(), 1, Qt::DotLine);
    // Padrao "dotted" preciso: 1px ponto, 2px espaco (independe do estilo do SO).
    pen.setDashPattern({1.0, 2.0});

    const bool prevAA = painter->testRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::Antialiasing, false); // pontilhado pixel-perfeito
    painter->setPen(pen);
    painter->drawLine(x0, y, x0 + txtW, y);
    painter->setRenderHint(QPainter::Antialiasing, prevAA); // restaura estado
}

} // namespace GlossaryUnderline
