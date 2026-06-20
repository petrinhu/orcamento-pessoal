# Mockups do redesign de UI (2026-06-19)

Mockups HTML de alta fidelidade da exploracao visual do redesign, abertos no Brave durante o brainstorm. Sao **referencia de design** (nao codigo de producao); as decisoes finais estao no spec.

> **Manual de estilo (canonico):** [`../../manual_estilo.md`](../../manual_estilo.md). Consolida TODAS as decisoes de design (paleta, tipografia, formas, componentes, icones, estados, a11y, dark/light). E a referencia unica daqui pra frente; estes mockups sao a referencia VISUAL que o alimenta.
> Spec: [`../../superpowers/specs/2026-06-19-redesign-ui-design.md`](../../superpowers/specs/2026-06-19-redesign-ui-design.md)
> Plano: [`../../superpowers/plans/2026-06-19-redesign-ui.md`](../../superpowers/plans/2026-06-19-redesign-ui.md)

Abrir local: `xdg-open docs/design/mockups/<arquivo>.html` (precisa de internet p/ as fontes/emoji via CDN).

## Indice

| Arquivo | O que mostra |
|---|---|
| `peubud_redesign_dashboard.html` | Direcao geral: paleta azul/navy, **sidebar agrupada**, cards de metrica, grafico, botoes. |
| `peubud_redesign_gastos.html` | **Tabela** moderna (densidade, tags categoria/tipo, hover, rodape-resumo) + barra de acoes. |
| `peubud_redesign_graficos.html` | **Graficos** (linha/area, donut por categoria, barras) na aba Tendencias. |
| `peubud_redesign_interacoes.html` | **Hover/orientacoes/menus**: rodape da sidebar, cluster "... Mais", tooltip de glossario, empty state. |
| `peubud_redesign_galeria.html` | **As 9 abas restantes** (Cartoes, Recorrencias, Lembretes, Planejamento, Metas, Dividas, Investimentos, Proventos, Bolsa), cada uma com seu padrao. |
| `peubud_redesign_ativo.html` | **Detalhe do ativo** (Bolsa, pos-1.0): candlestick dark + book de ofertas + ferramentas de tracado (LTA/LTB, S/R, Fib) + indicadores (MM/Bollinger/RSI) + botao IA. |
| `peubud_redesign_config.html` | **Configuracoes**: todas as secoes + aba **Assistente de IA** (Claude/GPT/Gemini/Grok/Ollama/OpenRouter/DeepSeek/Mistral/sem-IA). |
| `peubud_botoes.html` | Comparacao de estilos de **botao** (bevel x shadow x misto) com feedback de clique. |
| `peubud_currency_symbols.html` | Simbolos de **moeda** (Unicode): decisao foi que a Inter ja cobre, sem embutir fonte extra. |
| `peubud_tipografia_ajuda_no_mdash.html` | **Tipografia dos blocos de ajuda** do TabShell (Descricao + Instrucoes), light+dark, conteudo real (Cartoes/Entradas). Spec: [`../typography-help-blocks.md`](../typography-help-blocks.md). Sufixo `no_mdash` = contem em-dash literal da microcopy canonica de `TabHelp.cpp`. |
