# Fix #178 — warning com [Cancelar]/[Criar pasta] quando pasta de save/autosave não existe

**Status:** entregue — ctest 35/35 verde, build debug + release limpos  
**Branch:** redesign-ui  
**Commit:** ver hash abaixo

---

## Superfície do bug (onde a pasta pode não existir)

| Contexto | Pasta | Comportamento anterior | Comportamento novo |
|---|---|---|---|
| `onSalvar()` | `data/` (banco principal) | `encryptAndSave` falha, mostra "Não foi possível salvar." — sem causa | Chama `garantirPastaExiste("data")`; exibe aviso com [Criar pasta] / [Cancelar]; se cancelar → statusBar "Save cancelado." e `m_dirty` permanece `true` |
| `onSalvarComo()` | `savesFolder()` + diretório pai do destino final digitado no QFileDialog | `QDir().mkpath()` silencioso (ignora bool); `exportToFile` falha silencioso | Chama `garantirPastaExiste()` na pasta de início e no dir-pai do destino final; mesma proteção |
| `onAutosave()` | `autosavesFolder()` | `QDir().mkpath()` silencioso; falha ignorada | `mkpath` silencioso; se falhar → flag `m_autosaveAvisado` → **uma única mensagem** na statusBar por sessão (10 s); não abre popup por ciclo; sem crash do app |

---

## Implementação

### `MainWindow.h`
- `bool garantirPastaExiste(const QString &dir)` — declarado em `private:` com comentário explicando o invariante (não usar no autosave).
- `bool m_autosaveAvisado = false` — flag de sessão para evitar spam de aviso no autosave.

### `MainWindow.cpp` — `bool garantirPastaExiste(const QString &dir)`
- Fast path: `QDir(dir).exists()` → retorna `true` imediatamente.
- Ramo "não existe": `QMessageBox::Warning` com texto da pasta, botões `addButton` customizados [Criar pasta] (AcceptRole) / [Cancelar] (RejectRole), defaultButton = Criar pasta.
- [Criar pasta] → `QDir().mkpath(dir)`; se falhar → `QMessageBox::critical` e retorna `false`.
- [Cancelar] → retorna `false`; chamador não grava, `m_dirty` permanece inalterado.

### `onSalvar()` — novo guard
```cpp
if (!garantirPastaExiste(QStringLiteral("data"))) {
    statusBar()->showMessage(tr("Save cancelado."), 3000);
    return; // m_dirty permanece true — nenhum dado perdido
}
```
Após save bem-sucedido, zera `m_autosaveAvisado` (pasta voltou ao normal).

### `onSalvarComo()` — dois guards
1. Antes de abrir o QFileDialog: `garantirPastaExiste(startDir)`.
2. Após o QFileDialog retornar: `garantirPastaExiste(QFileInfo(dest).absolutePath())` — cobre caminho digitado manualmente com pasta inexistente.

### `onAutosave()` — sem popup, com flag
```cpp
if (!QDir(folder).exists() && !QDir().mkpath(folder)) {
    if (!m_autosaveAvisado) {
        m_autosaveAvisado = true;
        statusBar()->showMessage(tr("Auto-save: pasta '...' não existe..."), 10000);
    }
    return;
}
m_autosaveAvisado = false; // pasta existe/restaurada
```

---

## Teste (`tests/test_dir_guard.cpp`)

4 casos headless (QTEST_GUILESS_MAIN):

| Caso | O que testa |
|---|---|
| `pastaExistente_naoRequerCriacao` | mkpath em pasta existente → idempotente, retorna true |
| `pastaInexistente_mkpathCria` | mkpath cria pasta nova; QDir.exists() confirma |
| `subpastaInexistente_mkpathCriaRecursivo` | mkpath cria árvore aninhada (PeuBud/saves/autosaves) |
| `mkpathEmCaminhoInvalido_retornaFalse` | arquivo como componente de path → mkpath retorna false → helper retorna false sem crash |

O ramo QMessageBox (GUI) não é testável headless por design; a lógica de decisão coberta é o contrato de mkpath que o helper delega.

---

## Resultado

- `cmake --build build -j$(nproc)` — limpo (2 warnings nodiscard pré-existentes em BackupService, não relacionados)
- `cmake --build build-rel -j$(nproc)` — limpo
- `QT_QPA_PLATFORM=offscreen ctest --test-dir build` — **35/35 passed, 0 failed**
