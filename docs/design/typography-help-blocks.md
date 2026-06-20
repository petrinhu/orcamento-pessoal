# Spec de tipografia: blocos de ajuda do TabShell (Descrição + Instruções)

> Status: **proposta de consultoria** (aguardando aprovação do líder do projeto).
> Escopo: tipografia + tratamento de bloco dos dois textos recolhíveis do `TabShell`
> (Descrição e Instruções). NÃO altera a microcopy (texto de `TabHelp.cpp` é canônico).
> Alvo de implementação: `frontend-engineer` (QSS + rich-text), depois de aprovada.
> Preview renderizado: [`mockups/peubud_tipografia_ajuda_no_mdash.html`](mockups/peubud_tipografia_ajuda_no_mdash.html)
> Relacionada ao redesign "azuis confiáveis": ver `.superpowers/sdd/` (specs do redesign).

---

## 1. Problema

Hoje os dois blocos de ajuda do `TabShell` (`src/ui/widgets/TabShell.cpp`) usam o mesmo
estilo plano: `QLabel#subtitle { font-size: 12px; color: subtext-cinza; }`. Resultado:
texto "sem graça", baixa hierarquia, não convida o leigo a ler. A Descrição (o que a aba
serve) e as Instruções (passo a passo) ficam visualmente idênticas, achatando a leitura.

Meta: tipografia que **chame atenção de forma elegante** sem fugir à identidade
("amigável + azuis confiáveis", Inter, paleta de `Theme.cpp`), em light e dark, mantendo
acessibilidade WCAG 2.2 AA.

## 2. Pesquisa (referências e o que extraí)

