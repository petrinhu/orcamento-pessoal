#pragma once

#include <QString>
#include <QFont>
#include <QPalette>

// Forest Neutral palette + Inter font + QSS.
// Light/dark: respects OS setting (Auto) or overridden via setMode().
namespace Theme {

enum class ThemeMode { Auto, Light, Dark };

void      aplicar();
void      setMode(ThemeMode mode);   // forces light/dark, or restores auto
ThemeMode currentMode();

// Raw tokens
namespace Color {
    // Light
    extern const QString BgLight;
    extern const QString SurfaceLight;
    extern const QString BorderLight;
    extern const QString TextLight;
    extern const QString SubtextLight;
    extern const QString AccentLight;
    extern const QString PositiveLight;
    extern const QString NegativeLight;

    // Dark
    extern const QString BgDark;
    extern const QString SurfaceDark;
    extern const QString BorderDark;
    extern const QString TextDark;
    extern const QString SubtextDark;
    extern const QString AccentDark;
    extern const QString PositiveDark;
    extern const QString NegativeDark;
}

// Convenience getters — return the correct token for the active scheme
QString acento();
QString negativo();
QString positivo();
QString background();
QString surface();
QString texto();
QString borda();

} // namespace Theme
