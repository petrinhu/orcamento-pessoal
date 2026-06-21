# impl-166-cue-glossarylabel-report.md

## GlossaryLabel — cue estático de glossário (§9.6)

**Item:** #166 Peça 3 §9.6 — sublinhado pontilhado nos termos de glossário  
**Branch:** redesign-ui  
**Status:** implementado e testado

---

## Hash

Pendente geração do commit (vide seção abaixo).

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
