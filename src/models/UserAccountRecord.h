#pragma once

#include <QByteArray>
#include <QString>

#include <cstddef>   // size_t

// (ADR-001 #212 §3/§5.1/§6) Modelo do registro de usuários da instalação
// (users.json). Papel decidido pelo admin na criação — controla acesso ao
// AdminPanelWindow por ARQUITETURA (o painel gestor nunca inclui os widgets
// financeiros), não por checagem de UI em runtime.
enum class Role {
    Admin,
    User,
};

// Registro completo de um usuário. NUNCA expor este struct para a UI/telas
// que só precisam listar usuários — usar UserPublicInfo abaixo (ISP).
//
// (ADR-001 §8 passo 6/D1/D7, Fatia 4) Os wraps de K por senha/recovery
// (saltPw, saltRk, kdfOps, kdfMem, wrappedKeyByPass, wrappedKeyByRecovery)
// MIGRARAM daqui para `VaultHeaderData` (core/crypto/VaultFile.h) — o `.pbd`
// v2 de CADA cofre (admin ou usuário comum) é agora a ÚNICA fonte de
// verdade desses 6 campos. Mutação de senha/recovery de usuário comum passa
// a tocar só o `.pbd` dele (sem admin, sem assinatura) — ver
// UserAccountService::resetPasswordViaRecovery.
//
// Campos deliberadamente OMITIDOS pelo ADR-001 (convergência dos dois
// especialistas, §2):
//   pass_verifier — removido: a tag do AEAD ao desembrulhar K já É o
//                   verificador (falha de tag = senha errada). Campo
//                   redundante = superfície de bug.
//   vault_path    — derivado de AppSettings::savePathFixo(slug(username)),
//                   nunca gravado em claro no registro — anti path-traversal
//                   (defesa em profundidade mesmo com a assinatura MAC).
struct UserAccountRecord {
    QString    username;
    Role       role = Role::User;
    bool       invitePending = false;   // true = ainda no fluxo de invite key (sem senha própria definida)

    // (ADR-001 §8 passo 6/D3, Fatia 4 — decisão do líder 2026-07-11) Canal
    // de convite: a invite key embrulha um MARCADOR ALEATÓRIO descartável
    // de 32B — NUNCA o K real (que só nasce em
    // UserAccountService::completeFirstLogin, gerado pelo PRÓPRIO usuário).
    // Só relevantes enquanto invitePending==true; ficam vestigiais (não
    // apagados) depois — o marcador desembrulhado nunca vira dado de fato,
    // então não há risco em deixá-los no arquivo (ver prova de segurança do
    // T6 no relatório da tarefa).
    QByteArray saltInvite;               // 16B — salt do Argon2id sobre a invite key
    int        kdfOpsInvite = 0;         // crypto_pwhash_OPSLIMIT_* gravado
    size_t     kdfMemInvite = 0;         // crypto_pwhash_MEMLIMIT_* gravado
    QByteArray wrappedMarkerByInvite;    // 72B: nonce(24)+secretbox(marcador)+MAC

    // (ADR-001 #212 §4/§5.1, Fatia 2) Signing secret key Ed25519 (64B) DO
    // ADMIN, embrulhada como K (por senha e por recovery key do admin) —
    // permite ao admin logado assinar mutações futuras do registro sem
    // gravar a secret key em claro em lugar nenhum. Vazios (QByteArray())
    // para usuários comuns — só o(s) registro(s) com role==Admin usam.
    QByteArray wrappedSignKeyByPass;
    QByteArray wrappedSignKeyByRecovery;
};

// DTO sem NENHUM campo cripto (ISP — ADR-001 §6). Único formato seguro pra
// expor listas de usuário à camada de UI (ex.: AdminPanelWindow::listUsers).
struct UserPublicInfo {
    QString username;
    Role    role = Role::User;
    bool    hasVault = false;
};
