# Design Doc: Redesign de UI do PeuBud (moderno)

Status: aprovado na exploracao visual (brainstorm + mockups), 2026-06-19.
Escopo: redesign completo da interface. Implementacao = Onda 9 (itens #163-#173), roda ANTES da distribuicao (Onda 8).
Mockups de referencia: `/tmp/peubud_redesign_*.html` (Dashboard, Gastos/tabela, Tendencias/graficos, Entradas/interacoes, galeria das 9 abas, ativo, config).

---

## 1. Contexto e objetivo

PeuBud e um app de financas pessoais **para leigos** (pessoas comuns, nao profissionais). A UI atual parece "anos 90": barra de abas no topo estourando com 13 abas, barra de menu classica, botoes flat, pouca hierarquia. Objetivo: um visual **moderno, amigavel e confiavel** que ensine enquanto o usuario usa.

Restricoes fixas (canonicas, NAO mudam): **nome "PeuBud" e o icone do app**. Stack: C++20 / Qt6 (Widgets) / QSS. O Qt nao tem `box-shadow` nativo no QSS; sombras reais via `QGraphicsDropShadowEffect` ou pintura.

## 2. Direcao de design (decidida)

- **Personalidade:** amigavel (B) com a paleta de **azuis confiaveis** (A). Cards arredondados, emoji colorido (ja implementado nesta fase), microinteracoes, linguagem simples.
- **Principios:** calmo/confiavel (azul/navy/branco), data storytelling (mostrar visual > numero cru), minimalismo com cards, 1 acento, hierarquia tipografica forte, microinteracoes com feedback, acessibilidade, "uma aba por coisa".

## 3. Design system (tokens) -- #164

### Paleta (light)
- Fundo: `#EEF3F9` (azul-acinzentado suave) | Superficie/cards: `#FFFFFF` | Superficie-2: `#F7FAFD`
- Sidebar: degrade navy `#1B2A41 -> #16233A`, texto `#AFBDD0`
- Primario/acento: `#3B82F6` (hover `#5CC6FA`, pressed `#1F7AAD`, soft `#E7F0FE`)
- Texto: `#15233B` | Subtexto: `#67768D` | Borda/divisoria: `#E4EAF2`
- Positivo: `#16C784` (esmeralda) | Negativo: `#EA3943` | Dourado: `#E8A93B`
- Categoricos (graficos): azul, esmeralda, dourado, vermelho, roxo `#8B5CF6`, slate `#94A3B8`

### Paleta (dark)
- Fundo `#0F172A`/`#131A24`, superficie `#1B2430`, mesmos acentos (mais vivos). Sidebar mantem o navy.

### Tipografia
- **Inter** (ja embutida): corpo 13-14px, titulos 18-23px peso 700, subtexto 12-12.5px, numeros com `font-variant-numeric: tabular-nums`.

### Forma / elevacao
- Raios: cards 14px, botoes/inputs 9px, tags/pills 20px.
- Sombras: card repouso `0 1px 3px rgba(20,40,80,.05)`; card hover sobe 2px + sombra maior; botao primario com sombra de acento; pressed = afunda (ja implementado: ButtonShadowFilter + QSS `:pressed`).
- Espacamento base 4px; padding de card 16-20px; gap de grid 14-16px.

## 4. Navegacao: sidebar agrupada a esquerda -- #165

Substitui o `QTabWidget` no topo. Sidebar navy fixa (largura ~236px), **colapsavel** (vira so icones)
na concepcao original deste doc de design — **nao entregue** na implementacao final; corte de
escopo decidido pelo lider (#165/DIST-12, 2026-07-10). Ver `docs/manual_estilo.md` §8 para o
comportamento real (sidebar sempre expandida, sem colapso):
- Topo: icone do app (canonico) + wordmark "PeuBud".
- Item "Dashboard" solto no topo.
- Grupos com rotulo maiusculo pequeno:
  - **Dia a dia:** Entradas, Gastos, Cartoes, Recorrencias, Lembretes
  - **Planejar:** Planejamento/Orcamento, Metas, Dividas, Tendencias
  - **Investir:** Investimentos, Proventos, Bolsa
- Item ativo: **pill azul** com sombra; hover: leve clareada.
- **Rodape da sidebar** (substitui parte da menu bar): Preferencias, Ajuda, Tema (claro/escuro/auto), e o usuario (avatar + nome).
- **Navegacao por chave estavel** (objectName/enum), nao por indice -> resolve o #157 (cards do Dashboard / busca global iam pra aba errada apos arrastar).

## 5. Layout de conteudo (por aba)

De cima para baixo:
1. **Header:** titulo da aba + subtitulo curto; a direita: navegacao de mes (quando aplica) + cluster de utilidades (busca, calculadora).
2. **Banner de ajuda** (azul suave, recolhivel): descricao da aba + link "Como usar" que expande as instrucoes (refina o TabShell atual). -- #166
3. **Barra de acoes:** CTA primario (ex.: "Novo gasto") + secundarios + busca.
4. **Conteudo:** cards / tabela / graficos.

## 6. Componentes base -- #166

- **Cards** arredondados com hover-lift; **chip de emoji** colorido como icone do card.
- **Tags/pills** coloridas (categoria, tipo fixo/variavel, status).
- **Banner de ajuda** azul recolhivel.
- **Tooltips de glossario:** termos (IRRF, INSS, CET, DY...) com sublinhado pontilhado -> tooltip escuro explicando em linguagem simples no hover. (Substitui o GlossarioDecorator e seu bug de sobrescrever.)
- **Empty states guiados:** emoji grande + frase + CTA ("Nenhuma entrada ainda... + Novo lancamento").
- **Botoes:** ja implementado nesta sessao (emoji via `Theme::emojiIcon`, sombra projetada, afundar no clique).

## 7. Tabelas -- #167

- Densidade **confortavel** (linha ~48px); opcao "Compacta" nas Preferencias.
- Cabecalho sutil (maiusculas pequenas, fundo levissimo); **so divisorias horizontais finas**.
- **Hover de linha** (azul suave); valores a direita, tabulares, negrito.
- Categoria/Tipo como **tags** coloridas.
- **Rodape-resumo** com mini-barra (ex.: % de fixos).

## 8. Graficos -- #168

- **Linha/area** (tendencia): linha + area com degrade, gridlines finas, ponto de pico destacado.
- **Donut** (proporcao): rosca com total no centro + legenda com %.
- **Barras** (comparacao): arredondadas, valor no topo, eixo discreto.
- Paleta categorica harmonica; "comunicar > enfeitar" (sem 3D exagerado).
- Hover com tooltip de valor exato.

## 9. Microinteracoes / hover / orientacoes

- Hover: cards sobem, linhas realcam, botoes ganham sombra, tooltips aparecem.
- Glossario explicado no hover (item 6).
- Banner de ajuda + "Como usar" + central de Ajuda (rodape, F1).
- Empty states orientam quem esta comecando.

## 10. Menus -- #170

Aposenta a `QMenuBar` classica (Arquivo/Editar/Configuracoes/Ajuda). No lugar:
- **Rodape da sidebar:** Preferencias, Ajuda, Tema, usuario.
- **Cluster topo-direito:** busca global, calculadora, e **"... Mais"** (dropdown): Importar extrato, Exportar, Reconciliar, Imprimir, Sobre.

## 11. Redesign por aba -- #169

Mesmo sistema; cada aba com um padrao proprio:
- **Dashboard:** cards de metrica (emoji + numero + micro-delta) + grafico Entradas x Gastos + fechar mes / imprimir.
- **Entradas:** tabela + impostos retidos; tooltip de glossario (IRRF/INSS).
- **Gastos:** tabela com tags (categoria, fixo/variavel) + rodape Fixos/Variaveis.
- **Cartoes:** "cartoes" visuais (fatura + barra de limite usado) + tabela de parcelas.
- **Recorrencias:** lista com tags de frequencia/proxima data + **interruptor ligar/pausar**.
- **Lembretes:** **status-cards** (borda colorida) + contagem regressiva dinamica + Pagar.
- **Planejamento/Orcamento:** **barra por categoria** (real vs planejado) com cor de aderencia.
- **Metas:** cards com **anel de progresso** + Aporte.
- **Dividas:** card por divida com **barra de quitacao** + juros a.m./a.a. + status.
- **Tendencias:** abas-pill (Mensal/Categoria/Pagamento/Fixo x Variavel) + cards de resumo + graficos.
- **Investimentos (placeholder pos-1.0):** donut de alocacao + carteira; no 1.0 fica como aba-placeholder.
- **Proventos (placeholder pos-1.0):** barras mensais + lista por ticker.
- **Bolsa (placeholder pos-1.0):** busca + cards de cotacao.

## 12. Bolsa: detalhe do ativo -- #171 (POS-1.0, placeholder)

Abre ao clicar numa acao. Estilo trading:
- **Grafico candlestick dark** (velas verde/vermelho, MM20/MM50, Bandas de Bollinger, volume, grid pontilhado, seletor de periodo).
- **Book de ofertas:** vendas (vermelho) em cima, compras (verde) embaixo, barra de profundidade, spread.
- **Ferramentas de tracado:** Linha de Tendencia (LTA/LTB), Suporte/Resistencia, Canal, Fibonacci, retangulo, anotacao, apagar.
- **Indicadores:** Medias Moveis, Bollinger, RSI (painel inferior), MACD. Reusa `IndicadoresTecnicos` (#52) e `SuporteResistencia` (#53) que ja existem no codigo.
- **Dados:** candlestick/historico via brapi/yfinance (viavel); **book nivel 2 = fonte a definir** (corretora/B3 paga/crawling).
- **POS-1.0:** so placeholder no 1.0.

## 13. Assistente de IA 🪄 -- #172 (POS-1.0, EPIC, placeholder)

Botao "Auxilio de IA" contextual em cada aba + um assistente global. Tudo isto e **placeholder pos-1.0**.

- **(a) 🪄 per-aba** (onde gera valor):
  - Alto: Dashboard (resumo do mes em 1 paragrafo), Gastos (categorizar automatico), Planejamento (sugerir orcamento), Dividas (estrategia de quitacao), Bolsa (ler o grafico em linguagem simples).
  - Medio: Entradas (imposto IRRF/INSS), Cartoes (vale parcelar?), Tendencias (explicar padroes), Metas (quanto poupar/mes), Investimentos (diversificacao), Proventos (projecao/tributacao).
- **(b) 🪄 global:** chat "pergunte qualquer coisa sobre suas financas".
- **(c) Ler arquivos:** PDF, CSV, OFX, XLSX, imagem -> a IA **extrai os dados e preenche** (entradas/gastos/cartoes), com revisao do usuario antes de salvar.
- **(d) Provedores (Config > Assistente de IA):** Claude, GPT, Gemini, Grok, Ollama (local), OpenRouter, DeepSeek, Mistral, e **"Sem IA"** (offline). **Radio** escolhe o ativo quando ha mais de um. Chaves no **keystore** (libsecret).
- **(e) Aviso de privacidade:** IA na nuvem **envia dados financeiros** ao provedor; destacar o Ollama (local) e o "Sem IA".
- **(f) Disclaimer:** Dividas/Investimentos/Bolsa = **educativo**, nao consultoria financeira.

## 14. Tela de Configuracoes -- #173

Sub-navegacao a esquerda + secoes no novo estilo: Aparencia (tema/cor/densidade/fonte), Moeda, Categorias (cor + tipo), Fontes de renda, Cartoes, Mercado (provedor + token + book toggle), **Assistente de IA** (#172), Backup, Seguranca, Sobre.

## 15. Escopo 1.0 vs Pos-1.0

- **No 1.0 (este redesign):** design system (#164), sidebar agrupada (#165), componentes (#166), tabelas (#167), graficos (#168), redesign das abas do dia-a-dia e de planejar (#169), menus (#170), tela de Configuracoes (#173).
- **Pos-1.0 (placeholders, criar depois):** **Investimentos** (visual completo), **Bolsa** (detalhe do ativo + analise tecnica, #171), **Assistente de IA** (#172). No 1.0, essas abas existem como placeholders simples.

## 16. Implementacao (alto nivel)

- Tokens no `Theme` (QSS + getters); novo shell de `MainWindow` com sidebar (substitui QTabWidget); componentes reutilizaveis; migracao aba a aba; navegacao por chave.
- Reaproveita o que ja foi feito nesta sessao: emoji colorido (`Theme::emojiIcon` + Noto embutida), botoes com sombra/feedback, cores do Metas, #161/#162.
- Plano detalhado de implementacao: proximo passo (skill writing-plans).

## 17. Riscos / decisoes abertas

- Refactor grande do `MainWindow` (sidebar substitui QTabWidget): migrar 13 abas + navegacao + persistencia.
- Qt + color emoji em botoes: ja resolvido (emojiIcon).
- Book de ofertas (nivel 2): fonte de dados real a definir (pos-1.0).
- IA: custo, privacidade, latencia, e como expor sem assustar o leigo (pos-1.0).
- Dark mode: garantir contraste de todos os tokens novos.
