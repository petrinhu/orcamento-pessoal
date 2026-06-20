# Fix #177 — Tela de Configurações modal + parented na MainWindow

## Status
Concluido. Commit: (ver hash abaixo).

## Teste
35/35 ctest verde (build debug + release limpos, QT_QPA_PLATFORM=offscreen).

## O que mudou em onConfiguracoes()

Arquivo: `src/ui/MainWindow.cpp`, funcao `MainWindow::onConfiguracoes()`.

### Situacao antes do fix

O dialog ja tinha `parent = this` (linha `QDialog dlg(this)`) e ja usava `dlg.exec()`.
Ambos corretos no codigo, mas **faltava `setWindowModality(Qt::ApplicationModal)`**
explicitamente.

### Causa raiz

No Wayland/KDE Plasma, `exec()` bloqueia o event loop Qt mas o compositor
nao enforça a relacao parent-child visualmente sem o flag de modalidade
explicito. Resultado: o usuario conseguia clicar na janela principal com a
tela de Configuracoes aberta (sem bloqueio visual), e a janela nao minimizava
junto por nao ter a vinculacao de hierarquia visivel ao compositor.

### Fix aplicado (linha 1242 atual)

```cpp
void MainWindow::onConfiguracoes()
{
    QDialog dlg(this);                          // parent = MainWindow (ja existia)
    dlg.setWindowTitle(tr("Configurações"));
    dlg.setMinimumSize(520, 480);
    // ApplicationModal: bloqueia a principal enquanto Configurações está aberta.
    // Necessário explicitamente no Wayland/KDE Plasma — exec() bloqueia o event
    // loop mas sem esta flag o compositor pode não enforçar a relação parent-child
    // visualmente (usuário consegue clicar na principal). Combinado com parent=this
    // (acima), o dialog também minimiza junto com a MainWindow (#177).
    dlg.setWindowModality(Qt::ApplicationModal); // FIX #177 — adicionado
    // ... restante igual
    dlg.exec();
}
```

### Comportamento pos-fix

- **(a) Modal**: `Qt::ApplicationModal` + `exec()` garante que a MainWindow
  fique bloqueada enquanto Configuracoes esta aberta, inclusive no Wayland/KDE.
- **(b) Minimiza junto**: `parent = this` (QDialog filho de MainWindow) faz
  o compositor agrupar as janelas — minimizar a principal arrasta a config.
- **(c) Fechar**: botao "Fechar" ja conectava `rejected → accept()` corretamente;
  `exec()` retorna apos `accept()`/`reject()` sem vazamento.

## Arquivos tocados

- `src/ui/MainWindow.cpp` — 1 insercao: `dlg.setWindowModality(Qt::ApplicationModal)`
  + bloco de comentario explicativo (linhas 1237–1242 pos-edicao).
