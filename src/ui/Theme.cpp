#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QStyleHints>
#include <QPalette>

namespace Theme {

// ── Palette: GitHub-inspired, optimised for long-duration use ────────────────
// Light = GitHub Light Default; Dark = GitHub Dark Default
namespace Color {
    // Light
    const QString BgLight       = QStringLiteral("#F6F8FA");
    const QString SurfaceLight  = QStringLiteral("#FFFFFF");
    const QString BorderLight   = QStringLiteral("#D0D7DE");
    const QString TextLight     = QStringLiteral("#1F2328");
    const QString SubtextLight  = QStringLiteral("#656D76");
    const QString AccentLight   = QStringLiteral("#1A7F37");  // muted forest green
    const QString PositiveLight = QStringLiteral("#1A7F37");
    const QString NegativeLight = QStringLiteral("#CF222E");

    // Dark
    const QString BgDark        = QStringLiteral("#0D1117");
    const QString SurfaceDark   = QStringLiteral("#161B22");
    const QString BorderDark    = QStringLiteral("#30363D");
    const QString TextDark      = QStringLiteral("#C9D1D9");  // not pure white — less tiring
    const QString SubtextDark   = QStringLiteral("#8B949E");
    const QString AccentDark    = QStringLiteral("#3FB950");  // soft green for dark bg
    const QString PositiveDark  = QStringLiteral("#3FB950");
    const QString NegativeDark  = QStringLiteral("#F85149");
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

// ── QSS generation ───────────────────────────────────────────────────────────

static void carregarFontes()
{
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-Regular.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-Medium.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Inter-SemiBold.ttf"));
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
    // Alternating row: very subtle — half-way between bg and surface
    const QString alt  = dark ? QStringLiteral("#0D1117") : QStringLiteral("#F0F3F6");

    // %1=txt %2=bg %3=surf %4=acc %5=sub %6=neg %7=brd %8=alt
    return QStringLiteral(R"(
        QWidget {
            font-family: "Inter";
            font-size: 13px;
            color: %1;
            background-color: %2;
        }
        QMainWindow, QDialog {
            background-color: %2;
        }
        QFrame#surface, QGroupBox {
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
        QPushButton#primary:hover  { background-color: %4; }
        QPushButton#primary:pressed { background-color: %4; }
        QPushButton {
            background-color: transparent;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 5px 14px;
            color: %1;
        }
        QPushButton:hover  { border-color: %4; color: %4; }
        QPushButton:pressed { background-color: %3; }
        QPushButton#themeBtn {
            border: none;
            padding: 4px 10px;
            font-size: 12px;
            color: %5;
        }
        QPushButton#themeBtn:checked {
            color: %4;
            font-weight: 600;
            border-bottom: 2px solid %4;
        }
        QPushButton#themeBtn:hover { color: %1; }
        /* ── Inputs ── */
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit {
            background-color: %3;
            border: 1px solid %7;
            border-radius: 6px;
            padding: 5px 8px;
            color: %1;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus,
        QComboBox:focus, QDateEdit:focus {
            border-color: %4;
            outline: none;
        }
        /* ── Table ── */
        QTableWidget {
            background-color: %3;
            alternate-background-color: %8;
            gridline-color: %7;
            border: 1px solid %7;
            border-radius: 6px;
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
        QHeaderView::section:last { border-right: none; }
        QHeaderView::section:hover { color: %4; }
        /* ── Labels ── */
        QLabel#title    { font-size: 16px; font-weight: 600; }
        QLabel#subtitle { font-size: 12px; color: %5; }
        /* ── Status bar ── */
        QStatusBar { background-color: %2; color: %5; border-top: 1px solid %7; }
        /* ── Tabs ── */
        QTabWidget::pane {
            border: 1px solid %7;
            border-top: none;
        }
        QTabBar::tab {
            background: %2;
            color: %5;
            padding: 8px 20px;
            border: 1px solid %7;
            border-bottom: none;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            color: %1;
            background: %3;
            border-bottom: 2px solid %4;
        }
        QTabBar::tab:hover { color: %1; }
        /* ── Scroll bars ── */
        QScrollBar:vertical {
            background: %2; width: 8px; margin: 0;
        }
        QScrollBar:horizontal {
            background: %2; height: 8px; margin: 0;
        }
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: %7; border-radius: 4px; min-length: 20px;
        }
        QScrollBar::handle:hover { background: %5; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
    )")
    .arg(txt, bg, surf, acc, sub, neg, brd, alt);
}

// ── Public API ────────────────────────────────────────────────────────────────

void setMode(ThemeMode mode)
{
    g_mode = mode;
    qApp->setStyleSheet(gerarQss());
}

void aplicar()
{
    carregarFontes();

    QFont font(QStringLiteral("Inter"), 13);
    QApplication::setFont(font);

    qApp->setStyleSheet(gerarQss());

    // Re-apply when OS scheme changes (only matters in Auto mode)
    QObject::connect(
        QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
        qApp, [](Qt::ColorScheme) {
            if (g_mode == ThemeMode::Auto)
                qApp->setStyleSheet(gerarQss());
        }
    );
}

} // namespace Theme
