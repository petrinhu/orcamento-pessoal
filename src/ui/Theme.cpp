#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QPalette>

namespace Theme {

// ── Palette: KDE Breeze Light / Breeze Dark ───────────────────────────────────
namespace Color {
    // Light — Breeze Light
    const QString BgLight       = QStringLiteral("#EFF0F1");
    const QString SurfaceLight  = QStringLiteral("#FFFFFF");
    const QString BorderLight   = QStringLiteral("#BDC3C7");
    const QString TextLight     = QStringLiteral("#232629");
    const QString SubtextLight  = QStringLiteral("#7F8C8D");
    const QString AccentLight   = QStringLiteral("#3DAEE9");
    const QString PositiveLight = QStringLiteral("#27AE60");
    const QString NegativeLight = QStringLiteral("#DA4453");

    // Dark — Breeze Dark
    const QString BgDark        = QStringLiteral("#31363B");
    const QString SurfaceDark   = QStringLiteral("#1B1E20");
    const QString BorderDark    = QStringLiteral("#575B5F");
    const QString TextDark      = QStringLiteral("#EFF0F1");
    const QString SubtextDark   = QStringLiteral("#7F8C8D");
    const QString AccentDark    = QStringLiteral("#3DAEE9");
    const QString PositiveDark  = QStringLiteral("#27AE60");
    const QString NegativeDark  = QStringLiteral("#DA4453");
}

// ── ThemeMode state ───────────────────────────────────────────────────────────

static ThemeMode g_mode = ThemeMode::Auto;

static bool isDark()
{
    switch (g_mode) {
        case ThemeMode::Light: return false;
        case ThemeMode::Dark:  return true;
        case ThemeMode::Auto:
        default:
            return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    }
}

ThemeMode currentMode() { return g_mode; }

// ── Token getters ─────────────────────────────────────────────────────────────

QString acento()     { return isDark() ? Color::AccentDark    : Color::AccentLight;    }
QString negativo()   { return isDark() ? Color::NegativeDark  : Color::NegativeLight;  }
QString positivo()   { return isDark() ? Color::PositiveDark  : Color::PositiveLight;  }
QString background() { return isDark() ? Color::BgDark        : Color::BgLight;        }
QString surface()    { return isDark() ? Color::SurfaceDark   : Color::SurfaceLight;   }
QString texto()      { return isDark() ? Color::TextDark      : Color::TextLight;      }
QString borda()      { return isDark() ? Color::BorderDark    : Color::BorderLight;    }

// Âmbar "atenção" dark-mode-aware. Mesmos tons do antigo helper local do
// Lembretes (fundo de linha "vence em breve"); reaproveitado como fill de barra
// de progresso < 50% no Metas. Pastel em fundo claro, âmbar escuro em dark.
QColor  atencao()
{
    return isDark() ? QColor(QStringLiteral("#8A6D00"))   // âmbar escuro (dark)
                    : QColor(QStringLiteral("#FFF3C4"));  // âmbar pastel (light)
}

// ── QSS generation ───────────────────────────────────────────────────────────

static void carregarFontes()
{
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-Regular.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-Medium.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-SemiBold.ttf"));
    // Noto Color Emoji (subset, COLRv1) embutida: garante emoji colorido em
    // qualquer distro, sem depender da fonte de emoji do sistema.
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/NotoColorEmoji.ttf"));
}

static QString gerarQss()
{
    const bool dark = isDark();
    const QString bg   = dark ? Color::BgDark       : Color::BgLight;
    const QString surf = dark ? Color::SurfaceDark  : Color::SurfaceLight;
    const QString brd  = dark ? Color::BorderDark   : Color::BorderLight;
    const QString acc  = dark ? Color::AccentDark   : Color::AccentLight;
    const QString txt  = dark ? Color::TextDark     : Color::TextLight;
    const QString sub  = dark ? Color::SubtextDark  : Color::SubtextLight;
    const QString neg  = dark ? Color::NegativeDark : Color::NegativeLight;
    // Alternating row
    const QString alt  = dark ? QStringLiteral("#232629") : QStringLiteral("#F5F5F5");
    // Hover background
    const QString hov  = dark ? QStringLiteral("#3B4045") : QStringLiteral("#D0E9F6");

    // %1=txt  %2=bg  %3=surf  %4=acc  %5=sub  %6=brd  %7=alt  %8=hov
    // (neg is unused in QSS — multiArg skips absent markers, causing drift)
    QString qss = QStringLiteral(R"(
        /* ── Base ── */
        QWidget {
            font-family: "Inter";
            font-size: 13px;
            color: %1;
            background-color: %2;
            outline: 0;
        }
        QMainWindow, QDialog {
            background-color: %2;
        }
        /* ── Menu bar ── */
        QMenuBar {
            background-color: %2;
            border-bottom: 1px solid %7;
            padding: 1px 0;
        }
        QMenuBar::item {
            padding: 4px 10px;
            background: transparent;
            border-radius: 4px;
        }
        QMenuBar::item:selected { background-color: %9; }
        QMenuBar::item:pressed  { background-color: %7; }
        /* ── Menus ── */
        QMenu {
            background-color: %3;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 4px 0;
        }
        QMenu::item {
            padding: 5px 28px 5px 16px;
            border-radius: 4px;
            margin: 1px 4px;
        }
        QMenu::item:selected { background-color: %4; color: white; }
        QMenu::item:disabled { color: %5; }
        QMenu::separator { height: 1px; background: %7; margin: 3px 8px; }
        QMenu::indicator { width: 14px; height: 14px; }
        /* ── Frames / surfaces ── */
        QFrame#surface {
            background-color: %3;
            border-radius: 8px;
            border: 1px solid %7;
        }
        /* ── Buttons ── */
        QPushButton#primary {
            background-color: %4;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 6px 18px;
            font-weight: 500;
        }
        QPushButton#primary:hover   { background-color: %4; }
        QPushButton#primary:pressed { background-color: %4; }
        QPushButton {
            background-color: %3;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 5px 14px;
            color: %1;
        }
        QPushButton:hover   { border-color: %4; color: %4; }
        QPushButton:pressed { background-color: %9; }
        QPushButton:disabled { color: %5; border-color: %7; }
        /* ── Theme cycle button ── */
        QToolButton#themeBtn {
            border: none;
            padding: 4px 8px;
            border-radius: 4px;
            color: %5;
            font-size: 14px;
            background: transparent;
        }
        QToolButton#themeBtn:hover   { background-color: %9; color: %1; }
        QToolButton#themeBtn:pressed { background-color: %7; }
        /* ── Inputs ── */
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit {
            background-color: %3;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 5px 8px;
            color: %1;
            selection-background-color: %4;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus,
        QComboBox:focus, QDateEdit:focus {
            border-color: %4;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox::down-arrow {
            width: 10px;
            height: 10px;
        }
        QComboBox QAbstractItemView {
            background: %3;
            border: 1px solid %7;
            selection-background-color: %4;
            selection-color: white;
        }
        /* ── Table ── */
        QTableWidget {
            background-color: %3;
            alternate-background-color: %8;
            gridline-color: %7;
            border: 1px solid %7;
            border-radius: 0px;
        }
        QTableWidget::item {
            padding: 4px 8px;
            min-height: 34px;
            border: none;
        }
        QTableWidget::item:selected {
            background-color: %4;
            color: white;
        }
        QHeaderView {
            background-color: %2;
        }
        QHeaderView::section {
            background-color: %2;
            border: none;
            border-right: 1px solid %7;
            border-bottom: 2px solid %4;
            padding: 6px 8px;
            font-weight: 600;
            color: %1;
            font-size: 12px;
        }
        QHeaderView::section:hover { color: %4; }
        /* ── Labels ── */
        QLabel#title    { font-size: 16px; font-weight: 600; }
        QLabel#subtitle { font-size: 12px; color: %5; }
        QLabel#error    { font-size: 12px; color: %6; }
        /* ── Status bar ── */
        QStatusBar { background-color: %2; color: %5; border-top: 1px solid %7; }
        /* ── Tabs ─────────────────────────────────────────────────────────────
           Inactive tabs use bg (recessed); selected tab uses surf (elevated).
           A 2px accent line on top of the selected tab gives clear affordance.
           No gradients — pure colour-contrast depth (BreezeStyleSheets pattern).
        ── */
        QTabWidget::pane {
            background-color: %2;
            border: 1px solid %7;
            border-top: none;
        }
        QTabBar::tab {
            background-color: %2;
            color: %5;
            padding: 7px 20px;
            border: 1px solid %7;
            border-bottom: none;
            border-top-left-radius: 5px;
            border-top-right-radius: 5px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: %2;
            color: %1;
            border-top: 2px solid %4;
            margin-top: -1px;
        }
        QTabBar::tab:hover:!selected { background-color: %9; color: %1; }
        /* ── Scroll bars ── */
        QScrollBar:vertical {
            background: %2; width: 8px; margin: 0;
        }
        QScrollBar:horizontal {
            background: %2; height: 8px; margin: 0;
        }
        QScrollBar::handle:vertical   { background: %7; border-radius: 4px; min-height: 20px; }
        QScrollBar::handle:horizontal { background: %7; border-radius: 4px; min-width:  20px; }
        QScrollBar::handle:hover { background: %5; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
        /* ── GroupBox ── */
        QGroupBox {
            border: 1px solid %7;
            border-radius: 6px;
            margin-top: 1.2em;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 6px;
            color: %5;
            font-size: 11px;
            font-weight: 600;
        }
        /* ── Dialog button boxes ── */
        QDialogButtonBox QPushButton { min-width: 80px; }
    )")
    .arg(txt, bg, surf, acc, sub, neg, brd, alt, hov);

    return qss;
}

// ── QPalette generation ───────────────────────────────────────────────────────
// Fusion style draws selection/focus using QPalette::Highlight. Without a
// custom palette, it inherits the system palette — which on KDE may be red or
// any colour the user chose. Forcing Breeze colours here ensures consistent
// rendering regardless of the system palette.
static QPalette gerarPaleta()
{
    const bool dark = isDark();

    const QColor bg   = dark ? QColor(QStringLiteral("#31363B")) : QColor(QStringLiteral("#EFF0F1"));
    const QColor surf = dark ? QColor(QStringLiteral("#1B1E20")) : QColor(QStringLiteral("#FFFFFF"));
    const QColor alt  = dark ? QColor(QStringLiteral("#232629")) : QColor(QStringLiteral("#F5F5F5"));
    const QColor txt  = dark ? QColor(QStringLiteral("#EFF0F1")) : QColor(QStringLiteral("#232629"));
    const QColor sub  = QColor(QStringLiteral("#7F8C8D"));
    const QColor acc  = QColor(QStringLiteral("#3DAEE9"));   // Breeze Blue

    QPalette pal;
    pal.setColor(QPalette::Window,          bg);
    pal.setColor(QPalette::WindowText,      txt);
    pal.setColor(QPalette::Base,            surf);
    pal.setColor(QPalette::AlternateBase,   alt);
    pal.setColor(QPalette::Text,            txt);
    pal.setColor(QPalette::Button,          bg);
    pal.setColor(QPalette::ButtonText,      txt);
    pal.setColor(QPalette::Highlight,       acc);
    pal.setColor(QPalette::HighlightedText, Qt::white);
    pal.setColor(QPalette::Link,            acc);
    pal.setColor(QPalette::LinkVisited,     acc);
    // Disabled
    pal.setColor(QPalette::Disabled, QPalette::WindowText, sub);
    pal.setColor(QPalette::Disabled, QPalette::Text,       sub);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, sub);

    return pal;
}

