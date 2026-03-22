#include "IconGenerator.h"

#include <QPainter>
#include <QPixmap>
#include <QFont>
#include <QFontDatabase>
#include <QLinearGradient>
#include <QPainterPath>

QIcon IconGenerator::generate()
{
    QIcon icon;
    for (const int sz : {16, 32, 48, 64, 128, 256})
        icon.addPixmap(render(sz), QIcon::Normal, QIcon::Off);
    return icon;
}

QPixmap IconGenerator::render(int size)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);

    const qreal m    = size * 0.06;          // margin
    const qreal r    = size * 0.22;          // corner radius
    const QRectF bg(m, m, size - 2*m, size - 2*m);

    // ── Background: gradient green ───────────────────────────────────────
    QLinearGradient grad(bg.topLeft(), bg.bottomLeft());
    grad.setColorAt(0.0, QColor(QStringLiteral("#1E8B45")));
    grad.setColorAt(1.0, QColor(QStringLiteral("#136633")));
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(bg, r, r);

    // ── Letters "Pb" ─────────────────────────────────────────────────────
    QFont font(QStringLiteral("Inter"), 1, QFont::DemiBold);
    font.setPixelSize(qRound(size * 0.46));
    p.setFont(font);
    p.setPen(QColor(255, 255, 255, 245));

    // Vertically & horizontally centered
    p.drawText(bg, Qt::AlignCenter, QStringLiteral("Pb"));

    return pm;
}
