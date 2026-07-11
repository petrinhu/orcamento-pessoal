# Spec — Gestão de usuários (#179) + Recuperação de senha (#180)

Decisão do líder 2026-07-11. App **distribuído**, multi-usuário por instalação.
Prioridade do projeto: Precisão > Robustez > **Segurança** > UX. Toca o **núcleo cripto** → design-first + review de segurança obrigatório antes de codar.

## Modelo (Opção 2 — admin gestor)

- **1º run** (instalação nova, registro vazio): fluxo de **criação do admin** (user + senha + confirmar). O admin é o dono daquela instalação. Papel `admin`.
- **Admin loga** → **painel de gestão de usuários** (SEM abas financeiras): listar · **criar usuário** (user + senha + confirmar → gera **recovery key** exibida 1× pro usuário guardar) · remover usuário. Zero acesso a dado financeiro.
- **Usuário comum** (papel `user`, criado só pelo admin) → loga no **seu cofre cifrado** (abas de finanças normais).
- **Login**: valida o usuário contra o **registro**; nome inexistente → "usuário não existe" (acaba o cofre-fantasma por typo). Senha errada → nega, sem revelar se o user existe (anti-enumeração, se viável).

## Registro de usuários

Arquivo `users.json` (ou similar) por instalação, em `QStandardPaths::AppDataLocation` (0700). Por usuário:
```
{ username, role: admin|user, kdf_params (Argon2id salt/ops/mem),
  pass_verifier,            # verificador da senha (NÃO a senha)
  wrapped_key_by_pass,      # K cifrada com KDF(senha)
  wrapped_key_by_recovery,  # K cifrada com KDF(recovery_key)  ← envelope p/ #180
  vault_path }              # caminho do .pbd cifrado com K
```
O registro em si é integridade-protegido (MAC) e só o admin o altera (criar/remover). NÃO contém senhas nem K em claro.

## Cripto — envelope encryption (o ponto crítico)

- Cada cofre tem uma **chave de dados aleatória `K`** (que cifra o `.pbd` como hoje, AEAD AES-GCM/XChaCha20).
- `K` é embrulhada **duas vezes** na criação:
  - `wrapped_key_by_pass = seal(K, KDF_Argon2id(senha, salt))`
  - `wrapped_key_by_recovery = seal(K, KDF_Argon2id(recovery_key, salt))`
- **Login normal:** deriva `KDF(senha)` → desembrulha `K` → abre o cofre.
- **Recuperação (#180):** usuário informa o **recovery key** → desembrulha `K` → **re-embrulha** `wrapped_key_by_pass` com a **senha nova** (dado preservado, `K` inalterada).
- **Admin NÃO decifra dado** (garantia por CRIPTO, não por permissão): o admin nunca tem a senha nem o recovery key de ninguém → não obtém `K` → não lê o `.pbd`. Ele só edita metadados do registro (criar/remover), o que é o "sem acesso a informações".
- **Recovery key**: 128+ bits, exibida uma única vez (formato legível tipo `xxxx-xxxx-xxxx-...`), com aviso forte "guarde — sem ela E sem a senha o dado é irrecuperável". Confirmar que anotou.

## Fluxos de UI (básico, público leigo)

- **Criar admin (1º run):** user · senha · confirmar senha · checklist (≥8, maiúscula, número — já existe).
- **Admin → criar usuário:** user · senha · confirmar → tela mostrando a recovery key (copiar/salvar) + "anotei".
- **Login:** user · senha · 👁 · link "Esqueci a senha" → fluxo recovery-key → nova senha.
- **Admin → remover usuário:** confirmação dupla (apaga registro + `.pbd` — irreversível).

## Migração (usuários existentes hoje)

Cofres criados no modelo antigo (nome→cofre direto) precisam de rota de migração: no 1º boot pós-feature, se houver `.pbd` legado sem registro, tratar o dono como admin OU migrar pro novo formato de envelope no próximo unlock (embrulhar `K` com a senha atual + gerar recovery key). Definir na implementação.

## Fora de escopo (por ora)

- Reset remoto / e-mail (não há servidor — é desktop local).
- Papéis além de admin/user.
- Auditoria de acesso / logs de login.

## Plano de build (design-first, security-critical)

1. **Design cripto + arquitetura** (software-architect + security-engineer/narciso-CISO): validar o envelope, o formato do registro, a migração, anti-enumeração. ADR.
2. **Impl backend** (SodiumCryptoService/vault: envelope K; UserRegistry; roles) — TDD pesado (envelope round-trip, recovery reset preserva dado, admin não obtém K).
3. **Impl UI** (criar-admin, painel admin, signup c/ recovery key, login+recuperação).
4. **Review adversarial de segurança** (tentar obter K sendo admin; recovery key fraca; registro adulterável) + testes.
5. Distribuição: entra como bloqueador pré-1.0 (é multi-usuário num app que será distribuído).

Refs: [[CONTRACT]] · [[AUDITORIAS]] (A3 segurança) · substitui o backlog #179/#180.
