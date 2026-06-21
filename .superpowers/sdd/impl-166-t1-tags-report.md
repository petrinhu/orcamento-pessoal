# Impl #166 Peça 1 — Tags coloridas (Theme::criarTag)

**Data:** 2026-06-21
**Branch:** redesign-ui
**Item:** #166 Fase 3, Peça 1 — componente reutilizável de Tag/chip colorido

---

## Hash do commit

_(ver git log após o commit)_

---

## Testes

**Suíte:** `test_tag_render` — **17 casos, 17 PASSED** em 20ms.

Casos cobertos:
1. `criarTag_objectName_tag` — objectName == "tag"
2. `criarTag_propriedadeVariante_sucesso` — property("variante") == "sucesso"
3. `criarTag_propriedadeVariante_perigo`
4. `criarTag_propriedadeVariante_atencao`
5. `criarTag_propriedadeVariante_info`
6. `criarTag_propriedadeVariante_neutro`
7. `criarTag_texto_correto` — label->text() == texto passado
8. `qss_contemSeletorTag_sucesso` — QSS contém `QLabel#tag[variante="sucesso"]`
9. `qss_contemSeletorTag_perigo`
10. `qss_contemSeletorTag_atencao`
11. `qss_contemSeletorTag_info`
12. `qss_contemSeletorTag_neutro`
13. `qss_semTokensTagResidu` — nenhum `@@TAG_*@@` residual no QSS
14. `renderLight_naoNulo` — grab offscreen light não nulo; salva /tmp/tags_light.png
15. `renderDark_naoNulo` — grab offscreen dark não nulo; salva /tmp/tags_dark.png

**ctest completo:** 42/42 PASSED (gate do pre-commit verde).

---

## API implementada

`Theme::criarTag(const QString &texto, Tag::Variante v, QWidget *parent = nullptr) -> QLabel*`

**Justificativa da API:** helper no namespace Theme (token-driven, igual aos helpers
de ajuda `#166`). A propriedade dinâmica `"variante"` dispara re-avaliação do QSS
global pelo Qt automaticamente — sem setStyleSheet local por widget. O caller faz
ownership do QLabel (ou passa parent).

---

## Variantes com HEX (fg/bg, light+dark) e ratio WCAG AA

| Variante | Light FG | Light BG | Ratio L | Dark FG | Dark BG | Ratio D | AA? |
|---|---|---|---|---|---|---|---|
| sucesso | #0D7A50 | #D1FAE5 | 4.73:1 | #34D399 | #064E3B | 5.06:1 | OK |
| perigo | #9B1C1C | #FEE2E2 | 6.67:1 | #FCA5A5 | #7F1D1D | 5.28:1 | OK |
| atencao | #92400E | #FEF3C7 | 6.37:1 | #FCD34D | #78350F | 6.29:1 | OK |
| info | #1D4ED8 | #DBEAFE | 5.49:1 | #93C5FD | #1E3A5F | 6.38:1 | OK |
| neutro | #374151 | #F1F5F9 | 9.41:1 | #94A3B8 | #1E293B | 5.71:1 | OK |
| roxo | #5B21B6 | #EDE9FE | 7.57:1 | #A78BFA | #2E1065 | 5.60:1 | OK |

> Aliases: Fixo→info, Variavel→roxo, Azul→info, Esmeralda→sucesso,
> Dourado→atencao, Vermelho→perigo, Slate→neutro. Todos passam WCAG AA.

---

## Onde as tags foram aplicadas

**`RecorrenciaWidget` (2 pontos):**

1. **Col 1 "Tipo"** — substituiu `QTableWidgetItem` texto cru `"Entrada"/"Gasto"` por chip tonal:
   - Entrada → `Tag::Variante::Sucesso` (esmeralda)
   - Gasto → `Tag::Variante::Perigo` (vermelho)

2. **Col 3 "Frequência"** — substituiu texto cru por chip com mapeamento determinístico frequência→matiz:
   - mensal → `Tag::Variante::Azul` (azul categórico)
   - semanal → `Tag::Variante::Esmeralda`
   - quinzenal → `Tag::Variante::Roxo`
   - anual → `Tag::Variante::Dourado`

---

## Renders

**`/tmp/tags_light.png`** (light): 5 linhas de chips sobre fundo azul-acinzentado `#EEF3F9`.
- Linha 1 (status): Sucesso/verde, Perigo/vermelho, Atenção/amarelo, Info/azul, Neutro/cinza
- Linha 2 (tipo): Fixo/azul, Variável/roxo
- Linha 3 (categórico): Azul, Esmeralda, Dourado, Vermelho, Roxo, Slate
- Linha 4 (frequência): Mensal/azul, Semanal/verde, Quinzenal/roxo, Anual/amarelo
- Linha 5 (tipo recorrência): Entrada/verde, Gasto/vermelho

**`/tmp/tags_dark.png`** (dark): mesmas variantes sobre fundo `#0F172A`. Cores de fundo
são os tints escuros de cada matiz (emerald-900, red-900, amber-900, violet-950…).
Textos são os tons claros de cada matiz (emerald-400, red-300, amber-300, violet-400…).
Todos os chips claramente legíveis com contraste alto nos dois temas.

---

## Arquivos criados/modificados

- `src/ui/Tag.h` — enum `Tag::Variante` + helper `toQssKey()` inline
- `src/ui/Theme.h` — declaração de `criarTag()` + include `QLabel` + `Tag.h`
- `src/ui/Theme.cpp` — tokens `Color::Tag*` (12 pares fg/bg), QSS das 6 variantes,
  substituições de `@@TAG_*@@`, implementação de `criarTag()`
- `src/ui/widgets/RecorrenciaWidget.cpp` — aplicação nos 2 pontos naturais (Col Tipo + Col Frequência)
- `tests/test_tag_render.cpp` — suíte TDD com 17 casos
- `tests/CMakeLists.txt` — registro do `test_tag_render`
