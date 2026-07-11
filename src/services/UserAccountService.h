#pragma once

#include "models/UserAccountRecord.h"
#include "repositories/UserAccountRepository.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/secret/SecretStoreService.h"

#include <QByteArray>
#include <QString>
#include <QVector>

#include <optional>

// (ADR-001 #212 §3/§4/§6/§8, Fatia 2) Regra de negócio de gestão de
// usuários: envelope encryption (wrap/unwrap de K), fluxo invite-key
// (D-A/CWE-522), recovery key (Crockford Base32 + checksum local), sessão
// de assinatura do admin (signing key Ed25519 em memória, só enquanto
// logado) e verificação cruzada do admin_pubkey do arquivo contra a âncora
// gravada no SecretStoreService (defesa em profundidade anti pubkey-swap —
// achado do spot-check da Fatia 1, item E do brief desta tarefa).
//
// Sem exceção como controle de fluxo (CONTRACT §6.4/§413): toda operação
// pública devolve um struct de resultado ou um enum de status; nenhuma
// lança para o caller decidir. UserAccountRepository/SodiumCryptoService
// (Foundation) ainda lançam std::runtime_error para violação de invariante
// de PROGRAMAÇÃO (ex.: chave de tamanho errado) — este Service nunca
// alimenta esses métodos com tamanhos inválidos (os gera/deriva ele mesmo),
// então essas exceções nunca deveriam disparar em uso normal.
//
// DECISÃO DE DESIGN MAIS DELICADA (documentada aqui + no relatório da
// tarefa): UserAccountRepository::saveAll() SEMPRE exige a signing secret
// key Ed25519 do admin para gravar o registro central (users.json) —
// garantia deliberada da Fatia 1 contra CWE-269 (auto-promoção a admin).
// completeFirstLogin() e resetPasswordViaRecovery() (usuário comum) MUTAM
// esse registro (gravam o wrap por senha pela 1ª vez / trocam o wrap por
// senha), mas o usuário comum NUNCA tem uma signing key própria (só o
// admin tem, por design do ADR-001 §4). A solução "correta" e completa do
// ADR (§5.2/§8 passo 6: gravar esses wraps no header auto-contido do
// próprio .pbd v2 do usuário, que não exige assinatura do admin) depende
// de DatabaseManager — Foundation, FORA DO ESCOPO desta fatia (Back/regra
// de negócio). Resolução adotada para esta fatia:
//   • completeFirstLogin(): exige m_adminSignKey preenchida (mesma sessão
//     de assinatura de createUser/removeUser) para poder resignar o
//     registro. Isso NÃO viola CWE-522/D-A — a senha do novo usuário nunca
//     passa pelo código do admin, só a ASSINATURA de metadados (JSON sem a
//     senha em claro) precisa da chave do admin. É, porém, uma limitação
//     temporária: em produção, o fluxo 100% "admin nunca online durante o
//     1º login do usuário" só fecha quando o passo 6 (.pbd v2 +
//     DatabaseManager::rekey) for implementado numa fatia futura.
//   • resetPasswordViaRecovery(): quando o ALVO é o próprio admin, a
//     recovery key do admin já desembrulha a signing key dele mesmo (via
//     wrappedSignKeyByRecovery) — a operação se AUTO-assina, sem depender
//     de m_adminSignKey. Isso cobre 100% o cenário real de recovery (admin
//     esqueceu a senha, faz login via recovery, sem ninguém mais
//     envolvido). Quando o alvo é um usuário comum, a mesma limitação de
//     completeFirstLogin() se aplica (exige m_adminSignKey na sessão).
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

    // (ADR-001 §2) minúsculo, sem acento, sem espaço — usado para detectar
    // colisão "João" vs "joao" em usernameExists() e para derivar
    // vault_path via AppSettings::savePathFixo(slugify(username)).
    // Superset ligeiramente mais correto que UserSession::slug() (que só
    // remove caracteres fora de [a-z0-9_] SEM decompor Unicode — não
    // reusável aqui de qualquer forma, pois é método de instância sobre
    // UserSession::m_name, não uma função livre; ver relatório da tarefa).
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

    struct RecoveryResult {
        bool    ok = false;
        QString error;
    };
    // K e wrappedKeyByRecovery permanecem intactos (D-D/§2 — recovery não
    // rotaciona); só wrappedKeyByPass (+ saltPw) é regravado.
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
    // clearSession()/destrutor. Usada para assinar createUser/removeUser/
    // completeFirstLogin (usuário comum)/resetPasswordViaRecovery (usuário
    // comum) — ver nota de design no topo do arquivo.
    QByteArray m_adminSignKey;

    // (item E do brief) Chave reservada no SecretStoreService para a cópia
    // âncora do admin_pubkey — nunca colide com chaves de app existentes
    // (brapiToken, salt de login legado etc.) por causa do padrão "__x__".
    static constexpr const char *kAdminPubKeyAnchorKey = "__admin_pubkey__";

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
};
