#include "Theme.h"

#include "ButtonShadowFilter.h"
#include "FocusVisibleFilter.h"
#include "TableHoverFilter.h"
#include "Tag.h"

#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QHeaderView>
#include <QPainter>
#include <QPixmap>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QPalette>
#include <QTableWidget>

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
    // #198 (A11Y-04): blue-700 — mesmo tom do HelpEyebrowLight/HelpNumFgLight
    // (#166), AA-safe como texto onde o accent cru reprova.
    const QString AccTextLight    = QStringLiteral("#1D4ED8");
    const QString PositiveLight   = QStringLiteral("#16C784");
    const QString NegativeLight   = QStringLiteral("#EA3943");
    const QString DouradoLight    = QStringLiteral("#E8A93B");
    const QString Sidebar1Light   = QStringLiteral("#1B2A41");
    const QString Sidebar2Light   = QStringLiteral("#16233A");
    const QString SidebarTextLight = QStringLiteral("#D6DEEA");

    // Dark
    const QString BgDark          = QStringLiteral("#0F172A");
    const QString Surface2Dark    = QStringLiteral("#131A24");
    const QString SurfaceDark     = QStringLiteral("#1B2430");
    const QString BorderDark      = QStringLiteral("#2A3340");
    const QString TextDark        = QStringLiteral("#EAF0F8");
    const QString SubtextDark     = QStringLiteral("#8A97A8");
    const QString AccentDark      = QStringLiteral("#3B82F6");
    // #198 (A11Y-04): mesmo tom do HelpEyebrowDark (#166) — AA-safe sobre os
    // fundos escuros onde o accent cru reprova.
    const QString AccTextDark     = QStringLiteral("#7FB0FF");
    const QString PositiveDark    = QStringLiteral("#16C784");
    const QString NegativeDark    = QStringLiteral("#EA3943");
    const QString DouradoDark     = QStringLiteral("#E8A93B");
    // Sidebar dark = mesmo navy do light (tom próprio, não inverte com o tema)
    const QString Sidebar1Dark    = QStringLiteral("#1B2A41");
    const QString Sidebar2Dark    = QStringLiteral("#16233A");
    const QString SidebarTextDark = QStringLiteral("#D6DEEA");

    // ── Tokens de ajuda (#166 tipografia dos blocos de ajuda) ────────────────
    // Derivados do accent #3B82F6 (tints azuis).
    // No light, eyebrow e número usam blue-700 #1D4ED8 — o accent #3B82F6 reprova
    // WCAG AA sobre o tint azul-claro (3.24:1 falha; #1D4ED8 = 5.91:1 PASS).
    // Light
    const QString HelpDescBgLight   = QStringLiteral("#EAF1FE");

    const QString HelpDescBarLight  = QStringLiteral("#3B82F6");
    const QString HelpInstrBgLight  = QStringLiteral("#F4F8FE");
    const QString HelpNumBgLight    = QStringLiteral("#E2ECFD");
    const QString HelpNumFgLight    = QStringLiteral("#1D4ED8"); // blue-700 (AA sobre tint)
    const QString HelpEyebrowLight  = QStringLiteral("#1D4ED8"); // blue-700 (AA sobre tint)
    // Dark
    const QString HelpDescBgDark    = QStringLiteral("#16243D");
    const QString HelpDescBarDark   = QStringLiteral("#5B9BFF");
    const QString HelpInstrBgDark   = QStringLiteral("#141E2E");
    const QString HelpNumBgDark     = QStringLiteral("#21344F");
    const QString HelpNumFgDark     = QStringLiteral("#5B9BFF"); // AA sobre num-bg dark
    const QString HelpEyebrowDark   = QStringLiteral("#7FB0FF"); // AA sobre desc-bg dark

    // ── Tokens de tag/chip colorido (#166 Fase 3, Peça 1) ───────────────────
    // Chips tonais: fundo = tint suave do matiz, texto = matiz escuro.
    // WCAG AA verificado nos dois temas (≥4.5:1). 2026-06-21.
    //
    // sucesso   = esmeralda (positivo semântico)
    //   L: #0D7A50/#D1FAE5 = 4.73:1   D: #34D399/#064E3B = 5.06:1
    const QString TagSucessoFgLight  = QStringLiteral("#0D7A50"); // emerald-700
    const QString TagSucessoBgLight  = QStringLiteral("#D1FAE5"); // emerald-100
    const QString TagSucessoFgDark   = QStringLiteral("#34D399"); // emerald-400
    const QString TagSucessoBgDark   = QStringLiteral("#064E3B"); // emerald-900
    //
    // perigo    = vermelho (negativo semântico)
    //   L: #9B1C1C/#FEE2E2 = 6.67:1   D: #FCA5A5/#7F1D1D = 5.28:1
    const QString TagPerigoFgLight   = QStringLiteral("#9B1C1C"); // red-800
    const QString TagPerigoBgLight   = QStringLiteral("#FEE2E2"); // red-100
    const QString TagPerigoFgDark    = QStringLiteral("#FCA5A5"); // red-300
    const QString TagPerigoBgDark    = QStringLiteral("#7F1D1D"); // red-900
    //
    // atencao   = dourado (aviso semântico)
    //   L: #92400E/#FEF3C7 = 6.37:1   D: #FCD34D/#78350F = 6.29:1
    const QString TagAtencaoFgLight  = QStringLiteral("#92400E"); // amber-800
    const QString TagAtencaoBgLight  = QStringLiteral("#FEF3C7"); // amber-100
    const QString TagAtencaoFgDark   = QStringLiteral("#FCD34D"); // amber-300
    const QString TagAtencaoBgDark   = QStringLiteral("#78350F"); // amber-900
    //
    // info      = azul (também cobre Fixo e Azul categórico)
    //   L: #1D4ED8/#DBEAFE = 5.49:1   D: #93C5FD/#1E3A5F = 6.38:1
    const QString TagInfoFgLight     = QStringLiteral("#1D4ED8"); // blue-700
    const QString TagInfoBgLight     = QStringLiteral("#DBEAFE"); // blue-100
    const QString TagInfoFgDark      = QStringLiteral("#93C5FD"); // blue-300
    const QString TagInfoBgDark      = QStringLiteral("#1E3A5F"); // blue-950 (custom navy)
    //
    // neutro    = slate (também cobre Slate categórico)
    //   L: #374151/#F1F5F9 = 9.41:1   D: #94A3B8/#1E293B = 5.71:1
    const QString TagNeutroFgLight   = QStringLiteral("#374151"); // slate-700
    const QString TagNeutroBgLight   = QStringLiteral("#F1F5F9"); // slate-100
    const QString TagNeutroFgDark    = QStringLiteral("#94A3B8"); // slate-400
    const QString TagNeutroBgDark    = QStringLiteral("#1E293B"); // slate-800
    //
    // roxo      = violeta (cobre Variavel categórico)
    //   L: #5B21B6/#EDE9FE = 7.57:1   D: #A78BFA/#2E1065 = 5.60:1
    const QString TagRoxoFgLight     = QStringLiteral("#5B21B6"); // violet-800
    const QString TagRoxoBgLight     = QStringLiteral("#EDE9FE"); // violet-100
    const QString TagRoxoFgDark      = QStringLiteral("#A78BFA"); // violet-400
    const QString TagRoxoBgDark      = QStringLiteral("#2E1065"); // violet-950
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
QString acentoTexto(){ return isDark() ? Color::AccTextDark     : Color::AccTextLight;     } // #198 (A11Y-04)
QString negativo()   { return isDark() ? Color::NegativeDark    : Color::NegativeLight;    }
QString positivo()   { return isDark() ? Color::PositiveDark    : Color::PositiveLight;    }
QString background() { return isDark() ? Color::BgDark          : Color::BgLight;          }
QString surface()    { return isDark() ? Color::SurfaceDark     : Color::SurfaceLight;     }
QString texto()      { return isDark() ? Color::TextDark        : Color::TextLight;        }
QString borda()      { return isDark() ? Color::BorderDark      : Color::BorderLight;      }
QString subtexto()   { return isDark() ? Color::SubtextDark     : Color::SubtextLight;     }

