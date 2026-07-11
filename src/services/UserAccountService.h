#pragma once

#include "models/UserAccountRecord.h"
#include "repositories/UserAccountRepository.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/crypto/VaultFile.h"
#include "core/secret/SecretStoreService.h"

#include <QByteArray>
#include <QString>
#include <QVector>

#include <optional>

// (ADR-001 #212 §3/§4/§6/§8, Fatia 2 + Fatia 4) Regra de negócio de gestão
// de usuários: envelope encryption (wrap/unwrap de K), fluxo invite-key
// (D-A/CWE-522, com marcador descartável — D3), recovery key (Crockford
// Base32 + checksum local), sessão de assinatura do admin (signing key
// Ed25519 em memória, só enquanto logado) e verificação cruzada do
// admin_pubkey do arquivo contra a âncora gravada no SecretStoreService
// (defesa em profundidade anti pubkey-swap — item E do brief da Fatia 1).
//
// Sem exceção como controle de fluxo (CONTRACT §6.4/§413): toda operação
// pública devolve um struct de resultado ou um enum de status; nenhuma
// lança para o caller decidir. UserAccountRepository/SodiumCryptoService
// (Foundation) ainda lançam std::runtime_error para violação de invariante
// de PROGRAMAÇÃO (ex.: chave de tamanho errado) — este Service nunca
// alimenta esses métodos com tamanhos inválidos (os gera/deriva ele mesmo),
// então essas exceções nunca deveriam disparar em uso normal.
//
// (Resolvido na Fatia 4 — ver ADR-001 §8 passo 6/D1-D5) Os wraps de K por
// senha/recovery de QUALQUER conta (admin ou usuário comum) moram
// EXCLUSIVAMENTE no `.pbd` v2 (VaultHeaderData) do próprio cofre — o
// `users.json` central deixou de guardá-los. Isso elimina a limitação
// temporária da Fatia 2 (completeFirstLogin/resetPasswordViaRecovery de
// usuário comum exigiam uma sessão de assinatura do admin só pra resignar
// o registro): agora essas duas operações tocam SÓ o `.pbd` do próprio
// usuário — nenhuma delas grava/assina `users.json`, então nenhuma delas
// depende de `m_adminSignKey`. O admin fica 100% offline durante o 1º
// login de um usuário convidado e durante um reset de senha via recovery
// (provas T6/T8 no relatório da tarefa).
class UserAccountService {
public:
    UserAccountService(UserAccountRepository &repo, SodiumCryptoService &crypto,
                        SecretStoreService &secrets);
    ~UserAccountService();

    // Segura material de assinatura em memória (m_adminSignKey) — não faz
    // sentido copiar o Service (também tem membros de referência).
    UserAccountService(const UserAccountService &) = delete;
    UserAccountService &operator=(const UserAccountService &) = delete;

    static constexpr int kKeyBytes         = 32; // K — chave de dados (AEAD do .pbd)
    static constexpr int kRecoveryKeyBytes = 20; // recovery key / invite key — 160 bits

    // (item E do brief da Fatia 1) Chave reservada no SecretStoreService
    // para a cópia âncora do admin_pubkey — nunca colide com chaves de app
    // existentes (brapiToken, salt de login legado etc.) por causa do
    // padrão "__x__". Público (não é segredo, só o NOME de uma entrada do
    // keychain) porque LegacyVaultMigrationService (Fatia 5) também grava a
    // âncora — mesma ordem/chave que createAdmin(), sem duplicar o literal.
    static constexpr const char *kAdminPubKeyAnchorKey = "__admin_pubkey__";

    // ── Encoding de recovery/invite key — Crockford Base32 + checksum ──────
    // 20 bytes -> 32 símbolos de dado (160/5, exato, sem padding) + 1
    // símbolo de checksum, formatados em 8 grupos de 4 + "-checksum":
    // "xxxx-xxxx-xxxx-xxxx-xxxx-xxxx-xxxx-xxxx-c". Alfabeto Crockford
    // (exclui I/L/O/U), minúsculo na exibição, mas decode aceita qualquer
    // caixa e ignora hífens/espaços. O checksum é um mod-32 simples sobre
    // os 32 símbolos de dado — NÃO é o algoritmo oficial de checksum do
    // Crockford (que usa alfabeto estendido mod 37); serve só para pegar
    // erro de transcrição local antes de gastar Argon2id, sem dar
    // informação extra a um atacante (ver ADR-001 §3.4).
    [[nodiscard]] static QString encodeRecoveryKey(const QByteArray &raw);
    [[nodiscard]] static std::optional<QByteArray> decodeRecoveryKey(const QString &formatted);

