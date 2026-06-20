# Manual de Estilo: PeuBud

> **Manual e guia de design da interface do PeuBud.**
> O documento canônico do design visual do app. Consolida todas as decisões
> tomadas no redesign "amigável + azuis confiáveis" (Ondas 9-10, itens #163-#173,
> #166, #165) e é a referência única daqui pra frente: o que o `frontend-engineer`
> consulta antes de mexer em qualquer pixel.
>
> **Status:** canônico · **Versão:** 1.0 · **Item:** #182 · **Atualizado:** 2026-06-20
> **Autoridade de tokens:** `src/ui/Theme.cpp` (este manual reflete o código; em caso de
> divergência, o `Theme.cpp` vence e este doc é corrigido).

---

## Índice

1. [Sobre o PeuBud: a personalidade](#1-sobre-o-peubud--a-personalidade)
2. [Princípios de design](#2-princípios-de-design)
3. [A marca: nome e ícone (canônicos)](#3-a-marca--nome-e-ícone-canônicos)
4. [Tipografia](#4-tipografia)
5. [Paleta de cores: o significado](#5-paleta-de-cores--o-significado)
6. [Paleta de cores: os valores exatos (tokens)](#6-paleta-de-cores--os-valores-exatos-tokens)
7. [Forma e elevação](#7-forma-e-elevação)
8. [Navegação: a sidebar navy](#8-navegação--a-sidebar-navy)
9. [Componentes](#9-componentes)
10. [Blocos de ajuda: Descrição e Instruções](#10-blocos-de-ajuda--descrição-e-instruções)
11. [Tabelas](#11-tabelas)
12. [Gráficos](#12-gráficos)
13. [Política de ícones e emoji](#13-política-de-ícones-e-emoji)
14. [Estados e microinterações](#14-estados-e-microinterações)
15. [Acessibilidade](#15-acessibilidade)
16. [Light e Dark](#16-light-e-dark)
17. [Regras de ouro](#17-regras-de-ouro)
18. [Referências](#18-referências)

---

## 1. Sobre o PeuBud: a personalidade

*O que a marca comunica antes do primeiro pixel.*

O PeuBud é um app de **finanças pessoais para leigos** (pessoas comuns, não profissionais de finanças). Quem usa não sabe (e não precisa saber) o que é CET, IRRF
ou Dividend Yield. O design existe para **ensinar enquanto a pessoa usa**, sem assustar.

A interface antiga parecia "anos 90": barra de 13 abas estourando no topo, menu clássico,
botões chapados, hierarquia fraca. O redesign troca isso por uma personalidade deliberada,
escolhida no brainstorm e aprovada pelo líder:

> **Amigável + azuis confiáveis.**
> Calmo, claro, acolhedor. Cards arredondados, emoji colorido como guia visual,
> microinterações com feedback, linguagem simples. O azul carrega confiança; o navy
> carrega solidez; o branco deixa o dinheiro respirar.

Esta personalidade **decide todo o resto** deste manual. Quando houver dúvida entre duas
opções de design, vence a que for mais **calma, mais clara e mais acolhedora** para o leigo.

### Quem usa, e em que contexto

- **Público:** pessoa comum cuidando do próprio dinheiro (não contador, não trader).
- **Plataforma:** desktop Linux (Fedora/KDE Plasma primário; Debian e Arch secundários),
  Qt6 Widgets. Sem versão mobile no 1.0.
- **Prioridade do produto:** **Precisão financeira > Robustez > Segurança > UX.**
  O design serve a tarefa; beleza nunca sabota a leitura de um número.

---

## 2. Princípios de design

*As regras que regem toda decisão visual. Em ordem de prioridade.*

1. **Calmo e confiável.** Azul, navy e branco. Um único acento. Nada grita por atenção
   que não mereceu. (Linear: "estrutura sentida, não vista".)
2. **Mostrar visual, não número cru.** Data storytelling: barra de progresso, anel de meta,
   donut de proporção comunicam mais rápido que um valor solto. "Comunicar > enfeitar."
3. **Minimalismo com cards.** Conteúdo em cards arredondados sobre fundo azul-acinzentado.
   Hierarquia tipográfica forte; espaço para respirar.
4. **Uma aba por coisa.** Cada tarefa tem seu lugar; a sidebar agrupa por intenção
   (Dia a dia / Planejar / Investir).
5. **Ensinar no contexto.** Banner de ajuda recolhível, tooltips de glossário, empty states
   guiados. A explicação aparece exatamente onde o leigo precisa dela.
6. **Feedback em toda interação.** Hover, clique que afunda, foco visível. O app responde.
7. **Acessível por padrão.** WCAG 2.2 AA é piso, não enfeite (seção 15).
8. **Dark é redesenho, não inversão** (seção 16).

### Diferença entre este manual e o manual de marca

O **manual de marca** (do Dr. Petrus) rege identidade impressa: logo, papelaria, cartão.
Este **manual de estilo** rege a **interface de software** do PeuBud. São documentos
distintos com paletas distintas: o PeuBud tem a própria paleta (azuis de produto), que
**não** é a paleta da marca pessoal. O que este documento herda do manual de marca é só a
**forma de organizar e o tom didático** (significado antes de pixel, do's e don'ts, racional
explícito).

---

## 3. A marca: nome e ícone (canônicos)

*Os dois elementos que NÃO se redesenham sem ordem explícita do líder.*

| Elemento | Regra |
|---|---|
| **Nome "PeuBud"** | Canônico. Não traduzir, não abreviar, não estilizar de forma diferente. Aparece como wordmark no topo da sidebar. |
| **Ícone do app** | Canônico. Não redesenhar. Aparece no topo da sidebar (ao lado do wordmark) e como ícone de janela/sistema. |

✅ **Do:** wordmark "PeuBud" + ícone no topo da sidebar navy, em texto claro (`sidebarText`).
🚫 **Don't:** recolorir o ícone, trocar a tipografia do nome, ou aplicar efeito sobre eles
sem autorização explícita.

> Esta regra existe porque nome e ícone carregam reconhecimento. Mudá-los é decisão de
> produto do líder (petrus), não de design.

---

## 4. Tipografia

*Uma única família, embutida, com hierarquia clara para o leigo ler sem esforço.*

A fonte do PeuBud é a **Inter**, embutida via `.qrc` (não depende da fonte do sistema, mesmo render em qualquer distro). Três pesos embutidos:

| Peso | Arquivo | Uso |
|---|---|---|
| **Regular (400)** | `Inter-Regular.ttf` | Corpo de texto, células de tabela, descrições. |
| **Medium (500)** | `Inter-Medium.ttf` | Texto ao lado de ícone; botões; primeira oração de ênfase; itens de navegação. |
| **SemiBold (600)** | `Inter-SemiBold.ttf` | Títulos, números, cabeçalhos de tabela, termos-chave, item de nav ativo. |

> **Por que Inter, e por que só ela.** Inter tem x-height alta e desenho neutro legível: ideal para público amplo, incluindo quem tem baixa visão. Uma família só mantém coerência
> e peso de binário baixo. **Não** misturar com outra família de texto. (Diferente do manual
> de marca, que usa Montserrat/Times: aquilo é identidade impressa, isto é UI.)

### Hierarquia (escala em uso no `Theme.cpp`)

| Papel | Tamanho | Peso | Token / seletor |
|---|---|---|---|
| Título de seção / card | 16px | SemiBold (600) | `QLabel#title` |
| Corpo padrão (base) | 13px | Regular | `QWidget` base |
| Corpo de descrição (callout) | 15px | Regular | `#helpDesc` |
| Corpo de instrução (passo) | 14px | Regular | bloco Instruções |
| Subtítulo / texto secundário | 12px | Regular | `QLabel#subtitle` |
| Cabeçalho de tabela | 12px | SemiBold (600) | `QHeaderView::section` |
| Eyebrow / rótulo | 11px | SemiBold (600) + `letter-spacing` 0.08em + CAIXA-ALTA | `#helpEyebrow` |
| Rótulo de grupo (sidebar) | 10px | SemiBold (600) | `#grupo` |
| Item de navegação | 13px | Medium (500) → SemiBold (600) ativo | `#navItem` |

> No mockup de referência (web) os títulos de página aparecem em 18-23px peso 700, e isso é
> a **direção** de hierarquia. No app Qt, a escala efetiva é a da tabela acima (`Theme.cpp`).
> O mockup é referência de direção, não conversão pixel-a-pixel (stack nativa, seção 9).

### Regras tipográficas

- **Números sempre tabulares.** Valores monetários e quantidades usam alinhamento tabular
  (`tabular-nums`), alinhados à direita nas tabelas. Coluna de R$ nunca "dança".
- **Bold com parcimônia.** SemiBold marca o que importa (título, número, termo-chave).
  Negritar tudo achata a hierarquia.
- **Sem itálico e sem sublinhado para ênfase.** O peso (Regular → Medium → SemiBold)
  carrega a ênfase. Sublinhado pontilhado é reservado para **termos de glossário** (seção 9).
- **Line-height confortável:** corpo ~1.5-1.6; título ~1.2.

---

## 5. Paleta de cores: o significado

*Cada cor do PeuBud existe por um motivo. O significado vem antes do hex.*

A paleta é **azul de confiança + navy de solidez + branco que respira**, com acentos
afiados de status financeiro. Predominância de claro; o acento entra com moderação.

| Cor | Papel | O que comunica |
|---|---|---|
| 🔵 **Azul acento** `#3B82F6` | Ação primária, links, foco, item ativo | **Confiança, clareza, "clique aqui".** A única cor de marca da interface. |
| 🟦 **Navy** `#1B2A41` → `#16233A` | Sidebar, statusbar | **Solidez, estabilidade, ancoragem.** A "casa" da navegação, fixa em light e dark. |
| ⬜ **Branco** `#FFFFFF` | Superfície de cards | **Respiro.** Onde o dinheiro mora; deixa o número limpo. |
| 🟩 **Positivo** `#16C784` | Entradas, saldo positivo, aderência boa | **Dinheiro que entra / no caminho certo.** Esmeralda calma, não neon. |
| 🟥 **Negativo** `#EA3943` | Gastos, saldo negativo, dívida, erro | **Dinheiro que sai / atenção.** Vermelho de alerta, usado com critério. |
| 🟡 **Dourado** `#E8A93B` | Metas, destaque, aviso suave | **Conquista / cuidado.** Acento âmbar de valor a alcançar. |
| 🔷 **Azul-acinzentado** `#EEF3F9` | Fundo da aplicação | **Calma de fundo.** Tela que não cansa, deixa os cards saltarem. |

### Cores categóricas (gráficos)

Quando um gráfico precisa de várias séries, a ordem harmônica é:

🔵 azul `#3B82F6` · 🟩 esmeralda `#16C784` · 🟡 dourado `#E8A93B` · 🟥 vermelho `#EA3943` ·
🟣 roxo `#8B5CF6` · ⬜ slate `#94A3B8`

> **Regra de ouro do significado:** **valor negativo é SEMPRE `#EA3943`; valor positivo é
> SEMPRE `#16C784`.** Essa associação é fixa em todo o app: nunca inverter, nunca usar
> outro vermelho/verde para dinheiro. Mas cor **nunca** carrega o significado sozinha:
> sempre acompanha sinal (`-`/`+`), rótulo ou ícone (seção 15).

---

## 6. Paleta de cores: os valores exatos (tokens)

*Os hex canônicos, light e dark, como estão no `Theme.cpp`. Nunca hardcodar: usar o token.*

> **Regra de ouro dos tokens:** toda cor é um token semântico (`@@ACC@@`, `@@SURF@@`,
> `@@SUB@@`...). Hex solto espalhado no código é proibido: quebra o dark mode e a manutenção.

### 6.1 Tokens base (variam com o tema)

| Token (semântico) | Papel | **Light** | **Dark** |
|---|---|---|---|
| `@@BG@@` / `bg` | Fundo da aplicação | `#EEF3F9` | `#0F172A` |
| `bg2` / Surface-2 | Fundo alternativo, zebra | `#F7FAFD` | `#131A24` |
| `@@SURF@@` / `surface` | Superfície de card / input | `#FFFFFF` | `#1B2430` |
| `@@BRD@@` / `border` | Borda, divisória | `#E4EAF2` | `#2A3340` |
| `@@TXT@@` / `text` | Texto principal | `#15233B` | `#EAF0F8` |
| `@@SUB@@` / `subtext` | Texto secundário, glifo em repouso | `#67768D` | `#8A97A8` |
| `@@ACC@@` / `accent` | Acento / ação / foco / ativo | `#3B82F6` | `#3B82F6` |
| `@@ACC_HOVER@@` | Hover do botão primário | `#60A5FA` | `#60A5FA` |
| `@@ACC_PRESSED@@` | Pressed do botão primário | `#2563EB` | `#2563EB` |
| `@@HOV@@` / hover bg | Fundo de hover (linha, icon-button) | `#DCE8FA` | `#222D3D` |
| `@@ALT@@` / alt | Fundo pressed / linha alternada | `#F7FAFD` | `#171F2D` |
| `@@NEG@@` / negativo | Gasto, saldo negativo, erro | `#EA3943` | `#EA3943` |
| positivo | Entrada, saldo positivo | `#16C784` | `#16C784` |
| dourado | Meta, aviso, destaque | `#E8A93B` | `#E8A93B` |

> **Atenção (divergência resolvida):** um design doc inicial citava `hover #5CC6FA / pressed
> #1F7AAD` para o acento. O **`Theme.cpp` é a autoridade** e usa **`accHover #60A5FA`**
> (blue-400) **e `accPressed #2563EB`** (blue-600). Use estes.

### 6.2 Tokens da sidebar (navy fixo: NÃO inverte com o tema)

| Token | Papel | Light | Dark |
|---|---|---|---|
| `@@SIDEBAR1@@` | Topo do gradiente navy | `#1B2A41` | `#1B2A41` |
| `@@SIDEBAR2@@` | Base do gradiente navy / statusbar | `#16233A` | `#16233A` |
| `@@SIDEBARTXT@@` | Texto da sidebar | `#D6DEEA` | `#D6DEEA` |
| `#grupo` (rótulo de grupo) | Rótulo maiúsculo de grupo | `#8595AC` | `#8595AC` |

> **Por que o navy não inverte.** A sidebar e a statusbar têm tom próprio, independente de
> light/dark. Isso dá continuidade de marca (a "casa" é sempre navy) e elimina a faixa clara
> no canto inferior (statusbar = `@@SIDEBAR2@@`, fix do #165).

### 6.3 Tokens dos blocos de ajuda (#166)

Derivados do acento. **No light, eyebrow e número usam blue-700 `#1D4ED8`** porque o
`#3B82F6` reprova WCAG AA sobre o tint azul-claro (3.24:1 falha → `#1D4ED8` = 5.91:1 passa).

| Token | Papel | Light | Dark |
|---|---|---|---|
| `@@HELP_DESC_BG@@` | Fundo do callout Descrição | `#EAF1FE` | `#16243D` |
| `@@HELP_DESC_BAR@@` | Faixa de acento à esquerda (3px) | `#3B82F6` | `#5B9BFF` |
| `@@HELP_INSTR_BG@@` | Fundo do callout Instruções (mais sutil) | `#F4F8FE` | `#141E2E` |
| `@@HELP_NUM_BG@@` | Pílula do número do passo | `#E2ECFD` | `#21344F` |
| `@@HELP_NUM_FG@@` | Número dentro da pílula | `#1D4ED8` | `#5B9BFF` |
| `@@HELP_EYEBROW@@` | Rótulo/eyebrow maiúsculo | `#1D4ED8` | `#7FB0FF` |

### 6.4 Tokens de aviso (âmbar)

| Função | Light | Dark |
|---|---|---|
| `atencao()` (banner de aviso suave) | `#FFF3C4` (pastel) | `#8A6D00` (âmbar escuro) |

---

## 7. Forma e elevação

*Cantos suaves, sombras discretas, espaço deliberado. A "textura" amigável.*

### 7.1 Raios (border-radius)

| Elemento | Raio | Token / regra |
|---|---|---|
| **Cards / superfícies** | **14px** | `QFrame#surface` |
| **Botões e inputs** | **9px** | `#primary`, `QPushButton`, `QLineEdit`, `#navItem` |
| **Realce de icon-button** | 8px | `#iconButton` |
| **Pílula de número (passo)** | 11px (≈círculo de 22px) | bloco Instruções |
| **Tags / pills** | ~20px (totalmente arredondado) | tags de categoria/status |
| **Avatar (sidebar)** | 14px (círculo) | `#sidebarAvatar` |
| **Elementos pequenos** (menu item, kbd, scrollbar) | 4-6px | diversos |

> **Regra:** card = 14, botão/input = 9. Esses dois raios definem a "cara" do app.
> Não usar cantos vivos (0px) em cards nem em botões de texto.

### 7.2 Elevação (sombra)

O Qt **não tem `box-shadow` no QSS**. Sombras reais são feitas via `QGraphicsDropShadowEffect`
ou pintura (`ButtonShadowFilter`, já implementado). A linguagem de profundidade:

| Estado | Sombra (direção) |
|---|---|
| Card em repouso | Sombra muito sutil (`0 1px 3px rgba(20,40,80,.05)` de referência. |
| Card em hover | **Sobe ~2px** + sombra um pouco maior (lift). |
| Botão primário | Sombra de acento (azul suave) que dá relevo. |
| Botão **pressed** | **Afunda** (sombra some / inverte): feedback tátil. Já implementado. |

> **Direção da luz é sempre de cima.** Sombra projetada para baixo. Profundidade vem de
> sombra + camada, **não** de cor sólida pesada (evita "AI slop").

### 7.3 Espaçamento

- **Base 4px.** Múltiplos de 4 para padding e gap.
- **Padding de card:** 16-20px.
- **Gap de grid:** 14-16px.
- **Padding de botão:** ~5-14px (texto); icon-button tem padding 0 (o glifo centraliza).

---

## 8. Navegação: a sidebar navy

*A espinha dorsal da navegação. Substitui as 13 abas no topo.*

A `QTabWidget` no topo foi aposentada. No lugar, uma **sidebar navy fixa à esquerda**
(~236px), colapsável (vira só ícones).

### Anatomia (de cima para baixo)

1. **Topo:** ícone do app (canônico) + wordmark "PeuBud".
2. **Item "Dashboard"** solto.
3. **Grupos** com rótulo maiúsculo pequeno (`#grupo`, 10px SemiBold, `#8595AC`):
   - **DIA A DIA:** Entradas, Gastos, Cartões, Recorrências, Lembretes
   - **PLANEJAR:** Planejamento/Orçamento, Metas, Dívidas, Tendências
   - **INVESTIR:** Investimentos, Proventos, Bolsa
4. **Rodapé:** Preferências, Ajuda, Tema (claro/escuro/auto), e o usuário (avatar + nome).

### Estados do item de navegação (`#navItem`)

| Estado | Aparência |
|---|---|
| **Repouso** | Texto `sidebarText` (`#D6DEEA`), Medium (500), fundo transparente, sem borda. |
| **Hover** | Fundo `rgba(255,255,255,0.10)` (leve clareada), texto branco. |
| **Ativo** | **Pill azul** (`@@ACC@@` sólido) + texto branco + SemiBold (600). |
| **Foco (teclado)** | Sem outline retangular (`NoFocusStyle` suprime o dotted-rect do Fusion). |

> **Regra de ouro da sidebar:** o item ativo é uma **pill azul sólida**, sem box-shadow
> (QSS não suporta em widget), sem artefato retangular. O navy é sempre navy (não inverte).

### Navegação por chave estável

A navegação usa **chave estável** (objectName/enum), **não por índice**. Isso resolve o #157
(cards do Dashboard / busca global iam para a aba errada depois de arrastar/reordenar).

---

## 9. Componentes

*A biblioteca visual. Cada componente com todos os seus estados.*

> **Nota de stack (honestidade de fidelidade):** o PeuBud é **Qt6/QSS nativo**. Os mockups
> HTML em `docs/design/mockups/` são **referência de direção visual** (hierarquia, cor, ritmo,
> densidade), **não** promessa de conversão pixel-a-pixel. Algumas coisas (sombra real,
> pílula numérica em rich-text, `box-shadow`) exigem adaptação no Qt. O engenheiro traduz a
> intenção; o mock comunica o alvo.

### 9.1 Botão primário (`#primary`)

Ação principal da tela (ex.: "Novo gasto"). Azul cheio, texto branco, com relevo.

| Estado | Aparência |
|---|---|
| Repouso | Fundo `@@ACC@@` `#3B82F6`, texto branco, Medium (500), raio 9px, sombra de acento. |
| Hover | Fundo `@@ACC_HOVER@@` `#60A5FA`. |
| Pressed | Fundo `@@ACC_PRESSED@@` `#2563EB` + afunda (sombra inverte). |
| Disabled | Texto `@@SUB@@`, sem relevo. |

### 9.2 Botão padrão (`QPushButton` base, com texto)

Ação secundária. Contorno, não preenchido.

| Estado | Aparência |
|---|---|
| Repouso | Fundo `@@SURF@@`, borda 1px `@@BRD@@`, raio 9px, texto `@@TXT@@`. |
| Hover | Borda e texto viram `@@ACC@@` (acende azul). |
| Pressed | Afunda + borda `@@ACC@@`. |
| Disabled | Texto `@@SUB@@`, borda `@@BRD@@`. |

### 9.3 Botão só-ícone (`#iconButton`): variante SEM CAIXA (#165)

> **Regra de ouro:** **o icon-button NUNCA tem caixa em repouso.** Sem borda, sem fundo: some sobre qualquer fundo (header, surface, sidebar). O realce (fundo suave + glifo azul)
> aparece **só no hover/foco**. É affordance silenciosa.

Usado em: setas de navegação de mês (◀ ▶), fechar mês (🔒), imprimir (🖨️), toggles de
colapso (🔼/🔽), mostrar senha, browse de pasta (📁). Vale para `QPushButton#iconButton`
**e** `QToolButton#iconButton` (mesma regra).

| Estado | Background | Glifo (Light) | Glifo (Dark) |
|---|---|---|---|
| **Normal** | transparent (sem borda) | `#67768D` `@@SUB@@` | `#8A97A8` `@@SUB@@` |
| **Hover** | `#DCE8FA` `@@HOV@@` | `#3B82F6` `@@ACC@@` | `#3B82F6` `@@ACC@@` |
| **Pressed** | `#F7FAFD` `@@ALT@@` | `#3B82F6` | `#3B82F6` |
| **Focus** (teclado) | `@@HOV@@` + borda 1px `@@ACC@@` | `#3B82F6` | `#3B82F6` |
| **Disabled** | transparent | `#E4EAF2` `@@BRD@@` | `#2A3340` `@@BRD@@` |

- **Tamanho:** `min-width/min-height: 32px`, padding 0 (o glifo centraliza). **Nunca** usar
  `setFixedWidth(32)` sozinho (o QSS controla; se travar, `setFixedSize(32,32)` quadrado).
- **Emoji ignora `color`:** 🔒🖨️🔼🔽 permanecem coloridos; o que muda no hover é só o
  **fundo** do botão (que é o necessário para remover a "caixa").

🚫 **Don't:** dar borda "da cor do fundo" ao icon-button (vaza quando reusado sobre card).
Transparente é universal. Botão **com texto** mantém a caixa; `#iconButton` é exclusivo de
botão **sem rótulo**.

### 9.4 Cards (`QFrame#surface`)

- Superfície `@@SURF@@`, borda 1px `@@BRD@@`, raio **14px**.
- **Hover-lift:** sobe ~2px + sombra maior.
- **Chip de emoji** colorido como ícone do card (via `Theme::emojiIcon`).
- Padding 16-20px.

### 9.5 Tags / pills

Categoria, tipo (fixo/variável), status, frequência. Totalmente arredondadas (~20px),
coloridas conforme a categoria. Texto curto, peso Medium.

### 9.6 Tooltips de glossário

Termos que o leigo não conhece (IRRF, INSS, CET, DY...) recebem **sublinhado pontilhado**
→ no hover, tooltip escuro explicando em **linguagem simples**. (Substitui o GlossarioDecorator
e seu bug de sobrescrever.)

### 9.7 Empty states guiados

Quando não há dados: **emoji grande + frase acolhedora + CTA**. Ex.: "Nenhuma entrada ainda…"
+ botão "➕ Novo lançamento". Orienta quem está começando, nunca deixa a tela vazia e muda.

### 9.8 Statusbar navy

Faixa full-width abaixo do conteúdo. Fundo `@@SIDEBAR2@@` (navy idêntico à sidebar), texto
`@@SIDEBARTXT@@`, borda-top 1px `@@SIDEBAR1@@`. Continuidade visual com a sidebar (fix #165).

---

## 10. Blocos de ajuda: Descrição e Instruções

*A educação contextual. Dois callouts irmãos que ensinam o leigo a usar cada aba. (#166)*

Cada aba tem dois textos recolhíveis no topo (no `TabShell`): **Descrição** ("o que é esta aba")
e **Instruções** ("como usar, passo a passo"). Eles são **diferentes mas irmãos**: mesma família
visual (tokens, eyebrow, contêiner), tratamentos distintos para separar "o quê" de "como".

> **Por que diferentes.** Descrição = leitura passiva, prosa de acolhimento. Instruções =
> leitura ativa, escaneável, sequencial. Tratar igual achata a hierarquia e faz o leigo reler
> para descobrir onde está o quê. O degrau de tamanho + a pílula numérica criam a ordem natural.

### 10.1 Eyebrow / rótulo (comum aos dois)

Inter SemiBold (600), 11px, `letter-spacing` 0.08em, CAIXA-ALTA, cor `@@HELP_EYEBROW@@`,
precedido de um **dot** de 5px. Textos fixos: **"O QUE É ESTA ABA"** (Descrição) e
**"COMO USAR, PASSO A PASSO"** (Instruções).

### 10.2 Bloco DESCRIÇÃO: prosa acolhedora

- **Contêiner:** fundo `@@HELP_DESC_BG@@`, borda 1px accent a 22% alpha,
  **`border-left: 3px solid @@HELP_DESC_BAR@@`**, raio 9px, padding `13px 16px 14px`.
- **Corpo:** Inter Regular (400), **15px**, line-height 1.6, cor `@@TXT@@`, largura de leitura
  ~62ch.
- **Âncora:** a **primeira oração** em Medium (500): ponto focal sem virar bloco-negrito.
  Destaques da microcopy (`<strong>`, IRPF/INSS) em SemiBold (600).
- **Ícone-âncora:** o emoji que abre cada descrição (💳, 💵, 🧾…) a 16px, inline.

### 10.3 Bloco INSTRUÇÕES: escaneável

- **Contêiner:** fundo `@@HELP_INSTR_BG@@` (mais sutil que a Descrição), borda 1px accent a
  16% alpha, raio 9px, **sem faixa lateral** (evita "parede" de duas barras).
- **Passos (`<ol>`):** cada passo com **número em pílula** (círculo 22px, fundo
  `@@HELP_NUM_BG@@`, número SemiBold 12px cor `@@HELP_NUM_FG@@`) + texto Regular 14px/1.5.
- **Divisória entre passos:** `border-top: 1px dashed` (estrutura sentida, não gritada).
- **Termos-chave e nomes de botão** ("➕ Novo item") em SemiBold (600).
- **Atalhos de teclado:** `Ctrl+←` como `<kbd>` (fundo surface, borda 1px, raio 5px): opcional.

> **Regra de ouro:** a distinção Descrição/Instruções é por **rótulo + estrutura**, não só
> por cor. Em light, eyebrow e número são `#1D4ED8` (não `#3B82F6`) por contraste AA.

---

## 11. Tabelas

*Onde os lançamentos vivem. Densidade confortável, hierarquia clara, valores legíveis.*

| Aspecto | Decisão |
|---|---|
| **Densidade** | Confortável (linha ~48px). Opção "Compacta" nas Preferências. |
| **Cabeçalho** | Sutil: maiúsculas pequenas (12px SemiBold), fundo levíssimo, borda-bottom 2px `@@ACC@@`. |
| **Divisórias** | **Só horizontais finas** (`@@BRD@@`). Sem grade vertical pesada. |
| **Hover de linha** | Fundo azul suave (`@@HOV@@`). |
| **Valores** | À direita, **tabulares**, SemiBold. Negativo `#EA3943`, positivo `#16C784`. |
| **Categoria / Tipo** | Como **tags** coloridas (não texto cru). |
| **Rodapé-resumo** | Mini-barra (ex.: % de fixos vs variáveis). |
| **Linha TOTAL** | Id `-1` (convenção); destacada. |

🚫 **Don't:** zebra + grade vertical + bordas grossas ao mesmo tempo (polui). Divisória
horizontal fina é suficiente.

---

## 12. Gráficos

*Comunicar, não enfeitar. O visual conta a história que o número esconde.*

| Tipo | Quando usar | Estilo |
|---|---|---|
| **Linha / área** | Tendência ao longo do tempo | Linha + área com degradê, gridlines finas, ponto de pico destacado. |
| **Donut** | Proporção / composição | Rosca com **total no centro** + legenda com %. |
| **Barras** | Comparação entre itens | Barras arredondadas, valor no topo, eixo discreto. |

- **Paleta categórica harmônica** (seção 5): azul, esmeralda, dourado, vermelho, roxo, slate.
- **Hover** com tooltip de valor exato.
- Sem 3D exagerado, sem efeitos que distraiam do dado. "Comunicar > enfeitar."

---

## 13. Política de ícones e emoji

*Emoji colorido é a linguagem visual amigável do PeuBud, mas com regras rígidas.*

O PeuBud usa **emoji colorido Noto** embutido (`NotoColorEmoji.ttf` via `.qrc`) como ícones,
porque comunica de forma calorosa para o leigo. Mas emoji em UI tem armadilhas; daí as regras:

> **Regra de ouro nº 1:** nos botões/ícones, emoji é **SEMPRE** via `Theme::emojiIcon` (gera um
> `QIcon` a partir do glifo). **NUNCA** emoji-no-texto do botão. Isso garante render colorido
> consistente em qualquer distro, independente da fonte de emoji do sistema.

> **Regra de ouro nº 2 (FE0F / VS16):** o seletor de variação emoji (`️`, U+FE0F) força o Noto a
> renderizar a **caixa colorida de media-button**. Para glifos que devem ser **monocromáticos**
> (setas de texto, triângulos), **garantir que NÃO há VS16**.

### Setas de navegação (decisão #165)

As setas de troca de mês são **emoji laranja consistentes com os toggles**: decisão final do líder após iteração. Renderizadas via `Theme::emojiIcon` (não como glifo no texto do botão),
dentro do `#iconButton` (que dá o hover sem caixa). Histórico da decisão:

- ❌ `‹ ›` (guillemet): stroke fino, parece `< >`. **Vetado.**
- ❌ `← →` / `◀ ▶` mono: risco de filete fino ou render inconsistente.
- ✅ **emoji laranja** (consistente com os toggles 🔼🔽): decisão final do líder (#165).

### Emoji em texto corrido (ajuda)

🚫 **Don't:** embutir glifo de seta em texto de ajuda (`Use as setas ◀️ ▶️…`). Fora de um
botão e sem controle de tamanho, fica órfão. ✅ **Do:** descrever em palavras, ancorando na
posição ("Use as setas no topo da tela pra trocar o mês"). **Exceção:** atalho de teclado
(`Ctrl+←`) pode citar o caractere de seta (é tecla, não decoração).

---

## 14. Estados e microinterações

*O app responde a cada toque. Feedback é confiança.*

Todo componente interativo cobre **todos** os estados, nunca só o "happy path":

| Estado | O que acontece |
|---|---|
| **Default** | Repouso, discreto. |
| **Hover** | Cards sobem; linhas realçam (azul suave); botões ganham relevo; icon-buttons acendem; tooltips aparecem. |
| **Pressed / Active** | Botão **afunda** (sombra inverte); pill azul no item de nav ativo. |
| **Focus** (teclado) | Realce visível (anel fino azul no icon-button; highlight `@@ACC@@`). **Sem** dotted-rect do Fusion. |
| **Disabled** | Apagado (`@@SUB@@`/`@@BRD@@`), sem relevo, cursor padrão. |
| **Loading** | Skeleton / estado de carregamento (não tela congelada). |
| **Empty** | Empty state guiado (emoji + frase + CTA, seção 9.7). |
| **Error** | Mensagem clara em `@@NEG@@` + ícone/rótulo (cor nunca sozinha). |

- **Glossário no hover** (seção 9.6): termo sublinhado pontilhado → tooltip explicativo.
- **Central de Ajuda** acessível pelo rodapé da sidebar (F1).
- **`prefers-reduced-motion`:** o reveal escalonado dos mockups é só demonstração web; no Qt
  não há animação obrigatória que atrapalhe quem reduz movimento.

---

## 15. Acessibilidade

*WCAG 2.2 AA é piso, não enfeite. O app é para todo mundo, inclusive baixa visão e idosos.*

### Contraste (mínimos)

- Texto normal: **≥ 4.5:1**.
- Texto grande / componentes: **≥ 3:1**.

Pares já verificados (blocos de ajuda, #166):

| Par | Light | Dark |
|---|---|---|
| Corpo Descrição sobre `desc-bg` | 13.86:1 ✅ | 13.53:1 ✅ |
| Corpo Instruções sobre `instr-bg` | 14.76:1 ✅ | 14.60:1 ✅ |
| Eyebrow (`#1D4ED8` light / `#7FB0FF` dark) | 5.91:1 ✅ | 7.06:1 ✅ |
| Número na pílula (`#1D4ED8` / `#5B9BFF`) | 5.63:1 ✅ | 4.54:1 ✅ |
| `#3B82F6` sobre tint azul-claro (light) | 3.24:1 ❌ **REJEITADO** | n/a |

> **Regra de ouro do contraste:** **`#3B82F6` NÃO pode ser usado como texto sobre fundo
> azul-claro no light** (reprova AA). Nestes casos use **blue-700 `#1D4ED8`**. Foi o fix
> aplicado em eyebrow e número de passo (#166).

### Cor nunca sozinha

🚫 **Don't:** comunicar "erro" só com vermelho, ou "positivo/negativo" só com verde/vermelho.
Quem é daltônico perde. ✅ **Do:** cor **+** sinal (`-`/`+`), **+** rótulo ou **+** ícone/emoji
com palavra. Os emojis 🟢🔵🔴 da microcopy sempre acompanham a palavra.

### Foco e teclado

- Foco sempre visível (anel/realce `@@ACC@@`), mas **sem** o dotted-rect do Fusion
  (`NoFocusStyle` o suprime).
- Navegação por teclado funcional (Tab, atalhos `Ctrl+←`/`Ctrl+→` para mês).

---

## 16. Light e Dark

*Dois temas projetados juntos, com hex próprios. Dark não é "light invertido".*

> **Regra de ouro do dark mode:** dark é **redesenho**, não inversão de cores. Cada token tem
> hex **próprio** no dark (azul-escuro dessaturado, não preto + acento clareado). Ver a tabela
> de tokens (seção 6): toda linha tem coluna Light **e** Dark.

- **Troca automática** via `QStyleHints` (Qt6) seguindo o tema do sistema (KDE), com opção
  manual (claro/escuro/auto) no rodapé da sidebar.
- **Acentos** (`#3B82F6`, `#16C784`, `#EA3943`, `#E8A93B`) são os mesmos hex em light e dark
  (já calibrados para funcionar nos dois).
- **Navy da sidebar/statusbar** é idêntico nos dois temas (tom próprio, seção 6.2).
- **Fundos e superfícies** mudam: fundo `#EEF3F9` → `#0F172A`; surface `#FFFFFF` → `#1B2430`.
- **Verificação obrigatória:** todo componente novo precisa ser conferido em light **e** dark
  antes de "pronto", com o contraste validado nos dois.

---

## 17. Regras de ouro

*O resumo de bolso. Se ler só uma seção, leia esta.*

1. 🎯 **A personalidade decide tudo.** Na dúvida, escolha o mais **calmo, claro e acolhedor**
   para o leigo.
2. 🎨 **Token, nunca hex solto.** Toda cor é `@@TOKEN@@` semântico. Hex espalhado quebra o dark.
3. 🟥 **Valor negativo é SEMPRE `#EA3943`; positivo SEMPRE `#16C784`.** Nunca inverter.
4. 🌈 **Cor nunca sozinha.** Status sempre com sinal/rótulo/ícone também (daltônicos).
5. 😀 **Emoji só via `Theme::emojiIcon`** nos botões, nunca emoji-no-texto. E cuidado com o
   **VS16/FE0F** (força caixa colorida onde se quer mono).
6. 🔘 **`#iconButton` NUNCA tem caixa em repouso.** Realce só no hover/foco. Botão **com texto**
   mantém a caixa.
7. 🟦 **A sidebar/statusbar é sempre navy** (não inverte com o tema). Item ativo = pill azul.
8. 🔵 **`#3B82F6` não serve de texto sobre azul-claro no light** → use `#1D4ED8` (AA).
9. 🌓 **Dark é redesenho, não inversão.** Cada token tem hex próprio nos dois temas.
10. 📐 **Card = raio 14, botão/input = raio 9.** É a "cara" do app.
11. 📊 **Comunicar > enfeitar.** Gráfico/visual conta a história; sem 3D, sem ruído.
12. 🧩 **O mockup HTML é direção, não conversão 1:1.** Stack é Qt6/QSS; o engenheiro traduz a
    intenção.
13. ✋ **Nome e ícone do PeuBud são canônicos.** Não se redesenham sem ordem do líder.
14. ♿ **WCAG 2.2 AA é piso.** Contraste 4.5:1 texto / 3:1 componente, verificado em light e dark.

---

## 18. Referências

*De onde vêm as decisões deste manual.*

### Fontes de verdade no repositório

| Doc | O que define |
|---|---|
| `src/ui/Theme.cpp` | **Autoridade dos tokens** (hex light+dark, raios, fontes, QSS). |
| `docs/superpowers/specs/2026-06-19-redesign-ui-design.md` | Design doc do redesign completo (#163-#173). |
| `docs/design/typography-help-blocks.md` | Spec dos blocos de ajuda Descrição + Instruções (#166). |
| `.superpowers/sdd/spec-botoes-icone.md` | Spec do `#iconButton` sem-caixa (#165). |
| `docs/design/mockups/` | Mockups HTML de referência visual (9 telas + variações). |
| `docs/design/mockups/README.md` | Índice dos mockups. |
| `.claude/CLAUDE.md` | Stack, público leigo, prioridade Precisão > Robustez > Segurança > UX. |

### Referências externas de UI (do brainstorm)

- **Atlassian Design System**: tipografia aplicada e padrões de mensagem inline.
- **Eleken, Modern fintech design guide**: voz "human, transparent, calm"; educação contextual.
- **Linear, design refresh**: "estrutura sentida, não vista"; acento com moderação.
- **USWDS / CFPB**: acessibilidade para público amplo (contraste, x-height, espaçamento).
- **uxmovement, eyebrow text**: rótulo maiúsculo de acento como metadado.

---

> **Manutenção deste manual.** Quando uma decisão de design nova for tomada e aprovada pelo
> líder, atualize a seção correspondente **e** o `Theme.cpp` no mesmo passo, citando o ID do
> item no commit (`docs(#NN): ...`). Em divergência entre este doc e o código, o `Theme.cpp`
> é a autoridade: corrija o doc. Implementação sempre via `frontend-engineer`.
