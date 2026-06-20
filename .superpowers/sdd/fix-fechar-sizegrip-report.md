# Report: fix(#176) — Sombra Fechar + QSizeGrip branco

**Branch:** redesign-ui  
**Data:** 2026-06-20  
**Commit:** (ver hash abaixo)

---

## Status

- Bug A (QSizeGrip branco): corrigido
- Bug B (sombra clipada em onConfiguracoes): corrigido via linha manual
- Bug B+ (varredura geral): todos os QDialogButtonBox do projeto receberam margem interna

---

## Bug A — QSizeGrip branco na statusbar

**Causa:** O QSizeGrip nativo do QStatusBar não herda o QSS navy da statusbar e
renderiza como um quadrado branco no canto inferior-direito ao redimensionar.

**Fix aplicado em** `src/ui/MainWindow.cpp` — função `buildUi()`, antes de
`addPermanentWidget(m_statusTotal)`:

```cpp
statusBar()->setSizeGripEnabled(false);
```

A janela continua redimensionavel pelas bordas do frame — apenas o handle visual
some. Comportamento identico ao esperado.

---

## Bug B — Sombra do botao Fechar das Configuracoes clipada

**Causa raiz:** O `QDialogButtonBox(Close)` é o parent imediato do QPushButton.
O `QGraphicsDropShadowEffect` (blur=8, offsetY=1, extensao ~5px) é clipado no
bounding box do `QDialogButtonBox`, que tem zero padding interno. A margem de
16px no QVBoxLayout do dialogo atua em nivel acima do clip — nao resolve.

**Fix aplicado:** Opcao 1 (linha manual), em `src/ui/MainWindow.cpp`,
funcao `onConfiguracoes()`:

- Removido: `auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);`
- Adicionado: QHBoxLayout `btnRow` com `setContentsMargins(0, 6, 10, 10)` +
  QPushButton filho direto de `&dlg` + `Theme::emojiIcon("❌", 14)`.
- `lay->setContentsMargins` ajustado de `(12,12,12,16)` para `(12,12,12,12)`
  (bottom 16 era workaround; nao e mais necessario).

**Por que a linha manual funciona:** o QPushButton e parented em `&dlg`
diretamente; o clip do layout ocorre no nivel do dialogo, nao de um box interno.
As margens combinadas (lay bottom=12 + btnRow bottom=10) dao 22px de folga
para a sombra de ~5px.

---

## Verificacao de render — /tmp/fechar_fix.png

Snippet offscreen compilado e executado com `QT_QPA_PLATFORM=offscreen`.
Resultado: sombra do botao "X Fechar" **inteira** (sem clip em borda inferior
nem direita). O botao flutua no canto inferior-direito com gradiente de sombra
completo visivelmente separado da borda do dialogo.

---

## Bug B+ — Varredura completa de QDialogButtonBox

`grep -rn 'QDialogButtonBox' src/` retornou 22 arquivos com uso.
Fix aplicado (`btns->setContentsMargins(0, 6, 10, 10)`) em todos:

### Corrigidos — dialogs de fechar (Close isolado):

| Arquivo | Tipo |
|---|---|
| `src/ui/MainWindow.cpp` | linha manual (ver Bug B) |
| `src/ui/widgets/HelpDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/AboutDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/AmortizacaoDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/ReconciliacaoDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/BuscaGlobalDialog.cpp` | setContentsMargins no box |

### Corrigidos — dialogs CRUD (Save/Ok + Cancel):

| Arquivo | Tipo |
|---|---|
| `src/ui/widgets/GastoDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/LancamentoEntradaDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/AddCompraDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/InvestimentoDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/MetaDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/LembreteDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/DividaDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/RecorrenciaDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/ItemOrcamentoDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/CategoriaDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/ProventoDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/AjusteCorporativoDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/PagadorDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/ExportDialog.cpp` | setContentsMargins no box |
| `src/ui/widgets/ParcelasWidget.cpp` (2x) | setContentsMargins no box |

### Nao ajustados (sem risco):

- `src/ui/Theme.cpp:370` — QSS rule `QDialogButtonBox QPushButton { min-width: 80px }`,
  nao e instanciacao de widget, sem clip.

---

## Testes

- `cmake --build build -j$(nproc)`: OK (warnings nodiscard pre-existentes, sem novos erros)
- `cmake --build build-rel -j$(nproc)`: OK
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build`: 36/36 passou