// Getters novos (#164)
QString dourado()    { return isDark() ? Color::DouradoDark     : Color::DouradoLight;     }
QString sidebar1()   { return isDark() ? Color::Sidebar1Dark    : Color::Sidebar1Light;    }
QString sidebar2()   { return isDark() ? Color::Sidebar2Dark    : Color::Sidebar2Light;    }
QString sidebarText(){ return isDark() ? Color::SidebarTextDark : Color::SidebarTextLight; }

// ── Getters de ajuda (#166) ───────────────────────────────────────────────────
QString helpDescBg()  { return isDark() ? Color::HelpDescBgDark  : Color::HelpDescBgLight;  }
QString helpDescBar() { return isDark() ? Color::HelpDescBarDark : Color::HelpDescBarLight; }
QString helpInstrBg() { return isDark() ? Color::HelpInstrBgDark : Color::HelpInstrBgLight; }
QString helpNumBg()   { return isDark() ? Color::HelpNumBgDark   : Color::HelpNumBgLight;   }
QString helpNumFg()   { return isDark() ? Color::HelpNumFgDark   : Color::HelpNumFgLight;   }
QString helpEyebrow() { return isDark() ? Color::HelpEyebrowDark : Color::HelpEyebrowLight; }

// Âmbar "atenção" dark-mode-aware. Mesmos tons do antigo helper local do
// Lembretes (fundo de linha "vence em breve"); reaproveitado como fill de barra
// de progresso < 50% no Metas. Pastel em fundo claro, âmbar escuro em dark.
QColor  atencao()
{
    return isDark() ? QColor(QStringLiteral("#8A6D00"))   // âmbar escuro (dark)
                    : QColor(QStringLiteral("#FFF3C4"));  // âmbar pastel (light)
}

