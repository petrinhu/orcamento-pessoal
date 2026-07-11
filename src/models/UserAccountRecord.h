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

// Registro completo de um usuário — inclui os campos de envelope cripto
// (wraps de K por senha/recovery). NUNCA expor este struct para a UI/telas
// que só precisam listar usuários — usar UserPublicInfo abaixo (ISP).
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
    QByteArray saltPw;                  // 16B — salt do Argon2id sobre a senha
    QByteArray saltRk;                  // 16B — salt do Argon2id sobre a recovery key
    int        kdfOps = 0;              // crypto_pwhash_OPSLIMIT_* gravado (permite auto-rehash futuro)
    size_t     kdfMem = 0;              // crypto_pwhash_MEMLIMIT_* gravado
    QByteArray wrappedKeyByPass;        // 72B: nonce(24)+secretbox(K)+MAC. Ausente até o 1º login (fluxo invite key)
    QByteArray wrappedKeyByRecovery;    // 72B — idem, via recovery key
    bool       invitePending = false;   // true = ainda no fluxo de invite key (sem senha própria definida)
};

// DTO sem NENHUM campo cripto (ISP — ADR-001 §6). Único formato seguro pra
// expor listas de usuário à camada de UI (ex.: AdminPanelWindow::listUsers).
struct UserPublicInfo {
    QString username;
    Role    role = Role::User;
    bool    hasVault = false;
};
