# Fatia 4 — Integração `.pbd` v2 autocontido + rewire do serviço (ADR-001)

Spec de implementação (software-architect, aprovado pelo líder 2026-07-11). Fecha a lacuna do ADR §8 passo 6. Fatias 1-3 no HEAD (`5d063e3`), suíte 81/81.

**Decisão do líder (2026-07-11):** invite key embrulha um **marcador descartável** (não o `K` real). O `K` real é gerado pelo próprio usuário no 1º login → invite key vira inerte após 1º uso, admin nunca toca no `K` de ninguém.

## D1 — Onde cada wrap mora (autoritativo)
- **`.pbd` v2 (`VaultHeaderData`) é a ÚNICA fonte de verdade** de `saltPw, saltRk, kdfOps, kdfMem, wrappedKeyByPass, wrappedKeyByRecovery` de **qualquer** conta (admin e comum). O struct (Fatia 3) já tem esses 7 campos — **não muda**.
- **`users.json` deixa de guardar esses 6 campos.** Fica com: `username`, `role`, `invitePending` (informativo), `admin_pubkey`, `signature`, e — só para `role==Admin` — `wrappedSignKeyByPass`/`wrappedSignKeyByRecovery`. E o wrap de convite (ver D3), enquanto `invitePending`.
- Mutação de senha/recovery de usuário comum toca **só o `.pbd` dele** (sem admin, sem assinatura).

## D2 — Chicken-egg da assinatura do admin
Reusar o **mesmo `kdfPass`** do cofre do admin p/ desembrulhar a signing key (um único Argon2id no login):
1. `verifyLogin` lê o header do `.pbd` do usuário (autoautenticado pelo AEAD do secretbox).
2. Deriva `kdfPass` de `header.saltPw/kdfOps/kdfMem` (1 Argon2id).
3. Desembrulha `header.wrappedKeyByPass` → `K` (prova senha).
4. Se `role==Admin`: mesmo `kdfPass` (custo zero) desembrulha `wrappedSignKeyByPass` (do **registro**, não do header) → `m_adminSignKey`.
5. Verificação Ed25519 do `users.json` roda antes, só com a **pubkey** (sem segredo). Sem ciclo.
Nenhum salt novo, nenhum `kdf_params` sobrevive no registro.

## D3 — Bootstrap do cofre no 1º login (marcador descartável)
`createUser()` NÃO gera mais o `K` real. Gera um **marcador aleatório de 32B** e embrulha ELE sob `KDF(invite_key)`. O `K` real nasce no `completeFirstLogin`.

`completeFirstLogin(username, inviteKey, newPassword)`:
- Guarda: `if (DatabaseManager::instance().isOpen()) erro` (defensivo).
1. `m_repo.loadAll()` (verifica assinatura/âncora).
2. Acha o registro; exige `invitePending==true` **e** `.pbd` do slug **não existe** (idempotência).
3. `kdfInvite = deriveKey(inviteKey, u.saltInvite, u.kdfOpsInvite, u.kdfMemInvite)`.
4. `unwrapKey(u.wrappedMarkerByInvite, kdfInvite)` → sucesso prova a invite key; valor **descartado**.
5. Gera `K` **novo**, `saltPw`, `saltRk`; deriva `kdfPass`/`kdfRecovery` (senha nova + recovery key nova via `randombytes_buf`); embrulha `K` 2× → `wp`/`wr`.
6. Monta `VaultHeaderData{username, saltPw, saltRk, kdfOps, kdfMem, wp, wr}`.
7. `DatabaseManager::instance().setVaultHeader(header)` (API nova).
8. `DatabaseManager::instance().open(slug, K)` — `.pbd` não existe → branch "usuário novo" (schema vazio via providers) **sem mudança**, só preserva o header primed.
9. `DatabaseManager::instance().close()` — 1º save materializa `frameV2(header, cipherV1)`. Falha → aborta sem tocar registro (estado limpo, retentável).
10. `users.json` **NÃO é escrito** aqui → `invitePending` fica `true` vestigial. **É isso que fecha "1º login sem admin online".**
11. Retorna `{ok, dataKey=K, recoveryKeyPlaintext}` (shape inalterado).

`createAdmin()` = mesma sequência (helper privado `bootstrapVault(username, header, K)`), síncrono (sem convite): gera K/salts/wraps + par Ed25519 + âncora no `SecretStoreService` (antes do registro) + bootstrapa o `.pbd` v2 do admin + grava `users.json` (só campos slim + `wrappedSignKey*`).

Depois de `completeFirstLogin` retornar ok, o caller (UI/Fatia 6) chama o MESMO `open(slug, dataKey)` de um login normal (o `.pbd` já existe). Sem `if (1º login)` no call site.