// Cor do sublinhado pontilhado do cue de glossário (#166 §9.6).
// CINZA dedicado — deliberadamente NÃO usa o azul accent (reservado a ações no
// manual de estilo: pintar glossário de azul competiria com os CTAs). Um tom
// mais firme que subtexto() + alpha alto (~86%) para ficar claramente visível
// sem agredir. Light: cinza-ardósia escuro; Dark: cinza-ardósia claro.
QColor  glossarioUnderline()
{
    QColor c = isDark() ? QColor(QStringLiteral("#9AA8BC"))  // cinza claro (dark)
                        : QColor(QStringLiteral("#5A6B82")); // cinza escuro (light)
    c.setAlpha(220); // ~86% — visível e elegante
    return c;
}

// ── Skin compartilhado das tabelas (#167 T1, §11 manual de estilo) ────────────

// Fundo de hover de linha (token @@HOV@@). Mesmos tons usados em gerarQss()
// (menu/toolbar hover) — reaproveitado aqui via getter para o TableHoverDelegate
// pintar a linha inteira sem duplicar o hex.
QString hoverBg()
{
    return isDark() ? QStringLiteral("#222D3D") : QStringLiteral("#DCE8FA");
}

// Estado em memória da densidade das tabelas. Default = Confortável (false).
// Deliberadamente NÃO lê AppSettings aqui (ver comentário no .h): o caller
// (main.cpp no boot, PrefsWidget::onDensidadeToggled no toggle) sincroniza.
static bool g_densidadeTabelasCompacta = false;

void setDensidadeTabelasCompacta(bool compacta) { g_densidadeTabelasCompacta = compacta; }
bool densidadeTabelasCompacta() { return g_densidadeTabelasCompacta; }

int alturaLinhaTabela()
{
    return g_densidadeTabelasCompacta ? kAlturaLinhaCompacta : kAlturaLinhaConfortavel;
}

void aplicarDensidadeTabela(QTableWidget *table)
{
    if (!table)
        return;
    table->verticalHeader()->setDefaultSectionSize(alturaLinhaTabela());
}

