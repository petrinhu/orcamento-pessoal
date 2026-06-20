#pragma once

#include <QString>
#include <QFont>
#include <QColor>
#include <QIcon>
#include <QPalette>

// Paleta azul-confiável (redesign Fase 1, #164) + Inter font + QSS.
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
    extern const QString Surface2Light;   // novo (#164)
    extern const QString SurfaceLight;
    extern const QString BorderLight;
    extern const QString TextLight;
    extern const QString SubtextLight;
    extern const QString AccentLight;
    extern const QString PositiveLight;
    extern const QString NegativeLight;
    extern const QString DouradoLight;    // novo (#164)
    extern const QString Sidebar1Light;   // novo (#164)
    extern const QString Sidebar2Light;   // novo (#164)
    extern const QString SidebarTextLight; // novo (#164)

    // Dark
    extern const QString BgDark;
    extern const QString Surface2Dark;    // novo (#164)
    extern const QString SurfaceDark;
    extern const QString BorderDark;
    extern const QString TextDark;
    extern const QString SubtextDark;
    extern const QString AccentDark;
    extern const QString PositiveDark;
    extern const QString NegativeDark;
    extern const QString DouradoDark;     // novo (#164)
    extern const QString Sidebar1Dark;    // novo (#164)
    extern const QString Sidebar2Dark;    // novo (#164)
    extern const QString SidebarTextDark; // novo (#164)
}

// Convenience getters — return the correct token for the active scheme
QString acento();
QString negativo();
QString positivo();
QString background();
QString surface();
QString texto();
QString borda();
QString subtexto(); // alias de subtext para consumidores Qt

// Getters novos (#164 redesign fase 1)
QString bg2();          // Surface2: fundo de cartão secundário
QString dourado();      // âmbar/dourado (avisos e destaques)
QString sidebar1();     // fundo primário da sidebar
QString sidebar2();     // fundo secundário/hover da sidebar
QString sidebarText();  // texto na sidebar

// Âmbar "atenção / vence em breve" — dark-mode-aware, sem equivalente Breeze
// direto. Usado em barras de progresso e fundos de aviso (Metas, Lembretes).
QColor  atencao();

// Renderiza um glifo de emoji da fonte embutida "Noto Color Emoji" num QIcon
// colorido (pixmap transparente, HiDPI-aware). Usar em QPushButton/QToolButton
// via setIcon — QSS não renderiza fonte colorida no texto de botão de forma
// confiável.
QIcon   emojiIcon(const QString &emoji, int px = 16);

// Expõe o QSS gerado para testes de regressão de tokens. Garante que os
// tokens de borda nunca recebam a cor negativa (e vice-versa). Não chamar
// aplicar() antes — só setMode() é suficiente.
QString qssParaTeste();

} // namespace Theme
