# ADR-001 — Gestão de usuários + Recuperação de senha (#179 / #180)

- **Status:** ✅ Aprovado pelo líder em 2026-07-11 (senha ≥12 confirmada). Implementação autorizada na ordem §8.
- **Data:** 2026-07-11
- **Decisão do líder:** 2026-07-11 (spec `docs/design/user-management-spec.md` + AskUserQuestion)
- **Autores do desenho:** security-engineer (envelope + threat model) + software-architect (arquitetura), consolidado pelo orquestrador
- **Prioridade do projeto:** Precisão financeira > Robustez > **Segurança** > UX → toca o núcleo cripto de app **já em produção com dado real** → design-first + review adversarial obrigatório antes de codar.
- **Refs:** [[CONTRACT]] · [[AUDITORIAS]] (A3 segurança) · `docs/design/user-management-spec.md` · substitui backlog #179/#180 · gera #212.

---

## 1. Contexto e problema

Hoje qualquer nome digitado no login **cria um cofre `.pbd` novo implicitamente** (nome+senha → `Argon2id(senha)` cifra o cofre direto). O líder quer:

1. **Admin gestor** exclusivo para criar/remover usuários, **sem acesso a dado financeiro** (garantia por cripto, não por permissão de UI).
2. **Usuário comum** criado só pelo admin, com seu próprio cofre cifrado.
3. **Recuperação de senha** (#180) sem servidor (app desktop local) — via *recovery key* gerada na criação.
4. Fim do "cofre-fantasma por typo": nome inexistente → erro claro, não cria cofre.

---

## 2. Decisões do líder (AskUserQuestion 2026-07-11)

| # | Decisão | Escolha |
|---|---------|---------|
| **D-A** | Fluxo de criação (furo CWE-522: admin não pode ver a senha do usuário) | ✅ **Invite key** — admin gera username + chave de convite; o **próprio usuário** define a senha no 1º login |
| **D-B** | Admin tem cofre financeiro próprio? | ✅ **`vaultPath` opcional** em qualquer conta (1 modelo de dado só) |
| **D-C** | Integridade do registro (anti auto-promoção a admin) | ✅ **Assinatura Ed25519** (single-writer/many-readers) |
| **D-D** | Resiliência a perda do `users.json` | ✅ **`.pbd` v2 auto-contido** (wraps duplicados no header do cofre) |

**Decisões que ambos os especialistas convergiram (aplicadas sem consulta):**

- **Remover `pass_verifier`** do schema — a tag AEAD ao desembrulhar `K` **já é** o verificador (falha de tag = senha errada). Campo redundante = superfície de bug.
- **Derivar `vault_path`** do slug (`AppSettings::savePathFixo(slug(username))`), **não** gravar path livre no JSON — anti path-traversal (defesa em profundidade mesmo com MAC).
- **Mensagem explícita "usuário não existe"** no login — é app local sem superfície de rede; quem acessa a máquina já lê os usernames do `users.json`. Anti-enumeração de SaaS não se aplica. (Latência constante entre os caminhos de falha mesmo assim.)
- **MAC/assinatura falhou → bloqueia boot** com erro claro ("registro corrompido/adulterado, restaure de backup"), **sem** auto-reparo silencioso (perderia a garantia de integridade).
- **Recovery key não rotaciona** no reset de senha — só `wrapped_key_by_pass` muda; `K` e `wrapped_key_by_recovery` ficam intactos.
- **Senha mínima ↑ de ≥8 → ≥12 chars** — agora a senha é a **única** porta pro dado financeiro de cada pessoa (antes o risco era compartilhado). *(Confirmar: o líder pode vetar e manter ≥8.)*

---

## 3. Modelo criptográfico (envelope encryption)

### 3.1 Chave de dados `K` e insight de baixo impacto

- `K` = **32 bytes aleatórios** (`randombytes_buf`) — mesmo tamanho de chave que `SodiumCryptoService` já usa.
- `K` cifra o `.pbd` **exatamente como hoje** — `SodiumCryptoService::encrypt/decrypt` **não muda de contrato**; só passa a receber `K` em vez de `Argon2id(senha)` direto.
- **`DatabaseManager::open()` já trata a chave como `QByteArray` opaco** → não sabe (nem precisa saber) se veio de KDF direto ou de um *unwrap* de envelope. **Zero primitiva cripto nova** — o envelope é 100% problema de Back (`UserAccountService`) reusando o AEAD existente.

### 3.2 Primitiva de *wrap* — `crypto_secretbox_easy` (XSalsa20-Poly1305)

**Não** reaproveitar o AEAD AES-GCM do `.pbd` para os wraps: `crypto_aead_aes256gcm_*` do libsodium **exige AES-NI em runtime** (sem fallback por software) → um wrap feito numa máquina com AES-NI ficaria ilegível ao restaurar backup num equipamento sem AES-NI. Como o payload é sempre pequeno (32B a 64B), não há ganho em reusar o AEAD grande. `crypto_secretbox_easy` é portável entre CPUs e desacopla o *wrap* da disponibilidade de AES-NI.

```
wrap(segredo, chaveDeWrap) = nonce(24B) || secretbox_ciphertext   # 32B payload + 16B MAC + 24B nonce = 72 bytes
chaveDeWrap = Argon2id(segredo, salt, ops=3, mem=256MB) → 32B      # SodiumCryptoService::deriveKey, sem mudança
```

### 3.3 Os wraps de `K`

| Wrap | Segredo de origem | Salt | Params |
|------|-------------------|------|--------|
| `wrapped_key_by_pass` | senha do usuário | `salt_pw` (16B) | Argon2id MODERATE (ops=3, mem=256MB), gravados por-usuário |
| `wrapped_key_by_recovery` | recovery key | `salt_rk` (16B) | idem (canal de recovery **não** mais barato que o de senha) |

Params gravados no registro (`kdf_params`) → permite auto-rehash silencioso no próximo login se um default futuro for endurecido (padrão "senha hash auto-rehash"), sem quebrar usuários antigos.

### 3.4 Recovery key (#180)

- **Entropia:** 160 bits (20B) — folga sobre o piso de 128b, casa exato com Base32 (20B × 8/5 = 32 símbolos).
- **Encoding:** Crockford Base32 (exclui `I L O U`), minúsculo, 8 grupos de 4: `xxxx-xxxx-xxxx-xxxx-xxxx-xxxx-xxxx-xxxx`, + 1 símbolo de **checksum** (valida transcrição localmente antes de gastar Argon2id, sem ajudar atacante).
- **Exibição:** tela dedicada, **1× só**, sem cópia automática pro clipboard (limpar após ~30s se houver botão copiar). Aviso forte: *"sem esta chave E sem a senha, o dado é irrecuperável para sempre — nem o admin ajuda"*. Confirmação ativa ("guardei" + redigitar últimos 4 chars) antes de fechar.

### 3.5 Fluxo de **invite key** (D-A — fecha o CWE-522)

"Criar usuário" = emitir um **código de convite**, mesmo mecanismo da recovery key:

1. Admin digita só `username` (+ role). Sistema gera `K` aleatória + **invite key**.
2. `K` é embrulhada **apenas** sob `KDF(invite_key)` nesse momento — ainda **não existe** wrap por senha.
3. Admin vê o invite key **1×** e entrega ao novo usuário. **O admin nunca vê nem processa senha alguma.**
4. **Novo usuário**, sozinho no 1º login, informa o invite key → define a **própria** senha → gera `wrapped_key_by_pass` pela 1ª vez **+ uma recovery key nova definitiva** (não reusar o invite key como recovery — não misturar "convite" com "recuperação de longo prazo").

Garantia: o processo do admin **nunca recebe a senha de ninguém como input** em nenhuma etapa. A garantia da spec passa a ser **verdadeira por construção**.

### 3.6 Reset via recovery (#180)

`recovery_key` → `KDF` → desembrulha `K` → usuário define senha nova → **novo** `salt_pw` + re-wrap `wrapped_key_by_pass`. `K` **nunca muda** → `.pbd` não é re-cifrado, dado preservado, operação só de metadados. Escrita atômica (temp + fsync + rename).

---

## 4. Threat model (resumo)

- **Admin não obtém `K` de terceiros:** `K` só sai de `wrapped_key_by_pass`/`wrapped_key_by_recovery` via `KDF(senha)` ou `KDF(recovery_key)` — segredos que **nunca** são persistidos e que o processo do admin nunca recebe (invite-key fecha o único gap). Garantia **criptográfica**, não de permissão.
- **Integridade do registro (D-C — Ed25519):** os wraps protegem só o campo `K`, **não** `role`/`username`/`vault_path`. Sem assinatura, um user comum editaria `role:user→admin` no disco (CWE-269). Solução:
  - 1º run gera par Ed25519 **do admin**. Pubkey em claro no `users.json` (`admin_pubkey`, âncora de verificação). Signing key embrulhada como `K` (por senha + por recovery key do admin).
  - Toda mutação exige admin **logado** (senha desembrulha a signing key) → assina o conteúdo canônico (`version + admin_pubkey + users[]`, chaves ordenadas, sem espaços, excluindo `signature`).
  - **Verificação não exige segredo** (chave pública) → roda na tela de login, antes de qualquer auth. Falha → **fail secure** (bloqueia todo login).
  - Por que assimétrica e não HMAC+libsecret: HMAC-key no keychain fica acessível via D-Bus a **qualquer processo da mesma sessão Linux** → não isola admin de user no mesmo PC. Ed25519 amarra a escrita a "quem tem a senha do admin".
- **Brute-force:** Argon2id MODERATE (~1–2s, 256MB, memory-hard) nos dois canais; senha ≥12 (D folded) reduz o espaço de busca offline. Ataque offline não tem rate-limit possível — defesa = custo do KDF + entropia.
- **Riscos residuais (documentar na UI/backup):** (1) perda de senha **E** recovery key = irrecuperável por design; (2) shoulder-surfing na exibição única; (3) fronteira admin/user é de app, não de SO (keylogger/memory-scraping na mesma sessão está fora de escopo); (4) `.pbd` v2 (D-D) mitiga a perda do registro central.

---

## 5. Formatos de dado

### 5.1 `users.json` (registro da instalação)

`QStandardPaths::AppDataLocation` (0700), via novo `AppSettings::userRegistryPath()`. `QJsonDocument` (built-in, precedente no `ExportService`). Por usuário:

```jsonc
{
  "username": "...", "role": "admin|user",
  "kdf_params": { "salt_pw": "...", "salt_rk": "...", "ops": 3, "mem": 268435456 },
  "wrapped_key_by_pass": "...",       // 72B base64 (ausente até o 1º login no fluxo invite-key)
  "wrapped_key_by_recovery": "...",   // 72B base64
  "invite_pending": true|false        // wrap por invite key, some após 1º login
  // vault_path: NÃO gravado — derivado do slug
  // pass_verifier: REMOVIDO
}
```

Topo do arquivo: `version`, `admin_pubkey` (Ed25519, 32B), `signature` (Ed25519 sobre o conteúdo canônico). Signing key do admin embrulhada nos wraps do registro do admin.

### 5.2 `.pbd` v2 (D-D — auto-contido)

- **Payload cifrado não muda** (mesmo AEAD, mesma `K`). v2 é **estritamente aditivo**.
- `magic PBD\x02` (4º byte distingue de `\x01`) + bloco length-prefixed `{username, salt_pw, kdf_params, wrapped_key_by_pass, salt_rk, wrapped_key_by_recovery}` (os mesmos wraps de 72B, duplicados) + payload v1 inalterado.
- Esses campos já são seguros em claro (Argon2id+AEAD) → duplicar não expõe segredo novo, só remove o ponto único de falha do registro. Viabiliza futura ferramenta "reconstruir registro a partir dos cofres".
- `isNewFormat()` continua detectando por magic; leitura de v1 legado e v1-pós-migração seguem sem alteração.

---

## 6. Arquitetura (classes por camada)

Alinhado ao **gosto real do projeto** (interface só onde há ≥2 implementações: reusa `ICryptoService`, **não** cria interface nova; `IRepository` é aspiracional/inexistente e `ISecretStore` foi removida por YAGNI #203a).

| Camada | Arquivo | Novo/Tocado | Responsabilidade |
|--------|---------|-------------|------------------|
| Modelo | `src/models/UserAccountRecord.h` | Novo (POD) | struct + `enum class Role{Admin,User}` + `struct UserPublicInfo` (DTO sem campos cripto — ISP) |
| Persist. | `src/repositories/UserAccountRepository.h/.cpp` | Novo | I/O de `users.json` + verifica assinatura; **não** interpreta campos. Path injetável (testes com `QTemporaryDir`). Sem exceção (CONTRACT) |
| Back | `src/services/UserAccountService.h/.cpp` | Novo | Regra de negócio: `createUser`(invite-key), `verifyLogin`, `removeUser`(guarda último admin), `resetPasswordViaRecovery`, `listUsers`. Depende de `UserAccountRepository&` + `ICryptoService&` (DI) |
| Back | `src/services/LegacyVaultMigrationService.h/.cpp` | Novo | Migração de cofre antigo → envelope + recovery key (§7) |
| Front | `src/ui/CreateAdminDialog.h/.cpp` | Novo | 1º run: cria admin (+ checkbox "controlar minhas finanças" — D-B) |
| Front | `src/ui/AdminPanelWindow.h/.cpp` | Novo | Painel gestor **top-level** (não subclasse de MainWindow). **Isolamento por arquitetura**: nunca inclui `ParcelasWidget.h`/etc. → fisicamente impossível instanciar finanças |
| Front | `src/ui/widgets/CreateUserDialog.h/.cpp` | Novo | Admin cria usuário → mostra invite key 1× |
| Front | `src/ui/RecoveryDialog.h/.cpp` | Novo | 3 páginas (`QStackedWidget`): username → recovery key → nova senha |
| Front | `src/ui/widgets/PasswordChecklistWidget.h/.cpp` | Novo (extraído do LoginDialog) | Reuso em 4 telas (regra-de-3 DRY) |
| Front | `src/ui/LoginDialog.h/.cpp` | Tocado | **Para de tocar cripto/keychain direto** (corrige violação DIP atual); só chama `UserAccountService::verifyLogin` + interpreta `enum Status`; link "Esqueci a senha"; expõe `role()` + `dataKey()` |
| Front | `src/main.cpp` | Tocado | Bootstrap 1º run + roteamento admin/user + gatilho de migração |
| Found. | `src/core/crypto/SodiumCryptoService.h/.cpp` | Tocado | overload `deriveKey(pwd,salt,ops,mem)` (mantém o de 2 args — OCP) + `randomBytes(n)` |
| Found. | `src/core/database/DatabaseManager.h/.cpp` | Tocado | novo `rekey(QByteArray)` — troca a chave do próximo save. **`open()` não muda** |
| Found. | `src/core/settings/AppSettings.h/.cpp` | Tocado | `userRegistryPath()` (mkpath+chmod 0700) |
| Found. | `src/core/session/UserSession.h/.cpp` | Tocado | campo `Role m_role` + accessor; `logout()` limpa |
| Testes | `tests/test_user_account_{service,repository}.cpp`, `test_legacy_vault_migration.cpp` | Novos | + CMakeLists |

---

## 7. Migração (cofres do modelo antigo)

Reusa o idioma que `DatabaseManager` já tem (`m_needsReencrypt` já regrava no formato novo no `close()`), **sem** caminho paralelo:

1. `isFirstRun()==true` **mas** existe `.pbd` no caminho fixo/legado → pede a senha atual (UI de login normal), `DatabaseManager::open(slug, Argon2id(pw), PBKDF2(pw))` como hoje.
2. Abriu → diálogo obrigatório "vamos proteger sua conta" → gera `K` nova + recovery key → registra o dono como **admin com cofre próprio** (`vault_path` derivado; D-B força o campo preenchido) → embrulha `K` 2×.
3. `DatabaseManager::rekey(K)` → próximo `save()` grava com `K` nova. Dado **nunca perdido** (a sessão roda com o `.db` de trabalho já decifrado; só muda a chave que cifra o próximo save).
4. Ordem atômica de rename + mantém `.bak` pré-migração até 2 unlocks bem-sucedidos.

---

## 8. Ordem de implementação (TDD red→green→refactor)

1. `UserAccountRecord` + `UserAccountRepository` — round-trip JSON + **assinatura Ed25519** (grava/lê/detecta adulteração). Sem envelope ainda.
2. `UserAccountService::createUser`/`usernameExists`/`listUsers` — envelope completo + invite-key + colisão de slug + unicidade. Teste prova: `K` recuperável via senha **e** via recovery key; **admin não obtém `K` de terceiro** (senha errada de outro user falha).
3. `verifyLogin` — 3 casos de mensagem (inexistente/senha errada/ok).
4. `removeUser` — guarda do último admin.
5. `resetPasswordViaRecovery` — prova que o cofre é idêntico antes/depois.
6. `DatabaseManager::rekey` + `LegacyVaultMigrationService` — fixture de `.pbd` legado real → migra → abre no fluxo novo.
7. UI: extrai `PasswordChecklistWidget` (refactor puro, testes de login seguem verdes) → `CreateAdminDialog` → rework `LoginDialog` → `AdminPanelWindow`+`CreateUserDialog` → `RecoveryDialog`.
8. **Review adversarial de segurança** (security-engineer, agent diferente do implementador): tentar obter `K` sendo admin; recovery key fraca; registro adulterado; timing. **EXECUTA o código, não só lê.**
9. `#212` — aba de exportação (design-first próprio, depois desta onda).

---

## 9. Consequências

- **Positivas:** garantia "admin sem acesso a dado" verdadeira por cripto; recuperação de senha real sem servidor; fim do cofre-fantasma; resiliência a perda do registro (`.pbd` v2); corrige violação DIP existente no `LoginDialog`; `DatabaseManager`/`SodiumCryptoService` mantêm contrato.
- **Custos:** +1 tela no onboarding (invite-key); par Ed25519 + wrap extra; formato `.pbd` v2 (aditivo); política de senha mais estrita (≥12).
- **Migração:** transparente no 1º unlock pós-upgrade; `.bak` como rede de segurança.
- **`SecretStoreService`/libsecret:** perde o consumidor de salt (salts vão pro registro), mas **ganha** a MAC-key do registro / âncora — mantida.

---

## 10. Aprovação

Aguardando **OK do líder** neste ADR antes de qualquer código. Após aprovação: implementar na ordem §8, commit por fatia citando #179/#180, review adversarial antes de fechar a onda.
