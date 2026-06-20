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
    // Tons coerentes com o accent #3B82F6 da nova paleta (#164 Fase 1 fix):
    //   hover   = #60A5FA (blue-400 Tailwind — mais claro que o base)
    //   pressed = #2563EB (blue-600 Tailwind — mais escuro que o base)
    const QString accHover   = QStringLiteral("#60A5FA");  // accent +light
    const QString accPressed = QStringLiteral("#2563EB");  // accent +dark

    // Tokens sidebar (#165): navy independente de light/dark (tom proprio da sidebar).
    const QString sb1  = sidebar1();
    const QString sb2  = sidebar2();
    const QString sbtx = sidebarText();

    // Tokens nomeados: eliminam o esquema posicional %N que causou 2 bugs
    // (hover e agora neg/brd trocados). Cada @@TOKEN@@ tem semântica inequívoca.
    // @@TXT@@ @@BG@@ @@SURF@@ @@ACC@@ @@SUB@@ @@NEG@@ @@BRD@@ @@ALT@@ @@HOV@@
    // @@ACC_HOVER@@ @@ACC_PRESSED@@
    // @@SIDEBAR1@@ @@SIDEBAR2@@ @@SIDEBARTXT@@ (sidebar, fixos independente de tema)
    // Raio de cards: 14px; inputs/botões: 9px (#164 Fase 1).
    QString qss = QStringLiteral(R"(
        /* ── Base ── */
        QWidget {
            font-family: "Inter";
            font-size: 13px;
            color: @@TXT@@;
            background-color: @@BG@@;
            outline: 0;
        }
        QMainWindow, QDialog {
            background-color: @@BG@@;
        }
        /* ── Menu bar ── */
        QMenuBar {
            background-color: @@BG@@;
            border-bottom: 1px solid @@BRD@@;
            padding: 1px 0;
        }
        QMenuBar::item {
            padding: 4px 10px;
            background: transparent;
            border-radius: 4px;
        }
        QMenuBar::item:selected { background-color: @@HOV@@; }
        QMenuBar::item:pressed  { background-color: @@ALT@@; }
        /* ── Menus ── */
        QMenu {
            background-color: @@SURF@@;
            border: 1px solid @@BRD@@;
            border-radius: 6px;
            padding: 4px 0;
        }
        QMenu::item {
            padding: 5px 28px 5px 16px;
            border-radius: 4px;
            margin: 1px 4px;
        }
        QMenu::item:selected { background-color: @@ACC@@; color: white; }
        QMenu::item:disabled { color: @@SUB@@; }
        QMenu::separator { height: 1px; background: @@BRD@@; margin: 3px 8px; }
        QMenu::indicator { width: 14px; height: 14px; }
        /* ── Frames / surfaces (cards) — raio 14px (#164) ── */
        QFrame#surface {
            background-color: @@SURF@@;
            border-radius: 14px;
            border: 1px solid @@BRD@@;
        }
        /* ── Buttons ──────────────────────────────────────────────────────────
           Depth + click feedback ("Shadow" style). O raio passa de 6 → 9px
           (#164 Fase 1). A sombra real vem do ButtonShadowFilter; o QSS
           fornece o *sink* no press — padding assimétrico + fill escurece.
        ── */
        QPushButton#primary {
            background-color: @@ACC@@;
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
        QPushButton#primary:disabled { background-color: @@BRD@@; color: @@SUB@@; }
        QPushButton {
            background-color: @@SURF@@;
            border: 1px solid @@BRD@@;
            border-radius: 9px;
            padding: 5px 14px;
            color: @@TXT@@;
        }
        QPushButton:hover   { border-color: @@ACC@@; color: @@ACC@@; }
        QPushButton:pressed {
            background-color: @@HOV@@;
            border-color: @@ACC@@;
            padding-top: 7px;
            padding-bottom: 3px;
        }
        QPushButton:disabled { color: @@SUB@@; border-color: @@BRD@@; }
        /* ── Theme cycle button ── */
        QToolButton#themeBtn {
            border: none;
            padding: 4px 8px;
            border-radius: 4px;
            color: @@SUB@@;
            font-size: 14px;
            background: transparent;
        }
        QToolButton#themeBtn:hover   { background-color: @@HOV@@; color: @@TXT@@; }
        QToolButton#themeBtn:pressed { background-color: @@ALT@@; }
        /* ── Inputs — raio 9px (#164) ── */
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QDateEdit {
            background-color: @@SURF@@;
            border: 1px solid @@BRD@@;
            border-radius: 9px;
            padding: 5px 8px;
            color: @@TXT@@;
            selection-background-color: @@ACC@@;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus,
        QComboBox:focus, QDateEdit:focus {
            border-color: @@ACC@@;
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
            background: @@SURF@@;
            border: 1px solid @@BRD@@;
            selection-background-color: @@ACC@@;
            selection-color: white;
        }
        /* ── Table ── */
        QTableWidget {
            background-color: @@SURF@@;
            alternate-background-color: @@ALT@@;
            gridline-color: @@BRD@@;
            border: 1px solid @@BRD@@;
            border-radius: 0px;
        }
        QTableWidget::item {
            padding: 4px 8px;
            min-height: 34px;
            border: none;
        }
        QTableWidget::item:selected {
            background-color: @@ACC@@;
            color: white;
        }
        QHeaderView {
            background-color: @@BG@@;
        }
        QHeaderView::section {
            background-color: @@BG@@;
            border: none;
            border-right: 1px solid @@BRD@@;
            border-bottom: 2px solid @@ACC@@;
            padding: 6px 8px;
            font-weight: 600;
            color: @@TXT@@;
            font-size: 12px;
        }
        QHeaderView::section:hover { color: @@ACC@@; }
        /* ── Labels ── */
        QLabel#title    { font-size: 16px; font-weight: 600; }
        QLabel#subtitle { font-size: 12px; color: @@SUB@@; }
        QLabel#error    { font-size: 12px; color: @@NEG@@; }
        /* ── Status bar ── */
        QStatusBar { background-color: @@BG@@; color: @@SUB@@; border-top: 1px solid @@BRD@@; }
        /* ── Tabs ─────────────────────────────────────────────────────────────
           Inactive tabs use bg (recessed); selected tab uses surf (elevated).
           A 2px accent line on top of the selected tab gives clear affordance.
           No gradients — pure colour-contrast depth (BreezeStyleSheets pattern).
        ── */
        QTabWidget::pane {
            background-color: @@BG@@;
            border: 1px solid @@BRD@@;
            border-top: none;
        }
        QTabBar::tab {
            background-color: @@BG@@;
            color: @@SUB@@;
            padding: 7px 20px;
            border: 1px solid @@BRD@@;
            border-bottom: none;
            border-top-left-radius: 5px;
            border-top-right-radius: 5px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: @@BG@@;
            color: @@TXT@@;
            border-top: 2px solid @@ACC@@;
            margin-top: -1px;
        }
        QTabBar::tab:hover:!selected { background-color: @@HOV@@; color: @@TXT@@; }
        /* ── Scroll bars ── */
        QScrollBar:vertical {
            background: @@BG@@; width: 8px; margin: 0;
        }
        QScrollBar:horizontal {
            background: @@BG@@; height: 8px; margin: 0;
        }
        QScrollBar::handle:vertical   { background: @@BRD@@; border-radius: 4px; min-height: 20px; }
        QScrollBar::handle:horizontal { background: @@BRD@@; border-radius: 4px; min-width:  20px; }
        QScrollBar::handle:hover { background: @@SUB@@; }
        QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
        /* ── GroupBox ── */
        QGroupBox {
            border: 1px solid @@BRD@@;
            border-radius: 6px;
            margin-top: 1.2em;
            padding-top: 8px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 6px;
            color: @@SUB@@;
            font-size: 11px;
            font-weight: 600;
        }
        /* ── Dialog button boxes ── */
        QDialogButtonBox QPushButton { min-width: 80px; }

        /* ── Sidebar navy (#165 redesign Fase 2 Task 6) ─────────────────────
           Gradiente navy do topo (@@SIDEBAR1@@) ao fundo (@@SIDEBAR2@@).
           Largura fixa 230px garante coluna lateral estavel sem encolher.
           Separador: linha sutil entre zona de itens e footer.
        ── */
        QWidget#sidebarNav {
            background-color: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                stop:0 @@SIDEBAR1@@, stop:1 @@SIDEBAR2@@);
            min-width: 230px;
            max-width: 230px;
        }
        /* Filhos e descendentes da sidebar: fundo transparente para gradiente aparecer */
        QWidget#sidebarNav > QWidget,
        QWidget#sidebarNav QWidget#sidebarUsuario {
            background-color: transparent;
        }
        QFrame#sidebarSep {
            background: rgba(255,255,255,0.08);
            max-height: 1px;
            border: none;
        }
        /* Rotulo de grupo: dim em relacao ao sidebarText; maiusculas via codigo */
        QWidget#sidebarNav QLabel#grupo {
            color: rgba(175,189,208,0.55);
            font-size: 10px;
            font-weight: 600;
            letter-spacing: 1px;
            padding: 12px 10px 4px;
            background: transparent;
        }
        /* Itens de navegacao: fundo transparente por padrao */
        QWidget#sidebarNav QPushButton#navItem {
            color: @@SIDEBARTXT@@;
            background: transparent;
            border: none;
            text-align: left;
            padding: 9px 11px;
            border-radius: 9px;
            font-weight: 500;
            font-size: 13px;
        }
        QWidget#sidebarNav QPushButton#navItem:hover {
            background: rgba(255,255,255,0.06);
            color: white;
        }
        /* Pill ativo: accent + sombra azul (property dinamica setada por setAtivo()) */
        QWidget#sidebarNav QPushButton#navItem[ativo="true"] {
            background: @@ACC@@;
            color: white;
            font-weight: 600;
        }
        /* Avatar (circulo inicial) e nome do usuario no rodape */
        QWidget#sidebarNav QLabel#sidebarAvatar {
            background: @@ACC@@;
            color: white;
            border-radius: 14px;
            font-weight: 600;
            font-size: 13px;
        }
        QWidget#sidebarNav QLabel#sidebarNome {
            color: @@SIDEBARTXT@@;
            font-size: 13px;
            font-weight: 500;
            background: transparent;
        }
    )");

    // Substituição de tokens nomeados → valores da paleta. Ordem não importa
    // (cada token é único no literal). Eliminado o .arg() posicional que causou
    // bugs de troca neg↔brd e alt↔hov (#164 fix).
    qss.replace(QStringLiteral("@@TXT@@"),          txt);
    qss.replace(QStringLiteral("@@BG@@"),           bg);
    qss.replace(QStringLiteral("@@SURF@@"),         surf);
    // @@ACC_HOVER@@ e @@ACC_PRESSED@@ antes de @@ACC@@ para evitar replace parcial
    // (ex.: "@@ACC_H..." poderia casar com "@@ACC@@" se a ordem fosse inversa).
    qss.replace(QStringLiteral("@@ACC_HOVER@@"),    accHover);
    qss.replace(QStringLiteral("@@ACC_PRESSED@@"),  accPressed);
    qss.replace(QStringLiteral("@@ACC@@"),          acc);
    qss.replace(QStringLiteral("@@SUB@@"),          sub);
    qss.replace(QStringLiteral("@@NEG@@"),          neg);
    qss.replace(QStringLiteral("@@BRD@@"),          brd);
    qss.replace(QStringLiteral("@@ALT@@"),          alt);
    qss.replace(QStringLiteral("@@HOV@@"),          hov);
    // Tokens sidebar (#165): substituidos por ultimo (nao conflitam com os acima).
    qss.replace(QStringLiteral("@@SIDEBAR1@@"),     sb1);
    qss.replace(QStringLiteral("@@SIDEBAR2@@"),     sb2);
    qss.replace(QStringLiteral("@@SIDEBARTXT@@"),   sbtx);

    return qss;
}

// ── qssParaTeste ─────────────────────────────────────────────────────────────
// Acessor mínimo que expõe o QSS gerado para testes de regressão de tokens.
// Não chama aplicar() — só depende de setMode() ter sido chamado antes.
// Garante que testes de regressão pegam a classe de bug: troca neg↔brd.
QString qssParaTeste()
{
    return gerarQss();
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