## D4 — API nova do `DatabaseManager` (aditiva; `open()` mantém assinatura)
- `void setVaultHeader(const VaultHeaderData &header)` — injeta o header da PRÓXIMA `save()`/`close()`. `username` vazio ⇒ força v1 (reservado p/ migração Fatia 5, só documentar).
- `VaultHeaderData vaultHeader() const` + `bool vaultIsV2() const` — getters (teste).
- `open()` interno: após ler bytes crus do `.pbd`, `VaultFile::detect()`. Se `V2`: `parseV2()` → guarda `m_vaultHeader`/`m_vaultIsV2=true`, passa **só o innerBlob** ao caminho decrypt v1 existente (`SodiumCryptoService::decrypt`, **zero mudança**). `parseV2` falho = `open()` falho (fail-closed). Se `V1`/legado: **reseta** `m_vaultHeader={}`/`m_vaultIsV2=false` (singleton reusado entre logins — sem reset, header vaza de sessão anterior). Se arquivo **não existe**: **NÃO mexe** em `m_vaultHeader`/`m_vaultIsV2` (preserva o primed do bootstrap). Documentar o ordering no `.h`.
- `encryptAndSave()` (privado, mesma assinatura): após `cipher = crypto.encrypt(plain, key)`, se `m_vaultIsV2` → `VaultFile::frameV2(m_vaultHeader, cipher)` antes de escrever tmp. `save`/`close`/`exportToFile` herdam de graça.
- `close()`: junto do `sodium_memzero` de `m_key`, reseta `m_vaultHeader={}`/`m_vaultIsV2=false`.
- `rekey(newKey)`: **FORA DE ESCOPO** (só migração Fatia 5). Nem 1º login nem reset mudam `K`.

## D5 — `verifyLogin`/`resetPasswordViaRecovery` reescritos (structs de retorno INALTERADOS)
`verifyLogin(username, password)`:
1. `loadAll()` + assinatura/âncora (inalterado).
2. Acha registro → `UserNotFound` se ausente (com dummy-KDF de timing).
3. `vaultPath = AppSettings::savePathFixo(AppSettings::slugify(username))`.
4. **Gate InvitePending por existência de arquivo**: `!QFile::exists(vaultPath)` → `u.invitePending ? InvitePending : IntegrityError`. Existência do arquivo é autoritativa; a flag só desempata o caso anômalo (registro diz completo mas sem cofre → fail-secure).
5. `parseV2` → header; `kdfPass = deriveKey(password, header.saltPw, header.kdfOps, header.kdfMem)`; `unwrapKey(header.wrappedKeyByPass, kdfPass)` → `K` ou `WrongPassword`.
6. Se `role==Admin`: `unwrapKey(record.wrappedSignKeyByPass, kdfPass)` (mesmo kdfPass) → `m_adminSignKey`, senão `IntegrityError`.
7. `Ok` + `dataKey=K` + `role`.

`resetPasswordViaRecovery(username, recoveryKey, newPassword)` (usuário comum = operação de arquivo pura, sem `DatabaseManager`, sem `m_adminSignKey`):
1. `vaultPath`; `!exists → erro`.
2. Lê bytes, `parseV2` → header (não-v2 = conta pré-ADR em migração → erro apontando Fatia 5).
3. `kdfRecovery = deriveKey(recoveryKey, header.saltRk, header.kdfOps, header.kdfMem)`; `unwrapKey(header.wrappedKeyByRecovery, kdfRecovery)` → `K` ou "recovery inválida".
4. `newSaltPw`, `newKdfPass`, `newWp = wrapKey(K, newKdfPass)`.
5. Se `role==Admin`: também re-embrulha `wrappedSignKeyByRecovery`→`wrappedSignKeyByPass` (mesmo kdfRecovery) e grava `users.json` **auto-assinado** com a signing key recém-recuperada (inalterado). Idempotente/retentável (recovery wraps nunca mudam) → sem rollback.
6. Usuário comum: nenhum toque no `users.json`.
7. `frameV2(newHeader, innerBlob-verbatim)` atômico (temp+fsync+rename); `K` nunca muda, innerBlob copiado byte-a-byte.
Helper `rewriteVaultHeader(vaultPath, newHeader) -> bool` (read+parseV2+frameV2+write atômico).

