#include "Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QPalette>

namespace Theme {

// ── Palette: Forest Neutral — soft, friendly, low-fatigue ─────────────────────
namespace Color {
    // Light
    const QString BgLight       = QStringLiteral("#F5F4F0");
    const QString SurfaceLight  = QStringLiteral("#FFFFFF");
    const QString BorderLight   = QStringLiteral("#D8E0D9");
    const QString TextLight     = QStringLiteral("#1A2E22");
    const QString SubtextLight  = QStringLiteral("#667A6E");
    const QString AccentLight   = QStringLiteral("#166F4A");
    const QString PositiveLight = QStringLiteral("#166F4A");
    const QString NegativeLight = QStringLiteral("#C0392B");

    // Dark
    const QString BgDark        = QStringLiteral("#181A17");
    const QString SurfaceDark   = QStringLiteral("#1E2420");
    const QString BorderDark    = QStringLiteral("#2B3830");
    const QString TextDark      = QStringLiteral("#D8E0D8");
    const QString SubtextDark   = QStringLiteral("#7A9485");
    const QString AccentDark    = QStringLiteral("#22A367");
    const QString PositiveDark  = QStringLiteral("#22A367");
    const QString NegativeDark  = QStringLiteral("#E05555");
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
    // Alternating row — very subtle
    const QString alt  = dark ? QStringLiteral("#1B2118") : QStringLiteral("#F0F4F0");
    // Hover background — slightly lighter than bg, used for subtle hover states
    const QString hov  = dark ? QStringLiteral("#252C22") : QStringLiteral("#ECEEE9");

    // %1=txt  %2=bg  %3=surf  %4=acc  %5=sub  %6=neg  %7=brd  %8=alt  %9=hov
    QString qss = QStringLiteral(R"(
        /* ── Base ── */
        QWidget {
            font-family: "Inter";
            font-size: 13px;
            color: %1;
            background-color: %2;
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
        QHeaderView::section:last { border-right: none; }
        QHeaderView::section:hover { color: %4; }
        /* ── Labels ── */
        QLabel#title    { font-size: 16px; font-weight: 600; }
        QLabel#subtitle { font-size: 12px; color: %5; }
        /* ── Status bar ── */
        QStatusBar { background-color: %2; color: %5; border-top: 1px solid %7; }
        /* ── Tabs ─────────────────────────────────────────────────────────────
           Inactive tabs use bg (recessed); selected tab uses surf (elevated).
           A 2px accent line on top of the selected tab gives clear affordance.
           No gradients — pure colour-contrast depth (BreezeStyleSheets pattern).
        ── */
        QTabWidget::pane {
            background-color: %3;
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
            background-color: %3;
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
        QScrollBar::handle:vertical, QScrollBar::handle:horizontal {
            background: %7; border-radius: 4px; min-length: 20px;
        }
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
    qApp->setStyleSheet(gerarQss());
}

void aplicar()
{
    // NoFocusStyle wraps Fusion: gives us consistent cross-platform rendering
    // AND suppresses the PE_FrameFocusRect coloured border on all widgets.
    QApplication::setStyle(
        new NoFocusStyle(QStyleFactory::create(QStringLiteral("Fusion"))));

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
