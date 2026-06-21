# impl-166-cue-glossarylabel-report.md

## GlossaryLabel — cue estático de glossário (§9.6)

**Item:** #166 Peça 3 §9.6 — sublinhado pontilhado nos termos de glossário  
**Branch:** redesign-ui  
**Status:** implementado e testado

---

## Hash

- `9085b84` — implementação inicial (GlossaryLabel, 4 pontos curados)
- (2ª onda) — visibilidade reforçada + cobertura ampla (vide abaixo)

---

## 2ª onda — visibilidade + cobertura ampla

### (1) Visibilidade

Cor antiga: `Theme::subtexto()` (#67768D) com alpha **140 (~55%)** — quase
imperceptível. Substituída por getter dedicado `Theme::glossarioUnderline()`:

| Tema | Cor base | Alpha | Racional |
|---|---|---|---|
| Light | `#5A6B82` (cinza-ardósia escuro) | **220 (~86%)** | um tom mais firme que o subtexto |
| Dark | `#9AA8BC` (cinza-ardósia claro) | **220 (~86%)** | destaca sobre fundo escuro |

**Mantém-se CINZA** (saturação < 45%, B−R < 60) — NÃO é o azul accent
(`#3B82F6`), que continua reservado a ações/CTAs. Testes em `test_theme_tokens`
garantem: alpha ≥ 200, não-accent, cinza dessaturado, dark-aware (dark mais claro).

### (2) Cobertura ampla

`GlossarioDecorator::aplicarTooltips` agora, na MESMA varredura que já seta os
tooltips, instala o sublinhado pontilhado (via `GlossaryUnderlineFilter`) nos
labels-chave **curtos**. Não troca classe nem mexe no layout — o filter pinta o
pontilhado por cima no `QEvent::Paint` (re-despacho síncrono + overlay).

**Critério de elegância (`deveSublinhar`):** decora quando o texto é
essencialmente o termo — ≤ 28 chars, OU o termo cobre ≥ 55% do texto. Prosa
longa fica só com tooltip (não risca a frase inteira).

**Onde `aplicarTooltips` roda (cobertura efetiva):**
- `MainWindow` (`this`) → todas as telas/abas carregadas (Dashboard, Parcelas,
  Entradas, Gastos, Investimentos, Proventos, Bolsa, Metas, Recorrência…).
- Dialogs modais que se auto-decoram no fim do `buildUi`: `CalculadoraDialog`
  (todas as 8 abas), `ProventoDialog`, `InvestimentoDialog`, `AddCompraDialog`.

**Levantamento:** das 806 strings de UI ≤ 28 chars, **77 contêm termo de
glossário** e recebem o cue — CET, PV, PMT, FV, CDI, SELIC, IOF, IRPF, INSS,
JCP, PGBL, Ticker, Dividendo, Provento, Aporte, ROI, OFX, SAC, Price, CDB,
LCI/LCA, IPCA, Vencimento, Parcela, FIRE (+ variantes), etc.

**Tooltip-only (sem sublinhado):**
- Prosa longa com termo no meio (callouts de ajuda, frases explicativas) —
  ex.: "O CET considera juros, tarifas e impostos…".
- Texto em rich-text/HTML (`<b>DY:</b> …`) — o helper de pintura descarta
  `mightBeRichText` (baseline ambígua).
- Botões (`QPushButton`) — já têm chrome próprio (borda/sombra).

### Bônus — fix de precisão na varredura

`primeiroTermoEncontrado` ganhou `UseUnicodePropertiesOption` na regex de
word-boundary. Sem isso, `\bDAS\b` casava em "Conclu**í**das" (boundary falso
após letra acentuada) — falso-positivo que pintaria sublinhado/tooltip de "DAS"
em "Concluídas". Corrige tooltips E sublinhados. Regressão coberta por
`primeiroTermoNaoCasaDentroDePalavraAcentuada`.

### Arquivos novos (2ª onda)

- `src/ui/widgets/GlossaryUnderline.{h,cpp}` — helper de pintura compartilhado
  (DRY: `GlossaryLabel` e o filter usam a mesma cor/posição/padrão).
- `src/ui/GlossaryUnderlineFilter.{h,cpp}` — event filter por-label (cobertura ampla).
- `tests/test_glossary_coverage.cpp` — 14 testes (cobertura, prosa, idempotência, render).

---

## Linha-chave do teste

```
PASS   : TestGlossaryLabel::doisPontosFinaisIgnoradosNoLookup()
```
(Resume o contrato mais distinto: texto preservado + `:` ignorado no lookup + tooltip idêntico ao do serviço.)

Suíte completa: **12/12 PASS** (`test_glossary_label`) + **6/6 PASS** (`test_glossary_label_render`).

---

## Onde GlossaryLabel foi aplicado

### CalculadoraDialog — Tab Financeira (TVM)

| Posição | Termo | Definição no glossário? |
|---|---|---|
| Grid TVM — linha N | `N:` | Não → sem cue (sem cursor/tooltip) |
| Grid TVM — linha i | `i/mês:` | Não → sem cue |
| Grid TVM — linha PV | `PV:` | **Sim** → sublinhado + tooltip |
| Grid TVM — linha PMT | `PMT:` | **Sim** → sublinhado + tooltip |
| Grid TVM — linha FV | `FV:` | **Sim** → sublinhado + tooltip |
| Bloco CET (read-only) | `CET:` | **Sim** → sublinhado + tooltip |

**Por que N e i/mês não recebem cue:** `N` (número de períodos) e `i/mês` (taxa mensal) não têm entrada própria com essas chaves no `GlossarioService`. O `GlossaryLabel` é idempotente: sem definição = sem decoração, cursor padrão, sem tooltip.

**Restante do `CalculadoraDialog`** (DY, PAYBACK, CDI, SELIC, ITBI, etc.): aparecem em texto de prosa HTML dentro de `QLabel::setText()` → cobertos pelo `GlossarioDecorator::aplicarTooltips()` existente. Não aplicar `GlossaryLabel` em prosa.

**`AddCompraDialog`:** os labels de form (`tr("Juros mensais / CET a.a.")`) são strings internas do `QFormLayout::addRow(tr(...))` — não são `QLabel*` manipuláveis sem refatorar. Cobertos pelo `aplicarTooltips()`.

---

## Descrição dos renders

### `/tmp/glossary_cue_light.png` (light)
- Fundo `#EEF3F9` (BgLight do tema). Labels alinhados à direita no form.
- Termos curados (PV, PMT, FV, CET, CDI, SELIC, IOF, IRRF) mostram **sublinhado pontilhado sutil** na cor subtexto (#67768D) com alpha ~55%.
- N: e i/mês: sem sublinhado (termos não cadastrados) — comportamento correto.
- O separador `─────` aparece como texto — artefato do widget de demo, não afeta produção.

### `/tmp/glossary_cue_dark.png` (dark)
- Fundo `#0F172A` (BgDark). Texto claro `#EAF0F8`.
- Mesmos termos curados com sublinhado pontilhado — cor subtexto dark (#8A97A8) com alpha ~55%.
- Nitidez idêntica ao light: o pontilhado é visível sem competir com o texto.
- Confirmado pixel-diferente de `QLabel` comum via `glossaryLabel_difereDePlainLabel_dark()`.

**Ambos os renders:** sublinhado nítido, não-agressivo, semântico. O teste de diferença de pixel provou que o `paintEvent` está pintando corretamente em offscreen (sem depender de compositor).

---

## Arquivos criados/modificados

- `src/ui/widgets/GlossaryLabel.h` — nova classe
- `src/ui/widgets/GlossaryLabel.cpp` — `paintEvent` com `Qt::DotLine` + `aplicarCue()`
- `tests/test_glossary_label.cpp` — 11 testes TDD (contrato a-f)
- `tests/test_glossary_label_render.cpp` — 4 testes de render (smoke + diff de pixel)
- `tests/CMakeLists.txt` — 2 novos targets (`test_glossary_label`, `test_glossary_label_render`)
- `CMakeLists.txt` — `GlossaryLabel.cpp` adicionado ao target `orcamento-pessoal`
- `src/ui/widgets/CalculadoraDialog.cpp` — include + 6 labels promovidos a `GlossaryLabel`
