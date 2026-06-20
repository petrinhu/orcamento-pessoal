# Report fix #174 — Enter no login dispara Entrar, não Cancelar

## Status
CONCLUIDO. Commit na branch `redesign-ui`.

## Causa raiz
`m_btnCancelar` era construído antes de `m_btnEntrar` em `buildUi()` e nenhum
tinha `setDefault()`/`setAutoDefault()` explícitos. O Qt elege o primeiro
`QPushButton` como `autoDefault=true` implicitamente → Cancelar capturava Enter
→ `QDialog::reject()` → `main.cpp` recebia `!= Accepted` → app encerrava.

## Fix aplicado — `src/ui/LoginDialog.cpp`

Três linhas adicionadas em `buildUi()` após a criação dos botões:

1. `m_btnEntrar->setDefault(true)` — Enter sempre dispara Entrar.
2. `m_btnCancelar->setAutoDefault(false)` — Cancelar nunca captura Enter,
   nem como fallback quando Entrar está desabilitado.
3. `connect(m_nome, &QLineEdit::returnPressed, ...)` — belt-and-suspenders:
   Enter no campo nome invoca `onEntrar()` diretamente quando habilitado.
4. `connect(m_senha, &QLineEdit::returnPressed, ...)` (protegido por
   `#ifndef QT_DEBUG`) — mesmo comportamento no campo senha em release.

## Teste de regressão — `tests/test_login_enter.cpp`

Três contratos verificados headless (`QT_QPA_PLATFORM=offscreen`, compilado
com `QT_DEBUG` para pular bloco de cripto/libsecret em runtime):

| Contrato | Verificação |
|---|---|
| C1 | `m_btnEntrar->isDefault() == true` |
| C2 | `m_btnCancelar->autoDefault() == false` |
| C3 | `returnPressed` no campo nome dispara `QDialog::accepted` quando form válido |

## Resultado de build e testes

- `cmake --build build -j$(nproc)` — limpo, 0 warnings novos
- `cmake --build build-rel -j$(nproc)` — limpo
- `ctest --test-dir build` — **36/36 passed** (100%)

## Arquivos modificados

- `src/ui/LoginDialog.cpp` — fix em `buildUi()`
- `tests/test_login_enter.cpp` — novo arquivo (regressão)
- `tests/CMakeLists.txt` — registro de `test_login_enter`
