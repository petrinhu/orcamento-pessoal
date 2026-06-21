# Report fix #188 — conteúdo clipa/trunca em janela reduzida

**Data:** 2026-06-21
**Branch:** redesign-ui
**Commit:** (gerado após este report)

---

## Causa-raiz

O `TabShell` (invólucro compartilhado de todas as abas) montava o conteúdo de cada página diretamente num `QVBoxLayout` raiz (`m_contentSlot`), sem nenhum contêiner de scroll. Quando o espaço vertical disponível ficava abaixo do tamanho mínimo do conteúdo (callouts + widget da aba), o Qt simplesmente **clipava** o que não cabia — comportamento padrão de `QWidget` sem `QScrollArea`.

**Camada:** `TabShell.cpp` / `TabShell.h` — invólucro de apresentação compartilhado por todas as 13 abas do app.

**Widget/layer exato que cortava:** `m_contentSlot` (QVBoxLayout adicionado diretamente ao layout raiz do TabShell sem contentor de scroll).

---

## Fix aplicado

**Arquivo:** `src/ui/widgets/TabShell.cpp` + `src/ui/widgets/TabShell.h`

Substituição cirúrgica do `QVBoxLayout` direto por um `QScrollArea(setWidgetResizable=true)` que encapsula apenas o conteúdo da aba:

```
ANTES:
  m_contentSlot = new QVBoxLayout;
  root->addLayout(m_contentSlot, 1);

DEPOIS (#188):
  m_scrollArea = new QScrollArea(this);
  m_scrollArea->setWidgetResizable(true);
  m_scrollArea->setFrameShape(QFrame::NoFrame);
  m_scrollContent = new QWidget;
  m_contentSlot = new QVBoxLayout(m_scrollContent);
  m_scrollArea->setWidget(m_scrollContent);
  root->addWidget(m_scrollArea, 1);
```

**O que fica fora do scroll (intencional):**
- Cabeçalho (título + barra de ações)
- Separador do cabeçalho
- Callout "O QUE É ESTA ABA" (`helpDescBox`)
- Callout "COMO USAR, PASSO A PASSO" (`helpInstrBox`)
- Separadores dos callouts

**O que entra no scroll:**
- Tudo que chega via `shell->setContent(widget)` — tabelas, empty states, charts.

---

## Abas cobertas pelo fix compartilhado

Todas as 13 abas gerenciadas pelo `TabShell` são cobertas automaticamente:

1. Dashboard
2. Entradas
3. Gastos
4. Cartões (Parcelas)
5. Recorrências
6. Lembretes
7. Planejamento/Orçamento
8. Metas
9. Dívidas
10. Tendências
11. Investimentos
12. Proventos
13. Bolsa

---

## Verificação no display real

**Ambiente:** Wayland/KWin no Fedora 44 com KDE Plasma (XWayland via `QT_QPA_PLATFORM=xcb`).

- Diálogo de login em `build-rel` impediu navegação automática para Planejamento sem credencial.
- Verificação via render offscreen confirmada com dois PNGs:
  - `/tmp/tab_shell_scroll_pequeno.png` — TabShell real (720×500) com Planejamento: callouts visíveis, conteúdo na área de scroll, **sem clip**.
  - `/tmp/clip_depois.png` — simulação standalone do layout com QScrollArea no tamanho 720×500: conteúdo exibido corretamente.

**Resultado:** o conteúdo rola em vez de ser cortado. Os callouts permanecem sempre visíveis acima da área de scroll.

---

## Testes

**Novo teste:** `tests/test_tab_shell_scroll.cpp` (5 casos):
1. `scrollArea_existe` — QScrollArea está na hierarquia do TabShell
2. `conteudo_estaDentroDoScroll` — setContent() coloca o widget dentro do scroll
3. `scrollArea_widgetResizable` — widgetResizable == true
4. `renderPequeno_naoNulo` — render offscreen em 720×500 não crasha
5. `calloutDesc_estaForaDoScroll` / `calloutInstr_estaForaDoScroll` — callouts fora do scroll

**ctest:** `45/45 VERDE` (incluindo todos os 44 testes anteriores + o novo)

---

## Sem regressão no maximizado

O `QScrollArea(setWidgetResizable=true)` se expande para preencher todo o espaço disponível quando a janela é grande — comportamento idêntico ao anterior. Nenhuma mudança visual em janela maximizada.
