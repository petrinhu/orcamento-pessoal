#pragma once

#include "models/UserAccountRecord.h"
#include "services/UserAccountService.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/secret/SecretStoreService.h"

#include <QByteArray>
#include <QString>

// (ADR-001 #212 §7, Fatia 5) Migração de cofres single-user PRÉ-ADR-001
// (qualquer nome digitado no login criava um cofre `.pbd` implicitamente,
// cifrado direto por Argon2id(senha) — formato v1-novo — ou, mais antigo,
// AES-256-CBC/PBKDF2(senha) sobre um salt guardado no SecretStoreService
// por-username — formato legado) para o modelo admin+envelope (`.pbd` v2).
//
// O dono do cofre legado vira o ADMIN da instalação. O dado financeiro é
// PRESERVADO — nunca perdido nem relido do zero: o `.db` de trabalho é
// decifrado com a chave ANTIGA (mesmo caminho de derivação de
// LoginDialog::onEntrar, reusando DatabaseManager::open() como hoje) e
// depois RE-CIFRADO com uma chave `K` nova via DatabaseManager::rekey() —
// a sessão roda com o `.db` já decifrado o tempo todo; só a chave de
// cifragem do PRÓXIMO save()/close() muda.
//
// Reusa DatabaseManager::instance() (singleton) — só uma migração por vez
// no processo (mesma restrição defensiva de UserAccountService::
// createAdmin()/completeFirstLogin(): recusa se já houver uma sessão de
// cofre aberta nesta instância).
class LegacyVaultMigrationService {
public:
    LegacyVaultMigrationService(UserAccountService &accounts, SodiumCryptoService &crypto,
                                 SecretStoreService &secrets);

    // true SOMENTE quando (a) o registro admin+envelope está VAZIO
    // (accounts.isFirstRun()) E (b) existe um `.pbd` v1-novo (Argon2id) ou
    // legado (AES-CBC/PBKDF2) no caminho fixo do slug ATUAL
    // (UserAccountService::slugify — NFD-consciente, pós-Fatia-4) OU no
    // caminho fixo do slug ANTIGO (ADR-001 §6/D6 — algoritmo pré-Fatia-4,
    // sem decompor NFD; só diverge do slug atual pra usernames com
    // caractere fora de `[a-z0-9_ ]`, ex.: "João" -> "joo" vs "joao").
    // `slugEncontrado` (opcional, out-param) recebe qual dos dois foi
    // achado — fica vazio se nenhum. Não abre/decifra nada — só
    // QFile::exists (mesmo padrão de UserAccountService::listUsers).
    [[nodiscard]] bool legacyVaultPresente(const QString &username,
                                            QString *slugEncontrado = nullptr) const;

    struct MigrateResult {
        bool       ok = false;
        QString    error;
        QByteArray recoveryKeyPlaintext; // recovery key NOVA; exibir 1x, caller decide quando zerar
    };

    // Sequência (SEM perda de dado, atômica — ver .cpp para o passo-a-passo
    // comentado e a justificativa de segurança de cada ordem):
    //   1. deriva as chaves candidatas do modelo antigo EXATAMENTE como o
    //      login atual (LoginDialog::onEntrar) — salt do SecretStoreService
    //      por-username + Argon2id (candidato v1-novo) e PBKDF2 (candidato
    //      legado);
    //   2. DatabaseManager::open(slug, keyNovo, keyLegado) — reusa o
    //      caminho de abertura já existente (decide sozinho v1-novo vs
    //      legado, migração transparente do `data/` relativo se aplicável);
    //      falha = senha errada ou cofre corrompido, NADA é tocado;
    //   3. com o `.db` de trabalho JÁ decifrado em memória: gera K/salts/
    //      wraps novos + par Ed25519 do admin + recovery key nova (dado
    //      nunca perdido — só a chave de cifragem do PRÓXIMO save muda);
    //   4. setVaultHeader(header) + rekey(K);
    //   5. grava a âncora do admin_pubkey no SecretStoreService (ANTES do
    //      registro — mesma ordem de UserAccountService::createAdmin);
    //   6. close() materializa o `.pbd` v2 com K novo — dado do `.db` de
    //      trabalho preservado, re-cifrado, nunca relido; falha aqui aborta
    //      SEM gravar `users.json` (retentável — o `.pbd` original não foi
    //      destruído, ver relocalização de slug no .cpp);
    //   7. registra o dono como admin via
    //      UserAccountService::registerMigratedAdmin.
    [[nodiscard]] MigrateResult migrarParaAdmin(const QString &username, const QString &password);

private:
    UserAccountService  &m_accounts;
    SodiumCryptoService &m_crypto;
    SecretStoreService  &m_secrets;
};
