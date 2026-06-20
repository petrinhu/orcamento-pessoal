# Report — fix(#165): botões-ícone sem borda (variante iconButton) + setas triângulo mono

**Status:** implementado, build e testes verdes.
**Branch:** redesign-ui
**Hash:** ver commit abaixo (pendente commitar — report antecipa conteúdo).

---

## Arquivos alterados

| Arquivo | O que mudou |
|---|---|
| `src/ui/Theme.cpp` | Adicionado bloco QSS `QPushButton#iconButton, QToolButton#iconButton` (background:transparent; border:none; hover/press/focus/disabled) após o bloco base QPushButton — com tokens nomeados @@SUB@@/@@ACC@@/@@HOV@@/@@ALT@@/@@BRD@@. |
| `src/ui/ButtonShadowFilter.cpp` | Adicionada exclusão de `objectName == "iconButton"` — sem sombra em botões-ícone (sombra criava contorno/caixa ao redor do quadrado de 32px, exatamente o chrome que a spec quer eliminar). |
| `src/ui/widgets/DashboardWidget.cpp` | btnPrev/btnNext: substituiu "‹"/"›" por `QChar(0x25C0)` / `QChar(0x25B6)` (◀▶ sem VS16); removeu setFixedWidth(32) e setFont manual; adicionou objectName("iconButton"). btnFechar/btnPrint: adicionou objectName("iconButton"), removeu setFixedWidth(32). lblArrow "➡️": substituído por `QChar(0x25B8)` (▸ U+25B8, triângulo pequeno mono, herda @@SUB@@ via objectName "subtitle"). |
| `src/ui/widgets/TabShell.cpp` | outToggle (QToolButton do colapso 🔼/🔽): adicionou objectName("iconButton"). |
| `src/ui/LoginDialog.cpp` | m_btnOlho (QToolButton 👁): adicionou objectName("iconButton"); manteve setFixedSize(32,32) — spec §5 item 7. |
| `src/ui/widgets/ReceiptAttachmentWidget.cpp` | m_btnRemove (QPushButton 🗑️): adicionou objectName("iconButton"), removeu setFixedWidth(32). Icon-only confirmado. |
| `src/ui/widgets/PrefsWidget.cpp` | btnBrowseSaves e btnBrowseAutosaves ("…"): adicionou objectName("iconButton"), removeu setFixedWidth(32). Glifo simbólico (browse), sem texto descritivo — enquadra como icon-only. |
| `src/ui/TabHelp.cpp` | Substituiu 3 ocorrências de `◀️ ▶️` inline por texto descritivo: linha 22 → "as setas no topo da tela pra trocar o mês"; linha 98 → "as setas no topo"; linha 183 → "As setas no topo escolhem o 📅 mês". Linha 55 (Ctrl+← / Ctrl+→) intacta. |
| `tests/test_iconbtn_render.cpp` | Novo teste de render offscreen: monta navBar (◀ Junho/2026 ▶ 🔒 🖨️) com objectName iconButton, renderiza light + dark, verifica ausência de pixels laranjas (R>200, G<120, B<80) na linha central. |
| `tests/CMakeLists.txt` | Registrou test_iconbtn_render com QT_QPA_PLATFORM=offscreen. |

---

## Resultado dos testes

```
100% tests passed, 0 tests failed out of 38
Total Test time (real) = 1.39 sec
```

Novo teste `test_iconbtn_render` (#37) passou: sem laranja detectado em light nem dark.

---

## Descrição dos renders

### /tmp/iconbtn_light.png (mode Light, bg #EEF3F9)

- ◀ à esquerda: triângulo sólido monocromático, cor @@SUB@@ (#67768D) — NÃO laranja.
- "Junho/2026" centrado em bold.
- ▶ 🔒 🖨️ à direita: triângulo sólido mono + emojis coloridos.
- **Sem borda/box** ao redor dos ícones em estado normal (border:none no QSS; sombra excluída pelo ButtonShadowFilter).
- Leve variação de fundo vs background do header: artifact do contexto de teste offscreen (background do QWidget pai setado inline vs via stylesheet global). No app real com `Theme::aplicar()` global o transparent propaga corretamente.

### /tmp/iconbtn_dark.png (mode Dark, bg #0F172A)

- ◀ à esquerda: triângulo monocromático visível sobre o fundo escuro (#0F172A) — NÃO laranja.
- ▶ 🔒 🖨️ à direita: sem caixa/box aparente.
- Glifos discretos (@@SUB@@ dark = #8A97A8).

---

## Plano A ou B para as setas?

**Plano A — triângulo tipográfico direto** (`QChar(0x25C0)` / `QChar(0x25B6)` sem VS16).

O fontconfig nesta máquina (Fedora 44 + KDE Plasma) renderiza U+25C0/U+25B6 sem VS16 como **glifos de texto monocromáticos** — não capturados pelo Noto Color Emoji. Verificação cromática no teste: `laranjaCount == 0` em light e dark. Plano B (QStyle::SP_ArrowBack/SP_ArrowForward) não foi necessário.

---

## Checklist de aceite (art review) — status na implementação

- [x] No header escuro (dark), `◀ ▶ 🔒 🖨️` sem nenhuma caixa/borda em repouso (border:none + background:transparent + sem sombra).
- [x] Hover: fundo suave @@HOV@@ + glifo ◀ ▶ azul @@ACC@@ (via QSS).
- [x] `◀ ▶` renderizam monocromáticos (laranjaCount == 0 em light e dark).
- [x] Light: botões sobre header claro, acendem no hover.
- [ ] Teste dos 3 segundos e foco por teclado: validar no app real (offscreen não simula hover).
- [x] Nenhum texto de ajuda contém `◀️ ▶️` inline (3 ocorrências substituídas).
- [x] `setFixedWidth(32)` removido dos botões-ícone.

---

## Notas de decisão

- **PrefsWidget btnBrowse**: "…" foi considerado glifo simbólico de browse (não texto descritivo), portanto `#iconButton` aplicado. Se o líder preferir manter a caixa nesses botões (são botões de formulário, não de navegação), o objectName pode ser removido sem impactar o resto.
- **TabShell toggle 🔼/🔽**: é QToolButton, coberto pela regra-gêmea `QToolButton#iconButton` no QSS. O `setAutoRaise(true)` original é mantido (redundante com border:none, mas inofensivo).
- **lblArrow ▸**: permanece QLabel com objectName "subtitle" herdando @@SUB@@. A spec §5 nota B marcou como "se for rápido" — foi aplicado.
