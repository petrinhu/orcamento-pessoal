#pragma once

#include "models/UserAccountRecord.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/settings/AppSettings.h"

#include <QString>
#include <QVector>
#include <QByteArray>

// (ADR-001 #212 §5.1/§6) I/O BURRO de users.json: só serializa/desserializa
// e verifica a assinatura Ed25519 do registro. NÃO interpreta semântica
// (role, unicidade de username, validade de envelope) — isso é regra de
// negócio do UserAccountService (fatia 2, fora de escopo aqui — SRP).
//
// Fail-secure (ADR-001 §4): assinatura inválida OU registro malformado →
// loadAll() retorna ok=false, tampered=true, users VAZIO. Nunca tenta
// "auto-reparar" nem retorna dado parcial — quem chama decide o que fazer
// (bloquear boot com mensagem clara).
class UserAccountRepository {
public:
    struct LoadResult {
        bool ok = false;
        bool tampered = false;
        QByteArray adminPubKey;
        QVector<UserAccountRecord> users;
    };

    // filePath injetável — testes usam QTemporaryDir; produção usa o
    // default AppSettings::userRegistryPath() (users.json da instalação).
    explicit UserAccountRepository(SodiumCryptoService &crypto,
                                    QString filePath = AppSettings::userRegistryPath());

    bool exists() const;
    // Convenção de "arquivo ausente" (nova instalação) OU "0 usuários"
    // (registro válido mas vazio) — ambos contam como vazio. NÃO é uma
    // checagem de segurança: para decisão fail-secure sobre adulteração,
    // usar loadAll()/lastLoadWasTampered() diretamente.
    bool isEmpty() const;

    LoadResult loadAll() const;

    // Assina com adminSignSecretKey (64B) — o repositório NUNCA guarda essa
    // chave; quem fornece é o Service (que a desembrulhou da senha do admin
    // logado, fora de escopo aqui). Escrita atômica via QSaveFile
    // (arquivo temp + fsync + rename só no commit() bem-sucedido).
    bool saveAll(const QVector<UserAccountRecord> &users,
                 const QByteArray &adminPubKey,
                 const QByteArray &adminSignSecretKey);

    bool lastLoadWasTampered() const;

private:
    SodiumCryptoService &m_crypto;
    QString m_filePath;
    mutable bool m_lastLoadWasTampered = false;
};