    // (ADR-001 §6/D6, Fatia 4) Wrapper fino de AppSettings::slugify — fonte
    // ÚNICA de verdade do slug (compartilhada com UserSession::slug() e
    // com o path do cofre, AppSettings::savePathFixo). Mantido aqui com
    // esta assinatura porque é testado direto (usernameExists() e
    // verifyLogin() usam este método, não a versão de AppSettings, pra
    // deixar o call-site expressivo dentro deste Service).
    [[nodiscard]] static QString slugify(const QString &username);

    [[nodiscard]] bool isFirstRun() const;
    [[nodiscard]] bool usernameExists(const QString &username) const;

    struct CreateAdminResult {
        bool       ok = false;
        QString    error;
        QByteArray recoveryKeyPlaintext; // exibir 1x; caller decide quando zerar
    };
    // 1º run: cria o admin gestor com a PRÓPRIA senha (bootstrap — não
    // passa pelo fluxo invite-key, que é só para usuários subsequentes).
    [[nodiscard]] CreateAdminResult createAdmin(const QString &username, const QString &password);

    struct CreateUserResult {
        bool       ok = false;
        QString    error;
        QByteArray inviteKeyPlaintext; // exibir 1x
    };
    // Exige admin logado (m_adminSignKey preenchida via verifyLogin
    // anterior). O admin NUNCA recebe/gera a senha do novo usuário — só o
    // username+role como input (D-A, fecha CWE-522).
    [[nodiscard]] CreateUserResult createUser(const QString &username, Role role);

    struct FirstLoginResult {
        bool       ok = false;
        QString    error;
        QByteArray recoveryKeyPlaintext; // recovery key NOVA (não reusa o invite key)
        QByteArray dataKey;              // K — pronta pra abrir o .pbd do usuário
    };
    // (ADR-001 §8 passo 6/D3, Fatia 4) 100% self-service: NÃO exige
    // nenhuma sessão de admin nesta instância (nem m_adminSignKey) — só
    // toca o `.pbd` v2 NOVO do próprio usuário. `inviteKey` só prova posse
    // do convite (desembrulha um MARCADOR descartável, nunca o K real); `K`
    // nasce aqui, gerado pelo PRÓPRIO usuário. `users.json` NÃO é gravado
    // (D3 passo 10) — o gate de login passa a ser por existência do `.pbd`
    // (ver verifyLogin/D5).
    [[nodiscard]] FirstLoginResult completeFirstLogin(const QString &username,
                                                       const QByteArray &inviteKey,
                                                       const QString &newPassword);

    struct LoginResult {
        enum Status { Ok, UserNotFound, WrongPassword, InvitePending, IntegrityError };
        Status     status = UserNotFound;
        Role       role   = Role::User;
        QByteArray dataKey; // só preenchida quando status == Ok
    };
    [[nodiscard]] LoginResult verifyLogin(const QString &username, const QString &password);

    // Zera m_adminSignKey (sodium_memzero) — chamar no logout.
    void clearSession();

    // Recusa remover o último admin (travaria a instalação inteira).
    // Exige admin logado (mesma sessão de assinatura).
    [[nodiscard]] bool removeUser(const QString &username);

    // (ADR-001 §7, Fatia 5) Registra o dono de um cofre MIGRADO (modelo
    // single-user pré-ADR-001) como admin no registro central — usado
    // EXCLUSIVAMENTE por LegacyVaultMigrationService::migrarParaAdmin(),
    // DEPOIS que o `.pbd` v2 já foi materializado com sucesso (rekey() +
    // close()). Ao contrário de createAdmin(), NÃO bootstrapa cofre nenhum
    // (o cofre migrado já existe — foi re-cifrado in-place pelo chamador,
    // que tem acesso ao `.db` de trabalho decifrado); só grava
    // `users.json`. O chamador já embrulhou wrappedSignKeyByPass/
    // wrappedSignKeyByRecovery em `admin` com o MESMO kdfPass/kdfRecovery
    // usados no header do `.pbd` (coerência de qual senha/recovery abre o
    // quê é responsabilidade do chamador). Falha (false) se já existir
    // registro (não é mais um 1º run) ou se `admin.role != Role::Admin`.
    [[nodiscard]] bool registerMigratedAdmin(const UserAccountRecord &admin,
                                              const QByteArray &adminPubKey,
                                              const QByteArray &adminSignSecretKey);