void aplicarDensidadeTabelas()
{
    const int h = alturaLinhaTabela();
    const auto widgets = QApplication::allWidgets();
    for (QWidget *w : widgets) {
        if (auto *t = qobject_cast<QTableWidget *>(w))
            t->verticalHeader()->setDefaultSectionSize(h);
    }
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
    // #198 (A11Y-04): texto sobre accent — AA-safe (ver acentoTexto()).
    const QString accText = dark ? Color::AccTextDark : Color::AccTextLight;
    const QString txt  = dark ? Color::TextDark     : Color::TextLight;
    const QString sub  = dark ? Color::SubtextDark  : Color::SubtextLight;
    const QString neg  = dark ? Color::NegativeDark : Color::NegativeLight;
    // Alternating row — ligeiramente mais escuro que Surface2 para manter contraste
    // (usado em QMenu/QListWidget etc.; QTableWidget NÃO usa mais zebra, #167 T1).
    const QString alt  = dark ? QStringLiteral("#171F2D") : Color::Surface2Light;
    // Hover background — mais suave que o accent
    const QString hov  = hoverBg();
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

    // Tokens de ajuda (#166): callouts da Descrição + Instruções
    const QString helpDescBgTok  = dark ? Color::HelpDescBgDark  : Color::HelpDescBgLight;
    const QString helpDescBarTok = dark ? Color::HelpDescBarDark : Color::HelpDescBarLight;
    const QString helpInstrBgTok = dark ? Color::HelpInstrBgDark : Color::HelpInstrBgLight;
    const QString helpNumBgTok   = dark ? Color::HelpNumBgDark   : Color::HelpNumBgLight;
    const QString helpNumFgTok   = dark ? Color::HelpNumFgDark   : Color::HelpNumFgLight;
    const QString helpEyebrowTok = dark ? Color::HelpEyebrowDark : Color::HelpEyebrowLight;

    // Tokens de tag/chip (#166 Fase 3, Peça 1): fg/bg por variante, light+dark.
    const QString tagSucessoFg  = dark ? Color::TagSucessoFgDark  : Color::TagSucessoFgLight;
    const QString tagSucessoBg  = dark ? Color::TagSucessoBgDark  : Color::TagSucessoBgLight;
    const QString tagPerigoFg   = dark ? Color::TagPerigoFgDark   : Color::TagPerigoFgLight;
    const QString tagPerigoBg   = dark ? Color::TagPerigoBgDark   : Color::TagPerigoBgLight;
    const QString tagAtencaoFg  = dark ? Color::TagAtencaoFgDark  : Color::TagAtencaoFgLight;
    const QString tagAtencaoBg  = dark ? Color::TagAtencaoBgDark  : Color::TagAtencaoBgLight;
    const QString tagInfoFg     = dark ? Color::TagInfoFgDark     : Color::TagInfoFgLight;
    const QString tagInfoBg     = dark ? Color::TagInfoBgDark     : Color::TagInfoBgLight;
    const QString tagNeutroFg   = dark ? Color::TagNeutroFgDark   : Color::TagNeutroFgLight;
    const QString tagNeutroBg   = dark ? Color::TagNeutroBgDark   : Color::TagNeutroBgLight;
    const QString tagRoxoFg     = dark ? Color::TagRoxoFgDark     : Color::TagRoxoFgLight;
    const QString tagRoxoBg     = dark ? Color::TagRoxoBgDark     : Color::TagRoxoBgLight;

    // Tokens nomeados: eliminam o esquema posicional %N que causou 2 bugs
    // (hover e agora neg/brd trocados). Cada @@TOKEN@@ tem semântica inequívoca.
    // @@TXT@@ @@BG@@ @@SURF@@ @@ACC@@ @@SUB@@ @@NEG@@ @@BRD@@ @@ALT@@ @@HOV@@
    // @@ACC_HOVER@@ @@ACC_PRESSED@@
    // @@SIDEBAR1@@ @@SIDEBAR2@@ @@SIDEBARTXT@@ (sidebar, fixos independente de tema)
    // @@HELP_DESC_BG@@ @@HELP_DESC_BAR@@ @@HELP_INSTR_BG@@
    // @@HELP_NUM_BG@@ @@HELP_NUM_FG@@ @@HELP_EYEBROW@@
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
        /* ── Frames / surfaces (cards) — raio 14px (#164, #166 Peça 4) ── */
        /* QFrame#surface: seletor genérico para qualquer superfície de card.      */
        /* QFrame#dashCard: cards do Dashboard (makeCard) — idêntico ao #surface,  */
        /*   mas com objectName próprio para preservar o eventFilter de clique.     */
        QFrame#surface,
        QFrame#dashCard {
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
        /* #198 (A11Y-04): border fica @@ACC@@ (não-texto, 3:1 ok); o TEXTO usa
           @@ACC_TEXT@@ (blue-700/blue-300 AA-safe) — @@ACC@@ cru reprova AA
           como texto sobre @@SURF@@ (3.68:1 light / 4.26:1 dark, <4.5:1). */
        QPushButton:hover   { border-color: @@ACC@@; color: @@ACC_TEXT@@; }
        QPushButton:pressed {
            background-color: @@HOV@@;
            border-color: @@ACC@@;
            padding-top: 7px;
            padding-bottom: 3px;
        }
        QPushButton:disabled { color: @@SUB@@; border-color: @@BRD@@; }

        /* ── Botão só-ícone (nav de mês, fechar/imprimir, toggles) ──────────
           Sem chrome em repouso: sem borda, sem fundo. Realce só no hover/press.
           Some sobre qualquer fundo (header @@BG@@, surface, sidebar). #165.
           QToolButton#iconButton: gêmeo para o toggle 🔼/🔽 do TabShell. */
        QPushButton#iconButton,
        QToolButton#iconButton {
            background: transparent;
            border: none;
            border-radius: 8px;
            padding: 0;
            min-width: 32px;
            min-height: 32px;
            color: @@SUB@@;
            font-size: 17px;
        }
        /* #198 (A11Y-04): glifo em texto usa @@ACC_TEXT@@ (AA-safe) — @@ACC@@
           cru sobre @@HOV@@/@@ALT@@ reprova AA como texto (2.97:1/3.51:1
           light). O emoji colorido ignora `color` (nota §9.3 manual), então
           isto só afeta ícones monocromáticos (glifo tipográfico). */
        QPushButton#iconButton:hover,
        QToolButton#iconButton:hover {
            background: @@HOV@@;
            color: @@ACC_TEXT@@;
        }
        QPushButton#iconButton:pressed,
        QToolButton#iconButton:pressed {
            background: @@ALT@@;
            color: @@ACC_TEXT@@;
        }
        QPushButton#iconButton:disabled,
        QToolButton#iconButton:disabled {
            color: @@BRD@@;
            background: transparent;
        }
        QPushButton#iconButton:focus,
        QToolButton#iconButton:focus {
            background: @@HOV@@;
            border: 1px solid @@ACC@@;
        }

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
        /* ── Table (§11 manual de estilo, #167 T1: skin compartilhado) ──────
           Zebra REMOVIDA (alternate-background-color = mesmo tom do fundo:
           neutraliza setAlternatingRowColors(true) chamado em ~24 widgets sem
           precisar tocar cada um). Grade vertical REMOVIDA (gridline
           transparente); a única divisória é a horizontal fina no item
           (border-bottom). Hover de LINHA INTEIRA é pintado pelo
           TableHoverDelegate (TableHoverFilter arma toda QTableWidget no
           Show/Polish) — QSS por célula não bastaria. Densidade de linha
           (Confortável 48px / Compacta 34px) é aplicada via
           verticalHeader()->setDefaultSectionSize() pelo mesmo filter — QSS
           não controla altura de linha de QTableView. */
        QTableWidget {
            background-color: @@SURF@@;
            alternate-background-color: @@SURF@@;
            gridline-color: transparent;
            border: 1px solid @@BRD@@;
            border-radius: 0px;
        }
        QTableWidget::item {
            padding: 4px 8px;
            min-height: 34px;
            border: none;
            border-bottom: 1px solid @@BRD@@;
        }
        QTableWidget::item:selected {
            background-color: @@ACC@@;
            color: white;
            border-bottom: 1px solid @@ACC@@;
        }
        QHeaderView {
            background-color: @@BG@@;
        }
        QHeaderView::section {
            background-color: @@BG@@;
            border: none;
            border-bottom: 2px solid @@ACC@@;
            padding: 6px 8px;
            font-weight: 600;
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.06em;
            color: @@TXT@@;
        }
        /* #198 (A11Y-04): texto em hover usa @@ACC_TEXT@@ (@@ACC@@ cru sobre
           @@BG@@ é 3.30:1 no light, reprova AA). */
        QHeaderView::section:hover { color: @@ACC_TEXT@@; }
        /* ── Labels ── */
        /* color explícito em QLabel garante que o Fusion/palette não sobrescreva
           com WindowText em widgets frameless (ex.: LoginDialog). */
        QLabel { color: @@TXT@@; background-color: transparent; }
        QLabel#title    { font-size: 16px; font-weight: 600; color: @@TXT@@; }
        QLabel#subtitle { font-size: 12px; color: @@SUB@@; }
        QLabel#error    { font-size: 12px; color: @@NEG@@; }

        /* ── Callouts de ajuda — Descrição + Instruções (#166 tipografia) ─────
           helpDescBox: callout azul com faixa de acento à esquerda (Descrição).
           helpInstrBox: callout irmão, fundo mais sutil, sem faixa (Instruções).
           helpEyebrow: rótulo/eyebrow maiúsculo (11px, 600, letter-spacing 0.08em).
           helpDescBody: corpo da Descrição (15px / Regular / linha 1.6).
           helpInstrBody: corpo de cada passo (14px / Regular / linha 1.5).
           helpPill: pílula numerada (22x22, SemiBold 12px, circulo).
        ── */
        QFrame#helpDescBox {
            background-color: @@HELP_DESC_BG@@;
            border: 1px solid rgba(59, 130, 246, 0.22);
            border-left: 3px solid @@HELP_DESC_BAR@@;
            border-radius: 9px;
        }
        QFrame#helpInstrBox {
            background-color: @@HELP_INSTR_BG@@;
            border: 1px solid rgba(59, 130, 246, 0.16);
            border-radius: 9px;
        }
        QLabel#helpEyebrow {
            font-size: 11px;
            font-weight: 600;
            color: @@HELP_EYEBROW@@;
            background-color: transparent;
        }
        QLabel#helpDescBody {
            font-size: 15px;
            color: @@TXT@@;
            background-color: transparent;
        }
        QLabel#helpInstrBody {
            font-size: 14px;
            color: @@TXT@@;
            background-color: transparent;
        }
        QLabel#helpPill {
            font-size: 12px;
            font-weight: 600;
            color: @@HELP_NUM_FG@@;
            background-color: @@HELP_NUM_BG@@;
            border-radius: 11px;
            min-width: 22px;
            max-width: 22px;
            min-height: 22px;
            max-height: 22px;
        }
        QFrame#helpStepSep {
            background-color: @@BRD@@;
            max-height: 1px;
            border: none;
        }
        /* ── Status bar — ver seletor completo na secao sidebar (#165 fix) ── */
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

        /* ── Sidebar navy (#165 redesign Fase 2 Task 6 + fix paintEvent) ──────
           O gradiente navy e pintado pelo paintEvent() do SidebarNav (QPainter
           deterministico). O QSS aqui NAO define background do widget raiz —
           isso evita o conflito com o Fusion style que sobrescreve o QSS de
           QWidget customizado quando ha stylesheet global ativo no Wayland.
           Largura 252px: calculada para caber "Planejamento/Orcamento" sem clip
           (icone 16px + gap 4px + texto ~188px Inter 13px + 2x7px padding = ~222px
           + margem de folga). Fix #165 bug 1 (clip horizontal).
           Separador: linha sutil entre zona de itens e footer.
        ── */
        QWidget#sidebarNav {
            min-width: 252px;
            max-width: 252px;
        }
        /* QScrollArea e viewport da zona de itens: transparentes para o navy do
           paintEvent() do pai transparecer. Sem borda, sem scroll horizontal. */
        QScrollArea#sidebarScrollArea,
        QScrollArea#sidebarScrollArea > QWidget > QWidget {
            background-color: transparent;
            border: none;
        }
        /* Filhos diretos e descendentes nomeados: background transparent para
           o navy do paintEvent() do pai transparecer. Os objectNames foram
           adicionados no construtor (#165 fix) para garantir que os seletores
           aqui os alcancam — seletor > QWidget sem nome nao funciona de forma
           confiavel em todas as versoes do Fusion. */
        QWidget#sidebarMainArea,
        QWidget#sidebarFooterArea,
        QWidget#sidebarUsuario {
            background-color: transparent;
        }
        /* Scrollbar vertical da sidebar: fina e discreta sobre o navy */
        QScrollArea#sidebarScrollArea QScrollBar:vertical {
            background: transparent;
            width: 4px;
            margin: 0;
        }
        QScrollArea#sidebarScrollArea QScrollBar::handle:vertical {
            background: rgba(255,255,255,0.15);
            border-radius: 2px;
            min-height: 20px;
        }
        QScrollArea#sidebarScrollArea QScrollBar::handle:vertical:hover {
            background: rgba(255,255,255,0.30);
        }
        QScrollArea#sidebarScrollArea QScrollBar::add-line:vertical,
        QScrollArea#sidebarScrollArea QScrollBar::sub-line:vertical { height: 0; }
        QFrame#sidebarSep {
            background: rgba(255,255,255,0.08);
            max-height: 1px;
            border: none;
        }
        /* Rotulo de grupo: intermediario entre navy e sidebarText; dim mas legivel */
        QWidget#sidebarNav QLabel#grupo {
            color: #8595AC;
            font-size: 10px;
            font-weight: 600;
            letter-spacing: 1px;
            padding: 12px 10px 4px;
            background: transparent;
        }
        /* Itens de navegacao: fundo transparente por padrao.
           padding horizontal reduzido de 11px para 7px (#165 fix clip horizontal):
           economiza 8px totais sem comprometer o affordance de clique. */
        QWidget#sidebarNav QPushButton#navItem {
            color: @@SIDEBARTXT@@;
            background: transparent;
            border: none;
            text-align: left;
            padding: 9px 7px;
            border-radius: 9px;
            font-weight: 500;
            font-size: 13px;
        }
        QWidget#sidebarNav QPushButton#navItem:hover {
            background: rgba(255,255,255,0.10);
            color: #ffffff;
        }
        /* Foco de teclado (#198, A11Y-02, WCAG 2.4.7): o fix #165 suprimia
           TODO indicador (outline+border none) — item focado via Tab ficava
           visualmente idêntico ao repouso, invisível para quem navega sem
           mouse. Fix: fundo sutil (mais forte que o hover 0.10) + borda 1px
           @@ACC@@ (contraste accent/navy >= 3.93:1 light e >= 4.27:1 dark —
           acima do minimo 3:1 de indicador não-textual, WCAG 1.4.11).
           NoFocusStyle ainda suprime o dotted-rect nativo do Fusion; este é o
           substituto intencional, não um acidente do #165. */
        QWidget#sidebarNav QPushButton#navItem:focus {
            background: rgba(255,255,255,0.14);
            border: 1px solid @@ACC@@;
        }
        /* Pill ativo: accent solido + cantos arredondados (radius 9 herdado do base).
           Sem box-shadow (QSS widget nao suporta); sem artefato retangular. */
        QWidget#sidebarNav QPushButton#navItem[ativo="true"] {
            background: @@ACC@@;
            color: #ffffff;
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
        /* ── Status bar navy (#165 fix bug 2 + #47 moderniza câmbio) ───────────
           QStatusBar fica full-width ABAIXO do centralWidget (sidebar+stack).
           Fundo @@SIDEBAR2@@ (navy idêntico à sidebar) elimina a faixa clara
           no canto inferior esquerdo — continuidade visual com a sidebar.
           Texto em @@SIDEBARTXT@@ (claro sobre navy). Borda top em @@SIDEBAR1@@
           (linha sutil entre conteúdo e rodapé, coerente com a paleta navy). */
        QStatusBar {
            background-color: @@SIDEBAR2@@;
            color: @@SIDEBARTXT@@;
            border-top: 1px solid @@SIDEBAR1@@;
            font-size: 12px;
        }
        QStatusBar QLabel {
            color: @@SIDEBARTXT@@;
            background: transparent;
        }

        /* ── Tags / chips coloridos (#166 Fase 3, Peca 1) ──────────────────────
           Chips tonais: fundo tint suave + texto versao escura do matiz.
           Forma totalmente arredondada (~20px), fonte Inter Medium 12px.
           WCAG AA verificado nos dois temas (>=4.5:1). 2026-06-21.
           A propriedade dinamica "variante" dispara re-avaliacao do QSS
           automaticamente (Qt reaplica regras com atributo quando setProperty()
           e chamado). Nenhum estilo inline — tudo via token nomeado.
        ── */
        QLabel#tag {
            font-size: 12px;
            font-weight: 500;
            padding: 2px 10px;
            border-radius: 20px;
            border: none;
        }
        /* sucesso = esmeralda — positivo semantico */
        QLabel#tag[variante="sucesso"] {
            color: @@TAG_SUCESSO_FG@@;
            background-color: @@TAG_SUCESSO_BG@@;
        }
        /* perigo = vermelho — negativo semantico */
        QLabel#tag[variante="perigo"] {
            color: @@TAG_PERIGO_FG@@;
            background-color: @@TAG_PERIGO_BG@@;
        }
        /* atencao = dourado — aviso semantico */
        QLabel#tag[variante="atencao"] {
            color: @@TAG_ATENCAO_FG@@;
            background-color: @@TAG_ATENCAO_BG@@;
        }
        /* info = azul — informativo / fixo / azul categorico */
        QLabel#tag[variante="info"] {
            color: @@TAG_INFO_FG@@;
            background-color: @@TAG_INFO_BG@@;
        }
        /* neutro = slate — sem status / slate categorico */
        QLabel#tag[variante="neutro"] {
            color: @@TAG_NEUTRO_FG@@;
            background-color: @@TAG_NEUTRO_BG@@;
        }
        /* roxo = violeta — variavel / roxo categorico */
        QLabel#tag[variante="roxo"] {
            color: @@TAG_ROXO_FG@@;
            background-color: @@TAG_ROXO_BG@@;
        }

        /* ── Tooltip escuro — glossario de termos financeiros (#166 Peca 3) ───────
           QToolTip: fundo escuro com texto claro, padding confortavel, raio 6px.
           Largura maxima ~280px via wordWrap do Qt (max-width nao tem efeito em
           QToolTip via QSS, mas o texto usa wordwrap automatico do Qt).
           Contraste WCAG AA: #EAF0F8 sobre #1B2A41 = 9.07:1 (PASS).
           O navy #1B2A41 e o mesmo da sidebar — reforco de identidade de "definicao
           do PeuBud", distinto do tooltip padrao do sistema. (#166 §9.6)
        ── */
        QToolTip {
            background-color: #1B2A41;
            color: #EAF0F8;
            border: 1px solid #2A3340;
            border-radius: 6px;
            padding: 8px 12px;
            font-family: "Inter";
            font-size: 13px;
        }

        /* ── Empty states guiados (#166 Peca 2) ─────────────────────────────────
           EmptyStateWidget: emoji 64px (QLabel#emptyStateEmoji — so pixmap),
           titulo Inter SemiBold 16px cor @@TXT@@, frase Regular 13px cor @@SUB@@.
           Centralizados vertical+horizontalmente sobre o fundo da view.
        ── */
        QLabel#emptyStateTitle {
            font-size: 16px;
            font-weight: 600;
            color: @@TXT@@;
            background-color: transparent;
        }
        QLabel#emptyStateFrase {
            font-size: 13px;
            font-weight: 400;
            color: @@SUB@@;
            background-color: transparent;
        }
        QLabel#emptyStateEmoji {
            background-color: transparent;
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
    qss.replace(QStringLiteral("@@ACC_TEXT@@"),     accText); // #198 (A11Y-04) — antes de @@ACC@@ (mesma cautela do comentário acima)
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
    // Tokens de ajuda (#166): callouts Descrição + Instruções.
    qss.replace(QStringLiteral("@@HELP_DESC_BG@@"),  helpDescBgTok);
    qss.replace(QStringLiteral("@@HELP_DESC_BAR@@"), helpDescBarTok);
    qss.replace(QStringLiteral("@@HELP_INSTR_BG@@"), helpInstrBgTok);
    qss.replace(QStringLiteral("@@HELP_NUM_BG@@"),   helpNumBgTok);
    qss.replace(QStringLiteral("@@HELP_NUM_FG@@"),   helpNumFgTok);
    qss.replace(QStringLiteral("@@HELP_EYEBROW@@"),  helpEyebrowTok);
    // Tokens de tag (#166 Fase 3, Peça 1): chips tonais por variante.
    // Ordem: @@TAG_*_FG@@ antes de @@TAG_*_BG@@ (sem conflito; prefixos distintos).
    qss.replace(QStringLiteral("@@TAG_SUCESSO_FG@@"), tagSucessoFg);
    qss.replace(QStringLiteral("@@TAG_SUCESSO_BG@@"), tagSucessoBg);
    qss.replace(QStringLiteral("@@TAG_PERIGO_FG@@"),  tagPerigoFg);
    qss.replace(QStringLiteral("@@TAG_PERIGO_BG@@"),  tagPerigoBg);
    qss.replace(QStringLiteral("@@TAG_ATENCAO_FG@@"), tagAtencaoFg);
    qss.replace(QStringLiteral("@@TAG_ATENCAO_BG@@"), tagAtencaoBg);
    qss.replace(QStringLiteral("@@TAG_INFO_FG@@"),    tagInfoFg);
    qss.replace(QStringLiteral("@@TAG_INFO_BG@@"),    tagInfoBg);
    qss.replace(QStringLiteral("@@TAG_NEUTRO_FG@@"),  tagNeutroFg);
    qss.replace(QStringLiteral("@@TAG_NEUTRO_BG@@"),  tagNeutroBg);
    qss.replace(QStringLiteral("@@TAG_ROXO_FG@@"),    tagRoxoFg);
    qss.replace(QStringLiteral("@@TAG_ROXO_BG@@"),    tagRoxoBg);

    return qss;
}

