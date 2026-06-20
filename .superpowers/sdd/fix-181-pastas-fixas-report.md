# Report #181 — Pastas de save fixas

**Status:** Concluído  
**Commit:** `71ae570`  
**Branch:** redesign-ui  
**Teste:** `ctest 40/40` verde nos dois build-dirs (build + build-verif)

---

## Linha de teste

```
100% tests passed, 0 tests failed out of 40   (test_pastas_fixas: 10/10)
```

---

## Fluxo antigo (o que foi encontrado)

| Ponto | Caminho antigo |
|---|---|
| `DatabaseManager::open()` | `data/<slug>.pbd` (relativo ao CWD do processo) |
| `DatabaseManager::save()` | `data/<slug>.pbd` (idem) |
| `DatabaseManager::close()` | `data/<slug>.pbd` (idem) |
| `onAutosave()` | `AppSettings::autosavesFolder()/<slug>_<ts>.pbd` (timestampado, acumulava arquivos) |
| `onSalvar()` | `garantirPastaExiste("data")` → `DatabaseManager::save()` |
| `onSalvarComo()` | `QFileDialog::getSaveFileName` a partir de `savesFolder()` → `exportToFile(destEscolhido)` |
| `PrefsWidget` | Labels + QLineEdit editável + botão Browse para `savesFolder` e `autosavesFolder` |
| `AppSettings::savesFolder()` | `QSettings("saves_folder")` com default `Documents/PeuBud` |
| `AppSettings::autosavesFolder()` | `QSettings("autosaves_folder")` com default `savesFolder()/autosaves` |

---

## Fluxo novo (#181)

| Ponto | Caminho novo |
|---|---|
| `DatabaseManager::open()` | `AppSettings::savePathFixo(slug)` = `~/peubud/save/<slug>/database.pbd` |
| `DatabaseManager::save()` | idem |
| `DatabaseManager::close()` | idem |
| `onAutosave()` | `AppSettings::autosavePathFixo(slug)` = `~/peubud/save/<slug>/database.auto.pbd` (sobrescreve) |
| `onSalvar()` | Chama `DatabaseManager::save()` direto; sem `garantirPastaExiste("data")` para o cofre |
| `onSalvarComo()` | Export opcional (`exportToFile`) para destino escolhido pelo usuário; cofre principal não muda |
| `PrefsWidget` | Labels read-only exibindo os caminhos fixos; Browse removido |
| `AppSettings::savePathFixo(slug)` | `QDir::homePath()/peubud/save/<slug>/database.pbd` + `mkpath` |
| `AppSettings::autosavePathFixo(slug)` | `QDir::homePath()/peubud/save/<slug>/database.auto.pbd` + `mkpath` |
| `AppSettings::saveLegadoData(slug)` | `data/<slug>.pbd` (somente leitura pela migração) |

---

## Como a migração preserva o cofre existente

Executada em `DatabaseManager::open()` — função `migrarCofreSeNecessario()` (compilada apenas em `#ifndef QT_DEBUG`):

1. **Cofre novo já existe?** — retorna `true`, não faz nada. Idempotente.
2. **Cofre novo ausente + legado presente** (`data/<slug>.pbd`):
   - Copia legado → `<destino>.new` (escrita atômica — truncada se interrompida)
   - `rename(.new → destino)` — atômico no mesmo FS
   - `rename(legado → legado.bak)` — best-effort; se falhar (cross-device, FS read-only), loga warning mas o cofre novo já está seguro
   - **Nunca deleta o original** — fica como `<antigo>.bak`
3. **Nenhum cofre** — banco começa vazio (novo usuário). Correto.

Garantia adicional: `ensureSaveDirSecure()` faz `mkpath` da pasta `~/peubud/save/<slug>/` e `chmod 0700` antes de qualquer operação de I/O.

---

## O que foi removido do PrefsWidget

- `m_edtSavesFolder` (QLineEdit editável da pasta de saves)
- `m_edtAutosavesFolder` (QLineEdit editável da pasta de autosaves)
- Botões `btnBrowseSaves` e `btnBrowseAutosaves`
- Slots `onBrowseSavesFolder()` e `onBrowseAutosavesFolder()`
- `#include <QFileDialog>` (não mais necessário no PrefsWidget)

**Substituídos por:** dois `QLabel` read-only (`objectName="caminhoFixo"`) mostrando os caminhos fixos para informação do usuário.

Os getters/setters `savesFolder()`/`setSavesFolder()`/`autosavesFolder()`/`setAutosavesFolder()` foram **mantidos** em `AppSettings` (marcados deprecated no header) porque `BackupService` usa `diretorioPadrao()` próprio — não quebraria, mas a interface não foi removida para segurança.

---

## Correção colateral de link (#181)

`DatabaseManager.cpp` agora faz `#include "core/settings/AppSettings.h"` e chama `AppSettings::savePathFixo()`. Como 6 testes linkam `DatabaseManager.cpp` diretamente sem `AppSettings.cpp`, todos quebraram no linker. Corrigido adicionando `AppSettings.cpp` + `SecretStoreService.cpp` + `${LIBSECRET_LIBRARIES}` às chains de:

- `test_recorrencia`
- `test_resumo`
- `test_fechamento`
- `test_audit_triggers`
- `test_categoria`
- `test_gasto_service`

---

## Arquivos modificados

- `src/core/settings/AppSettings.h` — `savePathFixo`, `autosavePathFixo`, `saveLegadoData` (estáticos)
- `src/core/settings/AppSettings.cpp` — implementação dos 3 helpers
- `src/core/database/DatabaseManager.cpp` — `open/save/close` usam caminho fixo; migração transparente
- `src/main.cpp` — comentário atualizado (mkpath de `data/` para o `.db` temporário)
- `src/ui/MainWindow.cpp` — `onSalvar`, `onAutosave`, `onSalvarComo` atualizados
- `src/ui/widgets/PrefsWidget.cpp` — Browse removido; labels informativos adicionados
- `src/ui/widgets/PrefsWidget.h` — membros e slots do Browse removidos
- `tests/CMakeLists.txt` — novo `test_pastas_fixas` + correção de 6 chains de link
- `tests/test_pastas_fixas.cpp` — 10 casos TDD (formato, mkpath, migração, idempotência, novo usuário, atomicidade)
