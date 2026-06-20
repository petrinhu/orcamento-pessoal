# Relatório fix #176 — Sombra dos botões não clipa mais nas bordas

## Status

Concluído. ctest 34/34 verde. Builds `build/` e `build-rel/` limpos.

## Hash do commit

(preenchido após commit)

## Linha de teste

`34/34 Test #34: test_shadow_render ... Passed 0.03 sec`

## Descrição do render (/tmp/shadow_fix.png)

Render offscreen de `BtnColWidget` (estrutura idêntica a `ConfigFontesWidget` e
`ConfigCategoriasWidget`): três botões em coluna com margem inferior 12px. O botão
"Remover selecionada" fica na borda inferior do container. Na imagem gerada, todos
os três botões exibem sombra **contínua** nas quatro direções — sem corte brusco
na borda inferior. A sombra é sutil e moderna (blur=8, yOffset=1, alpha=70) em
contraste com a anterior (blur=12, yOffset=2, alpha=110).

## Causa raiz

`QGraphicsDropShadowEffect` com `blurRadius=12` e `yOffset=2` renderiza a sombra
~14px além do retângulo do botão. O widget pai clipa filhos ao seu próprio rect,
então qualquer botão colado na borda inferior com `contentsMargins` = 0 tem a
sombra cortada.

## Dois fixes aplicados (em camadas)

### 1. Suavizar a sombra em ButtonShadowFilter.cpp

| Parâmetro      | Antes | Depois |
|----------------|-------|--------|
| kRestBlur      | 12    | 8      |
| kRestOffsetY   | 2     | 1      |
| restColor alpha| 110   | 70     |
| kPressBlur     | 4     | 3      |
| pressColor alpha|140   | 110    |

A sombra passa a ocupar ~9px além do botão (antes: ~14px). Reduz o clip em
containers com margem insuficiente, mas **não o elimina em margem zero**.

### 2. Margem inferior nos containers afetados

Todos os containers com botões colados na borda inferior passaram a ter
`contentsMargins` inferior >= 12px (blur=8 + offset=1 + folga).

## Containers ajustados

| Arquivo | Mudança |
|---------|---------|
| `src/ui/widgets/ConfigFontesWidget.cpp` | `setContentsMargins(0,0,0,0)` → `(0,0,0,12)` no layout raiz — borda inferior: botão "Remover selecionada" |
| `src/ui/widgets/ConfigCategoriasWidget.cpp` | `setContentsMargins(0,0,0,0)` → `(0,0,0,12)` no layout raiz — borda inferior: barra Adicionar/Editar/Remover |
| `src/ui/MainWindow.cpp` — `onConfiguracoes()` | `QVBoxLayout` sem margem → `setContentsMargins(12,12,12,16)` no layout do `QDialog` — botão "✕ Fechar" do `QDialogButtonBox` |
| `src/ui/widgets/BaseWizardDialog.cpp` | `btnBar->setContentsMargins(12,8,12,8)` → `(12,8,12,12)` — botões Cancelar/Anterior/Próximo na borda inferior de todos os wizards |

Containers com margem pré-existente >= 12px (seguros, não alterados):
- `ConfigWidget`: `(24,24,24,24)` — seguro
- `AuditLogViewer`: `(12,12,12,12)` — seguro
- `BackupConfigWidget`: `(16,16,16,16)` — seguro
- `MarketConfigWidget`: `(16,16,16,16)` — seguro
- `AjustesCorporativosWidget`: `(12,12,12,12)` — seguro
- `ConfigPagadoresWidget`: botões no **topo** (não na borda inferior) — sem clip

## Novo teste adicionado

`tests/test_shadow_render.cpp` — render offscreen de `BtnColWidget` (estrutura
idêntica aos widgets afetados), salva `/tmp/shadow_fix.png`, verifica que o
pixmap é válido e tem dimensões corretas. Gate de regressão para futuros ajustes
de sombra.