// ── criarTag ─────────────────────────────────────────────────────────────────
// Cria um QLabel estilizado como chip tonal (§9.5 manual de estilo, #166 Fase 3).
// objectName = "tag" → seletor QLabel#tag no QSS global.
// Propriedade dinâmica "variante" = Tag::toQssKey(v) → seletor com atributo
// QLabel#tag[variante="X"] — o Qt re-avalia o QSS automaticamente.
// Padding 2px 10px, raio 20px (totalmente arredondado), Inter Medium 12px.
// Caller é responsável pelo ownership; parent é opcional.
QLabel *criarTag(const QString &texto, Tag::Variante v, QWidget *parent)
{
    auto *lbl = new QLabel(texto, parent);
    lbl->setObjectName(QStringLiteral("tag"));
    // setProperty dispara QEvent::StyleChange: o QSS global reavalia as regras
    // com atributo [variante="X"] automaticamente (sem setStyleSheet local).
    lbl->setProperty("variante", QLatin1StringView(Tag::toQssKey(v)));
    lbl->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    return lbl;
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

    // App-wide :focus-visible emulation for #iconButton: the focus ring shows
    // only for keyboard focus (Tab/Backtab/Shortcut), never for the automatic
    // focus Qt gives the first focusable widget when a window becomes active.
    // Fixes #165 (caixa no ◀️ no 1º render). Idempotent; lives for the app lifetime.
    FocusVisibleFilter::install();

    // App-wide skin de tabelas (#167 T1, §11 manual de estilo): arma toda
    // QTableWidget no Show/Polish com mouse tracking + TableHoverDelegate
    // (hover de linha inteira) + densidade de linha atual. Idempotente.
    TableHoverFilter::install();

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
