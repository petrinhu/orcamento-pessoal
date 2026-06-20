#include "Theme.h"

#include "ButtonShadowFilter.h"

#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPainter>
#include <QPixmap>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QPalette>

namespace Theme {

// ── Palette: azul-confiável (redesign #164) ───────────────────────────────────
namespace Color {
    // Light
    const QString BgLight         = QStringLiteral("#EEF3F9");
    const QString Surface2Light   = QStringLiteral("#F7FAFD");
    const QString SurfaceLight    = QStringLiteral("#FFFFFF");
    const QString BorderLight     = QStringLiteral("#E4EAF2");
    const QString TextLight       = QStringLiteral("#15233B");
    const QString SubtextLight    = QStringLiteral("#67768D");
    const QString AccentLight     = QStringLiteral("#3B82F6");
    const QString PositiveLight   = QStringLiteral("#16C784");
    const QString NegativeLight   = QStringLiteral("#EA3943");
    const QString DouradoLight    = QStringLiteral("#E8A93B");
    const QString Sidebar1Light   = QStringLiteral("#1B2A41");
    const QString Sidebar2Light   = QStringLiteral("#16233A");
    const QString SidebarTextLight = QStringLiteral("#AFBDD0");

    // Dark
    const QString BgDark          = QStringLiteral("#0F172A");
    const QString Surface2Dark    = QStringLiteral("#131A24");
    const QString SurfaceDark     = QStringLiteral("#1B2430");
    const QString BorderDark      = QStringLiteral("#2A3340");
    const QString TextDark        = QStringLiteral("#EAF0F8");
    const QString SubtextDark     = QStringLiteral("#8A97A8");
    const QString AccentDark      = QStringLiteral("#3B82F6");
    const QString PositiveDark    = QStringLiteral("#16C784");
    const QString NegativeDark    = QStringLiteral("#EA3943");
    const QString DouradoDark     = QStringLiteral("#E8A93B");
    // Sidebar dark = mesmo navy do light (tom próprio, não inverte com o tema)
    const QString Sidebar1Dark    = QStringLiteral("#1B2A41");
    const QString Sidebar2Dark    = QStringLiteral("#16233A");
    const QString SidebarTextDark = QStringLiteral("#AFBDD0");
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

QString acento()     { return isDark() ? Color::AccentDark      : Color::AccentLight;      }
QString negativo()   { return isDark() ? Color::NegativeDark    : Color::NegativeLight;    }
QString positivo()   { return isDark() ? Color::PositiveDark    : Color::PositiveLight;    }
QString background() { return isDark() ? Color::BgDark          : Color::BgLight;          }
QString surface()    { return isDark() ? Color::SurfaceDark     : Color::SurfaceLight;     }
QString texto()      { return isDark() ? Color::TextDark        : Color::TextLight;        }
QString borda()      { return isDark() ? Color::BorderDark      : Color::BorderLight;      }
QString subtexto()   { return isDark() ? Color::SubtextDark     : Color::SubtextLight;     }

// Getters novos (#164)
QString bg2()        { return isDark() ? Color::Surface2Dark    : Color::Surface2Light;    }
QString dourado()    { return isDark() ? Color::DouradoDark     : Color::DouradoLight;     }
QString sidebar1()   { return isDark() ? Color::Sidebar1Dark    : Color::Sidebar1Light;    }
QString sidebar2()   { return isDark() ? Color::Sidebar2Dark    : Color::Sidebar2Light;    }
QString sidebarText(){ return isDark() ? Color::SidebarTextDark : Color::SidebarTextLight; }

// Âmbar "atenção" dark-mode-aware. Mesmos tons do antigo helper local do
// Lembretes (fundo de linha "vence em breve"); reaproveitado como fill de barra
// de progresso < 50% no Metas. Pastel em fundo claro, âmbar escuro em dark.
QColor  atencao()
{
    return isDark() ? QColor(QStringLiteral("#8A6D00"))   // âmbar escuro (dark)
                    : QColor(QStringLiteral("#FFF3C4"));  // âmbar pastel (light)
}

// ── Emoji → QIcon ─────────────────────────────────────────────────────────────

QIcon emojiIcon(const QString &emoji, int px)
{
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pm(QSize(px, px) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont f(QStringLiteral("Noto Color Emoji"));
    f.setPixelSize(int(px * 0.86));
    p.setFont(f);
    p.drawText(QRectF(0, 0, px, px), Qt::AlignCenter, emoji);
    p.end();
    return QIcon(pm);
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
    // Alternating row — ligeiramente mais escuro que Surface2 para manter contraste
    const QString alt  = dark ? QStringLiteral("#171F2D") : Color::Surface2Light;
    // Hover background — mais suave que o accent
    const QString hov  = dark ? QStringLiteral("#222D3D") : QStringLiteral("#DCE8FA");
    // Accent tints para feedback de botão: hover = acento mais claro, pressed = mais escuro.
    // Valores fixos derivados do #3B82F6 da nova paleta.
    const QString accHover   = QStringLiteral("#5CC6FA");  // accent +light
    const QString accPressed = QStringLiteral("#1F7AAD");  // accent +dark

    // %1=txt  %2=bg  %3=surf  %4=acc  %5=sub  %6=brd  %7=alt  %8=hov  %9=hov(menu/tab)
    // (neg é substituído nomeadamente, não via %N — brd agora é %6 para manter posições)
    // Botões accent usam @@ACC_HOVER@@ / @@ACC_PRESSED@@: sem colisão com %10/%11.
    // Raio de cards: 14px; inputs/botões: 9px (#164 Fase 1).
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
            border: 1px solid %6;
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
        QMenu::separator { height: 1px; background: %6; margin: 3px 8px; }
        QMenu::indicator { width: 14px; height: 14px; }
        /* ── Frames / surfaces (cards) — raio 14px (#164) ── */
        QFrame#surface {
            background-color: %3;
            border-radius: 14px;
            border: 1px solid %6;
        }
        /* ── Buttons ──────────────────────────────────────────────────────────
           Depth + click feedback ("Shadow" style). O raio passa de 6 → 9px
           (#164 Fase 1). A sombra real vem do ButtonShadowFilter; o QSS
           fornece o *sink* no press — padding assimétrico + fill escurece.
        ── */
        QPushButton#primary {
            background-color: %4;
            color: white;
            border: none;
            border-radius: 9px;
            padding: 6px 18px;
            font-weight: 500;
        }
        QPushButton#primary:hover   { background-color: @@ACC_HOVER@@; }
        QPushButton#primary:pressed {
            background-color: @@ACC_PRESSED@@;
            padding-top: 8px;
            padding-bottom: 4px;
        }
        QPushButton#primary:disabled { background-color: %6; color: %5; }
        QPushButton {
            background-color: %3;
            border: 1px solid %6;
            border-radius: 9px;
            padding: 5px 14px;
            color: %1;
        }
        QPushButton:hover   { border-color: %4; color: %4; }
        QPushButton:pressed {
            background-color: %9;
            border-color: %4;
            padding-top: 7px;
            padding-bottom: 3px;
        }
        QPushButton:disabled { color: %5; border-color: %6; }
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
        /* ── Inputs — raio 9px (#164) ── */
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit {
            background-color: %3;
            border: 1px solid %6;
            border-radius: 9px;
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
            border: 1px solid %6;
            selection-background-color: %4;
            selection-color: white;
        }
        /* ── Table ── */
        QTableWidget {
            background-color: %3;
            alternate-background-color: %8;
            gridline-color: %6;
            border: 1px solid %6;
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
            border-right: 1px solid %6;
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
        QStatusBar { background-color: %2; color: %5; border-top: 1px solid %6; }
        /* ── Tabs ─────────────────────────────────────────────────────────────
           Inactive tabs use bg (recessed); selected tab uses surf (elevated).
           A 2px accent line on top of the selected tab gives clear affordance.
           No gradients — pure colour-contrast depth (BreezeStyleSheets pattern).
        ── */
        QTabWidget::pane {
            background-color: %2;
            border: 1px solid %6;
            border-top: none;
        }
        QTabBar::tab {
            background-color: %2;
            color: %5;
            padding: 7px 20px;
            border: 1px solid %6;
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
        QScrollBar::handle:vertical   { background: %6; border-radius: 4px; min-height: 20px; }
        QScrollBar::handle:horizontal { background: %6; border-radius: 4px; min-width:  20px; }
        QScrollBar::handle:hover { background: %5; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
        /* ── GroupBox ── */
        QGroupBox {
            border: 1px solid %6;
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

    qss.replace(QStringLiteral("@@ACC_HOVER@@"),   accHover);
    qss.replace(QStringLiteral("@@ACC_PRESSED@@"), accPressed);

    return qss;
}

// ── QPalette generation ───────────────────────────────────────────────────────
// Fusion style draws selection/focus using QPalette::Highlight. Without a
// custom palette, it inherits the system palette — which on KDE may be red or
// any colour the user chose. Forcing palette colours here ensures consistent
// rendering regardless of the system palette.
static QPalette gerarPaleta()
{
    const bool dark = isDark();

    const QColor bg   = dark ? QColor(Color::BgDark)      : QColor(Color::BgLight);
    const QColor surf = dark ? QColor(Color::SurfaceDark)  : QColor(Color::SurfaceLight);
    const QColor alt  = dark ? QColor(QStringLiteral("#171F2D")) : QColor(Color::Surface2Light);
    const QColor txt  = dark ? QColor(Color::TextDark)     : QColor(Color::TextLight);
    const QColor sub  = dark ? QColor(Color::SubtextDark)  : QColor(Color::SubtextLight);
    const QColor acc  = QColor(Color::AccentLight);  // accent é igual em light/dark (#3B82F6)

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
    if (qApp) {
        QApplication::setPalette(gerarPaleta());
        qApp->setStyleSheet(gerarQss());
    }
}

void aplicar()
{
    // NoFocusStyle wraps Fusion: gives us consistent cross-platform rendering
    // AND suppresses the PE_FrameFocusRect coloured border on all widgets.
    QApplication::setStyle(
        new NoFocusStyle(QStyleFactory::create(QStringLiteral("Fusion"))));

    // Override the system QPalette so Fusion's Highlight colour (used for
    // selection backgrounds and focus rings) uses the new blue palette.
    QApplication::setPalette(gerarPaleta());

    carregarFontes();

    QFont font(QStringLiteral("Inter"), 13);
    QApplication::setFont(font);

    qApp->setStyleSheet(gerarQss());

    // App-wide drop shadow + press-sink on every QPushButton (covers LoginDialog,
    // which is shown before MainWindow). Idempotent; lives for the app lifetime.
    ButtonShadowFilter::install();

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
