# fix-iconbtn-1render-v2-report

## Status
Implementado — pendente verificacao no KDE real (offscreen nao reproduz o quirk).

## Hash do commit
(ver git log apos este arquivo ser comitado — branch redesign-ui)

## Abordagem escolhida
**Repolish DEFERIDO via `QTimer::singleShot(0, ...)`**

O lambda agendado executa na proxima iteracao do event loop, depois que o
ciclo de polish/layout inicial ja assentou. Esse e exatamente o mesmo timing
que um resize dispara — e que o usuario confirmou consertar o visual no KDE
real (Plasma/Wayland/Fusion).

## Por que deve funcionar no KDE real (e nao no offscreen)

O quirk ocorre porque:
1. Qt Fusion aplica a regra-base `QPushButton { border:1px; background:... }`
   no primeiro polish do widget durante a sequencia de show/layout inicial.
2. A regra especifica `QPushButton#iconButton { border:none; background:transparent }`
   e selecionada por objectName e tem especificidade maior, MAS e avaliada no
   mesmo passo de polish imediato — antes do layout assentar no Wayland.
3. O resultado e que a regra-base vence no primeiro render.

O fix anterior (commit 5858671) chamava `unpolish/polish` SINCRONAMENTE dentro
do `showEvent`, ou seja, no mesmo passo de polish que causava o problema —
logo era sobrescrito pela sequencia normal de polish do Qt e nao tinha efeito.

`singleShot(0)` difere a execucao para depois que o event loop processa o show
event inteiro (incluindo o layout do Wayland compositor). Nesse ponto o ciclo
inicial ja terminou, e o repolish forcado aplica a regra `#iconButton` de forma
definitiva — identico ao que o resize faz quando o usuario arrasta a janela.

## Linha de teste (offscreen)
```
QT_QPA_PLATFORM=offscreen ctest --test-dir build: 40/40 Passed (1.59s)
```

Observacao: `test_iconbtn_render` (test #38) verifica a presenca do objectName
e a ausencia de borda no offscreen. O offscreen nao reproduz o quirk (sempre
passa), por isso o criterio de aceitacao REAL e o usuario confirmar no KDE
Plasma/Wayland rodando o build DEBUG.

## Verificacao final (requerida do usuario)
Rodar `./build/peubud`, efetuar login, abrir o Dashboard e verificar se os
botoes ◀️ ▶️ (e 🔒 🖨️) aparecem SEM caixa (borda+fundo) no primeiro render,
sem precisar redimensionar a janela.

## Arquivos alterados
- `src/ui/widgets/DashboardWidget.cpp`
  - Adicionado `#include <QTimer>`
  - `showEvent`: repolish imediato -> `QTimer::singleShot(0, ...)` com lambda;
    adicionado `btn->update()` apos o polish para forcar repaint imediato.
