#pragma once

#include <QString>
#include <QFont>
#include <QColor>
#include <QIcon>
#include <QLabel>
#include <QPalette>

#include "Tag.h"

class QTableWidget;

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
    // #198 (A11Y-04, WCAG 1.4.3): texto sobre o accent cru (#3B82F6) reprova
    // AA em vários fundos claros (3.68:1 no QPushButton:hover). Blue-700 —
    // mesmo tom já usado no eyebrow/número dos blocos de ajuda (#166 §15).
    extern const QString AccTextLight;
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
    extern const QString AccTextDark; // #198 (A11Y-04): idem, tom claro sobre fundo escuro.
    extern const QString PositiveDark;
    extern const QString NegativeDark;
    extern const QString DouradoDark;     // novo (#164)
    extern const QString Sidebar1Dark;    // novo (#164)
    extern const QString Sidebar2Dark;    // novo (#164)
    extern const QString SidebarTextDark; // novo (#164)

    // ── Tokens de ajuda (#166 tipografia dos blocos de ajuda) ────────────────
    // Light
    extern const QString HelpDescBgLight;   // fundo callout Descrição
    extern const QString HelpDescBarLight;  // faixa de acento à esquerda (3px)
    extern const QString HelpInstrBgLight;  // fundo callout Instruções (mais sutil)
    extern const QString HelpNumBgLight;    // fundo da pílula do número do passo
    extern const QString HelpNumFgLight;    // número dentro da pílula (blue-700 AA)
    extern const QString HelpEyebrowLight;  // rótulo/eyebrow maiúsculo (blue-700 AA)
    // Dark
    extern const QString HelpDescBgDark;
    extern const QString HelpDescBarDark;
    extern const QString HelpInstrBgDark;
    extern const QString HelpNumBgDark;
    extern const QString HelpNumFgDark;
    extern const QString HelpEyebrowDark;
}

// Convenience getters — return the correct token for the active scheme
QString acento();
// #198 (A11Y-04): cor de TEXTO derivada do acento, AA-safe (>=4.5:1) nos
// fundos onde o acento cru reprova (hover de botão, header hover). Usar em
// vez de acento() sempre que o accent for pintado como TEXTO (não borda/bg).
QString acentoTexto();
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

// Cor do sublinhado pontilhado do cue de glossário (#166 §9.6). CINZA dedicado
// (NÃO o azul accent — reservado a ações). Tom firme + alpha ~86%: claramente
// visível em light e dark sem competir com o texto nem com os CTAs. Usado por
// GlossaryLabel e GlossaryUnderlineFilter.
QColor  glossarioUnderline();

// Renderiza um glifo de emoji da fonte embutida "Noto Color Emoji" num QIcon
// colorido (pixmap transparente, HiDPI-aware). Usar em QPushButton/QToolButton
// via setIcon — QSS não renderiza fonte colorida no texto de botão de forma
// confiável.
QIcon   emojiIcon(const QString &emoji, int px = 16);

// Expõe o QSS gerado para testes de regressão de tokens. Garante que os
// tokens de borda nunca recebam a cor negativa (e vice-versa). Não chamar
// aplicar() antes — só setMode() é suficiente.
QString qssParaTeste();

// Getters de ajuda (#166) — retornam o token correto para o tema ativo.
QString helpDescBg();    // fundo do callout Descrição
QString helpDescBar();   // faixa de acento à esquerda
QString helpInstrBg();   // fundo do callout Instruções
QString helpNumBg();     // fundo da pílula numerada
QString helpNumFg();     // cor do número na pílula (blue-700 no light, AA garantido)
QString helpEyebrow();   // cor do eyebrow/rótulo (blue-700 no light, AA garantido)

// ── Tag / chip colorido (#166 Fase 3, Peça 1) ────────────────────────────────
// Cria um QLabel estilizado como chip tonal (§9.5 manual de estilo).
// objectName = "tag", propriedade dinâmica "variante" = Tag::toQssKey(v).
// Estilo aplicado via QSS global (gerarQss): QLabel#tag[variante="X"] { … }.
// A propriedade dinâmica dispara re-avaliação do QSS automaticamente no Qt.
// Caller é responsável pelo ownership (parent opcional).
QLabel *criarTag(const QString &texto, Tag::Variante v, QWidget *parent = nullptr);

// ── Skin compartilhado das tabelas (#167 T1, §11 manual de estilo) ──────────

// Fundo de hover de linha inteira (token @@HOV@@). Usado pelo TableHoverDelegate.
QString hoverBg();

// Altura de linha (px) das QTableWidget: "Confortável" é o default do
// redesign #167; "Compacta" é opt-in nas Preferências (PrefsWidget).
inline constexpr int kAlturaLinhaConfortavel = 48;
inline constexpr int kAlturaLinhaCompacta    = 34;

// Estado em memória da densidade — quem PERSISTE é AppSettings; o caller
// (main.cpp no boot, PrefsWidget no toggle) é responsável por sincronizar os
// dois. Theme.cpp fica deliberadamente desacoplado de AppSettings/libsecret
// para não arrastar essa dependência a todo alvo de teste que linka Theme.cpp.
void setDensidadeTabelasCompacta(bool compacta);
bool densidadeTabelasCompacta();
int  alturaLinhaTabela(); // kAlturaLinhaCompacta ou kAlturaLinhaConfortavel

// Aplica a altura de linha atual a UMA tabela. Chamado pelo TableHoverFilter
// no Show/Polish de cada QTableWidget (mesma varredura que arma o hover) —
// toda tabela nasce com a densidade correta sem código por-widget.
void aplicarDensidadeTabela(QTableWidget *table);

// Reaplica a densidade a TODAS as QTableWidget já construídas na sessão
// (QApplication::allWidgets()). Chamado quando o toggle "Compacta" muda nas
// Preferências, para efeito imediato sem reiniciar o app.
void aplicarDensidadeTabelas();

} // namespace Theme