| Fonte | Aprendizado aplicado |
|---|---|
| [Atlassian — Applying typography](https://atlassian.design/foundations/typography/applying-typography) | 4 pesos (regular/medium/semibold/bold); **medium para texto ao lado de ícone** e ênfase em componentes; **bold com parcimônia**; helper/secundário em "Body S" regular; line-height corpo ~1.5, heading ~1.2. |
| [Atlassian — Messages](https://atlassian.design/patterns/messages/) | Anatomia de inline message: ícone-âncora + rótulo curto + corpo; banner só para sistema/erro, **ajuda contextual = inline** (é o nosso caso). |
| [Eleken — Modern fintech design guide](https://www.eleken.co/blog-posts/modern-fintech-design-guide) | Voz "human, transparent, calm"; **educação contextual** (explicação curta exatamente onde o usuário precisa); vermelho só para erro; fundo neutro deixa o dado respirar. |
| [Linear — calmer interface refresh](https://linear.app/now/behind-the-latest-design-refresh) | "Structure should be felt, not seen": bordas suaves, contraste reduzido; **não competir por atenção que não se mereceu** (o acento entra com moderação). |
| [Eyebrow heading best practices (uxmovement / qcfixer)](https://uxmovement.com/content/increasing-headline-clicks-with-eyebrow-text/) | **Eyebrow/rótulo** em maiúsculas + `letter-spacing` ~0.08em + cor de acento marca o bloco como "metadado/rótulo" sem competir com o corpo; consistência entre seções. |
| [USWDS / CFPB design systems](https://designsystem.digital.gov/components/typography/) | Para público amplo (incl. idosos/baixa visão): contraste alto, x-height generosa, espaçamento claro. |

Síntese: o padrão moderno de "help que chama atenção sem poluir" é um **callout inline**
= (eyebrow/rótulo curto em acento) + (ícone-âncora) + (corpo confortável) dentro de um
contêiner com **faixa de acento à esquerda** e **fundo tingido sutil** — não uma caixa
cinza plana, nem cor chapada gritante.

## 3. Decisão central: estilos DIFERENTES (irmãos), não iguais

**Descrição e Instruções recebem estilos distintos, porém da mesma família.** Justificativa:

- **Função comunicativa diferente.** Descrição = "o que é isto" (leitura passiva, prosa
  de acolhimento). Instruções = "faça isto" (leitura ativa, escaneável, sequencial).
  Tratar igual achata a hierarquia e faz o usuário reler para descobrir onde está o quê.
- **Hierarquia de leitura intencional.** O leigo deve absorver primeiro o "o quê"
  (Descrição, corpo maior 15px) e só então o "como" (Instruções, 14px). O degrau de
  tamanho + a numeração em pílula criam essa ordem natural.
- **Coerência preservada.** Mesmos tokens de cor, mesmo eyebrow, mesma família de
  contêiner (faixa de acento + tint). São "dialetos da mesma língua": distinguíveis num
  relance, mas inequivocamente do mesmo sistema. (Linear: estrutura sentida, não gritada.)

Alternativa considerada (descartada): **mesmo estilo para os dois**, diferenciando só
pelo rótulo. Mais simples de implementar, mas devolve o problema atual (baixa hierarquia)
e desperdiça a chance de tornar os passos escaneáveis. Fica como plano B se o líder
preferir contenção máxima.

## 4. Especificação — tokens

Família Inter (já embutida via `.qrc`). Cores em tokens novos, derivados do accent
`#3B82F6`. **No light, eyebrow e número usam blue-700 `#1D4ED8`** (o `#3B82F6` reprova
AA sobre o tint azul-claro — ver §6).

| Token novo | Light | Dark | Uso |
|---|---|---|---|
| `--help-desc-bg` | `#EAF1FE` | `#16243D` | fundo do callout Descrição |
| `--help-desc-bar` | `#3B82F6` | `#5B9BFF` | faixa de acento à esquerda (3px) |
| `--help-instr-bg` | `#F4F8FE` | `#141E2E` | fundo do callout Instruções (mais sutil) |
| `--help-num-bg` | `#E2ECFD` | `#21344F` | pílula do número do passo |
| `--help-num-fg` | `#1D4ED8` | `#5B9BFF` | número dentro da pílula |
| `--help-eyebrow` | `#1D4ED8` | `#7FB0FF` | rótulo/eyebrow maiúsculo |
| `--help-text` | `#15233B` | `#EAF0F8` | corpo (reusa `text-1` do Theme) |

Reuso: `border` (`#E4EAF2` / `#2A3340`), `surface`, `accent` já existem em `Theme.cpp`.

## 5. Especificação — por bloco

### 5.1 Eyebrow/rótulo (comum aos dois, mesmo estilo)
Marca cada bloco como rótulo, sem competir com o corpo.
- Família Inter **SemiBold (600)**; `font-size: 11px`; `letter-spacing: 0.08em`;
  `text-transform: uppercase`; cor `--help-eyebrow`.
- Precedido por um **dot** de 5px (`currentColor`, opacidade 0.7) como pequena âncora.
- Texto: Descrição = "O QUE É ESTA ABA"; Instruções = "COMO USAR, PASSO A PASSO".
  (Rótulos novos, fixos no `TabShell`; não fazem parte da microcopy de `TabHelp`.)

### 5.2 Bloco DESCRIÇÃO — prosa acolhedora
- **Contêiner (callout):** fundo `--help-desc-bg`; borda 1px do accent a 22% alpha;
  **`border-left: 3px solid --help-desc-bar`**; `border-radius: 9px` (= raio de input do
  app); padding `13px 16px 14px 16px`.
- **Corpo:** Inter **Regular (400)**; `font-size: 15px`; `line-height: 1.6`;
  cor `--help-text`; `max-width: ~62ch` (medida de leitura confortável).
- **Ênfase-âncora:** a **primeira oração** ("Aqui você registra as compras...") em
  **Medium (500)** — dá um ponto focal sem virar bloco-negrito. Demais destaques que já
  existem na microcopy (`<strong>juros</strong>`, IRPF/INSS) permanecem em SemiBold(600).
- **Ícone-âncora:** o emoji que já abre cada descrição (💳, 💵, 🧾...) fica a 16px,
  inline no começo do corpo (já vem do texto de `TabHelp`).
- **BIU:** sem itálico, sem sublinhado. Peso (Regular→Medium→SemiBold) carrega a ênfase.

### 5.3 Bloco INSTRUÇÕES — escaneável, "faça isto"
- **Contêiner:** mesma família, fundo `--help-instr-bg` (mais sutil que a Descrição,
  para recuar um degrau); borda 1px accent a 16% alpha; `border-radius: 9px`; **sem**
  faixa lateral (evita "parede" de duas barras; a Descrição é quem ganha o acento).
- **Passos:** `<ol>` sem marcador nativo; cada `<li>`:
  - **Número em pílula:** círculo 22px, fundo `--help-num-bg`, número Inter SemiBold(600)
    12px cor `--help-num-fg`. Âncora visual de sequência (escaneável de relance).
  - **Texto do passo:** Inter Regular(400) `font-size: 14px`; `line-height: 1.5`;
    `padding-left: 34px` (alinha à pílula).
  - **Divisória entre passos:** `border-top: 1px dashed` (border a ~80% alpha) — separa
    sem peso de linha sólida (Linear: estrutura sentida).
  - **Termos-chave:** os `<b>`/`<strong>` que já existem na microcopy em **SemiBold(600)**;
    nomes de botão citados (ex.: "➕ Novo item") também em 600. (Atlassian: medium/semibold
    ao lado de ícone e para diferenciar.)
- **Atalhos de teclado:** quando o passo cita `Ctrl+←` etc., renderizar como `<kbd>`:
  fundo `surface`, borda 1px `border`, raio 5px, 12px SemiBold, cor subtext. (Refinamento
  opcional; se for custo alto em QSS, manter texto simples com os termos em 600.)

### 5.4 Toggle 🔼/🔽
Mantido como está (`#iconButton`), apenas movido para a barra do eyebrow de cada bloco,
alinhado à direita — sem mudança de comportamento ou de persistência (`AppSettings`).

## 6. Acessibilidade (WCAG 2.2 AA) — verificado

Contraste calculado (corpo precisa ≥4.5:1; o `#3B82F6` foi **rejeitado** para eyebrow/número):

| Par | Light | Dark |
|---|---|---|
| Corpo Descrição (`text-1` s/ desc-bg) | 13.86:1 PASS | 13.53:1 PASS |
| Corpo Instruções (`text-1` s/ instr-bg) | 14.76:1 PASS | 14.60:1 PASS |
| Eyebrow s/ fundo do bloco | **5.91:1** PASS (`#1D4ED8`) | 7.06:1 PASS (`#7FB0FF`) |
| Número na pílula | **5.63:1** PASS (`#1D4ED8`) | 4.54:1 PASS (`#5B9BFF`) |
| `#3B82F6` no light s/ tint (rejeitado) | 3.24:1 FALHA | — |

Outros critérios:
- **Cor nunca sozinha:** a distinção Descrição/Instruções é por rótulo (texto) + estrutura
  (numeração), não só por cor. Os emojis 🟢🔵🔴 da microcopy continuam acompanhados de
  palavra (já é assim em `TabHelp`).
- **Dark é redesenho, não inversão:** hex próprios por token (azul-escuro dessaturado,
  não preto + acento clareado).
- **`prefers-reduced-motion`:** o reveal de page-load do mock é só demonstração; na
  implementação Qt não há animação obrigatória nos blocos.

## 7. Resumo da recomendação (1 linha cada)

- **Descrição:** callout azul-claro com faixa de acento à esquerda; Inter 15px/1.6, 1ª
  oração em Medium(500) como âncora, ícone-âncora inline; eyebrow "O QUE É ESTA ABA".
- **Instruções:** callout irmão (fundo mais sutil, sem faixa); passos `<ol>` com número em
  pílula azul (SemiBold 12px) + texto 14px/1.5 e divisória tracejada; eyebrow "COMO USAR".
- **Mesmo estilo?** Não: **diferentes mas irmãos** (mesmos tokens/eyebrow/contêiner;
  variam tamanho, ênfase e tratamento de passo) — separa "o quê" de "como" preservando
  coerência.

## 8. Handoff (para o `frontend-engineer`, após aprovação)

Plano faseado sugerido:
1. **Tokens.** Adicionar os 7 tokens novos em `Theme.cpp` (light+dark) com os hex desta spec.
2. **QSS.** Substituir `QLabel#subtitle` plano por dois objectNames/seletores:
   `#helpDesc` (corpo 15px) e contêineres `#helpDescBox`/`#helpInstrBox` (fundo+borda+raio).
   Eyebrow como `QLabel#helpEyebrow`.
3. **Rich-text das Instruções.** Em `TabShell::applyHelp()`, trocar o `<ol>` atual por
   markup com a pílula numérica. Como `QLabel` rich-text não suporta `::before counter`,
   gerar o número no HTML por passo (ex.: tabela/`<table>` ou span com fundo via inline
   style), ou desenhar a pílula com um pequeno widget por item — decisão de implementação
   do engenheiro (o mock web é referência de direção, não conversão 1:1; ver restrição
   de stack nativa).
4. **Eyebrow + barra do toggle.** Inserir os rótulos fixos e mover o toggle para a linha
   do eyebrow em `montarBloco()`.
5. **Critério de pronto:** render Qt em light e dark batendo a hierarquia do mock;
   contraste AA mantido (validar com os pares da §6); estado de colapso intacto.

> Lembrete de stack: PeuBud é Qt6/QSS nativo. O mockup HTML é **referência de direção
> visual** (hierarquia, cor, ritmo, densidade), não promessa de conversão pixel-a-pixel.
> A pílula numérica e o `<kbd>` podem exigir adaptação no rich-text do `QLabel`.