// ── NoFocusStyle ──────────────────────────────────────────────────────────────
// Fusion draws PE_FrameFocusRect as a coloured dashed rectangle around the
// focused widget. QSS `outline: none` only suppresses it on a few widget types;
// the reliable fix is to intercept it at the style level.
class NoFocusStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    void drawPrimitive(QStyle::PrimitiveElement pe,
                       const QStyleOption *opt,
                       QPainter *p,
                       const QWidget *w = nullptr) const override
    {
        if (pe == QStyle::PE_FrameFocusRect) return;
        QProxyStyle::drawPrimitive(pe, opt, p, w);
    }
};

// ── Public API ────────────────────────────────────────────────────────────────

void setMode(ThemeMode mode)
{
    g_mode = mode;
    QApplication::setPalette(gerarPaleta());
    qApp->setStyleSheet(gerarQss());
}

void aplicar()
{
    // NoFocusStyle wraps Fusion: gives us consistent cross-platform rendering
    // AND suppresses the PE_FrameFocusRect coloured border on all widgets.
    QApplication::setStyle(
        new NoFocusStyle(QStyleFactory::create(QStringLiteral("Fusion"))));

    // Override the system QPalette so Fusion's Highlight colour (used for
    // selection backgrounds and focus rings) uses Breeze Blue, not whatever
    // the user has set in KDE System Settings.
    QApplication::setPalette(gerarPaleta());

    carregarFontes();

    QFont font(QStringLiteral("Inter"), 13);
    QApplication::setFont(font);

    qApp->setStyleSheet(gerarQss());

    // Re-apply when OS scheme changes (only matters in Auto mode)
    QObject::connect(
        QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
        qApp, [](Qt::ColorScheme) {
            if (g_mode == ThemeMode::Auto) {
                QApplication::setPalette(gerarPaleta());
                qApp->setStyleSheet(gerarQss());
            }
        }
    );
}

} // namespace Theme