    struct RecoveryResult {
        bool    ok = false;
        QString error;
    };
    // K e wrappedKeyByRecovery permanecem intactos (D-D/§2 — recovery não
    // rotaciona); só wrappedKeyByPass (+ saltPw) é regravado, direto no
    // header do `.pbd` v2 do usuário (D5) — operação de ARQUIVO PURA, sem
    // DatabaseManager (o innerBlob nunca é re-cifrado, só copiado
    // verbatim). Quando o alvo é usuário comum: NÃO toca `users.json`, não
    // exige sessão nenhuma (self-service completo). Quando o alvo é o
    // admin: também re-embrulha a signing key dele (wrappedSignKeyByPass) e
    // AUTO-assina a regravação de `users.json` com a própria signing key
    // recém-recuperada via recovery — continua sem depender de
    // m_adminSignKey.
    [[nodiscard]] RecoveryResult resetPasswordViaRecovery(const QString &username,
                                                           const QByteArray &recoveryKey,
                                                           const QString &newPassword);

    [[nodiscard]] QVector<UserPublicInfo> listUsers() const;

private:
    UserAccountRepository &m_repo;
    SodiumCryptoService    &m_crypto;
    SecretStoreService     &m_secrets;

    // 64B (crypto_sign_SECRETKEYBYTES) — só preenchida enquanto um admin
    // está logado NESTA instância do Service; sodium_memzero em
    // clearSession()/destrutor. Usada para assinar mutações de
    // `users.json`: createUser()/removeUser() (só o admin pode). Desde a
    // Fatia 4, completeFirstLogin() e resetPasswordViaRecovery() (usuário
    // comum) NÃO tocam `users.json` — não dependem mais deste campo (ver
    // nota de design no topo do arquivo).
    QByteArray m_adminSignKey;

    // Cruza o admin_pubkey do ARQUIVO com a cópia âncora do keychain.
    // false = mismatch OU âncora ausente (tratado como divergência,
    // fail-secure — só é chamada depois que já existe admin no arquivo,
    // então "âncora ausente" nesse ponto É adulteração/perda de keychain,
    // nunca "1º boot"). Defesa em profundidade: o keychain é acessível a
    // processos da MESMA sessão (D-Bus), então isto não isola de outro
    // processo do mesmo usuário Linux — só detecta adulteração do
    // users.json feita SEM também adulterar o keychain (ex.: restaurar um
    // users.json trocado de outra máquina/backup).
    [[nodiscard]] bool pubKeyMatchesAnchor(const QByteArray &filePubKey) const;

    // (ADR-001 §8 passo 6/D3, Fatia 4) Bootstrapa o `.pbd` v2 de UM cofre
    // novo (admin no createAdmin(), usuário comum no completeFirstLogin()):
    // setVaultHeader(header) → open(slug, K) → close(). Falha em qualquer
    // etapa aborta SEM tocar `users.json` — estado limpo e retentável (o
    // caller nunca chega a persistir um registro apontando pra um cofre
    // que não foi materializado). Não usa membros de instância (nem
    // m_crypto/m_secrets) — só o singleton DatabaseManager — por isso é
    // static.
    [[nodiscard]] static bool bootstrapVault(const QString &slug, const VaultHeaderData &header,
                                              const QByteArray &K);

    // (ADR-001 §8 passo 6/D5, Fatia 4) Regrava SÓ o header do `.pbd` v2 em
    // `vaultPath` com `newHeader`, preservando o innerBlob (payload cifrado
    // real) VERBATIM — usado por resetPasswordViaRecovery() pra trocar
    // wrappedKeyByPass/saltPw SEM re-cifrar o cofre (K nunca muda). Lê o
    // arquivo, valida com VaultFile::parseV2, re-monta com
    // VaultFile::frameV2(newHeader, innerBlob-lido) e escreve atômico
    // (temp + fsync + rename — mesmo idioma de UserAccountRepository::
    // saveAll()). false se o arquivo não existir/não for v2/falhar a
    // escrita — nunca lança.
    [[nodiscard]] static bool rewriteVaultHeader(const QString &vaultPath,
                                                  const VaultHeaderData &newHeader);
};
