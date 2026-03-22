#pragma once

#include <QString>
#include <QFont>
#include <QPalette>

// Forest Neutral palette + Inter font + QSS.
// Light/dark automatic via QStyleHints (Qt6).
namespace Theme {

void aplicar();

// Raw tokens
namespace Color {
    extern const QString BackgroundLight;
    extern const QString BackgroundDark;
    extern const QString SurfaceLight;
    extern const QString SurfaceDark;
    extern const QString Accent;
    extern const QString AccentDark;
    extern const QString Positive;
    extern const QString PositiveDark;
    extern const QString Negative;
    extern const QString NegativeDark;
    extern const QString TextLight;
    extern const QString TextDark;
    extern const QString SubtextLight;
    extern const QString SubtextDark;
}

// Returns accent color for current scheme
QString acento();
QString negativo();
QString positivo();
QString background();
QString surface();
QString texto();

} // namespace Theme
