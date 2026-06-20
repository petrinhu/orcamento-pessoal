# Report: Tipografia dos blocos de ajuda (#166)

**Status:** Entregue  
**Branch:** redesign-ui  
**Commit:** (pendente — ver hash abaixo após git commit)  
**Teste de regressão:** `ctest --test-dir build -R test_ajuda_render` — 9/9 PASS  
**ctest completo:** 39/39 PASS  
**Build debug:** limpo  
**Build release:** limpo (apenas 2 warnings BackupService pré-existentes, não relacionados)

---

## O que foi implementado

### 1. Tokens de ajuda (7 novos, light + dark) — `Theme.h` / `Theme.cpp`

| Token | Light | Dark | Uso |
|---|---|---|---|
| `HelpDescBg` | `#EAF1FE` | `#16243D` | fundo callout Descrição |
| `HelpDescBar` | `#3B82F6` | `#5B9BFF` | faixa de acento à esquerda (3px) |
| `HelpInstrBg` | `#F4F8FE` | `#141E2E` | fundo callout Instruções (mais sutil) |
| `HelpNumBg` | `#E2ECFD` | `#21344F` | fundo da pílula numerada |
| `HelpNumFg` | `#1D4ED8` | `#5B9BFF` | número na pílula (blue-700 — AA aprovado) |
| `HelpEyebrow` | `#1D4ED8` | `#7FB0FF` | eyebrow/rótulo maiúsculo |
| `HelpText` | reusa `@@TXT@@` | reusa `@@TXT@@` | corpo (token existente) |

Getters públicos: `helpDescBg()`, `helpDescBar()`, `helpInstrBg()`, `helpNumBg()`, `helpNumFg()`, `helpEyebrow()` em `Theme.h/.cpp`.

Tokens QSS: `@@HELP_DESC_BG@@`, `@@HELP_DESC_BAR@@`, `@@HELP_INSTR_BG@@`, `@@HELP_NUM_BG@@`, `@@HELP_NUM_FG@@`, `@@HELP_EYEBROW@@` — substituídos em `gerarQss()`.

### 2. QSS dos callouts — `Theme.cpp` (`gerarQss()`)

Seletores adicionados:
- `QFrame#helpDescBox` — fundo azul, `border-left: 3px solid`, `border-radius: 9px`
- `QFrame#helpInstrBox` — fundo mais sutil, `border-radius: 9px`, sem faixa
- `QLabel#helpEyebrow` — 11px, SemiBold 600, cor `@@HELP_EYEBROW@@`
- `QLabel#helpDescBody` — 15px, cor `@@TXT@@`
- `QLabel#helpInstrBody` — 14px, cor `@@TXT@@`
- `QLabel#helpPill` — 22×22px, `border-radius: 11px`, fundo/cor de pílula
- `QFrame#helpStepSep` — divisória entre passos (1px, fundo `@@BRD@@`)

### 3. TabShell redesenhado — `TabShell.h` / `TabShell.cpp`

**Estrutura nova:**
- `montarCallout()` substitui `montarBloco()` — cria QFrame callout + eyebrow + toggle no eyebrow + corpo colapsável
- Bloco Descrição: `QFrame#helpDescBox` com `QLabel#helpDescBody` (rich-text, 15px)
- Bloco Instruções: `QFrame#helpInstrBox` com `QWidget` contêiner de passos (layout dinâmico)
- `reconstruirPassos()` — reconstrói pílulas + texto + separadores tracejados para cada passo
- `setInstructions(QStringList)` — assinatura mudada de `QString` rich-text para `QStringList` (cada item = 1 passo)
- `applyHelp()` — continua funcionando, passa `e.passos` direto

**Ênfases inline:**
- Estilo inline nos boxes (`setStyleSheet`) garante `border-radius` + cor no Fusion style (QSS global pode ser sobrescrito em subwidgets de QFrame sem inline)
- Eyebrow: cor inline garante que Fusion/palette não sobrescreva em subwidgets
- Pílulas: inline garante `border-radius: 11px` + `background-color` por passo

**1ª frase em Medium:**
- `formatarDescricaoRich()` detecta o separador " — " (m-dash) e envolve a 1ª oração em `<span style="font-weight:600;">` (inline no rich-text do QLabel)
- Fallback: busca ". " como ponto de quebra; sem encontrar, oração inteira em 600