## D6 — Slug unificado
Fonte única: **`AppSettings::slugify(const QString&) -> QString`** (novo static em `AppSettings`; Foundation, é quem tem `savePathFixo`). Algoritmo = corpo atual de `UserAccountService::slugify` (NFD + remove `Mark_NonSpacing` + lowercase + espaço→`_` + `[^a-z0-9_]` fora).
- `UserSession::slug()` → delega a `AppSettings::slugify(m_name)` (conserta os call-sites automaticamente).
- `UserAccountService::slugify()` → wrapper fino de `AppSettings::slugify` (mantém nome/assinatura — é testado direto).
- **Compat (flag p/ Fatia 5, não bloqueia):** cofres single-user atuais com nome acentuado estão sob slug ANTIGO ("joo"). `migrarCofreSeNecessario()` precisa checar o caminho do slug antigo como 3º fallback p/ usernames com char fora de `[a-z0-9_ ]`. TODO comentado.

## D7 — Impacto na Fatia 2/3 commitada
- `UserAccountRecord`: **removidos** `saltPw, saltRk, kdfOps, kdfMem, wrappedKeyByPass, wrappedKeyByRecovery` (migram p/ `VaultHeaderData`). **Novos** (canal de convite, só enquanto `invitePending`): `saltInvite, kdfOpsInvite, kdfMemInvite, wrappedMarkerByInvite`. **Mantidos:** `username, role, invitePending, wrappedSignKeyByPass, wrappedSignKeyByRecovery`.
- `UserAccountRepository`: `userToJson`/`userFromJson`/`canonicalContent` refletem a nova forma; **bump `version` 1→2**.
- `UserAccountService.cpp`: reescreve `createAdmin`, `createUser`, `completeFirstLogin`, `verifyLogin`, `resetPasswordViaRecovery`; **remove a dependência de `m_adminSignKey`** em completeFirstLogin/reset(comum). Apaga a nota de limitação do topo do `.h` (substitui por "resolvido na Fatia 4"). `removeUser` **inalterado** (autoridade do admin, correto exigir sessão). `usernameExists`/`listUsers` só trocam o slugify.

## Ordem TDD (T0-T9)
- **T0** (pré-req infra): `test_user_account_service.cpp` isola `HOME`/`XDG_DATA_HOME` em `QTemporaryDir` (padrão de `test_working_db_path.cpp`) + `DatabaseManager::instance().close()` defensivo no `cleanup()` (singleton). Sem asserção nova.
- **T1**: `AppSettings::slugify` + delegações. Prova: `slugify("João")=="joao"`; `UserSession` login "João" → slug "joao" (regressão do bug "joo").
- **T2**: schema v2 do registro (campos removidos/adicionados, `version=2`). Ajusta round-trip de `test_user_account_repository.cpp`. Assinatura Ed25519 segue verificando.
- **T3**: `DatabaseManager` v2 (`setVaultHeader`/`vaultHeader`/`vaultIsV2` + open autodetect + encryptAndSave envelopa + close reseta). Nova suíte `tests/test_database_manager_vault_v2.cpp` (isola HOME/XDG). Provas: (a) setVaultHeader+open(inexistente)+close → magic `PBD\x02`, header recuperável; (b) reabrir com mesmo K, dado intacto; (c) `.pbd` v1 existente abre **sem regressão**; (d) `.pbd` legado (AES-CBC) abre via keyLegacy **sem regressão**.
- **T4**: `createAdmin` reescrito. Provas: `.pbd` v2 do admin existe e é v2; schema completo; registro sem wraps-de-K, com `wrappedSignKeyByPass`.
- **T5**: `createUser` (marcador). Provas: registro tem `wrappedMarkerByInvite`, não tem `wrappedKeyByPass`; invite errada falha o unwrap.
- **T6 (prova central #1)**: `completeFirstLogin` sem sessão de admin. `svcAdmin` cria user; **`svcSemSessao`** (instância nova, `m_adminSignKey` nunca preenchida) completa o 1º login → `ok=true`; depois `verifyLogin` com senha nova → `Ok`, mesmo `K`. **Prova de segurança:** re-desembrulhar `wrappedMarkerByInvite` com a invite original NÃO produz o `K` real (marcador inerte).
- **T7**: `verifyLogin` gate por arquivo. Provas: `invitePending` stale `true` mas cofre existe → loga `Ok`; sem cofre + pending → `InvitePending`; admin sem cofre → `IntegrityError`.
- **T8 (prova central #2)**: `resetPasswordViaRecovery` sem admin online. 3 instâncias novas sem sessão; reset com a recovery key → `ok=true`; **innerBlob idêntico byte-a-byte antes/depois** (prova do "não re-cifra"); `verifyLogin` com senha nova → mesmo `K`. Caso admin-alvo adaptado, segue self-sufficient.
- **T9**: regressão `removeUserRecusaUltimoAdmin`, `pubkeySwapDetectado`, `usernameExistsPegaColisaoDeSlug` (só forma do fixture). `ctest` completo verde.

Fora de escopo (comentário-ponte no código): `DatabaseManager::rekey`, `LegacyVaultMigrationService`, slug antigo na migração — Fatia 5/6.