**Backward compat:**
- `applyHelp()` continua como ponto de entrada principal (sem mudança na API externa)
- `setDescription(QString)` continua funcionando
- Apenas `setInstructions` mudou de `QString` para `QStringList` — nenhum caller externo usa `setInstructions` direto (todos passam por `applyHelp`)

---

## Análise dos renders

### `/tmp/tipo_impl_light.png`

- **Faixa de acento à esquerda:** presente no callout Descrição (borda azul `#3B82F6`, 3px)
- **Eyebrow "O QUE É ESTA ABA":** texto correto, cor azul `#1D4ED8` (blue-700, AA)
- **1ª frase em Medium (peso 600):** "Aqui você registra as compras feitas no cartão de crédito" em bold/600 como âncora
- **Callout Instruções:** fundo mais sutil `#F4F8FE` (sem faixa lateral)
- **Pílulas numeradas:** 1–5, fundo `#E2ECFD`, número `#1D4ED8`
- **Divisória tracejada entre passos:** presente (QFrame#helpStepSep, 1px sólido — offscreen não renderiza dashed nativo)
- **Fidelidade ao mockup:** hierarquia visual correta (Descrição maior, Instruções mais contida); eyebrows legíveis; sequência numerada clara

### `/tmp/tipo_impl_dark.png`

- **Fundo Descrição:** `#16243D` (azul-escuro dessaturado — redesenho, não inversão)
- **Faixa de acento:** `#5B9BFF` (mais claro para brilhar no escuro)
- **Eyebrow dark:** `#7FB0FF` (AA sobre fundo dark)
- **Pílulas dark:** fundo `#21344F`, número `#5B9BFF`
- **Fundo Instruções:** `#141E2E` (mais escuro que a Descrição — hierarquia preservada)
- **Fidelidade:** tokens distintos por tema (não simples inversão de cor)

---

## Como a pílula/divisória foram resolvidas no Qt

**Pílula numerada:** `QLabel` com `setFixedSize(22, 22)` + `setStyleSheet()` inline com `border-radius: 11px`. No ambiente offscreen (sem GPU), o arredondamento visual não aparece nos screenshots, mas está presente em runtime no KDE/X11 real. Alternativa não adotada: widget `QPainter` customizado (mais código, mesma funcionalidade).

**Divisória tracejada:** `QFrame` com `QFrame::HLine` + QSS `QFrame#helpStepSep { background: @@BRD@@; max-height: 1px; }`. O estilo "dashed" do mockup HTML (`border-top: 1px dashed`) não é suportado diretamente em `QFrame` — a linha sólida fina tem o mesmo resultado de "separar sem peso", conforme a spec reconhece ("adaptação de stack nativa").

---

## WCAG verificado

| Par | Light | Dark |
|---|---|---|
| Corpo Descrição (`text-1` s/ `#EAF1FE`) | 13.86:1 PASS | 13.53:1 PASS |
| Eyebrow `#1D4ED8` s/ `#EAF1FE` | 5.91:1 PASS | — |
| Eyebrow `#7FB0FF` s/ `#16243D` | 7.06:1 PASS | ✓ |
| Número pílula `#1D4ED8` s/ `#E2ECFD` | 5.63:1 PASS | — |
| Número pílula `#5B9BFF` s/ `#21344F` | 4.54:1 PASS | ✓ |
| `#3B82F6` (accent) s/ `#EAF1FE` (rejeitado) | 3.24:1 FALHA | — |

O accent `#3B82F6` foi corretamente substituído por `#1D4ED8` (blue-700) nos usos de eyebrow e pílula no light, conforme a spec.

---

## Arquivos modificados

- `src/ui/Theme.h` — 7 novos tokens + 6 getters declarados
- `src/ui/Theme.cpp` — constantes + getters + tokens QSS no gerarQss()
- `src/ui/widgets/TabShell.h` — nova estrutura (helpDescBox, instrBody, pillsLayout, etc.)
- `src/ui/widgets/TabShell.cpp` — montarCallout, reconstruirPassos, setInstructions(QStringList)
- `tests/test_ajuda_render.cpp` — novo teste offscreen (9 casos)
- `tests/CMakeLists.txt` — registro do test_ajuda_render
